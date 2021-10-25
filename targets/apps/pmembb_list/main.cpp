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
};

struct ListNode {
  struct Persistent {
    pmem::obj::persistent_ptr<ListNode::Persistent> next;
    InlineWriteRequest write_req;

    explicit Persistent(InlineWriteRequest&& wreq)
        : next(nullptr), write_req(std::move(wreq)) {}

    using create_arg_t = std::tuple<std::string_view, off64_t>;
    inline static auto create(PMEMobjpool* pop, void* ptr, void* arg) -> int {
      auto* src = static_cast<create_arg_t*>(arg);
      auto* dst = new (ptr) ListNode::Persistent{
          InlineWriteRequest{std::get<1>(*src), std::get<0>(*src).size()}};

      pmemobj_xpersist(pop, ptr, sizeof(ListNode::Persistent),
                       PMEMOBJ_F_RELAXED);
      pmemobj_memcpy(pop, dst->write_req.data(), std::get<0>(*src).data(),
                     dst->write_req.size, PMEMOBJ_F_MEM_NONTEMPORAL);
      return 0;
    }

    inline static void makePersistentAtomic(
        pmem::obj::pool_base& pop,
        pmem::obj::persistent_ptr<ListNode::Persistent>& ptr,
        pmem::obj::allocation_flag_atomic flag,
        std::string_view data,
        off64_t offset) {
      auto req_capacity = sizeof(ListNode::Persistent) + data.size();
      create_arg_t arg = std::make_tuple(data, offset);
      auto ret = pmemobj_xalloc(pop.handle(), ptr.raw_ptr(), req_capacity, 0,
                                flag.value, create, static_cast<void*>(&arg));
      if (ret != 0) {
        throw std::system_error(errno, std::system_category(),
                                "pmemobj_xalloc");
      }
    }
  };
  explicit ListNode(Persistent& persistent) : persistent_(persistent) {}

  auto offset() const -> off64_t { return persistent_.write_req.offset; }
  auto sv() const -> std::string_view {
    return std::string_view{persistent_.write_req.data(),
                            persistent_.write_req.size};
  }

 private:
  Persistent& persistent_;
  // LogEntry entry_;
};

// active -> headNode -> data -> data -> nullptr
struct BurstBuffer {
  struct Persistent {
    pmem::obj::persistent_ptr<ListNode::Persistent> active;
    pmem::obj::persistent_ptr<ListNode::Persistent> flushing;
  };

  class Writer {
   public:
    explicit Writer(BurstBuffer& bb)
        : bb_(bb), pop_(pmem::obj::pool_by_vptr(bb.persistent_)) {}

    auto pwrite(std::string_view data, off64_t offset) -> int {
      auto ret = tryPwrite(data, offset);
      // if(ret == -ENOMEM) {
      // TODO: wait for flushing
      // }
      return ret;
    }
    auto tryPwrite(std::string_view data, off64_t offset) -> int {
      try {
        ListNode::Persistent::makePersistentAtomic(
            pop_, bb_.tail_->next, pmem::obj::allocation_flag_atomic::none(),
            data, offset);
        bb_.tail_ = bb_.tail_->next.get();
        return 0;
      } catch (const std::system_error& e) {
        return -e.code().value();
      }
    }

    void forEach(std::function<bool(ListNode&&)> cb) {
      auto* cur = bb_.persistent_->active.get();
      while (cur->next != nullptr) {
        cur = cur->next.get();
        if (cb(ListNode{*cur})) {
          break;
        }
      }
    }

    auto tryEnqueueFlush() -> bool {
      std::lock_guard<std::mutex> lock(bb_.flush_mutex_);
      if (bb_.persistent_->flushing != nullptr) {
        return false;
      }

      pmem::obj::flat_transaction::run(pop_, [&]() {
        bb_.persistent_->flushing = bb_.persistent_->active->next;
        bb_.persistent_->active->next = nullptr;
      });
      bb_.tail_ = bb_.persistent_->active.get();
      return true;
    }

   private:
    BurstBuffer& bb_;
    pmem::obj::pool_base pop_;
  };

  class Flusher {
   public:
    explicit Flusher(BurstBuffer& bb)
        : bb_(bb), pop_(pmem::obj::pool_by_vptr(bb.persistent_)) {}

    using flush_func_t = std::function<bool(ListNode&& node)>;

