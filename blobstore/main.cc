#include <fmt/ranges.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <iostream>

#include "blobstore-rs/src/bridge.rs.h"
#include "blobstore/blobstore.h"

using folly::coro::blockingWait;
using folly::coro::collectAll;
// using org::blobstore::MultiBuf;
// using org::blobstore::new_blobstore_client;
// using org::blobstore::VecU8;
int main() {
  auto create_buf = [](std::string&& str1, std::string&& str2) {
    VecU8 v1, v2;
    std::ranges::move(str1, std::back_inserter(v1.value));
    std::ranges::move(str2, std::back_inserter(v2.value));
    return MultiBuf{rust::Vec{v1, v2}, 0};
  };

  // IO 线程 executor
  folly::ScopedEventBaseThread evb_thread("evb");
  auto* executor = evb_thread.getEventBase();

  auto client = new_blobstore_client();
  auto buf1 = create_buf("fearless", "concurrency");
  auto buf2 = create_buf("iterators", "closures");
  auto [id1, id2] = blockingWait(collectAll(
      client->put_coro(buf1).scheduleOn(executor),
      client->put_coro(buf2).scheduleOn(executor)));

  auto print_meta = [&client](uint64_t blobid, const std::string& tag) {
    client->tag(blobid, rust::String(tag));
    const auto metadata = client->metadata(blobid);
    std::vector<std::string> tags;
    std::transform(
        metadata.tags.cbegin(),
        metadata.tags.cend(),
        std::back_inserter(tags),
        [](const auto& elem) { return static_cast<std::string>(elem); });
    fmt::print("tags = [\"{}\"]\n", fmt::join(tags, "\", \""));
  };

  print_meta(id1, "threads");
  print_meta(id2, "functional");

  return 0;
}
