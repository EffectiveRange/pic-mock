#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "mock_hw.hpp"

namespace {
std::mutex main_mutex;
std::mutex irq_mutex;
std::condition_variable main_cv;
std::atomic<bool> main_running = true;

using callback_t = void (*)(void);

class TMR0_mock {
public:
  TMR0_mock(std::chrono::microseconds period) : m_period(period) {
    m_fut = std::async(std::launch::async, [this]() {
      while (!m_stopped) {
        if (m_running) {
          const auto start = std::chrono::high_resolution_clock::now();
          while (std::chrono::high_resolution_clock::now() - start < m_period) {
            std::this_thread::yield();
          }
          tick();
        } else {
          std::unique_lock lck(mtx);
          cv.wait(lck, [this] { return m_running || m_stopped; });
        }
      }
    });
  }
  void Write(uint16_t val) {
    m_val = val;
    // Do nothing
  }
  uint16_t Read(void) { return m_val.load(); }
  void Stop(void) {
    m_running = false;
    cv.notify_all();
  }
  void Start(void) {
    // Do nothing
    m_running = true;
    cv.notify_all();
  }
  void OverflowCallbackRegister(callback_t cb) { m_cb = cb; }

  void tick() {
    const auto old = m_val.fetch_add(1);
    if (old == std::numeric_limits<uint16_t>::max() && m_cb) {
      m_cb();
      hw_interrupt();
    }
  }

  ~TMR0_mock() {
    m_stopped = true;
    cv.notify_all();
    if (m_fut.valid()) {
      m_fut.wait();
    }
  }

  auto get_period() const { return m_period; }
  void set_period(std::chrono::microseconds period) { m_period = period; }
  void set_reload_value(uint16_t val) { m_reload = val; }
  void reload() { m_val = m_reload.load(); }

private:
  std::atomic<uint16_t> m_val = 0;
  std::atomic<uint16_t> m_reload = 0;
  callback_t m_cb = nullptr;
  std::mutex mtx;
  std::condition_variable cv;
  std::atomic<bool> m_running = false;
  std::atomic<bool> m_stopped = false;
  std::chrono::microseconds m_period;
  std::future<void> m_fut;
};

std::unique_ptr<TMR0_mock> tmr0;

static std::chrono::microseconds tmr0_period = std::chrono::microseconds(1);

} // namespace

std::chrono::microseconds TMR0_set_time_base(std::chrono::microseconds period) {
  std::lock_guard lck(main_mutex);
  const auto old_period = std::exchange(tmr0_period, period);
  if (tmr0) {
    tmr0->set_period(period);
  }
  return old_period;
}

extern "C" {
void TMR0_Write(uint16_t val) { tmr0->Write(val); }
uint16_t TMR0_Read(void) { return tmr0->Read(); }
void TMR0_Stop(void) {
  if (tmr0)
    tmr0->Stop();
}
void TMR0_Start(void) { tmr0->Start(); }
void TMR0_OverflowCallbackRegister(void (*callback)(void)) {
  std::lock_guard lck(main_mutex);
  tmr0 = std::make_unique<TMR0_mock>(tmr0_period);
  tmr0->OverflowCallbackRegister(callback);
}

uint16_t TMR0_CounterGet(void) { return TMR0_Read(); }

void TMR0_CounterSet(uint16_t counterValue) { TMR0_Write(counterValue); }

void TMR0_PeriodSet(uint16_t periodCount) {
  tmr0->set_reload_value(periodCount);
}

void TMR0_Reload(void) { tmr0->reload(); }

void execSleep() {
  std::unique_lock lck(main_mutex, std::adopt_lock);
  main_running = false;
  main_cv.wait(lck, [] { return main_running.load(); });
  lck.release();
}

void _MAIN_THREAD_CYCLE_BEGIN() { main_mutex.lock(); }
void _MAIN_THREAD_CYCLE_END() { main_mutex.unlock(); }
void _MAIN_THREAD_EXCLUSIVE_BEGIN() { irq_mutex.lock(); }
void _MAIN_THREAD_EXCLUSIVE_END() { irq_mutex.unlock(); }
}

std::mutex &get_main_mutex() { return main_mutex; }

void advance_main_thread() {
  main_running = true;
  main_cv.notify_one();
}

void hw_interrupt() { advance_main_thread(); }
