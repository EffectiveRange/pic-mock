#include <bits/types/timer_t.h>
#include <catch2/catch.hpp>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <sys/types.h>
#include <thread>
#include <vector>

#include "mock_hw.hpp"
#include "timers.h"

using namespace std::chrono;
using namespace std::chrono_literals;

using testclock = std::chrono::high_resolution_clock;
using timepoint = testclock::time_point;

struct timer_cb_data {
  struct tw_timer_t *first = nullptr;
  timepoint second = {};
  uint16_t time_ms = 0;
};

template <typename T> struct ts_queue {
private:
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<T> queue;

public:
  void clear() {
    std::lock_guard lck(mtx);
    while (!queue.empty()) {
      queue.pop();
    }
  }
  bool empty() {
    std::lock_guard lck(mtx);
    return queue.empty();
  }
  size_t size() {
    std::lock_guard lck(mtx);
    return queue.size();
  }
  void push(T item) {
    std::lock_guard lck(mtx);
    queue.push(std::move(item));
    cv.notify_one();
  }
  template <typename... Args> void emplace(Args &&...args) {
    std::lock_guard lck(mtx);
    queue.emplace(std::forward<Args>(args)...);
    cv.notify_one();
  }
  T pop() {
    std::unique_lock lck(mtx);
    cv.wait(lck, [this] { return !queue.empty(); });
    auto item = std::move(queue.front());
    queue.pop();
    return item;
  }
  template <typename Duration> std::optional<T> pop_for(Duration d) {
    std::unique_lock lck(mtx);
    if (!cv.wait_for(lck, d, [this] { return !queue.empty(); })) {
      return {};
    }
    auto item = std::move(queue.front());
    queue.pop();
    return item;
  }
};

ts_queue<timer_cb_data> timer_callbacks;

struct finally_reset_timers {
  ~finally_reset_timers() { timers_reset(); }
};

struct tw_timer_t *cb(struct tw_timer_t *timer) {
  timer_callbacks.emplace(timer, testclock::now(), timer->time_ms);
  return NULL;
}
struct tw_timer_t *cb_again(struct tw_timer_t *timer) {
  timer_callbacks.emplace(timer, testclock::now(), timer->time_ms);
  auto &cnt = *reinterpret_cast<int *>(timer->user_data);
  cnt--;
  return cnt > 0 ? timer : NULL;
}

template <typename Duration> auto get_single_timer(Duration ms) {
  auto timer = std::make_unique<tw_timer_t>();
  timer->time_ms =
      static_cast<uint16_t>(duration_cast<milliseconds>(ms).count());
  timer->callback = cb;
  return timer;
}

template <typename Duration> auto get_repeating_timer(Duration ms, int *cnt) {
  auto timer = std::make_unique<tw_timer_t>();
  timer->time_ms =
      static_cast<uint16_t>(duration_cast<milliseconds>(ms).count());
  timer->callback = cb_again;
  timer->user_data = cnt;
  return timer;
}

void enusre_no_active_timers() {
  std::lock_guard lck(get_main_mutex());
  REQUIRE(head_timer() == sentinel_timer());
  REQUIRE(sentinel_timer()->next == NULL);
  REQUIRE(sentinel_timer()->prev == NULL);
  REQUIRE(sentinel_timer()->time_ms == 0);
  REQUIRE(sentinel_timer()->expired == false);
  timers_reset();
}

