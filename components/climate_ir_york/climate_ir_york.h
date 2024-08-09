#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "york_ir_typedef.h"

#include <cinttypes>

namespace esphome {
namespace york {

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

   YORKData settings;
 
 
 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state_() override;
  // Handle received IR Buffer
  bool on_receive_(remote_base::RemoteReceiveData data) override;
  climate::ClimateTraits traits_() override;
};

}  // namespace mitsubishi
}  // namespace esphome
