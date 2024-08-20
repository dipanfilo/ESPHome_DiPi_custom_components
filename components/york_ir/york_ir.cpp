#include "york_ir.h"
#include "york_data.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace york_ir {

static const char *const TAG = "york_ir.climate";

float YorkClimateIR::get_setup_priority() const { 
  return setup_priority::PROCESSOR; 
}
void YorkClimateIR::setup() {

  if (this->sensor_) {
    this->sensor_->add_on_state_callback([this](float state) {
      this->current_temperature = state;
      // current temperature changed, publish state
      this->publish_state();
    });
    this->current_temperature = this->sensor_->state;
  } else
    this->current_temperature = NAN;
  // restore set points
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    // restore from defaults
    this->mode = climate::CLIMATE_MODE_OFF;
    // initialize target temperature to some value so that it's not NAN
    this->target_temperature =
        roundf(clamp(this->current_temperature, YORK_TEMPC_MIN, YORK_TEMPC_MAX));
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
    this->preset = climate::CLIMATE_PRESET_NONE;
  }
  // Never send nan to HA
  if (std::isnan(this->target_temperature)) {
    this->target_temperature = 24;
  }
}
void YorkClimateIR::update() {
  

  this->millis_now_ = millis();
  
  // ignore RX after TX 
  if (this->ignore_RX_after_TX_.activate) {
    if( (this->ignore_RX_after_TX_.millisOld + this->ignore_RX_after_TX_.config) < this->millis_now_) {
      this->ignore_RX_after_TX_.activate = false;
    }
  }
  else {
    this->ignore_RX_after_TX_.millisOld = this->millis_now_;
  }

  // recover old TimerON
  if (this->delay_Update_after_Forze_Power_On_Button_.activate) {
    if( (this->delay_Update_after_Forze_Power_On_Button_.millisOld + (this->delay_Update_after_Forze_Power_On_Button_.config * 1000)) < this->millis_now_) {

      this->delay_Update_after_Forze_Power_On_Button_.activate = false;
      this->virtual_power_status_AC_ = true;

      this->IRData.set_IR_OnTimer(this->old_TimerOn_.hour, this->old_TimerOn_.halfHour, this->old_TimerOn_.active);
      this->old_TimerOn_.hour = 0;
      this->old_TimerOn_.halfHour = false;
      this->old_TimerOn_.active = false;

      this->transmit_state(false);
    }
  }
  else {
    this->delay_Update_after_Forze_Power_On_Button_.millisOld = this->millis_now_;
  }

  // recover old TimerOFF
  if (this->delay_Update_after_Forze_Power_Off_Button_.activate) {
    if( (this->delay_Update_after_Forze_Power_Off_Button_.millisOld + (this->delay_Update_after_Forze_Power_Off_Button_.config * 1000)) < this->millis_now_) {

      this->delay_Update_after_Forze_Power_Off_Button_.activate = false;
      this->virtual_power_status_AC_ = false;

      this->IRData.set_IR_OffTimer(this->old_TimerOff_.hour, this->old_TimerOff_.halfHour, this->old_TimerOff_.active);
      this->old_TimerOff_.hour = 0;
      this->old_TimerOff_.halfHour = false;
      this->old_TimerOff_.active = false;

      this->transmit_state(false);
    }
  }
  else {
    this->delay_Update_after_Forze_Power_Off_Button_.millisOld = this->millis_now_;
  }

  this->loopCounter_++;
}
 void YorkClimateIR::dump_config() { 
    ESP_LOGCONFIG(TAG, "IRData York: %s", this->IRData.to_string().c_str()); 
    ESP_LOGCONFIG(TAG, "  [x] Visual settings:");
    ESP_LOGCONFIG(TAG, "      - Operation Mode: %s", this->IRData.operation_mode_to_string(this->IRData.get_IR_mode()));
    ESP_LOGCONFIG(TAG, "      - Fun Mode: %s", this->IRData.fan_mode_to_string(this->IRData.get_IR_fan_mode()));
    ESP_LOGCONFIG(TAG, "      - Current Time: %d:%d", this->IRData.get_IR_currentTime().hour, this->IRData.get_IR_currentTime().minute);
    ESP_LOGCONFIG(TAG, "      - Timer On: %s", this->IRData.get_IR_OnTimer().active ? "true" : "false");
    ESP_LOGCONFIG(TAG, "        - Time: %d:%s", this->IRData.get_IR_OnTimer().hour, this->IRData.get_IR_OnTimer().halfHour ? "30" : "00");
    ESP_LOGCONFIG(TAG, "      - Timer Off: %s", this->IRData.get_IR_OffTimer().active ? "true" : "false");
    ESP_LOGCONFIG(TAG, "        - Time: %d:%s", this->IRData.get_IR_OffTimer().hour, this->IRData.get_IR_OffTimer().halfHour ? "30" : "00");
    ESP_LOGCONFIG(TAG, "      - Target Temperature: %.0f", this->IRData.get_IR_temp());
    ESP_LOGCONFIG(TAG, "      - Swing: %s", this->IRData.get_IR_Swing() ? "VSWING_AUTO" : "VSWING_OFF");
    ESP_LOGCONFIG(TAG, "      - Sleep Mode: %s", this->IRData.get_IR_Sleep() ? "On" : "Off");
    ESP_LOGCONFIG(TAG, "      - Power Togel Bit: %s", this->IRData.get_IR_power() ? "true" : "false");
    ESP_LOGCONFIG(TAG, "");
    ESP_LOGCONFIG(TAG, "Info");
    ESP_LOGCONFIG(TAG, "      - Timer On Old: %s", this->old_TimerOn_.active ? "true" : "false");
    ESP_LOGCONFIG(TAG, "        - Time: %d:%s", this->old_TimerOn_.hour, this->old_TimerOn_.halfHour ? "30" : "00");
    ESP_LOGCONFIG(TAG, "      - Timer Off old: %s", this->old_TimerOff_.active ? "true" : "false");
    ESP_LOGCONFIG(TAG, "        - Time: %d:%s", this->old_TimerOff_.hour, this->old_TimerOff_.halfHour ? "30" : "00");
  }


