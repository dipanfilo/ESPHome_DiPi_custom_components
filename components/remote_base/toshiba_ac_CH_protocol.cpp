#include "toshiba_ac_CH_protocol.h"
#include "esphome/core/log.h"
#include <cinttypes>
#include <algorithm>
#include <iterator>

namespace esphome::remote_base {

static const char *const TAG = "remote.toshibaacch";

static constexpr uint32_t HEADER_HIGH_US = 4500;
static constexpr uint32_t HEADER_LOW_US = 4500;
static constexpr uint32_t BIT_HIGH_US = 560;
static constexpr uint32_t BIT_ONE_LOW_US = 1690;
static constexpr uint32_t BIT_ZERO_LOW_US = 560;
static constexpr uint32_t FOOTER_HIGH_US = 560;
static constexpr uint32_t FOOTER_LOW_US = 4500;
static constexpr uint16_t PACKET_SPACE = 5500;

void ToshibaAcCHProtocol::encode(RemoteTransmitData *dst, const ToshibaAcCHData &data) {
    dst->set_carrier_frequency(38000);
    // Reserve space for the timings: (Header + Bits + Footer) * 2 bursts
    dst->reserve((2 + (data.bit_count * 2) + 2) * 2);

    for (uint8_t repeat = 0; repeat < 2; repeat++) {
        // 1. Send Header
        dst->item(HEADER_HIGH_US, HEADER_LOW_US);
        
        // 2. Loop through every single bit up to data.bit_count
        for (uint8_t bit_idx = 0; bit_idx < data.bit_count; bit_idx++) {
            // Determine which byte the bit resides in
            uint8_t byte_pos = bit_idx / 8;
            // Determine the bit shift position (MSB first within the byte)
            uint8_t bit_pos = 7 - (bit_idx % 8);
            
            dst->mark(BIT_HIGH_US);
            
            // Extract the specific bit and encode it
            if ((data.bytes[byte_pos] >> bit_pos) & 1) {
                dst->space(BIT_ONE_LOW_US);
            } else {
                dst->space(BIT_ZERO_LOW_US);
            }
        }
        
        // 3. Send Footer
        dst->item(FOOTER_HIGH_US, FOOTER_LOW_US);
    }
}

optional<ToshibaAcCHData> ToshibaAcCHProtocol::decode(RemoteReceiveData src) {
    ToshibaAcCHData packet1; // Uses default struct initializers
    ToshibaAcCHData out; 
    
    packet1.bit_count = 0;
    out.bit_count = 0;

    // *** Packet 1 (120 bits -> 15 bytes)
    if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
        return {};
        
    for (uint8_t bit_counter = 0; bit_counter < (TOSHIBA_AC_CH_MAX_BYTE * 8); bit_counter++) {
        uint8_t byte_idx = bit_counter / 8;
        
        if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
            packet1.bytes[byte_idx] = (packet1.bytes[byte_idx] << 1) | 1;
            packet1.bit_count++;
        } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
            packet1.bytes[byte_idx] = (packet1.bytes[byte_idx] << 1) | 0;
            packet1.bit_count++;
        } else if (src.expect_item(BIT_HIGH_US, PACKET_SPACE)) {
            break;
        } else {
            return {};
        }
    }

    // *** Packet 2 (120 bits -> 15 bytes)
    if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
        return {};
        
    for (uint8_t bit_counter = 0; bit_counter < (TOSHIBA_AC_CH_MAX_BYTE * 8); bit_counter++) {
        uint8_t byte_idx = bit_counter / 8;
        
        if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
            out.bytes[byte_idx] = (out.bytes[byte_idx] << 1) | 1;
            out.bit_count++;
        } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
            out.bytes[byte_idx] = (out.bytes[byte_idx] << 1) | 0;
            out.bit_count++;
        } else if (src.expect_item(BIT_HIGH_US, PACKET_SPACE)) {
            break;
        } else {
            return {};
        }
    }
    
    // The two packets must match completely in length and payload
    if (packet1.bit_count != out.bit_count || 
        !std::equal(std::begin(packet1.bytes), std::end(packet1.bytes), std::begin(out.bytes))) {
        return {};
    }

    return out;
}

void ToshibaAcCHProtocol::dump(const ToshibaAcCHData &data) {
    // (15 bytes * 3 characters per byte "XX ") + 1 null terminator = 46
    char hex_str[(TOSHIBA_AC_CH_MAX_BYTE * 3) + 1]; 
    char *ptr = hex_str;

    for (int i = 0; i < TOSHIBA_AC_CH_MAX_BYTE; i++) {
        ptr += sprintf(ptr, "%02X ", data.bytes[i]);
    }
    
    if (ptr > hex_str) {
        *(ptr - 1) = '\0';
    }

    ESP_LOGI(TAG, "Received Toshiba AC (%u bits): %s", data.bit_count, hex_str);
}


//void ToshibaAcCHProtocol::dump(const ToshibaAcCHData &data) {
//    ESP_LOG_BUFFER_HEX(TAG, data.bytes, 15);
//}

}  // namespace esphome::remote_base
