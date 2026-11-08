#include "rabbitmq.h"

#include <future>
#include <poll.h>

#include <amqpcpp.h>
#include <amqpcpp/address.h>
#include <amqpcpp/callbacks.h>
#include <amqpcpp/channel.h>
#include <amqpcpp/connection.h>
#include <amqpcpp/deferredqueue.h>
#include <amqpcpp/exchangetype.h>
#include <amqpcpp/flags.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/linux_tcp/tcpconnection.h>
#include <amqpcpp/linux_tcp/tcphandler.h>

#include "logger.h"
#include "message.h"

namespace {
// Class reacting to the TCP events.
// AMQP also tells this class when it wants to read data from sockets.
// And when data is available on the socket, we tell AMQP to process this data.
// Like that, AMQP is completly non-blocking.
class TcpHandler : public AMQP::TcpHandler {
public:
  TcpHandler(std::map<int, int> &monitored_fds)
      : AMQP::TcpHandler{}, monitored_fds{monitored_fds} {}

private:
  // Sockets returned by the AMQP.
  // We need to wait for them to become ready and then report them back to AMQP.
  //
  // first - fd
  // second - AMQP::writable and/or AMQP::readable flags
  std::map<int, int> &monitored_fds;

  /**
   *  Method that is called by the AMQP library when a new connection
   *  is associated with the handler. This is the first call to your handler
   *  @param  connection      The connection that is attached to the handler
   */
  virtual void onAttached(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation, for example initialize things
    //  to handle the connection.
    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP attached");
  }

  /**
   *  Method that is called by the AMQP library when the TCP connection
   *  has been established. After this method has been called, the library
   *  still has take care of setting up the optional TLS layer and of
   *  setting up the AMQP connection on top of the TCP layer., This method
   *  is always paired with a later call to onLost().
   *  @param  connection      The connection that can now be used
   */
  virtual void onConnected(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation (probably not needed)
    Logger::get_logger().log(LoggerLevel::INFO,
                             "Connected to the RabbitMQ instance");
  }

  /**
   *  Method that is called when the secure TLS connection has been established.
   *  This is only called for amqps:// connections. It allows you to inspect
   *  whether the connection is secure enough for your liking (you can
   *  for example check the server certificate). The AMQP protocol still has
   *  to be started.
   *  @param  connection      The connection that has been secured
   *  @param  ssl             SSL structure from openssl library
   *  @return bool            True if connection can be used
   */
  virtual bool onSecured(AMQP::TcpConnection *connection,
                         const SSL *ssl) override {
    // @todo
    //  add your own implementation, for example by reading out the
    //  certificate and check if it is indeed yours
    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP secured");
    return true;
  }

  /**
   *  Method that is called by the AMQP library when the login attempt
   *  succeeded. After this the connection is ready to use.
   *  @param  connection      The connection that can now be used
   */
  virtual void onReady(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation, for example by creating a channel
    //  instance, and start publishing or consuming
    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP ready");
  }

  /**
   *  Method that is called by the AMQP library when a fatal error occurs
   *  on the connection, for example because data received from RabbitMQ
   *  could not be recognized, or the underlying connection is lost. This
   *  call is normally followed by a call to onLost() (if the error occurred
   *  after the TCP connection was established) and onDetached().
   *  @param  connection      The connection on which the error occurred
   *  @param  message         A human readable error message
   */
  virtual void onError(AMQP::TcpConnection *connection,
                       const char *message) override {
    // @todo
    //  add your own implementation, for example by reporting the error
    //  to the user of your program and logging the error
    Logger::get_logger().log(LoggerLevel::ERROR, "TCP error: {}", message);
  }

