#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
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
  TMR0_mock() {}
  void Write(uint16_t val) { m_val = val; }
  uint16_t Read(void) { return m_val.load(); }
  void Stop(void) { m_running = false; }
  void Start(void) {
    // Do nothing
    m_running = true;
  }
  void OverflowCallbackRegister(callback_t cb) { m_cb = cb; }

  void tick() {
    if (!m_running) {
      throw std::runtime_error("TMR0 tick called while stopped");
    }
    const auto old = m_val.fetch_add(1);
    if (old == std::numeric_limits<uint16_t>::max() && m_cb) {
      reload();
      m_cb();
      hw_interrupt();
    }
  }

  ~TMR0_mock() {}

  void set_reload_value(uint16_t val) { m_reload = val; }
  void reload() { m_val = m_reload.load(); }

private:
  std::atomic<uint16_t> m_val = 0;
  std::atomic<uint16_t> m_reload = 0;
  callback_t m_cb = nullptr;
  std::atomic<bool> m_running = false;
};

std::unique_ptr<TMR0_mock> tmr0;

} // namespace

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
  tmr0 = std::make_unique<TMR0_mock>();
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
void timer_tick() {
  if (tmr0) {
    tmr0->tick();
  }
}
