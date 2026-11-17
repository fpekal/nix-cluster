#pragma once
#include <map>
#include <string>

#include <message.h>

class BuilderInfo {
public:
  std::string uuid;
  std::string name;
  std::string client_config;
  Message::BuilderStatus status = Message::IDLE;
};

// Map of uuid -> BuilderInfo
typedef std::map<std::string, BuilderInfo> Builders;
