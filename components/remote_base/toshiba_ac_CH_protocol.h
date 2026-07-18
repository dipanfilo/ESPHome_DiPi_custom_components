#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

namespace esphome::remote_base {

  #define TOSHIBA_AC_CH_MAX_BYTE 15

// Define the new CH structure with the 15-byte array
struct ToshibaAcCHData {
    uint8_t bit_count = (TOSHIBA_AC_CH_MAX_BYTE * 8); // Writable field, defaults to 120 bits (15 bytes)
    uint8_t bytes[TOSHIBA_AC_CH_MAX_BYTE] = {0}; // Initializes all bytes to 0

    // Overloaded equality operator
    bool operator==(const ToshibaAcCHData &rhs) const { 
        return (bit_count == rhs.bit_count) && 
               std::equal(std::begin(bytes), std::end(bytes), rhs.bytes); 
    }
};

class ToshibaAcCHProtocol : public RemoteProtocol<ToshibaAcCHData> {
 public:
  void encode(RemoteTransmitData *dst, const ToshibaAcCHData &data) override;
  optional<ToshibaAcCHData> decode(RemoteReceiveData src) override;
  void dump(const ToshibaAcCHData &data) override;
};

DECLARE_REMOTE_PROTOCOL(ToshibaAcCH)

// Updated Action class to work seamlessly with the new 15-byte struct
template<typename... Ts> class ToshibaAcCHAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(ToshibaAcCHData, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    auto data_val = this->data_.value(x...);
    ToshibaAcCHProtocol().encode(dst, data_val);
  }
};

}  // namespace esphome::remote_base
