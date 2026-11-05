#pragma once
#include <string>

class Client {
public:
  // UUIDv4 of the client
  std::string uuid = "";

  // Name in the logs
  std::string name = "";

  // Some json struct used to generate client_config Nix expression
  std::string client_config = "";
};
