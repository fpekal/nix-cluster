#pragma once
#include <array>

// Made according to RFC 9562
// https://www.rfc-editor.org/info/rfc9562/

// UUIDv4
class UUID {
public:
  std::array<unsigned char, 16> data;

  // Converts `data` to a string in the form of:
  // ccb5fefb-a049-42e3-9b66-35f7f2474e57
  operator std::string() const;

  static UUID generate();
};
