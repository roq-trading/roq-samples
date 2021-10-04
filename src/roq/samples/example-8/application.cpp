/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/samples/example-8/application.h"

#include <cassert>
#include <vector>

#include "roq/exceptions.h"

#include "roq/samples/example-8/config.h"
#include "roq/samples/example-8/strategy.h"

using namespace roq::literals;

namespace roq {
namespace samples {
namespace example_8 {

int Application::main_helper(const roq::span<std::string_view> &args) {
  assert(!args.empty());
  if (args.size() == 1)
    log::fatal("Expected arguments"_sv);
  Config config;
  auto connections = args.subspan(1);
  client::Trader(config, connections).dispatch<Strategy>();
  return EXIT_SUCCESS;
}

int Application::main(int argc, char **argv) {
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  return main_helper(args);
}

}  // namespace example_8
}  // namespace samples
}  // namespace roq