TEST_CASE("add_future_timer_to_wheel", "[timers]") {
  finally_reset_timers frt;
  auto timer = get_single_timer(1000ms);
  timer_callbacks.clear();
  SECTION("single timer init") {
    on_main_thread(add_timer)(timer.get());
    const auto start = testclock::now();
    auto res = timer_callbacks.pop_for(2s);
    REQUIRE(res.has_value());
    REQUIRE(res->first == timer.get());
    REQUIRE(res->time_ms == 1000);
    REQUIRE(timer_callbacks.empty());
  }
  SECTION("multi timer close to each other") {
    auto timer2 = get_single_timer(1000ms);
    on_main_thread(add_timer)(timer.get());
    on_main_thread(add_timer)(timer2.get());
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(res1->first == timer.get());
    REQUIRE(res2->first == timer2.get());
    REQUIRE(res1->time_ms == 1000);
    REQUIRE(res2->time_ms == 1000);
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 5ms);
    }
  }

  SECTION("multi timer 2nd sooner") {
    auto timer2 = get_single_timer(500ms);
    const auto start = testclock::now();
    on_main_thread(add_timer)(timer.get());
    on_main_thread(add_timer)(timer2.get());
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(res1->first == timer2.get());
    REQUIRE(res2->first == timer.get());
    REQUIRE(res1->time_ms == 500);
    REQUIRE(res2->time_ms == 1000);
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 400ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 800ms);
    }
  }
  SECTION("multi timer 2nd later") {
    auto timer2 = get_single_timer(2000ms);
    auto timer3 = get_single_timer(1000ms);
    const auto start = testclock::now();
    on_main_thread(add_timer)(timer.get());
    on_main_thread(add_timer)(timer2.get());
    SECTION("vanilla") {
      auto res1 = timer_callbacks.pop_for(2s);
      auto res2 = timer_callbacks.pop_for(2s);
      REQUIRE(res1.has_value());
      REQUIRE(res2.has_value());
      REQUIRE(res1->first == timer.get());
      REQUIRE(res2->first == timer2.get());
      {
        const auto diff = res2->second - res1->second;
        REQUIRE(duration_cast<milliseconds>(diff) >= 800ms);
        REQUIRE(duration_cast<milliseconds>(diff) <= 1500ms);
      }
    }
    SECTION("3rd timer close to first") {
      on_main_thread(add_timer)(timer3.get());
      auto res1 = timer_callbacks.pop_for(2s);
      auto res2 = timer_callbacks.pop_for(2s);
      auto res3 = timer_callbacks.pop_for(2s);
      REQUIRE(res1.has_value());
      REQUIRE(res2.has_value());
      REQUIRE(res3.has_value());
      REQUIRE(timer_callbacks.empty());
      REQUIRE(res1->first == timer.get());
      REQUIRE(res2->first == timer3.get());
      REQUIRE(res3->first == timer2.get());
    }
    SECTION("3rd timer in between") {
      timer3->time_ms = 1500;
      on_main_thread(add_timer)(timer3.get());
      auto res1 = timer_callbacks.pop_for(2s);
      auto res2 = timer_callbacks.pop_for(2s);
      auto res3 = timer_callbacks.pop_for(2s);
      REQUIRE(res1.has_value());
      REQUIRE(res2.has_value());
      REQUIRE(res3.has_value());
      REQUIRE(timer_callbacks.empty());
      REQUIRE(res1->first == timer.get());
      REQUIRE(res2->first == timer3.get());
      REQUIRE(res3->first == timer2.get());
    }
    SECTION("1 closer than inbetween") {
      on_main_thread(add_timer)(timer3.get());
      auto timer4 = get_single_timer(1500ms);
      on_main_thread(add_timer)(timer4.get());
      auto res1 = timer_callbacks.pop_for(2s);
      auto res2 = timer_callbacks.pop_for(2s);
      auto res3 = timer_callbacks.pop_for(2s);
      auto res4 = timer_callbacks.pop_for(2s);
      REQUIRE(res1.has_value());
      REQUIRE(res2.has_value());
      REQUIRE(res3.has_value());
      REQUIRE(res4.has_value());
      REQUIRE(timer_callbacks.empty());
      REQUIRE(res1->first == timer.get());
      REQUIRE(res2->first == timer3.get());
      REQUIRE(res3->first == timer4.get());
      REQUIRE(res4->first == timer2.get());
    }
  }
  enusre_no_active_timers();
}

TEST_CASE("rearm timer from callback", "[timers]") {
  finally_reset_timers frt;

  int cnt = 3;
  auto timer = get_repeating_timer(1000ms, &cnt);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer.get());
  auto res1 = timer_callbacks.pop_for(2s);
  auto res2 = timer_callbacks.pop_for(2s);
  auto res3 = timer_callbacks.pop_for(2s);
  REQUIRE(res1.has_value());
  REQUIRE(res2.has_value());
  REQUIRE(res3.has_value());
  REQUIRE(timer_callbacks.empty());
  REQUIRE(res1->first == timer.get());
  REQUIRE(res2->first == timer.get());
  REQUIRE(res3->first == timer.get());
  enusre_no_active_timers();
}

TEST_CASE("rearm timer from callback while there are multiple expired ones",
          "[timers]") {
  finally_reset_timers frt;

  int cnt = 2;
  auto timer = get_repeating_timer(1000ms, &cnt);
  auto timer2 = get_single_timer(1000ms);
  auto timer3 = get_single_timer(1000ms);
  timer_callbacks.clear();
  {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
  }
  const auto start = testclock::now();
  auto res1 = timer_callbacks.pop_for(2s);
  auto res2 = timer_callbacks.pop();
  auto res3 = timer_callbacks.pop();
  auto res4 = timer_callbacks.pop_for(2s);
  REQUIRE(res1.has_value());
  REQUIRE(res4.has_value());
  REQUIRE(timer_callbacks.empty());

  REQUIRE(res1->first == timer.get());
  REQUIRE(res2.first == timer2.get());
  REQUIRE(res3.first == timer3.get());
  REQUIRE(res4->first == timer.get());
  enusre_no_active_timers();
}

