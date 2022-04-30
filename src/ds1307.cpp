#include "ds1307.hpp"
namespace arduino {
bool ds1307::initialize() {
    if (!m_initialized) {
        m_i2c.beginTransmission(i2c_address);
        m_i2c.write((uint8_t)0x00);
        if (m_i2c.endTransmission() != 0) {
            // clock doesn't exist
            return false;
        }
        m_initialized = true;
    }

    return true;
}
bool ds1307::now(time_t* result) const {
    tm tm;
    if (now(&tm)) {
        *result = mktime(&tm);
    }
    return false;
}
bool ds1307::now(tm* out_tm) const {
    if(!initialized()) {
        return false;
    }
    uint8_t sec;

    // get the time data
    m_i2c.requestFrom(i2c_address, 7);
    if (m_i2c.available() < 7) {
        return false;
    }
    sec = m_i2c.read();
    out_tm->tm_sec = bcd2dec(sec & 0x7f);
    out_tm->tm_min = bcd2dec(m_i2c.read());
    out_tm->tm_hour = bcd2dec(m_i2c.read() & 0x3f);
    out_tm->tm_wday = bcd2dec(m_i2c.read());
    out_tm->tm_mday = bcd2dec(m_i2c.read());
    out_tm->tm_mon = bcd2dec(m_i2c.read() - 1);  // ds1307 returns 1-12
    out_tm->tm_year = bcd2dec(m_i2c.read()) + 100;
    if (sec & 0x80) {
        return false;  // halted
    }
    return true;
}
bool ds1307::running() const {
    if (!initialized()) {
        return false;
    }
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write((uint8_t)0x00);
    m_i2c.endTransmission();
    // check high bit of seconds
    m_i2c.requestFrom(i2c_address, 1);
    return !(m_i2c.read() & 0x80);
}
bool ds1307::set(tm& tm) {
    if (!initialize()) {
        return false;
    }
    // temporarily stop the clock
    m_i2c.beginTransmission(i2c_address);

    m_i2c.write((uint8_t)0x00);
    m_i2c.write((uint8_t)0x80);  
    m_i2c.write(dec2bcd(tm.tm_min));
    m_i2c.write(dec2bcd(tm.tm_hour));
    m_i2c.write(dec2bcd(tm.tm_wday));
    m_i2c.write(dec2bcd(tm.tm_mday));
    m_i2c.write(dec2bcd(tm.tm_mon + 1));
    m_i2c.write(dec2bcd(tm.tm_year-100));

    if (m_i2c.endTransmission() != 0) {
        m_initialized = false;
        return false;
    }

    // restart the clock (set the seconds too)
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write((uint8_t)0x00);
    m_i2c.write(dec2bcd(tm.tm_sec));
    if (m_i2c.endTransmission() != 0) {
        m_initialized = false;
        return false;
    }
    return true;
}
int8_t ds1307::calibration() const {
    if (!initialized()) {
        return 0;
    }
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write((uint8_t)0x07);
    m_i2c.endTransmission();
    m_i2c.requestFrom(i2c_address, 1);
    uint8_t reg = m_i2c.read();
    int8_t result = reg & 0x1f;
    if (!(reg & 0x20)) {
        result = -result;
    }
    return result;
}
bool ds1307::calibrate(int8_t adjust) {
    if (!initialize()) {
        return false;
    }
    uint8_t reg = (uint8_t(adjust&0x7f)) & 0x1f;
    if (adjust >= 0) reg |= 0x20;
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write((uint8_t)0x07);  // Point to calibration register
    m_i2c.write(reg);
    m_i2c.endTransmission();
    return true;
}
ds1307_sqw ds1307::sqw() const {
    if (!initialized()) {
        return (ds1307_sqw)0;
    }
    ds1307_sqw value;
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write(0x07);
    m_i2c.endTransmission();

    m_i2c.requestFrom(i2c_address, 1);
    while(!m_i2c.available()) {};
    value = (ds1307_sqw)(m_i2c.read() & 0x93);
    m_i2c.endTransmission();
    return value;
}
bool ds1307::sqw(ds1307_sqw value) {
    if (!initialize()) {
        return false;
    }
    m_i2c.beginTransmission(i2c_address);
    m_i2c.write(0x07);
    m_i2c.write((uint8_t)value);
    m_i2c.endTransmission();
    return true;
}
}  // namespace arduino