  /**
   *  Method that is called when the AMQP protocol is ended. This is the
   *  counter-part of a call to connection.close() to graceful shutdown
   *  the connection. Note that the TCP connection is at this time still
   *  active, and you will also receive calls to onLost() and onDetached()
   *  @param  connection      The connection over which the AMQP protocol ended
   */
  virtual void onClosed(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation (probably not necessary, but it could
    //  be useful if you want to do some something immediately after the
    //  amqp connection is over, but do not want to wait for the tcp
    //  connection to shut down
    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP closed");
  }

  /**
   *  Method that is called when the TCP connection was closed or lost.
   *  This method is always called if there was also a call to onConnected()
   *  @param  connection      The connection that was closed and that is now
   * unusable
   */
  virtual void onLost(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation (probably not necessary)
    Logger::get_logger().log(LoggerLevel::ERROR,
                             "Lost connection to RabbitMQ instance");
  }

  /**
   *  Final method that is called. This signals that no further calls to your
   *  handler will be made about the connection.
   *  @param  connection      The connection that can be destructed
   */
  virtual void onDetached(AMQP::TcpConnection *connection) override {
    // @todo
    //  add your own implementation, like cleanup resources or exit the
    //  application
    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP detached");
  }

  /**
   *  Method that is called by the AMQP-CPP library when it wants to interact
   *  with the main event loop. The AMQP-CPP library is completely non-blocking,
   *  and only make "write()" or "read()" system calls when it knows in advance
   *  that these calls will not block. To register a filedescriptor in the
   *  event loop, it calls this "monitor()" method with a filedescriptor and
   *  flags telling whether the filedescriptor should be checked for readability
   *  or writability.
   *
   *  @param  connection      The connection that wants to interact with the
   * event loop
   *  @param  fd              The filedescriptor that should be checked
   *  @param  flags           Bitwise or of AMQP::readable and/or AMQP::writable
   */
  virtual void monitor(AMQP::TcpConnection *connection, int fd,
                       int flags) override {
    // AMQP tell us it need to read/write to the sockets but AMQP doesn't know
    // how to *wait* for these sockets to become ready.
    // So we need to tell it ourselves.

    // Waiting and reporting is done in the `RabbitMQ::Internal` class.

    // Stop monitoring
    if (flags == 0) {
      monitored_fds.erase(fd);
    } else { // Start monitoring
      monitored_fds.insert({fd, flags});
    }

    Logger::get_logger().log(LoggerLevel::DEBUG, "TCP monitor");
  }
};
} // namespace

// Putting most of the RabbitMQ logic in here allows us to not require including
// `amqpcpp.h` files in our `rabbitmq.h`.
// It makes the API cleaner.
class RabbitMQ::Internal {
public:
  TcpHandler handler;
  AMQP::TcpConnection connection;
  AMQP::TcpChannel channel;

