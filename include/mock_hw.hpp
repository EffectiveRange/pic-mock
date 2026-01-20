#pragma once

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

// THE PIC application must provide these functions
// to initialize, run, and deinitialize the application
extern "C" void init_application(void);
extern "C" void run_tasks(void);
extern "C" void deinitialize_modules(void);

std::mutex &get_main_mutex();

void advance_main_thread();

std::chrono::microseconds TMR0_set_time_base(std::chrono::microseconds period);

struct advance_main_guard {
  ~advance_main_guard() { advance_main_thread(); }
};

template <typename F, typename... Args>
[[nodiscard]] decltype(auto) on_main_thread(F &&f) {
  return [f = std::forward<F>(f)](auto &&...args) {
    std::lock_guard lck(get_main_mutex());
    advance_main_guard g;
    return f(std::forward<decltype(args)>(args)...);
  };
}

template <typename F, typename U, typename Duration = std::chrono::seconds>
auto wait_on_main(F &&reg_access, U val, Duration dur = std::chrono::seconds(1))
    -> std::decay_t<decltype(reg_access())> {
  auto start = std::chrono::high_resolution_clock::now();
  std::unique_lock lck(get_main_mutex());

  while (true) {
    const auto curr_val = reg_access();
    if (curr_val == val) {
      break;
    }
    if (std::chrono::high_resolution_clock::now() - start > dur) {
      return curr_val;
    }
    lck.unlock();
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    lck.lock();
  }
  return val;
}

void hw_interrupt();

void timer_tick();

enum class INTERRUPT_TYPE { RISING_EDGE, FALLING_EDGE, BOTH_EDGE };

#define DECLARE_HW_INTERRUPT_HANDLER_RISING(name)                              \
  constexpr inline auto mock_interrupt_type_##name() noexcept {                \
    return INTERRUPT_TYPE::RISING_EDGE;                                        \
  }                                                                            \
  extern "C" void name##_SetInterruptHandler(void (*callback)(void));          \
  void mock_set_##name(bool val)

#define DECLARE_HW_INTERRUPT_HANDLER_FALLING(name)                             \
  constexpr inline auto mock_interrupt_type_##name() noexcept {                \
    return INTERRUPT_TYPE::FALLING_EDGE;                                       \
  }                                                                            \
  extern "C" void name##_SetInterruptHandler(void (*callback)(void));          \
  void mock_set_##name(bool val)

#define DECLARE_HW_INTERRUPT_HANDLER_BOTH(name)                                \
  constexpr inline auto mock_interrupt_type_##name() noexcept {                \
    return INTERRUPT_TYPE::BOTH_EDGE;                                          \
  }                                                                            \
  extern "C" void name##_SetInterruptHandler(void (*callback)(void));          \
  void mock_set_##name(bool val);

#define DEFINE_HW_INTERRUPT_HANDLER(name)                                      \
  namespace {                                                                  \
  void (*name##_cb)(void) = nullptr;                                           \
  bool name##_pin = false;                                                     \
  std::mutex name##_pin_mtx;                                                   \
  };                                                                           \
  extern "C" void name##_SetInterruptHandler(void (*callback)(void)) {         \
    std::lock_guard lck(get_main_mutex());                                     \
    name##_cb = callback;                                                      \
  }                                                                            \
  void mock_set_##name(bool val) {                                             \
    std::lock_guard lck(name##_pin_mtx);                                       \
    const auto old = std::exchange(name##_pin, val);                           \
    const auto itype = mock_interrupt_type_##name();                           \
    if (itype == INTERRUPT_TYPE::FALLING_EDGE && old && !val && name##_cb) {   \
      name##_cb();                                                             \
    } else if (itype == INTERRUPT_TYPE::RISING_EDGE && !old && val &&          \
               name##_cb) {                                                    \
      name##_cb();                                                             \
    } else if (itype == INTERRUPT_TYPE::BOTH_EDGE && old != val &&             \
               name##_cb) {                                                    \
      name##_cb();                                                             \
    }                                                                          \
    hw_interrupt();                                                            \
  }
