#include "york_ir.h"
#include "york_data.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace york_ir {

static const char *const TAG = "york_ir.climate";

void ControlData::set_temp(float temp) {

  temp = esphome::clamp<float>(temp, MIN_TEMP, MAX_TEMP);
  
  this->set_temp(temp);
}

float ControlData::get_temp() const {
  return static_cast<float>(this->get_temp());
}

void ControlData::set_mode(ClimateMode mode) {
  switch (mode) {
    case ClimateMode::CLIMATE_MODE_OFF:
      this->set_IR_power_(true);
      return;
    case ClimateMode::CLIMATE_MODE_COOL:
      this->set_IR_mode_(MODE_COOL);
      break;
    case ClimateMode::CLIMATE_MODE_DRY:
      this->set_IR_mode_(MODE_DRY);
      break;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:
      this->set_IR_mode_(MODE_FAN_ONLY);
      break;
    default:
      this->set_IR_mode_(MODE_COOL);
      break;
  }
}

ClimateMode ControlData::get_mode() const {

  switch (this->get_IR_mode_()) {
    case MODE_COOL:
      return ClimateMode::CLIMATE_MODE_COOL;
    case MODE_DRY:
      return ClimateMode::CLIMATE_MODE_DRY;
    case MODE_FAN_ONLY:
      return ClimateMode::CLIMATE_MODE_FAN_ONLY;
    default:
      return ClimateMode::CLIMATE_MODE_COOL;
  }
}

void ControlData::set_fan_mode(ClimateFanMode mode) {
  switch (mode) {
    case ClimateFanMode::CLIMATE_FAN_LOW:
      this->set_IR_fan_mode_(FAN_LOW);
      break;
    case ClimateFanMode::CLIMATE_FAN_MEDIUM:
      this->set_IR_fan_mode_(FAN_MEDIUM);
      break;
    case ClimateFanMode::CLIMATE_FAN_HIGH:
      this->set_IR_fan_mode_(FAN_HIGH);
      break;
    default:
      this->set_IR_fan_mode_(FAN_AUTO);
      break;
  }
}

ClimateFanMode ControlData::get_fan_mode() const {
  switch (this->get_IR_fan_mode_()) {
    case FAN_LOW:
      return ClimateFanMode::CLIMATE_FAN_LOW;
    case FAN_MEDIUM:
      return ClimateFanMode::CLIMATE_FAN_MEDIUM;
    case FAN_HIGH:
      return ClimateFanMode::CLIMATE_FAN_HIGH;
    default:
      return ClimateFanMode::CLIMATE_FAN_AUTO;
  }
}

void YorkIR::control(const climate::ClimateCall &call) {
  // swing and preset resets after unit powered off
  if (call.get_mode() == climate::CLIMATE_MODE_OFF) {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
    this->preset = climate::CLIMATE_PRESET_NONE;
  } 
  
  
  /* 
  
  else if (call.get_swing_mode().has_value() && ((*call.get_swing_mode() == climate::CLIMATE_SWING_OFF && this->swing_mode == climate::CLIMATE_SWING_VERTICAL) ||
                                                   (*call.get_swing_mode() == climate::CLIMATE_SWING_VERTICAL && this->swing_mode == climate::CLIMATE_SWING_OFF))) {

  } else if (call.get_preset().has_value() &&
             ((*call.get_preset() == climate::CLIMATE_PRESET_NONE && this->preset == climate::CLIMATE_PRESET_BOOST) ||
              (*call.get_preset() == climate::CLIMATE_PRESET_BOOST && this->preset == climate::CLIMATE_PRESET_NONE))) {

  }
  climate_ir::ClimateIR::control(call);

  */
}



void YorkIR::transmit_(YorkData &data) {
  data.finalize();
  auto transmit = this->transmitter_->transmit();
  remote_base::YorkProtocol().encode(transmit.get_data(), data);
  transmit.perform();
}

void YorkIR::transmit_state() {
  ControlData data;
  data.set_temp(this->target_temperature);
  data.set_mode(this->mode);
  data.set_fan_mode(this->fan_mode.value_or(ClimateFanMode::CLIMATE_FAN_AUTO));
  data.set_sleep_preset(this->preset == climate::CLIMATE_PRESET_SLEEP);
  this->transmit_(data);
}



bool YorkIR::on_receive(remote_base::RemoteReceiveData data) {
  auto york = remote_base::YorkProtocol().decode(data);
  if (york.has_value())
    return this->on_york_(*york);
}

bool YorkIR::on_york_(const YorkData &data) {
  ESP_LOGV(TAG, "Decoded York IR data: %s", data.to_string().c_str());

  const ControlData status = data;

  this->target_temperature = status.get_temp();
  this->mode = status.get_mode();
  this->fan_mode = status.get_fan_mode();

  if (this->preset == climate::CLIMATE_PRESET_SLEEP) {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }
  
  this->swing_mode = this->swing_mode == climate::CLIMATE_SWING_VERTICAL ? climate::CLIMATE_SWING_OFF : climate::CLIMATE_SWING_VERTICAL;
  this->preset = this->preset == climate::CLIMATE_PRESET_BOOST ? climate::CLIMATE_PRESET_NONE : climate::CLIMATE_PRESET_BOOST;

  this->publish_state();
  return true;
  
}

}  // namespace york_ir
}  // namespace esphome
