#include <inttypes.h>
#include <time.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "valve.h"


valve::valve() {

    /* public members */

    /* public methods */


    /* private members */
    GPIO_PORT = static_cast<gpio_num_t>(0); // default 0. expected to be changed via methods

    /* private methods */

}

void valve::setGPIO(uint8_t num) {
    GPIO_PORT = static_cast<gpio_num_t>(num); // default 0. expected to be changed via methods
}
