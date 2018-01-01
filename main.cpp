#include "mbed.h"
 
I2C i2c(PC_9, PA_8); // SDA, SCL

#define I2C_ADDR 0x92

bool set_12bit_resolution()
{
    const char buf[] = {0x01, 0x60};
    bool i2c_operation_failed=i2c.write(I2C_ADDR, buf, sizeof(buf));
    return !i2c_operation_failed;
}
 
bool get_temperature(float& ref_temperature)
{ 
    int sign;
    bool i2c_operation_failed;

    char buf[] = {0x00, 0x00};

    i2c_operation_failed=i2c.write(I2C_ADDR, buf, 1);
    if(i2c_operation_failed) return false;

    i2c_operation_failed=i2c.read(I2C_ADDR | 0x01, buf, sizeof(buf));
    if(i2c_operation_failed) return false;
    
    sign = buf[0]>>7;

    int t;
    
    if (sign == 1)
    {
        buf[0] = ~buf[0];
        buf[1] = ~buf[1];

        t = buf[0];
        t = -(((t<<4) | (buf[1]>>4)) + 1);
    }
    else
    {
        t = buf[0];
        t = ((t<<4) | (buf[1]>>4));
    }
    
    ref_temperature = t/16.0;

    return true;
}
 
int main()
{
    float temperature;

    bool ok = set_12bit_resolution();

    while(true)
    {
        ok = get_temperature(temperature);

        if(!ok) break;

        wait_ms(10);
    }

    return -1;
}
