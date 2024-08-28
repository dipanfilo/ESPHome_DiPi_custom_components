#pragma once

#include <utility>

#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/sensor/sensor.h"

#include "york_data.h"

namespace esphome {
namespace york_ir {


class YorkClimateIR : public PollingComponent,
                      public climate::Climate,
                      public remote_base::RemoteReceiverListener,
                      public remote_base::RemoteTransmittable {            
                      

 public:
  YorkClimateIR() {}
      

  YorkIRData IRData;

  uint32_t millis_now_;



  void setup() override;
  void update() override;
  float get_setup_priority() const override;
  void dump_config() override;
  void set_sensor(sensor::Sensor *sensor) { 
    this->sensor_ = sensor; 
  }
  void set_ignore_rx_after_tx(uint32_t value) { 
    this->ignore_RX_after_TX_.config = value; 
  }
  void set_delay_after_power_forze_button(uint32_t value) {
    this->delay_Update_after_Forze_Power_On_Button_.config = value;
    this->delay_Update_after_Forze_Power_Off_Button_.config = value;
  }


  void button_force_power_on();
  void button_force_power_off();
  void button_togel_power_onoff();
  void button_dump_ir_data();


  struct timeout_t {
    uint32_t config;
    uint32_t millisOld;
    bool activate;
  };


  enum class SubBinarySensorType {
    POWER_ON_STATUS,
    SUB_BINARY_SENSOR_TYPE_COUNT
  };
  void set_sub_binary_sensor(SubBinarySensorType type, binary_sensor::BinarySensor *sens);

 protected:
  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override;
  /// Return the traits of this controller.
  climate::ClimateTraits traits() override;

  /// Transmit via IR the state of this climate controller.
  virtual void transmit_state(bool power_button);

  // Dummy implement on_receive so implementation is optional for inheritors
  bool on_receive(remote_base::RemoteReceiveData data) override;

  sensor::Sensor *sensor_{nullptr};

  void update_sub_binary_sensor_(SubBinarySensorType type, bool value);
  binary_sensor::BinarySensor *sub_binary_sensors_[(size_t) SubBinarySensorType::SUB_BINARY_SENSOR_TYPE_COUNT]{nullptr};
  int big_data_sensors_{0};

  uint32_t loopCounter_ = 0;

  bool virtual_power_status_AC_;
  timeout_t ignore_RX_after_TX_;
  timeout_t delay_Update_after_Forze_Power_On_Button_;
  timeout_t delay_Update_after_Forze_Power_Off_Button_;

  YorkIRData::timer_struct_t old_TimerOn_;
  YorkIRData::timer_struct_t old_TimerOff_;
};

}  // namespace york_ir
}  // namespace esphome
