/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/samples/example-7/collector.h"

#include "roq/logging.h"

using namespace roq::literals;

namespace roq {
namespace samples {
namespace example_7 {

void Collector::operator()(const Event<CustomMetricsUpdate> &event) {
  auto &[message_info, custom_metrics_update] = event;
  log::info("[{}:{}] CustomMetricsUpdate={}"_sv, message_info.source, message_info.source_name, custom_metrics_update);
}

}  // namespace example_7
}  // namespace samples
}  // namespace roq