TEST_CASE(
    "rearm timer from callback while there are expired ones and one active",
    "[timers]") {
  finally_reset_timers frt;

  int cnt = 2;
  auto timer = get_repeating_timer(1000ms, &cnt);
  auto timer2 = get_single_timer(1000ms);
  auto timer3 = get_single_timer(1000ms);
  timer_callbacks.clear();
  {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer.get());
    add_timer(timer2.get());
  }
  std::this_thread::sleep_for(4ms);
  on_main_thread(add_timer)(timer3.get());
  const auto start = testclock::now();
  auto res1 = timer_callbacks.pop_for(2s);
  auto res2 = timer_callbacks.pop();
  auto res3 = timer_callbacks.pop();
  auto res4 = timer_callbacks.pop_for(2s);
  REQUIRE(res1.has_value());
  REQUIRE(res4.has_value());
  REQUIRE(timer_callbacks.empty());

  REQUIRE(res1->first == timer.get());
  REQUIRE(res2.first == timer2.get());
  REQUIRE(res3.first == timer3.get());
  REQUIRE(res4->first == timer.get());
  enusre_no_active_timers();
}

TEST_CASE("readd timer from main thread", "[timers]") {
  finally_reset_timers frt;

  int cnt = 3;
  auto timer = get_single_timer(500ms);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer.get());
  auto res = timer_callbacks.pop_for(2s);
  REQUIRE(timer_callbacks.empty());
  REQUIRE(res.has_value());
  REQUIRE(res->first == timer.get());
  on_main_thread(add_timer)(timer.get());
  auto res2 = timer_callbacks.pop_for(200ms);
  REQUIRE_FALSE(res2.has_value());
  REQUIRE(timer_callbacks.empty());
  on_main_thread(add_timer)(timer.get());
  auto res3 = timer_callbacks.pop_for(2s);
  REQUIRE(timer_callbacks.empty());
  REQUIRE(res3.has_value());
  REQUIRE(res3->first == timer.get());
  enusre_no_active_timers();
}

