#pragma once

#include "remote_base.h"
#include <vector>

namespace esphome::remote_base {

#define TOSHIBA_AC_CH_MAX_BYTE 15


struct ToshibaAcChData {
    uint8_t nbits; 
    std::vector<uint8_t> data;

    bool operator==(const ToshibaAcChData &rhs) const { 
        return (nbits == rhs.nbits) && (data == rhs.data);
    }
};

class ToshibaAcChProtocol : public RemoteProtocol<ToshibaAcChData> {
 public:
  void encode(RemoteTransmitData *dst, const ToshibaAcChData &data) override;
  optional<ToshibaAcChData> decode(RemoteReceiveData src) override;
  void dump(const ToshibaAcChData &data) override;
};

DECLARE_REMOTE_PROTOCOL(ToshibaAcCh)

template<typename... Ts> 
class ToshibaAcChAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  // For dynamic template values (lambdas)
  TEMPLATABLE_VALUE(ToshibaAcChData, data)

  // Optimization: For static const arrays parsed from raw configurations
  void set_data_static(const uint8_t *data_ptr, size_t size, uint8_t nbits) {
    this->static_data_.nbits = nbits;
    this->static_data_.data.assign(data_ptr, data_ptr + size);
    this->is_static_ = true;
  }

  void encode(RemoteTransmitData *dst, Ts... x) override {
    if (this->is_static_) {
      ToshibaAcChProtocol().encode(dst, this->static_data_);
    } else {
      ToshibaAcChData dynamic_data = this->data_.value(x...);
      ToshibaAcChProtocol().encode(dst, dynamic_data);
    }
  }

 protected:
  bool is_static_{false};
  ToshibaAcChData static_data_{};
};

}  // namespace esphome::remote_base
