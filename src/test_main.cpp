
#include "xc.h"
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <future>

extern void hw_interrupt();
extern "C" void tasks_deinitialize(void);

#ifndef PIC_HW_MAIN_ENTRY_POINT
#define PIC_HW_MAIN_ENTRY_POINT hw_main
#endif

extern void PIC_HW_MAIN_ENTRY_POINT(std::promise<void>);

int main(int argc, char *argv[]) {
  std::promise<void> pr;
  auto fut = pr.get_future();
  auto mainfut =
      std::async(std::launch::async, PIC_HW_MAIN_ENTRY_POINT, std::move(pr));
  fut.get();
  const auto res = Catch::Session().run(argc, argv);
  MAIN_THREAD_CYCLE_BEGIN();
  MAIN_THREAD_EXCLUSIVE_BEGIN();
  tasks_deinitialize();
  MAIN_THREAD_EXCLUSIVE_END();
  MAIN_THREAD_CYCLE_END();
  hw_interrupt();
  return res;
}