TEST_CASE("remove timer from main thread before expiry", "[timers]") {
  finally_reset_timers frt;

  auto timer = get_single_timer(1000ms);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer.get());
  auto res1 = timer_callbacks.pop_for(200ms);
  REQUIRE_FALSE(res1.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  on_main_thread(remove_timer)(timer.get());
  auto res2 = timer_callbacks.pop_for(2000ms);
  REQUIRE_FALSE(res2.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  enusre_no_active_timers();
}

TEST_CASE("remove timer from main while there's another timer", "[timers]") {
  finally_reset_timers frt;

  auto timer = get_single_timer(1000ms);
  auto timer2 = get_single_timer(2000ms);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer.get());
  on_main_thread(add_timer)(timer2.get());
  auto res1 = timer_callbacks.pop_for(200ms);
  REQUIRE_FALSE(res1.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  on_main_thread(remove_timer)(timer.get());
  auto res2 = timer_callbacks.pop_for(5000ms);
  REQUIRE(res2.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  REQUIRE(res2->first == timer2.get());
  enusre_no_active_timers();
}

TEST_CASE(
    "remove timer from main while there's another timer with similar expiry",
    "[timers]") {
  finally_reset_timers frt;

  auto timer = get_single_timer(1000ms);
  auto timer2 = get_single_timer(1000ms);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer.get());
  on_main_thread(add_timer)(timer2.get());
  auto res1 = timer_callbacks.pop_for(200ms);
  REQUIRE_FALSE(res1.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  on_main_thread(remove_timer)(timer.get());
  auto res2 = timer_callbacks.pop_for(5000ms);
  REQUIRE(res2.has_value());
  REQUIRE(timer_callbacks.size() == 0);
  REQUIRE(res2->first == timer2.get());
  enusre_no_active_timers();
}

TEST_CASE("multiple timers added", "[timers]") {
  finally_reset_timers frt;

  auto timer1 = get_single_timer(50ms);
  auto timer2 = get_single_timer(50ms);
  auto timer3 = get_single_timer(100ms);
  auto timer4 = get_single_timer(100ms);
  auto timer5 = get_single_timer(100ms);

  timer_callbacks.clear();

  SECTION("scenario 1") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer5.get());
  }
  SECTION("scenario 2") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer1.get());
    add_timer(timer3.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
    add_timer(timer5.get());
  }
  SECTION("scenario 3") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
    add_timer(timer5.get());
  }
  SECTION("scenario 4") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer5.get());
  }
  SECTION("scenario 5") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 6") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer4.get());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 7") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 8") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 9") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 10") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
  }
  SECTION("scenario 11") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 12") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 13") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer4.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 14") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 15") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 16") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer1.get());
    add_timer(timer4.get());
    add_timer(timer3.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 17") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 18") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 19") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 20") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
  }
  SECTION("scenario 21") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 22") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer3.get());
    add_timer(timer2.get());
    add_timer(timer1.get());
    add_timer(timer4.get());
  }
  SECTION("scenario 23") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
  }
  SECTION("scenario 24") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
    add_timer(timer2.get());
    add_timer(timer1.get());
  }
  SECTION("scenario 25") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer4.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 26") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer4.get());
    add_timer(timer2.get());
    add_timer(timer1.get());
    add_timer(timer3.get());
  }
  SECTION("scenario 27") {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer5.get());
    add_timer(timer4.get());
    add_timer(timer3.get());
    add_timer(timer1.get());
    add_timer(timer2.get());
  }
  auto res1 = timer_callbacks.pop_for(2s);
  auto res2 = timer_callbacks.pop_for(2s);
  auto res3 = timer_callbacks.pop_for(2s);
  auto res4 = timer_callbacks.pop_for(2s);
  auto res5 = timer_callbacks.pop_for(2s);
  REQUIRE(res1.has_value());
  REQUIRE(res2.has_value());
  REQUIRE(res3.has_value());
  REQUIRE(res4.has_value());
  REQUIRE(res5.has_value());
  REQUIRE(timer_callbacks.empty());
  const auto f1 = res1->first == timer1.get() || res1->first == timer2.get();
  const auto f2 = res2->first == timer1.get() || res2->first == timer2.get();
  REQUIRE(f1);
  REQUIRE(f2);
  const auto t1 = res3->first == timer3.get() || res3->first == timer4.get() ||
                  res3->first == timer5.get();
  const auto t2 = res4->first == timer3.get() || res4->first == timer4.get() ||
                  res4->first == timer5.get();
  const auto t3 = res5->first == timer3.get() || res5->first == timer4.get() ||
                  res5->first == timer5.get();
  REQUIRE(t1);
  REQUIRE(t2);
  REQUIRE(t3);
  REQUIRE(res1->time_ms == 50);
  REQUIRE(res2->time_ms == 50);
  REQUIRE(res3->time_ms == 100);
  REQUIRE(res4->time_ms == 100);
  REQUIRE(res5->time_ms == 100);

  {
    const auto diff = res2->second - res1->second;
    REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
    REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
  }
  {
    const auto diff = res3->second - res2->second;
    REQUIRE(duration_cast<milliseconds>(diff) >= 40ms);
    REQUIRE(duration_cast<milliseconds>(diff) <= 200ms);
  }
  {
    const auto diff = res4->second - res3->second;
    REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
    REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
  }
  {
    const auto diff = res5->second - res4->second;
    REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
    REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
  }
  enusre_no_active_timers();
}

