#include<Arduino.h>
#include "us100.h"

void US100::set_serial(Stream *streamObject)
{
  _my_serial = streamObject;
  _temp_c = 25;
}

float US100::get_distance()
{   _dist_mm = 0;
    _my_serial->flush();                               // clear receive buffer of serial port
    _my_serial->write(0X55);                           // trig US-100 begin to measure the distance
    delay(100);                                   // delay 500ms to wait result
    if(_my_serial->available()>=2)                   // when receive 2 bytes 
    {
        _HighLen = _my_serial->read();                   // High byte of distance
        _LowLen  = _my_serial->read();                   // Low byte of distance
        _dist_temp  = _HighLen*256 + _LowLen;            // Calculate the distance
        if((_dist_temp > 1) && (_dist_temp < 10000))       // normal distance should between 1mm and 10000mm (1mm, 10m)
        {
            _dist_mm=_dist_temp;
        }
        else{
            _dist_mm=10000;
        }
    }
    else{
        _dist_mm=10000;
    }
    return _dist_mm;
}

float US100::get_temperature()
{ 
    _my_serial->flush();                               // clear receive buffer of serial port
    _my_serial->write(0X50);                           // trig US-100 begin to measure the temperature
    delay(100);                                   // delay 500ms to wait result
    if(_my_serial->available()>=1)                   // when receive 1 bytes 
    {
        _temp_c = _my_serial->read();     //Get the received byte (temperature)
        if((_temp_c > 1) && (_temp_c < 130))   //the valid range of received data is (1, 130)
        {
            _temp_c -= 45;                           //Real temperature = Received_Data - 45

        }
    }
    return _temp_c;
}