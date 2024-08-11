#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

#include <cinttypes>


#define selectLeftNibble(nibble) ((nibble >> 4 ) & 0xF)
#define selectRightNibble(nibble) (nibble & 0xF)


namespace esphome {
namespace remote_base {
    
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// take a look to ABBWelcomeData class maybe adjustit like this one whit set and get functions
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
struct YORKData {
  uint8_t buffer[8];
};

class YORKProtocol : public RemoteProtocol<YORKData> {
 public:
  void encode(RemoteTransmitData *dst, const YORKData &data) override;
  optional<YORKData> decode(RemoteReceiveData src) override;
  void dump(const YORKData &data) override;
  
};

DECLARE_REMOTE_PROTOCOL(YORK)

template<typename... Ts> class YORKAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    YORKData data{};
    
    data.buffer = this->data.value(x...);
   
    YORKProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