TEST_CASE("remove timer complex", "[timers]") {
  finally_reset_timers frt;

  auto timer1 = get_single_timer(50ms);
  auto timer2 = get_single_timer(50ms);
  auto timer3 = get_single_timer(100ms);
  auto timer4 = get_single_timer(200ms);
  auto timer5 = get_single_timer(600ms);
  timer_callbacks.clear();
  {
    std::lock_guard lck(get_main_mutex());
    add_timer(timer1.get());
    add_timer(timer2.get());
    add_timer(timer3.get());
    add_timer(timer4.get());
  }
  std::this_thread::sleep_for(10ms);
  SECTION("remove head timer") {
    on_main_thread(remove_timer)(timer1.get());
    auto res0 = timer_callbacks.pop_for(2s);
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res0.has_value());
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(timer_callbacks.size() == 0);
    REQUIRE(res0->first == timer2.get());
    REQUIRE(res1->first == timer3.get());
    REQUIRE(res1->first == timer3.get());
    REQUIRE(res0->time_ms == 50);
    REQUIRE(res1->time_ms == 100);
    REQUIRE(res2->time_ms == 200);
    {
      const auto diff = res1->second - res0->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 25ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 100ms);
    }
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 50ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 200ms);
    }
  }
  SECTION("remove second timer") {
    on_main_thread(remove_timer)(timer2.get());
    auto res0 = timer_callbacks.pop_for(2s);
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res0.has_value());
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(timer_callbacks.size() == 0);
    REQUIRE(res0->first == timer1.get());
    REQUIRE(res1->first == timer3.get());
    REQUIRE(res2->first == timer4.get());
    REQUIRE(res0->time_ms == 50);
    REQUIRE(res1->time_ms == 100);
    REQUIRE(res2->time_ms == 200);
    {
      const auto diff = res1->second - res0->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 25ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 100ms);
    }
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 50ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 200ms);
    }
  }
  SECTION("remove third timer") {
    on_main_thread(remove_timer)(timer3.get());
    auto res0 = timer_callbacks.pop_for(2s);
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res0.has_value());
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(timer_callbacks.size() == 0);
    REQUIRE(res0->first == timer1.get());
    REQUIRE(res1->first == timer2.get());
    REQUIRE(res2->first == timer4.get());
    REQUIRE(res0->time_ms == 50);
    REQUIRE(res1->time_ms == 50);
    REQUIRE(res2->time_ms == 200);
    {
      const auto diff = res1->second - res0->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
    }
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 150ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 300ms);
    }
  }
  SECTION("remove last timer") {
    on_main_thread(remove_timer)(timer4.get());
    auto res0 = timer_callbacks.pop_for(2s);
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    REQUIRE(res0.has_value());
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(timer_callbacks.size() == 0);
    REQUIRE(res0->first == timer1.get());
    REQUIRE(res1->first == timer2.get());
    REQUIRE(res2->first == timer3.get());
    REQUIRE(res0->time_ms == 50);
    REQUIRE(res1->time_ms == 50);
    REQUIRE(res2->time_ms == 100);
    {
      const auto diff = res1->second - res0->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
    }
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 25ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 100ms);
    }
  }
  SECTION("remove timer that was not added at all") {
    on_main_thread(remove_timer)(timer5.get());
    auto res0 = timer_callbacks.pop_for(2s);
    auto res1 = timer_callbacks.pop_for(2s);
    auto res2 = timer_callbacks.pop_for(2s);
    auto res3 = timer_callbacks.pop_for(2s);
    REQUIRE(res0.has_value());
    REQUIRE(res1.has_value());
    REQUIRE(res2.has_value());
    REQUIRE(res3.has_value());
    REQUIRE(timer_callbacks.size() == 0);
    REQUIRE(res0->first == timer1.get());
    REQUIRE(res1->first == timer2.get());
    REQUIRE(res2->first == timer3.get());
    REQUIRE(res3->first == timer4.get());
    REQUIRE(res0->time_ms == 50);
    REQUIRE(res1->time_ms == 50);
    REQUIRE(res2->time_ms == 100);
    REQUIRE(res3->time_ms == 200);
    {
      const auto diff = res1->second - res0->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 0ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 10ms);
    }
    {
      const auto diff = res2->second - res1->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 25ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 100ms);
    }
    {
      const auto diff = res3->second - res2->second;
      REQUIRE(duration_cast<milliseconds>(diff) >= 100ms);
      REQUIRE(duration_cast<milliseconds>(diff) <= 300ms);
    }
  }
  enusre_no_active_timers();
}

TEST_CASE("remove timer overflow protection", "[timers]") {
  finally_reset_timers frt;

  auto timer1 = get_single_timer(3s);
  auto timer2 = get_single_timer(milliseconds(TIMER_MAX_MS + 1000));
  auto timer3 = get_single_timer(30s);
  const auto old = TMR0_set_time_base(1us);
  timer_callbacks.clear();
  on_main_thread(add_timer)(timer1.get());
  std::this_thread::sleep_for(50ms);
  on_main_thread(add_timer)(timer2.get());
  std::this_thread::sleep_for(50ms);
  on_main_thread(add_timer)(timer3.get());
  std::this_thread::sleep_for(50ms);
  on_main_thread(remove_timer)(timer1.get());
  auto res1 = timer_callbacks.pop_for(5s);
  auto res2 = timer_callbacks.pop_for(10s);
  REQUIRE(res1.has_value());
  REQUIRE(res2.has_value());
  REQUIRE(timer_callbacks.empty());
  REQUIRE(res1->first == timer2.get());
  REQUIRE(res2->first == timer3.get());
  TMR0_set_time_base(old);
}