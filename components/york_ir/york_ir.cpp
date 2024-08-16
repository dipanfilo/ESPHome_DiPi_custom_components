#include "york_ir.h"
#include "york_data.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace york_ir {

static const char *const TAG = "york_ir.climate";

// bo bo bo
//void ControlData::set_temp(float temp) {
//
//  temp = esphome::clamp<float>(temp, YORK_TEMPC_MIN, YORK_TEMPC_MAX);
//  
//  this->set_IR_temp(temp);
//}
//
//float ControlData::get_temp() const {
//  return static_cast<float>(this->get_IR_temp());
//}
//
//void ControlData::set_mode(ClimateMode mode) {
//  switch (mode) {
//    case ClimateMode::CLIMATE_MODE_OFF:
//      this->set_IR_power_(true);
//      return;
//    case ClimateMode::CLIMATE_MODE_COOL:
//      this->set_IR_mode_(MODE_COOL);
//      break;
//    case ClimateMode::CLIMATE_MODE_DRY:
//      this->set_IR_mode_(MODE_DRY);
//      break;
//    case ClimateMode::CLIMATE_MODE_FAN_ONLY:
//      this->set_IR_mode_(MODE_FAN_ONLY);
//      break;
//    default:
//      this->set_IR_mode_(MODE_COOL);
//      break;
//  }
//}
//
//ClimateMode ControlData::get_mode() const {
//
//  switch (this->get_IR_mode_()) {
//    case MODE_COOL:
//      return ClimateMode::CLIMATE_MODE_COOL;
//    case MODE_DRY:
//      return ClimateMode::CLIMATE_MODE_DRY;
//    case MODE_FAN_ONLY:
//      return ClimateMode::CLIMATE_MODE_FAN_ONLY;
//    default:
//      return ClimateMode::CLIMATE_MODE_COOL;
//  }
//}
//
//void ControlData::set_fan_mode(ClimateFanMode mode) {
//  switch (mode) {
//    case ClimateFanMode::CLIMATE_FAN_LOW:
//      this->set_IR_fan_mode_(FAN_LOW);
//      break;
//    case ClimateFanMode::CLIMATE_FAN_MEDIUM:
//      this->set_IR_fan_mode_(FAN_MEDIUM);
//      break;
//    case ClimateFanMode::CLIMATE_FAN_HIGH:
//      this->set_IR_fan_mode_(FAN_HIGH);
//      break;
//    default:
//      this->set_IR_fan_mode_(FAN_AUTO);
//      break;
//  }
//}
//
//ClimateFanMode ControlData::get_fan_mode() const {
//  switch (this->get_IR_fan_mode_()) {
//    case FAN_LOW:
//      return ClimateFanMode::CLIMATE_FAN_LOW;
//    case FAN_MEDIUM:
//      return ClimateFanMode::CLIMATE_FAN_MEDIUM;
//    case FAN_HIGH:
//      return ClimateFanMode::CLIMATE_FAN_HIGH;
//    default:
//      return ClimateFanMode::CLIMATE_FAN_AUTO;
//  }
//}


//void YorkClimateIR::control(const climate::ClimateCall &call) {
//  // swing and preset resets after unit powered off
//  if (call.get_mode() == climate::CLIMATE_MODE_OFF) {
//    this->swing_mode = climate::CLIMATE_SWING_OFF;
//    this->preset = climate::CLIMATE_PRESET_NONE;
//  } 
//  climate_ir::ClimateIR::control(call);
//}


void YorkClimateIR::sendIR() 
{
  transmit_state();
}

void YorkClimateIR::transmit_state() {
  auto transmit = this->transmitter_->transmit();

  // Temp
  uint8_t temp = (uint8_t) roundf(clamp(this->target_temperature, YORK_TEMPC_MIN, YORK_TEMPC_MAX));
  this->IRData.set_IR_temp(temp);


  YorkIRData::time_struct_t currentTime;
  currentTime = this->IRData.get_IR_currentTime();
  if (currentTime.hour == 0 && currentTime.minute == 0) {
    this->IRData.set_IR_currentTime(0, 0);
    this->IRData.set_IR_OnTimer(0, false, false);
    this->IRData.set_IR_OffTimer(0, false, false);
  }


  // Fan speed
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      this->IRData.set_IR_fan_mode(IRData.FAN_HIGH);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      this->IRData.set_IR_fan_mode(this->IRData.FAN_MEDIUM);
      break;
    case climate::CLIMATE_FAN_LOW:
      this->IRData.set_IR_fan_mode(this->IRData.FAN_LOW);
      break;
    case climate::CLIMATE_FAN_AUTO:
      this->IRData.set_IR_fan_mode(this->IRData.FAN_AUTO);
      break;
    default:
      this->IRData.set_IR_fan_mode(this->IRData.FAN_AUTO);
      break;
  }

  // Mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      this->IRData.set_IR_mode(this->IRData.MODE_COOL);
      break;
    case climate::CLIMATE_MODE_DRY:
      this->IRData.set_IR_mode(this->IRData.MODE_DRY);
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->IRData.set_IR_mode(this->IRData.MODE_FAN_ONLY);
      break;
    //case climate::CLIMATE_MODE_OFF:
    //
    //  break;
    default:
      this->IRData.set_IR_mode(this->IRData.MODE_COOL);
      break;
  }


  this->IRData.finalize();
  remote_base::YorkProtocol().encode(transmit.get_data(), this->IRData);
}



bool YorkClimateIR::on_receive(remote_base::RemoteReceiveData data) {
  //auto YorkData = remote_base::YorkProtocol().decode(data);
  //if (YorkData.has_value()) {
  //  this->IRData = YorkData;    
  //  this->publish_state();
  //}

  return true;
}



}  // namespace york_ir
}  // namespace esphome
