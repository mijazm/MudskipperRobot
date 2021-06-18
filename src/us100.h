#ifndef us100_h
#define us100_h
#include<Arduino.h>

class US100
{
    public:
        void set_serial(Stream *streamObject);
        float get_distance();
        float get_temperature();
    private:
        Stream *_my_serial;
        unsigned int _HighLen,_LowLen,_dist_mm,_temp_c,_dist_temp;

};

#endif