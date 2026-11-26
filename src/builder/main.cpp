#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>
#include <variant>

#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

#include "remote_sink.h"

class Task {
public:
  Task(std::shared_ptr<BuilderRabbitMQ> rabbitmq,
       std::string_view client_config, std::string_view flake_url,
       std::string task_input)
      : rabbitmq{rabbitmq} {
    std::string flake_source =
        std::format(R"(
	{{
	  inputs = {{
	    nixpkgs.follows = "taskTemplate/nixpkgs";
		taskTemplate.url = "{1}";
		flake-parts.url = "github:hercules-ci/flake-parts";
	  }};
	  outputs = inputs@{{flake-parts, ...}}: flake-parts.lib.mkFlake
		{{inherit inputs;}} {{
		  systems = ["x86_64-linux"];
		  perSystem = {{pkgs, inputs', ...}}: {{
			packages.default = pkgs.callPackage inputs.taskTemplate.taskTemplate {{
			  builderConfig = {0};
			  taskInput = {2};
			}};
		  }};
		}};
	}}
	)",
                    client_config, flake_url, task_input);

    auto promise = std::make_shared<std::promise<std::string>>();
    result = promise->get_future().share();

    runner_thread = std::jthread{[rabbitmq, flake_source, promise]() {
      std::filesystem::path flake_dir = std::filesystem::temp_directory_path() /
                                        std::string{UUID::generate()};
      std::filesystem::create_directory(flake_dir);
      std::filesystem::path flake_path = flake_dir / "flake.nix";
      std::ofstream file{flake_path};
      file << flake_source;
      file.flush();

      std::ignore = std::system(
          std::format("nix build {}", std::string{flake_dir}).c_str());

      std::ifstream result_file{"./result", std::ios::binary};
      std::stringstream ss;
      ss << result_file.rdbuf();
      ss.flush();
      std::string result_file_content = ss.str();

      promise->set_value(result_file_content);
    }};
  }

  std::shared_future<std::string> get_result() { return result; }

private:
  std::jthread runner_thread;
  std::shared_future<std::string> result;
  std::shared_ptr<BuilderRabbitMQ> rabbitmq;
};

int main() {
  Logger::get_logger().set_level(LoggerLevel::DEBUG);

  UUID client_uuid = UUID::generate();
  Logger::get_logger().log(LoggerLevel::INFO, "Client UUID: {}",
                           std::string{client_uuid});
  auto rabbitmq = std::make_shared<BuilderRabbitMQ>(
      "amqp://nix-cluster:develop@amogus:5672/nix-cluster",
      std::string{client_uuid});

  auto remote_sink =
      std::make_unique<LoggerSinkRemote>(rabbitmq, std::string{client_uuid});
  Logger::get_logger().set_sink(std::move(remote_sink));

  MessageDispatcher message_dispatcher;
  std::unique_ptr<Task> running_task;
  std::jthread wait_for_task;

  message_dispatcher.bind_handler(
      Message::PING,
      std::make_shared<MessageDispatcher::Handler>(
          [client_uuid, rabbitmq](const AllMessagesVariant &message) {
            Logger::get_logger().log(DEBUG, "Got PING");
            const auto &m = std::get<MessagePing>(message);
            MessagePong new_message;
            new_message.sender = client_uuid;
            new_message.message_uuid = UUID::generate();
            new_message.content.ping_uuid = m.message_uuid;
            new_message.content.timestamp =
                std::chrono::high_resolution_clock::now();

            rabbitmq->send_message(new_message);
            Logger::get_logger().log(DEBUG, "Sent PONG");
          }));

  message_dispatcher.bind_handler(
      Message::DISCOVERY,
      std::make_shared<MessageDispatcher::Handler>(
          [client_uuid, rabbitmq](const AllMessagesVariant &message) {
            Logger::get_logger().log(DEBUG, "Got DISCOVERY");
            const auto &m = std::get<MessageDiscovery>(message);
            MessageDiscoveryResponse new_message;
            new_message.sender = client_uuid;
            new_message.message_uuid = UUID::generate();
            new_message.content.request_uuid = m.message_uuid;
            new_message.content.name = "Joshua";
            new_message.content.client_config = "{amo=\"gus\";}";
            new_message.content.status = BuilderStatus::IDLE;

            rabbitmq->send_message(new_message);
            Logger::get_logger().log(DEBUG, "Sent DISCOVERY_RESPONSE");
          }));

  message_dispatcher.bind_handler(
      Message::TASK_REQUEST,
      std::make_shared<MessageDispatcher::Handler>(
          [client_uuid, rabbitmq, &running_task,
           &wait_for_task](const AllMessagesVariant &message) {
            Logger::get_logger().log(DEBUG, "Got TASK_REQUEST");
            const auto &m = std::get<MessageTaskRequest>(message);
            MessageTaskRequestResponse new_message;
            new_message.sender = client_uuid;
            new_message.message_uuid = UUID::generate();
            new_message.content.request_uuid = m.message_uuid;

            if (running_task) {
              new_message.content.response = "busy";
            } else {
              new_message.content.response = "success";
            }

            rabbitmq->send_message(new_message);

            Logger::get_logger().log(DEBUG, "Sent TASK_REQUEST_RESPONSE");

            if (running_task)
              return;

            running_task = std::make_unique<Task>(
                rabbitmq, m.content.client_config, m.content.flake_url,
                m.content.task_input);

            wait_for_task =
                std::jthread{[rabbitmq, new_message, &running_task,
                              &wait_for_task, m](std::stop_token stop_token) {
                  auto future = running_task->get_result();
                  std::string results = future.get();

                  MessageTaskCompleted new_message2;
                  new_message2.message_uuid = UUID::generate();
                  new_message2.sender = "builder";
                  new_message2.content.request_uuid = m.message_uuid;
                  new_message2.content.response = "success";
                  new_message2.content.result = results;

                  rabbitmq->send_message(new_message2);

                  Logger::get_logger().log(INFO, "Sent task results");

                  running_task = std::unique_ptr<Task>{};
                }};
          }));

  rabbitmq->message_received_handler =
      [&message_dispatcher](const AllMessagesVariant &message) {
        message_dispatcher.dispatch(message);
      };

  std::jthread heartbeat_thread{[rabbitmq](std::stop_token stoken) {
    auto ready_future = rabbitmq->get_ready_future();
    ready_future.wait();

    while (!stoken.stop_requested()) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5s);

      rabbitmq->heartbeat();
    }
  }};

  auto foo = std::async([rabbitmq, client_uuid]() {
    auto ready_future = rabbitmq->get_ready_future();
    ready_future.wait();

    {
      MessageInitializeBuilder init_build;
      init_build.message_uuid = UUID::generate();
      init_build.sender = client_uuid;
      init_build.content.name = "Joshua";
      init_build.content.client_config = "{amo=\"gus\";}";

      rabbitmq->send_message(init_build);
    }

    Logger::get_logger().log(INFO, "Waiting for tasks");
  });

  while (true) {
    rabbitmq->monitor(10000);
  }

  heartbeat_thread.request_stop();
}
