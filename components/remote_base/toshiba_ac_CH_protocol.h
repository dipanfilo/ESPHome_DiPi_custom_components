#pragma once

#include "remote_base.h"
#include <vector>

namespace esphome::remote_base {

#define TOSHIBA_AC_CH_MAX_BYTE 11


struct ToshibaAcCHData {
    uint8_t nbits = 0; 
    std::vector<uint8_t> data;

    bool operator==(const ToshibaAcCHData &rhs) const { 
        return (nbits == rhs.nbits) && (data == rhs.data);
    }
};

class ToshibaAcCHProtocol : public RemoteProtocol<ToshibaAcCHData> {
 public:
  void encode(RemoteTransmitData *dst, const ToshibaAcCHData &data) override;
  optional<ToshibaAcCHData> decode(RemoteReceiveData src) override;
  void dump(const ToshibaAcCHData &data) override;
};

DECLARE_REMOTE_PROTOCOL(ToshibaAcCH)


template<typename... Ts> class ToshibaAcCHAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(ToshibaAcCHData, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    ToshibaAcCHData data = this->data_.value(x...); 
    ToshibaAcCHProtocol().encode(dst, data);
    //To Do Testing!!!! toshiba_ac_ch_protocol.encode(dst, data);
  }
};

}  // namespace esphome::remote_base
