/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/samples/metrics/flags/flags.hpp"

namespace roq {
namespace samples {
namespace metrics {

struct Settings final : public flags::Flags {
  Settings();
};

}  // namespace metrics
}  // namespace samples
}  // namespace roq
