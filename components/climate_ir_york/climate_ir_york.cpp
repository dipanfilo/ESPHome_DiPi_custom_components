#include "climate_ir_york.h"
#include "esphome/core/log.h"

namespace esphome {
namespace york {

static const char *const TAG = "york.climate";

climate::ClimateTraits YorkClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.set_supports_action(false);
  this->set_supports_heat(false);
  traits.set_visual_min_temperature(YORK_TEMP_MIN);
  traits.set_visual_max_temperature(YORK_TEMP_MAX);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF});

  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);

  if (this->supports_cool_ && this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);

  if (this->supports_dry_)
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->supports_fan_only_)
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  // Default to only 3 levels in ESPHome even if most unit supports 4. The 3rd level is not used.
  traits.set_supported_fan_modes(
      {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH, climate::CLIMATE_FAN_QUIET, climate::CLIMATE_FAN_FOCUS});

  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});

  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO,
                                climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_SLEEP});

  return traits;
}

void YorkClimate::transmit_state() {

  //OperationMode
  switch (this->mode) {
    case climate::CLIMATE_MODE_DRY:
      this->settings.setOperationMode(OPERATION_MODE_DRY);
      break;
    case climate::CLIMATE_MODE_COOL:
      this->settings.setOperationMode(OPERATION_MODE_COOL);
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->settings.setOperationMode(OPERATION_MODE_FAN);
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      this->settings.setOperationMode(OPERATION_MODE_COOL);
      break;
  }

  // Temperature
  this->settings.setTemperature( (uint8_t)roundf( clamp<float>(this->target_temperature, YARK_TEMP_MIN, YARK_TEMP_MAX)) );

  // Fan Speed 
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_LOW:
      this->settings.setFanMode(FAN_MODE_MANUAL_SPEED_1);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      this->settings.setFanMode(FAN_MODE_MANUAL_SPEED_2);
      break;
    case climate::CLIMATE_FAN_HIGH:
      this->settings.setFanMode(FAN_MODE_MANUAL_SPEED_3);
      break;
    case climate::CLIMATE_FAN_FOCUS:
      this->settings.setFanMode(FAN_MODE_TURBO);
      break;
    case climate::CLIMATE_FAN_QUIET:
      this->settings.setFanMode(FAN_MODE_QUIET);
      break;
    default:
      this->settings.setFanMode(FAN_MODE_AUTO);
      break;
  }

  // Vertical Vane
  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_OFF:
      this->settings.setSwing(SWING_MODE_VERTICAL_OFF);
      break;
    default:
      this->settings.setSwing(SWING_MODE_VERTICAL_AUTO);
      break;
  }

  // Special modes
  switch (this->preset.value()) {
    case climate::CLIMATE_PRESET_ECO:
      this->settings.setFanMode(FAN_MODE_MANUAL_SPEED_2);
      this->settings.setSleep(false);
      this->settings.setTemperature(25);
      break;
    case climate::CLIMATE_PRESET_SLEEP:
      this->settings.setFanMode(FAN_MODE_QUIET);
      this->settings.setSleep(true);
      this->settings.setTemperature(24);
      break;
    case climate::CLIMATE_PRESET_BOOST:
      this->settings.setFanMode(FAN_MODE_TURBO);
      this->settings.setSleep(false);
      this->settings.setTemperature(18);
      break;
    case climate::CLIMATE_PRESET_NONE:
    default:
      break;
  }


  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  transmit.perform();
}

bool YorkClimate::on_receive(remote_base::RemoteReceiveData data) {
  
  //get data from IR 
  //set data[] to this->setting....


  // On/Off and Mode
  switch (this->settings.getOperationMode()) {
    case OPERATION_MODE_DRY:
      this->mode = climate::CLIMATE_MODE_DRY;
      break;
    case OPERATION_MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
    case OPERATION_MODE_FAN:
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      break;
    default:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
  }
  

  // Temp
  this->target_temperature = (this->settings.getTemperature());

  // Fan
   // On/Off and Mode
  switch (this->settings.getFanMode()) {
    case FAN_MODE_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case FAN_MODE_MANUAL_SPEED_1:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case FAN_MODE_MANUAL_SPEED_2:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case FAN_MODE_MANUAL_SPEED_3:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case FAN_MODE_QUIET:
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
      break;
    case FAN_MODE_TURBO:
      this->fan_mode = climate::CLIMATE_FAN_FOCUS;
      break;
    default:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }


  // Vertical Vane
  if(this->settings.getSwing()) {
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  } else {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }

  this->preset = climate::CLIMATE_PRESET_NONE;
  
  this->publish_state();
  return true;
}

}  // namespace mitsubishi
}  // namespace esphome