  Internal(std::string_view address, std::map<int, int> &monitored_fds)
      : handler{monitored_fds},
        connection(&handler, AMQP::Address{std::string{address}}),
        channel(&connection) {}
};

RabbitMQ::RabbitMQ(std::string_view address) {
  internal = new Internal{address, monitored_fds};

  ready_future = ready_promise.get_future();
}

RabbitMQ::~RabbitMQ() { delete internal; }

// Takes all sockets that have been sent by the AMQP and checks their status.
// If they are ready, report it to the AMQP.
void RabbitMQ::monitor(int timeout) {
  std::vector<pollfd> polls;
  polls.reserve(monitored_fds.size());

  // Prepare data for poll syscall
  for (const auto &fd_flags : monitored_fds) {
    short events = 0;
    if (fd_flags.second & AMQP::readable) {
      events |= POLLIN;
    }
    if (fd_flags.second & AMQP::writable) {
      events |= POLLOUT;
    }
    polls.emplace_back(fd_flags.first, events, 0);
  }

  poll(polls.data(), polls.size(), timeout);

  for (const auto &poll : polls) {
    int flags = 0;
    if (poll.revents & POLLIN) {
      flags |= AMQP::readable;
    }
    if (poll.revents & POLLOUT) {
      flags |= AMQP::writable;
    }

    if (flags != 0) {
      Logger::get_logger().log(LoggerLevel::DEBUG, "fd {} status changed",
                               poll.fd);
    }

    std::lock_guard guard{mutex};
    internal->connection.process(poll.fd, flags);
  }
}

void RabbitMQ::heartbeat() {
  Logger::get_logger().log(LoggerLevel::DEBUG, "Heartbeat");
  std::lock_guard guard{mutex};
  internal->connection.heartbeat();
}

BuilderRabbitMQ::BuilderRabbitMQ(std::string_view address,
                                 std::string_view uuid)
    : RabbitMQ{address}, uuid{uuid} {
  routing_key = std::string{"builder."} + this->uuid;

  internal->channel.declareExchange("server", AMQP::fanout)
      .onError([](const char *message) {
        Logger::get_logger().log(LoggerLevel::ERROR,
                                 "Server exchange declaration error: {}",
                                 message);
      })
      .onSuccess([this]() {
        Logger::get_logger().log(LoggerLevel::INFO, "Server exchange declared");
      });

  // Create "exchange" for sending all messages
  internal->channel.declareExchange("builders", AMQP::topic)
      .onError([](const char *message) {
        Logger::get_logger().log(LoggerLevel::ERROR,
                                 "Builders exchange declaration error: {}",
                                 message);
      })
      .onSuccess([this]() {
        Logger::get_logger().log(LoggerLevel::INFO,
                                 "Builders exchange declared");

        // Create a queue for ONLY this builder.
        // Messages from exchange will move to this queue when they have the
        // correct `routing_key`.
        internal->channel.declareQueue(AMQP::exclusive)
            .onSuccess([this](const std::string &name, uint32_t messagecount,
                              uint32_t consumercount) {
              Logger::get_logger().log(LoggerLevel::INFO,
                                       "Queue declared, name={}", name);
              // If (and when) the queue was made successfully, bind it to the
              // exchange.
              internal->channel.bindQueue("builders", name, routing_key)
                  .onError([](const char *message) {
                    Logger::get_logger().log(LoggerLevel::ERROR,
                                             "Queue unique binding error: {}",
                                             message);
                  })
                  .onSuccess([]() {
                    Logger::get_logger().log(LoggerLevel::INFO,
                                             "Queue uniquely bound");
                  });

              internal->channel.bindQueue("builders", name, "builder.broadcast")
                  .onError([](const char *message) {
                    Logger::get_logger().log(
                        LoggerLevel::ERROR, "Queue broadcast binding error: {}",
                        message);
                  })
                  .onSuccess([]() {
                    Logger::get_logger().log(LoggerLevel::INFO,
                                             "Queue broadcast bound");
                  });

              // Start listening on the queue.
              internal->channel.consume(name)
                  .onReceived([this](const AMQP::Message &message,
                                     uint64_t deliveryTag, bool redelivered) {
                    // All messages in the queue will be processed by this
                    // function
                    Logger::get_logger().log(
                        LoggerLevel::DEBUG,
                        "Message received: length={}, body={}",
                        message.bodySize(),
                        std::string_view{message.body(), message.bodySize()});

                    // Confirm the message was processed successfully
                    internal->channel.ack(deliveryTag);
                  })
                  .onError([](const char *message) {
                    Logger::get_logger().log(LoggerLevel::ERROR,
                                             "Channel consuming error: {}",
                                             message);
                  })
                  .onSuccess([this]() {
                    Logger::get_logger().log(
                        LoggerLevel::INFO,
                        "Channel consuming started successfully");
                    ready_promise.set_value();
                  });
            })
            .onError([](const char *message) {
              Logger::get_logger().log(LoggerLevel::ERROR,
                                       "Queue declaration error: {}", message);
            });
      });
}

void BuilderRabbitMQ::send_message(const Message &message) {
  std::string message_str = message_to_json(message);

  {
    std::lock_guard guard{mutex};
    internal->channel.publish("server", "", message_str);
  }
  Logger::get_logger().log(LoggerLevel::DEBUG,
                           "Message sent: length={}, body={}",
                           message_str.length(), message_str);
}
