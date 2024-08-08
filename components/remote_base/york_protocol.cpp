#include "york_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.york";

static const uint32_t HEADER_HIGH_US = 4652;
static const uint32_t HEADER_LOW_US = 2408;

static const uint32_t BIT_HIGH_US = 368;
static const uint32_t BIT_ONE_LOW_US = 944;
static const uint32_t BIT_ZERO_LOW_US = 368;

static const uint32_t END_PULS = 20340;

void YORKProtocol::encode(RemoteTransmitData *dst, const YORKData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(2 + data.nbits * 2u);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (uint32_t mask = 1UL << (data.nbits - 1); mask != 0; mask >>= 1) {
    if (data.data & mask) {
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
    }
  }

  dst->mark(BIT_HIGH_US);
}
optional<YORKData> YORKProtocol::decode(RemoteReceiveData src) {
  YORKData out;

  out.data = 0;
  out.nbits = 0;

  bool End_OK = false;

  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  
  for (uint32_t mask = 1UL << 31; mask != 0; mask >>= 1) {
    if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
      out.data |= mask;
    } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
      out.data &= ~mask;
    } else {
      return {};
    }
  }


  uint32_t index = src.get_index();
  int32_t value = src.operator(index+1);
  ESP_LOGI(TAG, "Received YORK: test data=%08" PRIX32, data.value);


  if (src.expect_item(BIT_HIGH_US, END_PULS)) {
    out.nbits = 1;
    ESP_LOGI(TAG, "Received YORK: step 1");
    if (src.expect_mark(HEADER_HIGH_US)) {
       ESP_LOGI(TAG, "Received YORK: step 2");
    }
  }

  return out;
}
void YORKProtocol::dump(const YORKData &data) {
  ESP_LOGI(TAG, "Received YORK: data=0x%08" PRIX32 ", nbits=%d", data.data, data.nbits);
}

}  // namespace remote_base
}  // namespace esphome


// 134 = 64 + 
