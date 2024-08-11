#pragma once

#include "esphome/components/remote_base/york_protocol.h"
#include "esphome/components/climate/climate_mode.h"



// BYTE 0: The binary data header is 8 bits long 0x16. It seems to be a binary
// representation of the ASCII character 'Synchronous Idle (SYN) control character' 
// BYTE 1: right nibble is for operation mode and left nibble is for fan mode
// BYTE 2: right nibble is the right digit of current time in minutes (0M)
// and the left nibble is the left digit of the current time in minutes (M0)
// BYTE 3: right nibble is the right digit of the current time in hours (0H)
// and the left nibble is the left digit of the current time in hours (H0)
// BYTE 4: right nibble is the right digit of the on timer time in hours
// and the first two bits of the left nibble is the left digit of the on
// timer time in hours. The third bit of the nibble is 1 when the on
// timer time is at half past the hour, else 0. The last bit is 1 only when
// the on timer is active
// BYTE 5: right nibble is the right digit of the off timer time in hours
// and the first two bits of the left nibble is the left digit of the off
// timer time in hours. The third bit of the nibble is 1 when the off
// timer time is at half past the hour, else 0. The last bit is 1 only when
// the off timer is active
// BYTE 6: Left nibble is the right digit (1s) of the temperature in
// Celcius and the right nibble is the left digit (10s) of the temperature
// in Celcius
// BYTE 7: right nibble is a concatenation of 4-bits: Louvre Swing On/Off +
// Sleep Mode + 1 + Power Toggle. Left nibble is the reverse bit order
// checksum of all the reverse bit order nibbles before it.


namespace esphome {
namespace york_ir {

using climate::ClimateMode;
using climate::ClimateFanMode;
using remote_base::YorkData;

class ControlData : public YorkData {
 public:
  // Default constructor ()
  ControlData() : YorkData() {}
  // Copy from Base
  ControlData(const YorkData &data) : YorkData(data) {}

  void set_temp(float temp);
  float get_temp() const;

  void set_mode(ClimateMode mode);
  ClimateMode get_mode() const;

  void set_fan_mode(ClimateFanMode mode);
  ClimateFanMode get_fan_mode() const;

  void fix();

 protected:
  enum Mode : uint8_t {
    MODE_COOL = 0b0010,
    MODE_DRY = 0b0001,
    MODE_FAN_ONLY = 0b0100,
  };
  enum FanMode : uint8_t {
    FAN_AUTO = 0b0001,
    FAN_LOW = 0b1000,
    FAN_MEDIUM = 0b0100,
    FAN_HIGH = 0b0010,
    FAN_QUIET = 0b1001,
    FAN_TURBO = 0b0011,
  };


  void set_mode_(Mode mode) { this->set_value_(1, mode, 0b1111, 0); }
  Mode get_mode_() const { return static_cast<Mode>(this->get_value_(1, 0b1111, 0)); }

  void set_fan_mode_(FanMode mode) { this->set_value_(1, mode, 0b1111, 4); }
  FanMode get_fan_mode_() const { return static_cast<FanMode>(this->get_value_(1, 0b1111, 4)); }

  void set_temp(uint8_t val) 
  { 

    if((val <= MAX_TEMP && val >= MIN_TEMP)) {
      this->set_value_(6, (uint8_t)(val % 10), 0b1111, 0);
      this->set_value_(6, (uint8_t)(val / 10), 0b1111, 4);
    } else {
      this->set_value_(6, (uint8_t)(24 % 10), 0b1111, 0);
      this->set_value_(6, (uint8_t)(24 / 10), 0b1111, 4);
    }
  }
  float get_temp() const { 
    return ((this->get_value_(6, 0b1111, 4) * 10) + (this->get_value_(6, 0b1111, 0))); 
  }

  void set_power_(bool value) { this->set_value_(7, value, 0b1, 7); }
  bool get_power_() const { return this->get_value_(7, 0b1, 7); }


  static const uint8_t MAX_TEMP = 30;
  static const uint8_t MIN_TEMP = 16;

  static const uint8_t VSWING_OFF = 0;
  static const uint8_t VSWING_AUTO = 1;
  
};





}  // namespace midea_ir
}  // namespace esphome
