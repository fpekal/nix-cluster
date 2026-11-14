#pragma once
#include <map>
#include <string>

class BuilderInfo {
public:
  std::string uuid;
  std::string name;
  std::string client_config;
};

// Map of uuid -> BuilderInfo
typedef std::map<std::string, BuilderInfo> Builders;
