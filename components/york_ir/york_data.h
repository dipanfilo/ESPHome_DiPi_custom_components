#pragma once

#include "esphome/components/remote_base/york_protocol.h"
#include "esphome/core/log.h"



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

// Temperature
const float YORK_TEMPC_MIN = 16.0;  // Celsius
const float YORK_TEMPC_MAX = 30.0;  // Celsius

namespace esphome {
namespace york_ir {


class YorkIRData : public remote_base::YorkData {
 public:
  // Default constructor ()
  YorkIRData() : YorkData() {}
  // Copy from Base
  YorkIRData(const remote_base::YorkData &data) : remote_base::YorkData(data) {}

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
  enum VSwingMode : uint8_t {
    VSWING_OFF = 0b0000,
    VSWING_AUTO = 0b0001,
  };

  // The time_struct_t type is used to define a variable for storing the hour and
  // minute of the current time. This will be used to send the date/time to the
  // AC indoor unit.
  struct time_struct_t {
    unsigned int hour = 0;
    unsigned int minute = 0;
  };

  // The timer_struct_t type is used to define a variable to hold the auto on/off
  // data to send to the AC indoor unit. As with the remote controller, you can
  // only define a time with 30 minute increments.
  struct timer_struct_t {
    unsigned int hour = 0;
    bool halfHour = false;
    bool active = false;
  };

  const LogString *operation_mode_to_string(Mode mode) {
      switch (mode) {
      case MODE_COOL:
        return LOG_STR("MODE_COOL");
      case MODE_DRY:
        return LOG_STR("MODE_DRY");
      case MODE_FAN_ONLY:
        return LOG_STR("MODE_FAN_ONLY");
      default:
        return LOG_STR("UNKNOWN");
    }
  }
  const LogString *fan_mode_to_string(FanMode mode) {
    switch (mode) {
      case FAN_AUTO:
        return LOG_STR("FAN_AUTO");
      case FAN_LOW:
        return LOG_STR("FAN_LOW");
      case FAN_MEDIUM:
        return LOG_STR("FAN_MEDIUM");
      case FAN_HIGH:
        return LOG_STR("FAN_HIGH");
      case FAN_QUIET:
        return LOG_STR("FAN_QUIET");
      case FAN_TURBO:
        return LOG_STR("FAN_TURBO");
      default:
        return LOG_STR("UNKNOWN");
    }
  }
  const LogString *swing_mode_to_string(VSwingMode mode) {
    switch (mode) {
      case VSWING_OFF:
        return LOG_STR("VSWING_OFF");
      case VSWING_AUTO:
        return LOG_STR("VSWING_AUTO");
      default:
        return LOG_STR("UNKNOWN");
    }
  }

  // Make from initializer_list
  void setData(std::initializer_list<uint8_t> data) {
    std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
  }
  // Make from vector
  void setData(const std::vector<uint8_t> &data) {
    std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
  }
 

  void set_IR_mode(Mode mode) { 
    this->set_value_(1, mode, 0b1111, 0); 
  }
  Mode get_IR_mode() const { 
    return static_cast<Mode>(this->get_value_(1, 0b1111, 0)); 
  }

  void set_IR_fan_mode(FanMode mode) { 
    this->set_value_(1, mode, 0b1111, 4); 
  }
  FanMode get_IR_fan_mode() const { 
    return static_cast<FanMode>(this->get_value_(1, 0b1111, 4)); 
  }

  void set_IR_temp(uint8_t val) { 

    if((val <= YORK_TEMPC_MAX && val >= YORK_TEMPC_MIN)) {
      this->set_value_(6, (uint8_t)(val % 10), 0b1111, 0);
      this->set_value_(6, (uint8_t)(val / 10), 0b1111, 4);
    } else {
      this->set_value_(6, (uint8_t)(24 % 10), 0b1111, 0);
      this->set_value_(6, (uint8_t)(24 / 10), 0b1111, 4);
    }
  }
  float get_IR_temp() const { 
    return ((this->get_value_(6, 0b1111, 4) * 10) + (this->get_value_(6, 0b1111, 0))); 
  }

  void set_IR_power(bool value) { 
    this->set_value_(7, value, 0b0001, 3); 
  }
  bool get_IR_power() const { 
    return this->get_value_(7, 0b0001, 3); 
  }

  void set_IR_currentTime( uint8_t hour,  uint8_t minute) {
    if ((hour <= 24 && hour >= 0) && (minute <= 59 && minute >= 0)) {
      this->set_value_(2, (uint8_t)(minute % 10), 0b1111, 0);
      this->set_value_(2, (uint8_t)(minute / 10), 0b1111, 4);

      this->set_value_(3, (uint8_t)(hour % 10), 0b1111, 0);
      this->set_value_(3, (uint8_t)(hour / 10), 0b1111, 4);
    } else {
      this->set_value_(2, 0, 0);
      this->set_value_(3, 0, 0);
    }
  }
  time_struct_t get_IR_currentTime() {
    time_struct_t currentTime;
    currentTime.minute = ((this->get_value_(2, 0b1111, 4) * 10) + (this->get_value_(2, 0b1111, 0)));
    currentTime.hour = ((this->get_value_(3, 0b1111, 4) * 10) + (this->get_value_(3, 0b1111, 0))); 
    return currentTime;
  }
 
  void set_IR_OnTimer(uint8_t hour, bool halfhour, bool active) {
    if (hour <= 24 && hour >= 0) {
      this->set_value_(4, (uint8_t)(hour % 10), 0b1111, 0);
      this->set_value_(4, (uint8_t)(hour / 10), 0b0011, 4);
      this->set_value_(4, halfhour, 0b0001, 6);
      this->set_value_(4, active, 0b0001, 7);
    } else {
      this->set_value_(4, 0, 0);
    }
  }
  timer_struct_t get_IR_OnTimer() const {
    timer_struct_t onTimer;
    onTimer.hour =          ((this->get_value_(4, 0b0011, 4) * 10) + (this->get_value_(4, 0b1111, 0)));
    onTimer.halfHour = (bool)(this->get_value_(4, 0b0001, 6));
    onTimer.active   = (bool)(this->get_value_(4, 0b0001, 7));
    return onTimer;
  }    

  void set_IR_OffTimer(uint8_t hour, bool halfhour, bool active) {
    if (hour <= 24 && hour >= 0) {
      this->set_value_(5, (uint8_t)(hour % 10), 0b1111, 0);
      this->set_value_(5, (uint8_t)(hour / 10), 0b0011, 4);
      this->set_value_(5, halfhour, 0b0001, 6);
      this->set_value_(5, active, 0b0001, 7);
    } else {
      this->set_value_(5, 0, 0);
    }
  }
  timer_struct_t get_IR_OffTimer() const {
    timer_struct_t offTimer;
    offTimer.hour =          ((this->get_value_(5, 0b0011, 4) * 10) + (this->get_value_(5, 0b1111, 0)));
    offTimer.halfHour = (bool)(this->get_value_(5, 0b0001, 6));
    offTimer.active   = (bool)(this->get_value_(5, 0b0001, 7));
    return offTimer;
  }   
 
  void set_IR_Sleep(bool active) {
    this->set_value_(7, active, 0b0001, 1);
  }
  bool get_IR_Sleep() const {
    return (bool)(this->get_value_(7, 0b0001, 1));
  }

  void set_IR_Swing(bool active){
    this->set_value_(7, active, 0b0001, 0);
  }
  bool get_IR_Swing() const {
    return (bool)(this->get_value_(7, 0b0001, 0));
  } 

};

}  // namespace york_ir
}  // namespace esphome
