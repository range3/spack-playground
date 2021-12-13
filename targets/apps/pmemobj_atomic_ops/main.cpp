#include <fmt/core.h>
#include <fmt/ostream.h>
#include <sys/types.h>
#include <cstring>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <functional>
#include <gen_random_string.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <mutex>
#include <new>
#include <nlohmann/json.hpp>
#include <pretty_bytes/pretty_bytes.hpp>
#include <psync/barrier.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using json = nlohmann::json;

// auto* inline_write_req = new (buf + pos) InlineWriteRequest(size, offset);
// need a pre-allocated buffer: sizeof(InlineWriteRequest) + size
struct InlineWriteRequest {
  off64_t offset;
  size_t size;
  InlineWriteRequest() = default;
  InlineWriteRequest(off64_t ofs, size_t s) : offset(ofs), size(s) {}
  auto data() -> char* { return reinterpret_cast<char*>(this + 1); }
  auto data() const -> const char* {
    return reinterpret_cast<const char*>(this + 1);
  }

  using create_arg_t = std::tuple<std::string_view, off64_t>;
  inline static auto create(PMEMobjpool* pop, void* ptr, void* arg) -> int {
    auto* src = static_cast<create_arg_t*>(arg);
    auto* dst = new (ptr)
        InlineWriteRequest{std::get<1>(*src), std::get<0>(*src).size()};

    pmemobj_memcpy(pop, dst->data(), std::get<0>(*src).data(), dst->size,
                   PMEMOBJ_F_MEM_NONTEMPORAL);
    pmemobj_xpersist(pop, ptr, sizeof(InlineWriteRequest), PMEMOBJ_F_RELAXED);
    return 0;
  }

  inline static void makePersistentAtomic(
      pmem::obj::pool_base& pop,
      pmem::obj::persistent_ptr<InlineWriteRequest>& ptr,
      pmem::obj::allocation_flag_atomic flag,
      std::string_view data,
      off64_t offset) {
    auto req_capacity = sizeof(InlineWriteRequest) + data.size();
    create_arg_t arg = std::make_tuple(data, offset);
    auto ret =
        pmemobj_xalloc(pop.handle(), ptr.raw_ptr(), req_capacity, 0, flag.value,
                       InlineWriteRequest::create, static_cast<void*>(&arg));
    if (ret != 0) {
      throw std::system_error(errno, std::system_category(), "pmemobj_xalloc");
    }
  }
};

struct Root {
  pmem::obj::persistent_ptr<InlineWriteRequest> ptr;
  pmem::obj::persistent_ptr<InlineWriteRequest> ptr2;
  pmem::obj::persistent_ptr<InlineWriteRequest> ptr3;
};
using pool_t = pmem::obj::pool<Root>;

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("pmemobj_atomic_ops", "test pmemobj_xalloc / free");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("pmemobj_atomic_ops.pool"))
    ("S,pool_size", "pool size (bytes)", cxxopts::value<std::string>()->default_value("8M"))
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto pool_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["pool_size"].as<std::string>());

  // create or open a pmemobj pool
  auto const pool_file_path = parsed["path"].as<std::string>();
  static constexpr auto kPoolLayout = "pmemobj_atomic_ops";
  pool_t pop;

  if (pool_t::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_t::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_t::create(pool_file_path, kPoolLayout, pool_size);
  }
  auto r = pop.root();

  std::string rand_data =
      grs::generateRandomAlphanumericString(1 * 1024 * 1024);

  while (true) {
    InlineWriteRequest::makePersistentAtomic(
        pop, r->ptr, pmem::obj::allocation_flag_atomic::none(), rand_data, 0);
    InlineWriteRequest::makePersistentAtomic(
        pop, r->ptr2, pmem::obj::allocation_flag_atomic::none(), rand_data, 0);
    InlineWriteRequest::makePersistentAtomic(
        pop, r->ptr3, pmem::obj::allocation_flag_atomic::none(), rand_data, 0);
    pmem::obj::flat_transaction::run(pop, [&]() {
      pmem::obj::delete_persistent<InlineWriteRequest>(r->ptr.get());
      pmem::obj::delete_persistent<InlineWriteRequest>(r->ptr2.get());
      pmem::obj::delete_persistent<InlineWriteRequest>(r->ptr3.get());
    });
  }

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
