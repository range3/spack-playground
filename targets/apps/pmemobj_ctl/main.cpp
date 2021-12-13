#include <bits/exception.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <libpmemobj/base.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <gen_random_string.hpp>
#include <iomanip>
#include <iostream>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <pretty_bytes/pretty_bytes.hpp>
#include <psync/barrier.hpp>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

inline auto memcpyConstructor(PMEMobjpool* pop, void* ptr, void* arg) -> int {
  auto [src, size] = *static_cast<std::pair<char*, size_t>*>(arg);
  char* dest = static_cast<char*>(ptr);

  pmemobj_memcpy(pop, dest, src, size, PMEMOBJ_F_MEM_NONTEMPORAL);

  return 0;
}

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("pmemobj_ctl",
                           "testing pmemobj internal state");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("./pmemobjbench.pool"))
    ("S,file_size", "file size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("u,unit", "unit size", cxxopts::value<std::string>()->default_value("512"))
    ("U,nunits", "units per block", cxxopts::value<std::string>()->default_value("1K"))
    ("a,align", "alignment size", cxxopts::value<std::string>()->default_value("0"))
    ("header", "header type", cxxopts::value<std::string>()->default_value("compact"))
    ("b,block", "allocation block size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("read", "read")
    ("write", "write")
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto file_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["file_size"].as<std::string>());
  auto block_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["block"].as<std::string>());
  auto unit_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["unit"].as<std::string>());
  auto align =
      pretty_bytes::prettyTo<uint64_t>(parsed["align"].as<std::string>());
  auto nunits =
      pretty_bytes::prettyTo<uint64_t>(parsed["nunits"].as<std::string>());
  auto header = parsed["header"].as<std::string>();
  bool op_read = parsed.count("read") != 0U;
  bool op_write = parsed.count("write") != 0U;

  // check arguments
  if (!op_read && !op_write) {
    fmt::print(stderr, "Error: --read or --write required \n");
    exit(1);
  }

  struct Object {
    pmem::obj::persistent_ptr<char[]> data;  // NOLINT
  };

  struct Root {
    using obj_vector = pmem::obj::vector<Object>;

    obj_vector objs;
  };

  // create or open a pmemobj pool
  auto const pool_file_path = parsed["path"].as<std::string>();
  constexpr const char* kPoolLayout = "pmemobjbench";
  using pool_type = pmem::obj::pool<Root>;
  pool_type pop;

  if (pool_type::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_type::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_type::create(pool_file_path, kPoolLayout, file_size);
  }

  // set allocation class
  struct pobj_alloc_class_desc alloc_class_test;
  // alloc_class_test.header_type = POBJ_HEADER_NONE;
  alloc_class_test.header_type = header == "compact" ? POBJ_HEADER_COMPACT : POBJ_HEADER_NONE;
  alloc_class_test.alignment = align;
	alloc_class_test.unit_size = unit_size;
	alloc_class_test.units_per_block = static_cast<unsigned int>(nunits);
	alloc_class_test.class_id = 0;

  alloc_class_test = pop.ctl_set("heap.alloc_class.new.desc", alloc_class_test);
  fmt::print("class_id = {}\n", alloc_class_test.class_id);

  std::string random_string_data =
      grs::generateRandomAlphanumericString(block_size);

  // init root object
  auto r = pop.root();
  pmem::obj::transaction::run(pop, [&] { r->objs.resize(100); });

  // test
  if (op_write) {
    std::string buffer = grs::generateRandomAlphanumericString(block_size);

    // // alloc obj test
    // pmem::obj::make_persistent_atomic<char[]>(pop, r->objs[0].data,
    // block_size);

    // // memcpy using non-temporal store
    // pmemobj_memcpy(pop.handle(), r->objs[0].data.get(), buffer.data(),
    //                buffer.size(), PMEMOBJ_F_MEM_NONTEMPORAL);

    std::pair<char*, size_t> source = {buffer.data(), buffer.size()};
    int ret = pmemobj_xalloc(pop.handle(), r->objs[0].data.raw_ptr(), buffer.size(),
                  //  0, POBJ_CLASS_ID(alloc_class_test.class_id),
                   0, 0,
                   memcpyConstructor, static_cast<void*>(&source));
    if(ret != 0) {
      perror("pmemobj_xalloc");
      return -1;
    }

    size_t usable = pmemobj_alloc_usable_size(r->objs[0].data.raw());
    char* p = r->objs[0].data.get();

    fmt::print("addr = {}, usable = {}, write: {}...\n", fmt::ptr(p), usable, buffer.substr(0, 16));
  }

  if (op_read) {
    std::string buffer;
    buffer.resize(block_size);

    memcpy(buffer.data(), r->objs[0].data.get(), buffer.size());

    fmt::print("read: {}...\n", buffer.substr(0, 16));
  }
  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
