/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string_view>

namespace roq {
namespace samples {
namespace example_2 {

struct Flags final {
  static std::string_view futures_exchange();
  static std::string_view futures_symbol();
  static std::string_view cash_exchange();
  static std::string_view cash_symbol();
  static double alpha();
};

}  // namespace example_2
}  // namespace samples
}  // namespace roq
