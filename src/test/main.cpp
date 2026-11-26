#include <chrono>
#include <iostream>
#include <thread>

#include <framework.h>
#include <logger.h>

#include "task.h"

int main(int argc, char *argv[]) {
  Logger::get_logger().set_level(DEBUG);

  auto f = Framework::create(argc, argv);

  std::shared_ptr<Task> task;

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds{5});
    auto builders = f->get_builders().get();

    if (builders.size() == 0)
      continue;

    task = std::make_shared<Task>(f->create_task(
        builders.begin()->first, "{}", "/home/filip/dev/nix-cluster", "{}"));

    break;
  }

  Logger::get_logger().log(ERROR, "Waiting for result");
  Logger::get_logger().log(ERROR, "RESULT ON GENERATOR: {}",
                           task->get_future().get());
}
