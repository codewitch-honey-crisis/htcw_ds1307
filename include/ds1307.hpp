#pragma once
#include <Arduino.h>
#include <Wire.h>
namespace arduino {
// indicates the square wave cycle
enum struct ds1307_sqw : uint8_t {
    off = 0x00,  // Low
    on = 0x80,   // High
    cycle_1hz_off = 0x10,
    cycle_4khz_off = 0x11,
    cycle_8khz_off = 0x12,
    cycle_32khz_off = 0x13,
    cycle_1hz_on = 0x10 | 0x80,
    cycle_4khz_on = 0x11 | 0x80,
    cycle_8khz_on = 0x12 | 0x80,
    cycle_32khz_on = 0x13 | 0x80
};
class ds1307 final {
    TwoWire& m_i2c;
    bool m_initialized;
    inline static uint8_t dec2bcd(uint8_t num) {
        return ((num / 10 * 16) + (num % 10));
    }
    inline static uint8_t bcd2dec(uint8_t num) {
        return ((num / 16 * 10) + (num % 16));
    }

   public:
    constexpr static const int8_t i2c_address = 0x68;
    ds1307(TwoWire& i2c) : m_i2c(i2c), m_initialized(false) {
    }
    inline bool initialized() const {
        return m_initialized;
    }
    bool initialize();
    // gets the current date and time
    bool now(time_t* result) const;
    // gets the current date and time
    bool now(tm* out_tm) const;
    // indicates whether or not the clock is running
    bool running() const;
    bool set(tm& tm);
    inline bool set(time_t time) {
        return set(*localtime(&time));
    }
    int8_t calibration() const;
    bool calibrate(int8_t adjust);
    // gets the square wave info
    ds1307_sqw sqw() const;
    // sets the square wave info
    bool sqw(ds1307_sqw value);
};
}  // namespace arduino