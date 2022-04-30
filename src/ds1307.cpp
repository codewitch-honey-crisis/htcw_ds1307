#include "ds1307.hpp"
namespace arduino {
#include <Arduino.h>
#include <time.h>
#include <Wire.h>

// most of this is just low level clock driver code.

#define DS1307_ADDRESS 0x68 ///< I2C address for DS1307
#define DS1307_CONTROL 0x07 ///< Control register

static uint8_t ds1307_bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
static uint8_t ds1307_bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
uint8_t ds1307_read_reg(TwoWire& i2c,uint8_t reg)
{
  i2c.beginTransmission(DS1307_ADDRESS);
  i2c.write(reg);
  i2c.endTransmission();
  i2c.requestFrom(uint8_t(DS1307_ADDRESS), uint8_t(1));
  uint8_t value = i2c.read();
  i2c.endTransmission();
  return value;
}
void ds1307_write_reg(TwoWire& i2c,uint8_t reg, uint8_t value)
{
  i2c.beginTransmission(DS1307_ADDRESS);
  i2c.write(reg);
  i2c.write(value);
  i2c.endTransmission();
}
bool ds1307_read_clock(TwoWire& i2c,struct tm *tm) 
{
  // Find which register to read from
  uint8_t sz = DS1307_CONTROL;
  uint8_t reg = 0;

  while (true) {
    // Reset the register pointer
    i2c.beginTransmission(DS1307_ADDRESS);
    i2c.write(reg);
    i2c.endTransmission();

    i2c.requestFrom(uint8_t(DS1307_ADDRESS), sz);
    tm->tm_sec = ds1307_bcd2bin(i2c.read() & 0x7f);
    tm->tm_min = ds1307_bcd2bin(i2c.read() & 0x7f);
    uint8_t h = i2c.read();
    if (h & 0x40) {
      // Twelve hour mode
      tm->tm_hour = ds1307_bcd2bin(h & 0x1f);
      if (h & 0x20)
        tm->tm_hour += 12; // Seems notation for AM/PM is user-defined
    }
    else
      tm->tm_hour = ds1307_bcd2bin(h & 0x3f);

    tm->tm_wday = (i2c.read() & 0x07) - 1; // Clock uses [1..7]
    tm->tm_mday = ds1307_bcd2bin(i2c.read() & 0x3f);


    tm->tm_mon = ds1307_bcd2bin(i2c.read() & 0x1f)-1; // Clock uses [1..12]
    if (sz >= 7)
      tm->tm_year = ds1307_bcd2bin(i2c.read()) + 100; // Assume 21st century
    else
      tm->tm_year = 70;
    tm->tm_yday = -1;
    i2c.endTransmission();

    if (tm->tm_sec == ds1307_bcd2bin(ds1307_read_reg(i2c,reg) & 0x7f))
      break;
  }
  return true;
}
// get the current time
time_t ds1307_now(TwoWire& i2c) {
  tm tmt;
  if(ds1307_read_clock(i2c,&tmt)) {
    return mktime(&tmt);
  }
  return 0;
}
// start the clock if it's stopped
void ds1307_start(TwoWire& i2c)
{
 
  uint8_t s;
  uint8_t reg = 0;
  s = ds1307_read_reg(i2c,reg);
  
  uint8_t s2 = s;
  
    s2 &= 0x7f; // Clear clock halt
   

  // Write back the data if it is different to the contents of the
  // register.  Always write back if the data wasn't fetched with
  // readData as the contents of the stop bit are unknown.
  if (s != s2)
    ds1307_write_reg(i2c,reg, s2);
}
// stop the clock
void ds1307_stop(TwoWire& i2c)
{
  uint8_t reg = 0;
  uint8_t s = ds1307_read_reg(i2c,reg);
    s |= 0x80;
  ds1307_write_reg(i2c,reg, s);
}
// set the clock
bool ds1307_write_clock(TwoWire& i2c,tm* ptm) 
{
  //tm* ptm = gmtime(&t);
  // Find which register to read from
  uint8_t sz = DS1307_CONTROL;
  uint8_t reg = 0;

  uint8_t clockHalt = 0;
  uint8_t osconEtc = 0;

 
  ds1307_stop(i2c);

  clockHalt = 0x80; // Clock halt to be kept enabled for now


  i2c.beginTransmission(DS1307_ADDRESS);
  i2c.write(reg);
  // Now ready to write seconds



  // i2c.beginTransmission(address);
  // i2c.write(reg);
  i2c.write(ds1307_bin2bcd(ptm->tm_sec) | clockHalt);

  i2c.write(ds1307_bin2bcd(ptm->tm_min));
  i2c.write(ds1307_bin2bcd(ptm->tm_hour)); // Forces 24h mode

  
  i2c.write(ds1307_bin2bcd(ptm->tm_wday + 1) | osconEtc); // Clock uses [1..7]
  i2c.write(ds1307_bin2bcd(ptm->tm_mday));


  // Leap year bit on MCP7941x is read-only so ignore it
  i2c.write(ds1307_bin2bcd(ptm->tm_mon + 1));

  if (sz >= 7)
    i2c.write(ds1307_bin2bcd(ptm->tm_year % 100));

  i2c.endTransmission();

  
  ds1307_start(i2c);
  return true;
}

bool ds1307_init_clock_impl(TwoWire& i2c) {
  i2c.begin();
  i2c.beginTransmission(DS1307_ADDRESS);
  if (i2c.endTransmission() == 0)
    return true;
  return false;
}
// indicates if the clock is running
bool ds1307_is_started(TwoWire& i2c) {
  i2c.beginTransmission(DS1307_ADDRESS);
  i2c.write((byte)0);
  i2c.endTransmission();

  i2c.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = i2c.read();
  return !(ss >> 7);
}
// initializes the clock
bool ds1307_init_clock(TwoWire& i2c) {
  // look for the clock
  if (! ds1307_init_clock_impl(i2c)) {
    return false;
  }
  // try to jumpstart if not running
  if (! ds1307_is_started(i2c)) {
    ds1307_start(i2c);
  }  
  return true;
}
ds1307_sqw ds1307_sqw_read(TwoWire& i2c) {
    ds1307_sqw value;
    i2c.beginTransmission(DS1307_ADDRESS);
    i2c.write(0x07);
    i2c.endTransmission();

    i2c.requestFrom(DS1307_ADDRESS, 1);
    while (i2c.available()) {
    };
    value = (ds1307_sqw)(i2c.read() & 0x93);
    i2c.endTransmission();
    return value;
}
bool ds1307_sqw_write(TwoWire& i2c, ds1307_sqw value) {
    i2c.beginTransmission(DS1307_ADDRESS);
    i2c.write(0x07);
    i2c.write((uint8_t)value);
    i2c.endTransmission();
    return true;
}
bool ds1307::initialize() {
    if (!m_initialized) {
        if(ds1307_init_clock(m_i2c)) {
            m_initialized = true;
        }
    }

    return m_initialized;
}
bool ds1307::now(time_t* result) const {
    if (!initialized()) {
        return false;
    }
    *result = ds1307_now(m_i2c);
    return true;
}
bool ds1307::now(tm* out_tm) const {
    if (!initialized()) {
        return false;
    }
    return ds1307_read_clock(m_i2c,out_tm);
    
}
bool ds1307::running() const {
    if (!initialized()) {
        return false;
    }
    return ds1307_is_started(m_i2c);
}
bool ds1307::set(tm& tm) {
    if (!initialize()) {
        return false;
    }
    return ds1307_write_clock(m_i2c,&tm);
}

ds1307_sqw ds1307::sqw() const {
    if (!initialized()) {
        return (ds1307_sqw)0;
    }
    return ds1307_sqw_read(m_i2c);
}
bool ds1307::sqw(ds1307_sqw value) {
    if (!initialize()) {
        return false;
    }
    ds1307_sqw_write(m_i2c,value);
    return true;
}
}  // namespace arduino