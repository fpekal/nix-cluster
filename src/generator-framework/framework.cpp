#include "framework.h"

#include <fstream>
#include <functional>

#include <logger.h>
#include <nlohmann/json.hpp>
#include <uuid.h>

#include "remote_sink.h"

std::shared_ptr<Framework> Framework::create(int argc, char *argv[]) {
  auto self = std::shared_ptr<Framework>{new Framework{argc, argv}};
  self->self = self;
  return self;
}

Framework::Framework(int argc, char *argv[]) {
  if (argc != 3) {
    Logger::get_logger().log(ERROR, "Generator got {} arguments. Expected 2",
                             argc - 1);
    exit(1);
    return;
  }

  output_pipe = argv[1];
  input_pipe = argv[2];

  input_thread =
      std::jthread(std::bind_front<&Framework::input_thread_handler>(this));
  output_thread =
      std::jthread(std::bind_front<&Framework::output_thread_handler>(this));

  Logger::get_logger().set_sink(std::make_unique<LoggerSinkRemote>(*this));
}

std::future<Builders> Framework::get_builders() {
  MessageGetBuilders new_message;
  new_message.message_uuid = UUID::generate();
  new_message.sender = "generator";

  auto promise = std::make_shared<std::promise<Builders>>();
  auto get_builders_fn = std::make_shared<MessageDispatcher::Handler>();
  *get_builders_fn =
      [promise, new_message, this,
       get_builders_fn = std::weak_ptr<MessageDispatcher::Handler>{
           get_builders_fn}](const AllMessagesVariant &message) {
        const MessageGetBuildersResponse &m =
            std::get<MessageGetBuildersResponse>(message);

        if (m.content.request_uuid != new_message.message_uuid)
          return;

        message_dispatcher.unbind_handler(Message::GET_BUILDERS_RESPONSE,
                                          get_builders_fn.lock());

        Builders ret;
        for (auto &b : m.content.builders) {
          ret[b.uuid] = b;
        }

        promise->set_value(ret);
      };

  message_dispatcher.bind_handler(Message::GET_BUILDERS_RESPONSE,
                                  get_builders_fn);

  send_message(new_message);

  return promise->get_future();
}

Task Framework::create_task(std::string_view builder_uuid,
                            std::string_view client_config,
                            std::string_view flake_url,
                            std::string_view task_input) {
  return Task{self.lock(), builder_uuid, client_config, flake_url, task_input};
}

void Framework::input_thread_handler(std::stop_token stop_token) {
  std::ifstream input{input_pipe};

  try {
    while (!stop_token.stop_requested()) {
      nlohmann::json json;
      input >> json;

      auto message = json_to_message(json.dump());

      message_dispatcher.dispatch(message.value());
    }
  } catch (nlohmann::json::exception &e) {
    // `input` returned empty string
    // so the generator has died
    // Or generator returned invalid json
    //
    // Is this exception a bug? I don't think so?
    // It works... and it's not a likely branch of the program
    //
    // When such error occurs, stop listening to more messages.
  }
}

void Framework::output_thread_handler(std::stop_token stop_token) {
  std::ofstream output{output_pipe};

  std::unique_lock lock{output_queue_mutex};

  while (!stop_token.stop_requested()) {
    output_queue_cv.wait_for(lock, std::chrono::seconds{5},
                             [this]() { return !output_queue.empty(); });

    // Better to be sure
    if (output_queue.empty())
      continue;

    AllMessagesVariant message = output_queue.back();
    output_queue.pop();

    lock.unlock();

    output << message_to_json(*reinterpret_cast<Message *>(&message)) << ' ';
    output.flush();

    lock.lock();
  }
}

void Framework::send_message(const AllMessagesVariant &message) {
  std::lock_guard lock{output_queue_mutex};

  output_queue.push(message);
  output_queue_cv.notify_one();
}
