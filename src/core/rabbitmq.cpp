#include "rabbitmq.h"

#include <iostream>
#include <poll.h>

#include <amqpcpp.h>
#include <amqpcpp/address.h>
#include <amqpcpp/callbacks.h>
#include <amqpcpp/channel.h>
#include <amqpcpp/connection.h>
#include <amqpcpp/deferredqueue.h>
#include <amqpcpp/flags.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/linux_tcp/tcpconnection.h>
#include <amqpcpp/linux_tcp/tcphandler.h>

#include "message.h"

class TcpHandler : public AMQP::TcpHandler {
public:
  TcpHandler(std::map<int, int> &monitored_fds)
      : AMQP::TcpHandler{}, monitored_fds{monitored_fds} {}

private:
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
    std::cout << "HANDLER ATTACHED\n";
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
    std::cout << "HANDLER CONNECTED\n";
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
    std::cout << "HANDLER SECURED\n";
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
    std::cout << "HANDLER READY\n";
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
    std::cout << "HANDLER ERROR\n";
    std::cout << message << '\n';
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
    std::cout << "HANDLER CLOSED\n";
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
    std::cout << "HANDLER LOST\n";
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
    std::cout << "HANDLER DETACHED\n";
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
    // @todo
    //  add your own implementation, for example by adding the file
    //  descriptor to the main application event loop (like the select() or
    //  poll() loop). When the event loop reports that the descriptor becomes
    //  readable and/or writable, it is up to you to inform the AMQP-CPP
    //  library that the filedescriptor is active by calling the
    //  connection->process(fd, flags) method.

    // Stop monitoring
    if (flags == 0) {
      monitored_fds.erase(fd);
    } else { // Start monitoring
      monitored_fds.insert({fd, flags});
    }

    std::cout << "HANDLER MONITOR\n";
  }
};

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
}

RabbitMQ::~RabbitMQ() { delete internal; }

void RabbitMQ::monitor(int timeout) {
  internal->connection.heartbeat();

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
    internal->connection.process(poll.fd, flags);
  }
}

BuilderRabbitMQ::BuilderRabbitMQ(std::string_view address,
                                 std::string_view uuid)
    : RabbitMQ{address}, uuid{uuid} {
  routing_key = std::string{"builder."} + this->uuid;

  internal->channel.declareExchange("builders", AMQP::topic);

  internal->channel.declareQueue(AMQP::exclusive)
      .onSuccess([this](const std::string &name, uint32_t messagecount,
                        uint32_t consumercount) {
        internal->channel.bindQueue("builders", name, routing_key);
        internal->channel.consume(name).onReceived(
            [this](const AMQP::Message &message, uint64_t deliveryTag,
                   bool redelivered) {
              std::cerr << "MESSAGE RECEIVED: "
                        << std::string_view{message.body(), message.bodySize()}
                        << '\n';

              internal->channel.ack(deliveryTag);
            });
      });
}

void BuilderRabbitMQ::send_message(const Message &message) {
  std::string message_str = message_to_json(message);

  internal->channel.publish("builders", routing_key, message_str);
}
