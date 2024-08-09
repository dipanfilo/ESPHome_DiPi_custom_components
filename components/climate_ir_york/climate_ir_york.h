#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include <cinttypes>

namespace esphome {
namespace york {

/*
 * The following typedef blocks define custom variable types which hold data
 * structures specific to this library.
 */

// Temperature
const uint8_t YORK_TEMP_MIN = 18;  // Celsius
const uint8_t YORK_TEMP_MAX = 30;  // Celsius

// The time_struct_t type is used to define a variable for storing the hour and
// minute of the current time. This will be used to send the date/time to the
// AC indoor unit.
struct time_struct_t {
    unsigned int hour;
    unsigned int minute;
};

// The timer_struct_t type is used to define a variable to hold the auto on/off
// data to send to the AC indoor unit. As with the remote controller, you can
// only define a time with 30 minute increments.
struct timer_struct_t {
    unsigned int hour;
    bool halfHour;
    bool active;
};

// The OperationMode type is used to define a variable to hold the desired
// operation mode of the AC.
enum OperationMode {
    OPERATION_MODE_DRY  = 0b0001,
    OPERATION_MODE_COOL = 0b0010,
    OPERATION_MODE_FAN  = 0b0100
};

// The FanMode type is used to define a variable to hold the fan mode of the
// AC.
enum FanMode {
    FAN_MODE_AUTO           = 0b0001,
    FAN_MODE_MANUAL_SPEED_3 = 0b0010,
    FAN_MODE_MANUAL_SPEED_2 = 0b0100,
    FAN_MODE_MANUAL_SPEED_1 = 0b1000,
    FAN_MODE_QUIET          = 0b1001,
    FAN_MODE_TURBO          = 0b0011,
};

// The SwingModeVertical is used to define a variable to hold the fan mode of the AC.
enum SwingModeVertical {
    SWING_MODE_VERTICAL_OFF  = 0b0000,
    SWING_MODE_VERTICAL_AUTO = 0b0001,
};


class YorkClimate : public climate_ir::ClimateIR {
 public:
  YorkClimate()
      : climate_ir::ClimateIR(YORK_TEMP_MIN, YORK_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MIDDLE, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH, climate::CLIMATE_FAN_QUIET},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                              {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST,climate::CLIMATE_PRESET_SLEEP}) 
                              {}

  void set_supports_cool(bool supports_cool) { this->supports_cool_ = supports_cool; }
  void set_supports_dry(bool supports_dry) { this->supports_dry_ = supports_dry; }
  void set_supports_fan_only(bool supports_fan_only) { this->supports_fan_only_ = supports_fan_only; }
  void set_supports_heat(bool supports_heat) { this->supports_heat_ = supports_heat; }

  void set_fan_mode(fan_mode_t fan_mode) { this->fan_mode_ = fan_mode; }

  void set_vertical_default(VerticalDirection vertical_direction) {
    this->default_vertical_direction_ = vertical_direction;
  }

 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state() override;
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);

  fan_mode_t fan_mode_;

  VerticalDirection default_vertical_direction_;

  climate::ClimateTraits traits() override;
};

}  // namespace mitsubishi
}  // namespace esphome
