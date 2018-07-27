/*
*  @author  Mark Cooke
*  @version 1.0
*  @date    26-July-2018
*  @brief   Arduino library for HP203B Altitude/Pressure/Temperature sensor
*
* register and command settings are taken from the HopeRF HP203B datasheet, pg6 (HP203B_DataSheet_EN_V2.0.pdf)
*/

#ifndef HP203B_H
#define HP203B_H

#include <Arduino.h>
#include <Wire.h>

#define HP203B_RESET 0x06

#define HP203B_ADDR 0x76
#define HP203B_TEMP 0x32
#define HP203B_PRES 0x30
#define HP203B_ALTI 0x31
#define HP203B_MEASURE 0x40
// Note : when a conversion is requested, temp and pressure is always calculated
// HopeRF HP203B datasheet, pg5
// The device can compute the altitude according to the measured pressure and temperature. The
// altitude value is updated and available to read as soon as the temperature and pressure
// measurement is done.
#define HP203B_CHNL_PRES_AND_TEMP    0x00
#define HP203B_CHNL_TEMP             0x01
#define HP203B_OSR_4096              0x00
#define HP203B_OSR_2048              0x01
#define HP203B_OSR_1024              0x02
#define HP203B_OSR_512               0x03
#define HP203B_OSR_256               0x04
#define HP203B_OSR_128               0x05

// Measurement/conversion delay (max values rounded up) : HopeRF HP203B datasheet, pg5
//const uint8_t HP203B_CONV_DELAY_MS[6] = {79, 40, 20, 10, 5, 3}; // Temp only
const uint8_t HP203B_CONV_DELAY_MS[6] = {132, 66, 33, 17, 9, 5}; // Temp and Pressure

template <class T = TwoWire>
class HP203B
{
  public:
   HP203B(T &i2c_reference, uint8_t i2c_address = HP203B_ADDR) : _i2c_address(i2c_address), _osr(HP203B_OSR_2048) { _i2c = &i2c_reference; }
   void init();
   void setOSR(uint8_t osr = HP203B_OSR_2048) { _osr = osr; }
   float getTempCelcius() { startConversion(); return getSensor(HP203B_TEMP); }
   float getTempFahrenheit() { startConversion(); float temp_degC = getSensor(HP203B_TEMP); return (temp_degC * 1.8 + 32); }
   float getPressure() { startConversion(); return getSensor(HP203B_PRES); }
   float getAltitude() { startConversion(); return getSensor(HP203B_ALTI); }
   void getTempPresAlt(float (&tpa)[3]);
   void reset() { writeByte(HP203B_RESET); } // Soft reset
  protected:
   T * _i2c;
   uint8_t _osr;
   uint8_t _data[3];
   uint8_t _i2c_address;
   void writeByte(uint8_t one);
   float getSensor(uint8_t base_reg_location);
   void startConversion();
};

template <class T>
void HP203B<T>::init()
{
   _i2c->begin();
}

template <class T>
void HP203B<T>::writeByte(uint8_t one)
{
   _i2c->beginTransmission(_i2c_address);
   _i2c->write(one);
   _i2c->endTransmission();
}

template <class T>
void HP203B<T>::startConversion()
{
  
  // 1. Write to ADC_CVT to enable the sensor measurement (can this just be done once in init?)
  // ADC_CVT = [010 | OSR | CHNL]
  uint8_t ADC_CVT = (0x02 << 5) | (_osr << 2) | HP203B_CHNL_PRES_AND_TEMP;
  writeByte(ADC_CVT);

  // wait for the conversion
  delay(HP203B_CONV_DELAY_MS[_osr]);
}

template <class T>
float HP203B<T>::getSensor(uint8_t base_reg_location)
{
  // Read the appropriate 24b register
  _i2c->beginTransmission(_i2c_address);
  _i2c->write(base_reg_location);
  Wire.endTransmission();
  Wire.requestFrom(_i2c_address, 3);
  if (Wire.available())
   {
    _data[0] = Wire.read();
    _data[1] = Wire.read();
    _data[2] = Wire.read();
  }

  // Convert the data to 20b (upper 4b are meaningless - HopeRF HP203B datasheet, pg6)
  float _value = (((_data[0] & 0x0F) * 65536) + (_data[1] * 256) + _data[2]) / 100.00;
  return _value;
}

template <class T>
void HP203B<T>::getTempPresAlt(float (&tpa)[3])
{
  startConversion();

  tpa[0] = getSensor(HP203B_TEMP);
  tpa[1] = getSensor(HP203B_PRES);
  tpa[2] = getSensor(HP203B_ALTI);
}

#endif