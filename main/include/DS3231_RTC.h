#ifndef __DS3231_RTC_H__
#define __DS3231_RTC_H__

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

class DS3231_RTC {
public:

    DS3231_RTC();

    // data and time
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t weekday;

    // alarm settings
    bool alarm1_enabled;
    bool alarm2_enabled;
    uint8_t alarm1_minutes;
    uint8_t alarm1_hours;
    uint8_t alarm1_day;
    uint8_t alarm2_minutes;
    uint8_t alarm2_hours;
    uint8_t alarm2_day;

    // temperature reading
    float temperature;

    // control and status
    uint8_t control_register;
    uint8_t status_register;

    // public methods...
    void init();
    void setTime();
    void getTime();
    void setAlarm1();
    void setAlarm2();
    float readTemperature();

private:

    // private methods...
    esp_err_t i2c_master_init();
    esp_err_t i2c_send(uint8_t addr, uint8_t *data, size_t len);
    esp_err_t i2c_receive(uint8_t *data);
    esp_err_t i2c_send_receive(uint8_t addr, uint8_t *data, size_t len);

};

#endif // DS3231_RTC
