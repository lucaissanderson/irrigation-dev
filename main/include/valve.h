#ifndef __valve_h__
#define __valve_h__

#include <inttypes.h>
#include <time.h>
#include "driver/gpio.h"


class valve {
public:
    /* public members */

    /* public methods */
    valve();
    void setGPIO(gpio_num_t num);

private:
    /* private members */
    gpio_num_t GPIO_PORT;

    /* private methods */

};


#endif /* valve_h */
