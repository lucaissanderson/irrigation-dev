#ifndef __valve_h__
#define __valve_h__

#include <inttypes.h>
#include <sys/time.h>
#include "esp_err.h"
#include "driver/gpio.h"


class Valve {
public:
    /* public members */

    /* public methods */
    Valve(uint8_t hour, uint8_t minute, uint8_t days, uint8_t duration_sec, uint8_t num);
    void activate_valve();
    void deactivate_valve();
    void toggle_valve_on(bool);
    bool check_alarm(struct tm *current_time);
    uint8_t wday_bv; // bit mask for days of week: 0b X Su M T W Th F Sa
    struct tm alarm_time;

private:
    /* private members */
    gpio_num_t GPIO_PORT;
    uint8_t duration;
    bool is_active;
    bool toggle;

    /* private methods */

};


#endif /* valve_h */
