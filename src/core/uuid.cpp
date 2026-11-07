#include "uuid.h"

#include <format>
#include <random>

UUID::operator std::string() const {
  static constexpr auto fmt = "{:0>2x}{:0>2x}{:0>2x}{:0>2x}-"
                              "{:0>2x}{:0>2x}-"
                              "{:0>2x}{:0>2x}-"
                              "{:0>2x}{:0>2x}-"
                              "{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}";

  return std::format(fmt, data[0], data[1], data[2], data[3], data[4], data[5],
                     data[6], data[7], data[8], data[9], data[10], data[11],
                     data[12], data[13], data[14], data[15], data[16]);
}

UUID UUID::generate() {
  static std::random_device device{};
  static std::mt19937 rnd{device()};
  static std::uniform_int_distribution<unsigned char> dist{0, 255};

  UUID ret;
  for (int i = 0; i < 16; ++i) {
    ret.data[i] = dist(rnd);
  }

  // Required bits
  ret.data[6] = 0x40 | (ret.data[6] & 0x0F);
  ret.data[8] = 0b10000000 | (ret.data[8] & 0b00111111);

  return ret;
}
