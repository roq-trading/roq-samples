/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/api.hpp"

#include "roq/logging/flags/parser.hpp"
#include "roq/logging/flags/settings.hpp"

#include "roq/samples/example-3/application.hpp"

using namespace std::literals;

// === CONSTANTS ===

namespace {
roq::Service::Info const INFO{
    .description = "Example 3 (Roq Samples)"sv,
    .package_name = ROQ_PACKAGE_NAME,
    .build_version = ROQ_VERSION,
};
}  // namespace

// === IMPLEMENTATION ===

int main(int argc, char **argv) {
  roq::logging::flags::Parser parser{argc, argv, INFO.description, INFO.build_version};
  auto settings = roq::logging::flags::create_settings();
  return roq::samples::example_3::Application{parser, settings, INFO}.run();
}
