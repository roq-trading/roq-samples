/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/samples/republish/application.hpp"

#include <vector>

#include "roq/exceptions.hpp"

#include "roq/samples/republish/config.hpp"
#include "roq/samples/republish/strategy.hpp"

using namespace std::literals;

namespace roq {
namespace samples {
namespace republish {

int Application::main_helper(const std::span<std::string_view> &args) {
  assert(!std::empty(args));
  if (std::size(args) == 1)
    log::fatal("Expected arguments"sv);
  Config config;
  // note!
  //   absl::flags will have removed all flags and we're left with arguments
  //   arguments should be a list of unix domain sockets
  auto connections = args.subspan(1);
  client::Trader(config, connections).dispatch<Strategy>();
  return EXIT_SUCCESS;
}

int Application::main(int argc, char **argv) {
  // wrap arguments (prefer to not work with raw pointers)
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  return main_helper(args);
}

}  // namespace republish
}  // namespace samples
}  // namespace roq