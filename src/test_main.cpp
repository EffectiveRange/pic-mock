
#include "xc.h"
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <future>

extern void hw_interrupt();

#ifndef PIC_HW_MAIN_ENTRY_POINT
#define PIC_HW_MAIN_ENTRY_POINT test_main
#endif

extern void PIC_HW_MAIN_ENTRY_POINT();

volatile bool _task_main_running = true;
;

int main(int argc, char *argv[]) {
  std::promise<void> pr;
  auto mainfut = std::async(std::launch::async, PIC_HW_MAIN_ENTRY_POINT);
  const auto res = Catch::Session().run(argc, argv);
  _task_main_running = false;
  hw_interrupt();
  return res;
}