climate::ClimateTraits YorkClimateIR::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(YORK_TEMPC_MIN);
  traits.set_visual_max_temperature(YORK_TEMPC_MAX);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_fan_modes({climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});
  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});
  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP, climate::CLIMATE_PRESET_BOOST});
  return traits;
}



void YorkClimateIR::transmit_state(bool power_button) {
  auto transmit = this->transmitter_->transmit();

  // Power Button
  this->IRData.set_IR_power(power_button);

  // Temp
  uint8_t temp = (uint8_t) roundf(clamp(this->target_temperature, YORK_TEMPC_MIN, YORK_TEMPC_MAX));
  this->IRData.set_IR_temp(temp);

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

  this->ignore_RX_after_TX_.activate = true;

  remote_base::YorkProtocol().encode(transmit.get_data(), this->IRData);
  transmit.perform();

  this->IRData.set_IR_power(false);
}
bool YorkClimateIR::on_receive(remote_base::RemoteReceiveData data) {
  auto YorkIR_RxData = remote_base::YorkProtocol().decode(data);

  if ((!this->ignore_RX_after_TX_.activate) && YorkIR_RxData.has_value()) {
    const remote_base::YorkData IRData = *YorkIR_RxData;
    
    const uint8_t* data = IRData.data();
    std::vector<uint8_t> vec(data, data + IRData.size());
    this->IRData.setData(vec); 
    
    // Temp
    this->target_temperature = this->IRData.get_IR_temp();
  
    // Mode
    switch (this->IRData.get_IR_mode()) {
      case this->IRData.MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case this->IRData.MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case this->IRData.MODE_FAN_ONLY:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      default:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
    }


    switch (this->IRData.get_IR_fan_mode()) {
      case this->IRData.FAN_LOW:
        this->fan_mode = climate::CLIMATE_FAN_LOW;
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;
      case this->IRData.FAN_MEDIUM:
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;
      case this->IRData.FAN_HIGH:
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;
      case this->IRData.FAN_AUTO:
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;

      case this->IRData.FAN_QUIET:
        this->fan_mode = climate::CLIMATE_FAN_LOW;
        this->preset = climate::CLIMATE_PRESET_SLEEP;
        break;
      case this->IRData.FAN_TURBO:
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
        this->preset = climate::CLIMATE_PRESET_BOOST;
        break;
    }

    this->publish_state();
  }

  return true;
}




void YorkClimateIR::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();
  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();
  if (call.get_swing_mode().has_value())
    this->swing_mode = *call.get_swing_mode();
  if (call.get_preset().has_value())
    this->preset = *call.get_preset();
  this->transmit_state(false);
  this->publish_state();
}



void YorkClimateIR::button_force_power_on() {
  this->old_TimerOn_ = this->IRData.get_IR_OnTimer();
  this->IRData.set_IR_OnTimer(1,false,true);
  this->IRData.set_IR_currentTime(0,59);

  this->transmit_state(!this->virtual_power_status_AC_);
  this->delay_Update_after_Forze_Power_On_Button_.activate = true;
}
void YorkClimateIR::button_force_power_off(){
  this->old_TimerOff_ = this->IRData.get_IR_OffTimer();
  this->IRData.set_IR_OffTimer(2,false,true);
  this->IRData.set_IR_currentTime(1,59);

  this->transmit_state(this->virtual_power_status_AC_);
  this->delay_Update_after_Forze_Power_Off_Button_.activate = true;
}
void YorkClimateIR::button_togel_power_onoff(){
  this->transmit_state(true);
  this->virtual_power_status_AC_ = !this->virtual_power_status_AC_;
}
void YorkClimateIR::button_dump_ir_data(){
  YorkClimateIR::dump_config();
}



}  // namespace york_ir
}  // namespace esphome
