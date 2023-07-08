/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/samples/example-8/flags/flags.hpp"

namespace roq {
namespace samples {
namespace example_8 {

struct Settings final : public flags::Flags {
  Settings();
};

}  // namespace example_8
}  // namespace samples
}  // namespace roq
