#include "mbed.h"
 
I2C i2c(PB_9, PB_8); // SDA, SCL

#define I2C_ADDR 0x92

static Thread s_thread_poll_sensor;
static EventQueue s_eq_poll_sensor;

static Serial s_debug_serial_port(SERIAL_TX, SERIAL_RX, 115200);

static DigitalOut led1(LED1, false);

static InterruptIn btn(BUTTON1);

static InterruptIn temperature_sensor_alert(PH_1);

bool set_12bit_resolution()
{
    const char buf[] = {0x01, 0x60};
    bool i2c_operation_failed=i2c.write(I2C_ADDR, buf, sizeof(buf));
    return !i2c_operation_failed;
}

void convert_temperature_to_msb_and_lsb(float temperature, char& ref_msb, char& ref_lsb)
{
    int t;

    if (temperature < 0)
    {
        t = (int)(-temperature*16.0-1);
        t = t<<4;
        ref_lsb = t & 0xFF;
        ref_msb = t>>8;
        ref_lsb = ~ref_lsb;
        ref_msb = ~ref_msb;
    }
    else
    {
        t = (int)(temperature*16.0);
        t = t<<4;
        ref_lsb = t & 0xFF;
        ref_msb = t>>8;
    }
}

bool set_temperature_limits(float t_lo, float t_hi)
{
    char msb;
    char lsb;

    char buf[3];

    convert_temperature_to_msb_and_lsb(t_lo, msb, lsb);

    buf[0]=0x02;
    buf[1]=msb;
    buf[2]=lsb;

    bool i2c_operation_failed=i2c.write(I2C_ADDR, buf, sizeof(buf));
    if(i2c_operation_failed) return false;

    convert_temperature_to_msb_and_lsb(t_hi, msb, lsb);

    buf[0]=0x03;
    buf[1]=msb;
    buf[2]=lsb;

    i2c_operation_failed=i2c.write(I2C_ADDR, buf, sizeof(buf));
    if(i2c_operation_failed) return false;

    return true;
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

    // s_debug_serial_port.printf("ALERT %s\n", temperature_sensor_alert ? "DEASSERTED" : "ASSERTED");
}

void event_proc_alert_input_change(bool rising)
{
    // Nota: in questa funzione si assume che la polarità dell'alert sia "active-low"

    led1.write(!rising);

    if(!rising)
    {
        s_debug_serial_port.printf("Temperature Limit Alert ASSERTED\n");
    }
    else
    {
        s_debug_serial_port.printf("Temperature Limit Alert DEASSERTED\n");
    }
}

void isr_alert_input_rise()
{
    s_eq_poll_sensor.call(event_proc_alert_input_change, true);
}

void isr_alert_input_fall()
{
    s_eq_poll_sensor.call(event_proc_alert_input_change, false);
}
 
int main()
{
    if(!set_12bit_resolution())
    {
        s_debug_serial_port.printf("ERROR setting 12 bit resolution\n");
        return -1;
    }

    if(!set_temperature_limits(23.0, 24.0))
    {
        s_debug_serial_port.printf("ERROR setting temperature limits\n");
        return -1;
    }

    temperature_sensor_alert.rise(&isr_alert_input_rise);
    temperature_sensor_alert.fall(&isr_alert_input_fall);

    s_eq_poll_sensor.call_every(1000, event_proc_poll_sensor);
    s_thread_poll_sensor.start(callback(&s_eq_poll_sensor, &EventQueue::dispatch_forever));

    return 0;
}
