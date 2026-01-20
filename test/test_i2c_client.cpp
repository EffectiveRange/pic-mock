#include <catch2/catch.hpp>

#include "i2c_app.h"
#include "mock_hw.hpp"
#include "version.h"
#include "xc.h"

#include <concepts>
#include <cstdint>
#include <ranges>
#include <thread>
#include <vector>

/*
std::vector<uint8_t>
perform_i2c_host_read(uint8_t address, uint8_t len,
                      std::optional<uint8_t> error_loc = {}) {
  std::vector<uint8_t> res;
  res.reserve(len);
  I2C1STAT0bits.R = 0;
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH);
  I2C1RXB = address;
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_RX_READY);
  for (uint8_t i = 0; i < len; i++) {
    if (error_loc.has_value() && error_loc.value() == i) {
      isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_ERROR);
      break;
    }
    isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_TX_READY);
    res.push_back(I2C1TXB);
  }
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED);
  hw_interrupt();
  return res;
}

template <std::ranges::input_range Rng>
  requires std::convertible_to<std::ranges::range_value_t<Rng>, uint8_t>
void perform_i2c_write(uint8_t address, Rng &&data,
                       std::optional<uint8_t> error_loc = {}) {
  I2C1STAT0bits.R = 0;
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH);
  I2C1RXB = address;
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_RX_READY);
  std::size_t i = 0;
  for (auto const &val : data) {
    uint8_t byte = static_cast<uint8_t>(val);
    I2C1STAT0bits.R = 0;
    I2C1RXB = byte;
    if (error_loc.has_value() && error_loc.value() == i) {
      isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_ERROR);
      break;
    }
    isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_RX_READY);
    i++;
  }
  isr_i2c_client_application(I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED);
  hw_interrupt();
}

TEST_CASE("read from i2c", "[i2cclient]") {
  const auto res = perform_i2c_host_read(0x11, 3);
  REQUIRE(res.size() == 3);
  REQUIRE(res[0] == ER_FW_MAJOR);
  REQUIRE(res[1] == ER_FW_MINOR);
  REQUIRE(res[2] == ER_FW_PATCH);
}

TEST_CASE("write to i2c", "[i2cclient]") {
  perform_i2c_write(0xE, std::array{0x01, 0x02, 0x03});
  REQUIRE(wait_on_main([] { return i2c_reg_map[0x0E]; }, 0x01) == 0x01);
  REQUIRE(wait_on_main([] { return i2c_reg_map[0x0F]; }, 0x02) == 0x02);
  REQUIRE(wait_on_main([] { return i2c_reg_map[0x10]; }, 0x03) == 0x03);
}

TEST_CASE("clear on read", "[i2cclient]") {
  const auto regs_before = perform_i2c_host_read(REG_STAT_0_ADDR, 1);
  REQUIRE(regs_before.size() == 1);
  on_main_thread(
      [] { i2c_client_write_register_byte(REG_STAT_0_ADDR, 0X03); })();
  REQUIRE(wait_on_main([] { return i2c_shadow_map[REG_STAT_0_ADDR]; }, 0x03) ==
          0x03);
  const auto regs_after = perform_i2c_host_read(REG_STAT_0_ADDR, 1);
  REQUIRE(regs_after.size() == 1);
  REQUIRE(regs_after[0] == 0x03);
  const auto regs_after_cor = perform_i2c_host_read(REG_STAT_0_ADDR, 1);
  REQUIRE(regs_after_cor.size() == 1);
  REQUIRE(regs_after_cor[0] == 0x00);
}

TEST_CASE("clear on read with mask", "[i2cclient]") {
  i2c_shadow_map[REG_STAT_I2C_ERR_AND_STICKY_ADDR] = 0x83;
  const auto reg = perform_i2c_host_read(REG_STAT_I2C_ERR_AND_STICKY_ADDR, 1);
  REQUIRE(reg.size() == 1);
  REQUIRE(reg[0] == 0x83);
  for (int i = 0; i < 3; ++i) {
    const auto reg_after =
        perform_i2c_host_read(REG_STAT_I2C_ERR_AND_STICKY_ADDR, 1);
    REQUIRE(reg_after.size() == 1);
    REQUIRE(reg_after[0] == 0x80);
  }
}

TEST_CASE("errors during i2c xfer", "[i2cclient]") {
  perform_i2c_write(0xE, std::array{0x01, 0x02, 0x03}, 0);
  REQUIRE(
      wait_on_main([] { return i2c_reg_map[REG_STAT_I2C_ERR_UNKNOWN_CNTR]; },
                   0x01) == 0x01);
  const auto read1 = perform_i2c_host_read(REG_STAT_I2C_ERR_UNKNOWN_CNTR, 1);
  REQUIRE(read1.size() == 1);
  REQUIRE(read1[0] == 0x01);
  REQUIRE(
      wait_on_main([] { return i2c_reg_map[REG_STAT_I2C_ERR_UNKNOWN_CNTR]; },
                   0x00) == 0x00);
  const auto read2 = perform_i2c_host_read(REG_STAT_I2C_ERR_UNKNOWN_CNTR, 1);
  REQUIRE(read2.size() == 1);
  REQUIRE(read2[0] == 0x00);
  REQUIRE(
      wait_on_main([] { return i2c_reg_map[REG_STAT_I2C_ERR_UNKNOWN_CNTR]; },
                   0x00) == 0x00);
}
*/
// TEST_CASE("test write with event listener", "[i2cclient]") {
//   hb_pulse();
//   REQUIRE(wait_on_main([] { return rpi_get_heartbeat_status(); },
//                        PI_HB_STATUS_OK) == PI_HB_STATUS_OK);
//   perform_i2c_write(REG_CMD_HALT_ACTION,
//   std::array{REG_VAL_HALT_ACTION_RST}); REQUIRE(wait_on_main([] { return
//   rpi_get_heartbeat_status(); },
//                        PI_HB_STATUS_SHUT_REQUESTED) ==
//           PI_HB_STATUS_SHUT_REQUESTED);
// }