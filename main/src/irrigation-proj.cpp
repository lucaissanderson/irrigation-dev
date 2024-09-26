#include <inttypes.h>
#include <time.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "DFRobot_LCD.h"
#include "valve.h"

static enum STATE {
    HOME
};

/* define wifi symbols for LCD */
static uint8_t wifiSymbol[8] = {
    0b00000, // row 1
    0b01110, // row 2
    0b10001, // row 3
    0b00100, // row 4
    0b01010, // row 5
    0b00000, // row 6
    0b00100, // row 7
    0b00000 // row 8
};

static uint8_t noWifiSymbol[8] = {
    0b00001, // row 1
    0b01110, // row 2
    0b10011, // row 3
    0b00100, // row 4
    0b01110, // row 5
    0b01000, // row 6
    0b01100, // row 7
    0b10000 // row 8
};
/* end symbols */



extern "C" void app_main(void)
{

    DFRobot_LCD lcd(16,2);
    lcd.init();

    char text[32];
    snprintf(text, sizeof(text), "hello world");

    lcd.customSymbol(0, wifiSymbol);
    lcd.customSymbol(1, noWifiSymbol);

    lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.write(0);
    lcd.setCursor(0,2);
    lcd.write(1);
    

}
