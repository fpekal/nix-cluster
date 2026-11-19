#pragma once
#include <map>
#include <string>

enum BuilderStatus {
  IDLE,
  WORKING,
};

class BuilderInfo {
public:
  std::string uuid;
  std::string name;
  std::string client_config;
  BuilderStatus status = IDLE;
};

// Map of uuid -> BuilderInfo
typedef std::map<std::string, BuilderInfo> Builders;
