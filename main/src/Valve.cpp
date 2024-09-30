#include <inttypes.h>
#include <time.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Valve.h"

/* public */

Valve::Valve(uint8_t hour, uint8_t minute, uint8_t days, uint8_t duration_sec, uint8_t num) {

    /* set the alarm time */
    alarm_time.tm_hour = hour;
    alarm_time.tm_min = minute;
    alarm_time.tm_sec = 0;
    this->wday_bv = days;
    this->duration = duration_sec;
    this->is_active = false;
    this->toggle = true;

    GPIO_PORT = static_cast<gpio_num_t>(num); // default 0. expected to be changed via setGPIO method

    /* setup GPIO config */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_PORT);
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(0);
    gpio_config(&io_conf);
}

/* args:
 *      val: true means valve will operate at the times, as usual
 *           false means the valve will not open at all
 */
void Valve::toggle_valve_on(bool val)
{
    this->toggle = val;
}


/* private */

bool Valve::check_alarm(struct tm *current_time) {
    // get bit mask for current day
    uint8_t current_week = (1 << current_time->tm_wday);
    // compare current time to alarm time
    if(current_time->tm_hour == alarm_time.tm_hour &&
       current_time->tm_min  == alarm_time.tm_min  &&
       current_time->tm_sec  == alarm_time.tm_sec  &&
      (current_week & wday_bv))
    {
        return true;
    }
    return false;
}
