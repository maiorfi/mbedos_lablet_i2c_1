#include "mbed.h"
 
I2C i2c(PB_9, PB_8); // SDA, SCL

#define I2C_ADDR 0x92

static Thread s_thread_poll_sensor;
static EventQueue s_eq_poll_sensor;

static Serial s_debug_serial_port(SERIAL_TX, SERIAL_RX, 115200);

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

void event_proc_poll_sensor()
{
    float ref_temperature;

    if(get_temperature(ref_temperature))
    {
        s_debug_serial_port.printf("T=%f°C\n", ref_temperature);
    }
    else
    {
        s_debug_serial_port.printf("ERROR getting temperature\n");
    }
}
 
int main()
{
    if(!set_12bit_resolution())
    {
        s_debug_serial_port.printf("ERROR initializing sensor\n");
        return -1;
    }

    s_eq_poll_sensor.call_every(10, event_proc_poll_sensor);
    s_thread_poll_sensor.start(callback(&s_eq_poll_sensor, &EventQueue::dispatch_forever));

    return 0;
}
