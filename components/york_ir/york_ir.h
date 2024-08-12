#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "york_data.h"

namespace esphome {
namespace york_ir {


class YorkIR : public climate_ir::ClimateIR {
 public:
  YorkIR()
      : climate_ir::ClimateIR(
            YORK_TEMPC_MIN, YORK_TEMPC_MAX, 1.0f, true, true,
            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
            {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP, climate::CLIMATE_PRESET_BOOST}) { }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;


 protected:

  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  void transmit_(YorkData &data);
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool on_york_(const YorkData &data);
};

}  // namespace midea_ir
}  // namespace esphome
