#pragma once

#include <iostream>

inline auto &info() {
  std::cout << "[INFO] ";
  return std::cout;
}

inline auto &error() {
  std::cerr << "[ERROR] ";
  return std::cerr;
}


