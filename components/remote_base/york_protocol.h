#pragma once

#include "esphome/core/component.h"
#include "../climate_ir_york/york_ir_typedef.h"
#include "remote_base.h"

#include <cinttypes>

namespace esphome {
namespace remote_base {
    
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// take a look to ABBWelcomeData class maybe adjustit like this one whit set and get functions
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(operation_mode_t, operationMode)
  TEMPLATABLE_VALUE(fan_mode_t, fanMode)
  TEMPLATABLE_VALUE(uint8_t, currentTime_hour)
  TEMPLATABLE_VALUE(uint8_t, onTimer_hour)
  TEMPLATABLE_VALUE(uint8_t, currentTime_minute)
  TEMPLATABLE_VALUE(bool, onTimer_halfHour)
  TEMPLATABLE_VALUE(bool, onTimer_active)
  TEMPLATABLE_VALUE(uint8_t, offTimer_hour)
  TEMPLATABLE_VALUE(bool, offTimer_halfHour)
  TEMPLATABLE_VALUE(bool, offTimer_active)
  TEMPLATABLE_VALUE(uint8_t, temperature)
  TEMPLATABLE_VALUE(bool, swing)
  TEMPLATABLE_VALUE(bool, sleep)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};
    setOperationMode(data, this->operationMode_.value(x...));
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
