#include <fcntl.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <functional>
#include <gen_random_string.hpp>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <numeric>
#include <pretty_bytes/pretty_bytes.hpp>
#include <psync/barrier.hpp>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

auto parse(int argc, char** argv)
    -> std::pair<std::string, cxxopts::ParseResult> {
  constexpr std::string_view kDescription =
      "cxxopts subccommand example";
  // clang-format off
  std::map<std::string, std::function<cxxopts::Options(void)>>
      subcmd_options = {
          {"create", [&]() {
            cxxopts::Options options("pmemobjstorage create", std::string{kDescription});
            options.add_options()
              ("h,help", "Print help and exit.")
              ("s,size", "object size", cxxopts::value<std::string>()->default_value("2M"));
            return options;
          }},
          {"remove", [&]() {
            cxxopts::Options options("pmemobjstorage remove", std::string{kDescription});
            options.add_options()
              ("h,help", "Print help and exit.")
              ("oid", "object ID", cxxopts::value<uint64_t>());
            return options;
          }},
          {"get", [&]() {
            cxxopts::Options options("pmemobjstorage get", std::string{kDescription});
            options.add_options()
              ("h,help", "Print help and exit.")
              ("oid", "object ID", cxxopts::value<uint64_t>());
            return options;
          }},
          {"set", [&]() {
            cxxopts::Options options("pmemobjstorage set", std::string{kDescription});
            options.add_options()
              ("h,help", "Print help and exit.")
              ("oid", "object ID", cxxopts::value<uint64_t>());
            return options;
          }}
      };
  // clang-format on

  const auto subcmd_names =
      std::accumulate(subcmd_options.begin(), subcmd_options.end(),
                      std::vector<std::string>{}, [](auto acc, const auto& i) {
                        acc.push_back(i.first);
                        return acc;
                      });

  auto print_global_help = [&]() {
    fmt::print("Subcommand style option parser using cxxopts\n");
    fmt::print("Usage:\n");
    fmt::print("  {} {} [OPTION...]\n\n", argv[0], fmt::join(subcmd_names, "|"));
  };

  // parse subcmd name
  if (argc < 2) {
    print_global_help();
    exit(0);
  }
  auto subcmd_name = std::string{argv[1]};

  auto isubcmd = subcmd_options.find(subcmd_name);
  if (isubcmd == subcmd_options.end()) {
    print_global_help();
    fmt::print(stderr, "Error: unknown subcommand {}\n", subcmd_name);
    exit(1);
  }

  auto subcmd_option = isubcmd->second();
  auto parsed = isubcmd->second().parse(argc, argv);

  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", subcmd_option.help());
    exit(0);
  }

  return {subcmd_name, parsed};
}

auto main(int argc, char* argv[]) -> int {
  try {
    auto [subcmd, parsed] = parse(argc, argv);
  } catch (const cxxopts::OptionException& e) {
    fmt::print(stderr, "Error parsing options: {}\n", e.what());
    exit(1);
  }

  return 0;
}
