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

void ToshibaAcChProtocol::encode(RemoteTransmitData *dst, const ToshibaAcChData &data) {
    // Safety check: Ensure the vector actually contains data to send
    if (data.data.empty() || data.nbits == 0) {
        return;
    }

    dst->set_carrier_frequency(38000);
    // Reserve space for the timings: (Header + Bits + Footer) * 2 bursts
    dst->reserve((2 + (data.nbits * 2) + 2) * 2);

    for (uint8_t repeat = 0; repeat < 2; repeat++) {
        // 1. Send Header
        dst->item(HEADER_HIGH_US, HEADER_LOW_US);
        
        // 2. Loop through every single bit up to data.nbits
        for (uint8_t bit_idx = 0; bit_idx < data.nbits; bit_idx++) {
            // Determine which byte the bit resides in
            uint8_t byte_pos = bit_idx / 8;
            
            // Double safety: Prevent out-of-bounds crash if nbits is malformed
            if (byte_pos >= data.data.size()) {
                break;
            }

            // Determine the bit shift position (MSB first within the byte)
            uint8_t bit_pos = 7 - (bit_idx % 8);
            
            dst->mark(BIT_HIGH_US);
            
            // Extract the specific bit from the std::vector and encode it
            if ((data.data[byte_pos] >> bit_pos) & 1) {
                dst->space(BIT_ONE_LOW_US);
            } else {
                dst->space(BIT_ZERO_LOW_US);
            }
        }
        
        // 3. Send Footer
        dst->item(FOOTER_HIGH_US, FOOTER_LOW_US);
    }
}

optional<ToshibaAcChData> ToshibaAcChProtocol::decode(RemoteReceiveData src) {
    ToshibaAcChData packet1; 
    ToshibaAcChData out; 
    
    ESP_LOGD("toshiba_ac_ch", "Starting bit read...");

    packet1.nbits = 0;
    out.nbits = 0;

    packet1.data.assign(TOSHIBA_AC_CH_MAX_BYTE, 0);
    out.data.assign(TOSHIBA_AC_CH_MAX_BYTE, 0);

    // --- Packet 1 Decode ---
    if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US)) {
        // Uncomment this line if you want to see every single sync miss, but warning: it can be noisy!
        // ESP_LOGD("toshiba_ac_ch", "Packet 1 Header mismatch");
        return {};
    }
    
    ESP_LOGD("toshiba_ac_ch", "Packet 1 Header matched! Starting bit read...");
        
    for (uint8_t bit_counter = 0; bit_counter < (TOSHIBA_AC_CH_MAX_BYTE * 8); bit_counter++) {
        uint8_t byte_idx = bit_counter / 8;
        
        if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
            packet1.data[byte_idx] = (packet1.data[byte_idx] << 1) | 1;
            packet1.nbits++;
        } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
            packet1.data[byte_idx] = (packet1.data[byte_idx] << 1) | 0;
            packet1.nbits++;
        } else if (src.expect_item(BIT_HIGH_US, PACKET_SPACE)) {
            ESP_LOGD("toshiba_ac_ch", "Packet 1 hit PACKET_SPACE break at bit %d", packet1.nbits);
            break;
        } else {
            ESP_LOGD("toshiba_ac_ch", "Packet 1 failed at bit %d. Unexpected pulse timing.", bit_counter);
            return {};
        }
    }

    // --- Packet 2 Decode ---
    if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US)) {
        ESP_LOGD("toshiba_ac_ch", "Packet 2 Header mismatch after reading Packet 1 successfully.");
        return {};
    }
    
    ESP_LOGD("toshiba_ac_ch", "Packet 2 Header matched! Starting bit read...");
        
    for (uint8_t bit_counter = 0; bit_counter < (TOSHIBA_AC_CH_MAX_BYTE * 8); bit_counter++) {
        uint8_t byte_idx = bit_counter / 8;
        
        if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
            out.data[byte_idx] = (out.data[byte_idx] << 1) | 1;
            out.nbits++;
        } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
            out.data[byte_idx] = (out.data[byte_idx] << 1) | 0;
            out.nbits++;
        } else if (src.expect_item(BIT_HIGH_US, PACKET_SPACE)) {
            ESP_LOGD("toshiba_ac_ch", "Packet 2 hit PACKET_SPACE break at bit %d", out.nbits);
            break;
        }else if (src.expect_mark(BIT_HIGH_US) && (src.remaining() == 0 || src.peek_space() > PACKET_SPACE)) {
            ESP_LOGD("toshiba_ac_ch", "Packet 2 successfully hit the final trailing mark at bit %d", out.nbits);
            break;
        } else {
            ESP_LOGD("toshiba_ac_ch", "Packet 2 failed at bit %d. Unexpected pulse timing.", bit_counter);
            return {};
        }
    }
    
    // --- Post-processing & Validation ---
    uint8_t actual_bytes_p1 = (packet1.nbits + 7) / 8;
    uint8_t actual_bytes_out = (out.nbits + 7) / 8;
    packet1.data.resize(actual_bytes_p1);
    out.data.resize(actual_bytes_out);

    ESP_LOGD("toshiba_ac_ch", "P1 Bits: %d, P2 Bits: %d", packet1.nbits, out.nbits);

    // Validation Check
    if (packet1 != out) {
        ESP_LOGD("toshiba_ac_ch", "Validation Failed: Packet 1 data does not equal Packet 2 repeat data.");
        return {};
    }

    ESP_LOGD("toshiba_ac_ch", "Decode Successful! Matching payload passed to component.");
    return out;
}

void ToshibaAcChProtocol::dump(const ToshibaAcChData &data) {
    // If the vector is empty, log it immediately and exit
    if (data.data.empty()) {
        ESP_LOGI(TAG, "Received Toshiba AC (0 bits): [empty]");
        return;
    }

    // Allocate a buffer dynamically based on how many bytes are actually in the vector
    // 3 chars per byte ("XX ") + 1 for null terminator
    std::string hex_str;
    hex_str.reserve(data.data.size() * 3);

    char buf[4];
    for (uint8_t byte : data.data) {
        sprintf(buf, "%02X ", byte);
        hex_str += buf;
    }

    // Remove the trailing space if we added bytes
    if (!hex_str.empty()) {
        hex_str.pop_back();
    }

    ESP_LOGI(TAG, "Received Toshiba AC (%u bits): %s", data.nbits, hex_str.c_str());
}


//void ToshibaAcChProtocol::dump(const ToshibaAcChData &data) {
//    ESP_LOG_BUFFER_HEX(TAG, data.bytes, 15);
//}

}  // namespace esphome::remote_base