    void flushAll(flush_func_t&& cb) {
      std::lock_guard<std::mutex> lock(bb_.flush_mutex_);
      auto cur = bb_.persistent_->flushing;
      try {
        while (cur != nullptr) {
          if (cb(ListNode(*cur))) {
            // abort flushing
            return;
          }
          cur = cur->next;
        }
      } catch (...) {
        // something happen in flush_func_t
        throw;
      }

      // free all nodes transactionally
      pmem::obj::flat_transaction::run(pop_, [&]() {
        decltype(cur) next;
        cur = bb_.persistent_->flushing;
        while (cur != nullptr) {
          next = cur->next;
          pmem::obj::delete_persistent<ListNode::Persistent>(cur);
          cur = next;
        }
        bb_.persistent_->flushing = nullptr;
      });
    }

   private:
    BurstBuffer& bb_;
    pmem::obj::pool_base pop_;
  };

  explicit BurstBuffer(Persistent* persistent)
      : persistent_(persistent), pop_(pmem::obj::pool_by_vptr(persistent)) {}

  void runtimeInitialize() {
    // create a sentinel node (head)
    if (persistent_->active == nullptr) {
      ListNode::Persistent::makePersistentAtomic(
          pop_, persistent_->active, pmem::obj::allocation_flag_atomic::none(),
          std::string_view{}, 0);
    }
    // if (persistent_->flushing == nullptr) {
    //   ListNode::Persistent::makePersistentAtomic(
    //       pop_, persistent_->flushing,
    //       pmem::obj::allocation_flag_atomic::none(), std::string_view{}, 0);
    // }

    // retrieve tail
    tail_ = persistent_->active.get();
    while (tail_->next != nullptr) {
      tail_ = tail_->next.get();
    }
  }

 private:
  Persistent* persistent_;
  pmem::obj::pool_base pop_;
  ListNode::Persistent* tail_;
  std::mutex flush_mutex_;
};

struct Root {
  pmem::obj::persistent_ptr<BurstBuffer::Persistent> bb;
};
using pool_t = pmem::obj::pool<Root>;

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options(
      "pmembb_list", "pmem burst buffer implemented by singly-linked list");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("pmembb_list.pool"))
    ("S,pool_size", "pool size (bytes)", cxxopts::value<std::string>()->default_value("8M"))
  ;
  // clang-format on

  // // test inline_write_request
  // std::array<char, 100> buf;
  // auto sv = std::string_view{"hello!!"};
  // auto* iwr = new (buf.data()) InlineWriteRequest(1, sv.size());
  // std::memcpy(iwr->data(), sv.data(), sv.size());
  // // iwr->offset = 10;
  // // iwr->size = sv.size();
  // // ::memcpy(iwr->data(), sv.data(), sv.size());
  // fmt::print("ofs: {}, size: {}, data: {}\n", iwr->offset, iwr->size,
  //            iwr->data());
  // iwr = reinterpret_cast<InlineWriteRequest*>(buf.data());
  // fmt::print("ofs: {}, size: {}, data: {}\n", iwr->offset, iwr->size,
  //            iwr->data());

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
  static constexpr auto kPoolLayout = "pmembb_list";
  pool_t pop;

  if (pool_t::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_t::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_t::create(pool_file_path, kPoolLayout, pool_size);
  }
  auto r = pop.root();

  if (r->bb == nullptr) {
    pmem::obj::flat_transaction::run(pop, [&]() {
      r->bb = pmem::obj::make_persistent<BurstBuffer::Persistent>();
    });
  }

  // Burst Buffer
  BurstBuffer bb{r->bb.get()};
  bb.runtimeInitialize();

  std::thread flusher_thread([&]() {
    BurstBuffer::Flusher flusher(bb);
    for (size_t i = 0; i < 1000; ++i) {
      flusher.flushAll([&](ListNode&& node) {
        // fmt::print("flushing: ofs: {}, data: {}\n", node.offset(),
        //            std::string{node.sv()});
        return false;
      });
    }
  });

  std::thread writer_thread([&]() {
    BurstBuffer::Writer writer(bb);
    for (size_t i = 0; i < 1000; ++i) {
      writer.pwrite("hello", 0);
      writer.pwrite("burst buffer", 5);

      // writer.forEach([&](ListNode&& node) {
      //   fmt::print("wrote: ofs: {}, data: {}\n", node.offset(),
      //   std::string{node.sv()}); return false;
      // });

      writer.tryEnqueueFlush();
    }
  });
  writer_thread.join();
  flusher_thread.join();

  // pool close
  try {
    pop.close();
  } catch (const std::logic_error& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
