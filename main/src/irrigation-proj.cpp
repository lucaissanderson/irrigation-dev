#include <inttypes.h>
#include <time.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "DFRobot_LCD.h"
#include "DS3231_RTC.h"
#include "rotary_encoder.h"
#include "valve.h"

static const char *TAG = "IRRIGATION_TOP";

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


enum MenuState {
    HOME,
    VALVE_SELECT, 
    BACK,
    SETTINGS
};

enum Direction {
    CCW, // counter-clockwise
    CW   // clockwise
};

static volatile MenuState currentMenu = HOME; // starting state
static MenuState prevMenu = currentMenu;
static MenuState nextMenu = VALVE_SELECT;
static uint16_t prev_position = 0;

DFRobot_LCD lcd(16,2);

void displayMenu(MenuState menuState) {
    char top_row[17];
    char bot_row[17];

    lcd.clear();
    lcd.setCursor(0,0); // col, row
    snprintf(top_row, sizeof(top_row), "V1:0 V2:0");
    lcd.printstr(top_row);
    lcd.setCursor(15,0);
    lcd.write(0); // print status wifi - functionality later

    lcd.setCursor(0,1);
    /* determine current state */
    switch (menuState) {
        case HOME:
            /* show highlighted option (next option) */
            switch(nextMenu) {
                case VALVE_SELECT:
                    snprintf(bot_row, sizeof(bot_row), "   VALVE SEL.  >");
                    break;
                case SETTINGS:
                    snprintf(bot_row, sizeof(bot_row), "<   SETTINGS    ");
                    break;
               default:
                    snprintf(bot_row, sizeof(bot_row), "<   INVALID    >");
                    break;
            }
            lcd.printstr(bot_row);
            break;
        case VALVE_SELECT:
            snprintf(bot_row, sizeof(bot_row), "<     BACK     >");
            lcd.printstr(bot_row);
            break;
       case SETTINGS:
            snprintf(bot_row, sizeof(bot_row), "<     BACK     >");
            lcd.printstr(bot_row);
            break;
        default:
            ESP_LOGE(TAG, "Invalid state");
            break;
    }
}

static void refresh_disp_task(void* arg) {
    while(1) {
        // display current highlighted option on LCD
        displayMenu(currentMenu);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

}

void processSelection() {
    prevMenu = currentMenu;
    switch (currentMenu) {
        case HOME:
            currentMenu = nextMenu;
            break;
        case VALVE_SELECT:
            currentMenu = nextMenu;
            break;
       case SETTINGS:
            currentMenu = nextMenu;
            break;
        default:
            ESP_LOGE(TAG, "Invalid state");
            break;
    }
}

void updateMenuFromEncoder() {
    switch(currentMenu) {
        case HOME:
            if(position < 0 || position > 1) {// only 2 options in HOME screen
                position = prev_position; 
                break; 
            }
            switch(direction) {
                case CW:
                    if(nextMenu != SETTINGS) nextMenu = SETTINGS;
                    ESP_LOGI(TAG, "next state: settings");
                    break;
                case CCW:
                    if(nextMenu != VALVE_SELECT) nextMenu = VALVE_SELECT;
                    ESP_LOGI(TAG, "next state: valve sel");
                    break;
            }
            break;
        case SETTINGS:
            position = 0; // only one option for now... back
            nextMenu = prevMenu;
            ESP_LOGI(TAG, "next state: home");
            break;
        case VALVE_SELECT:
            position = 0; // only one option for now... back
            nextMenu = prevMenu;
            ESP_LOGI(TAG, "next state: home");
            break;
        default:
            break;
    }
}

extern "C" void app_main(void)
{

    esp_err_t ret;

    /* initialize display */
    ret = lcd.init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed LCD init: %s", esp_err_to_name(ret));
        return;
    }
    lcd.clear();

    /* initialize rotary encoder */
    ret = rotary_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize rotary components: %s", esp_err_to_name(ret));
        return;
    }

    /* initialize external RTC 
    DS3231_RTC rtc;
    rtc.init();
    */
    
    xTaskCreate(refresh_disp_task, "refresh_disp_task", 2048, NULL, 10, NULL);

    while(true) {

        // if button pressed, process the selection
        if(buttonPressed) {
            processSelection();
            buttonPressed = false; // reset flag after processing
        }

        if(prev_position != position) {
            updateMenuFromEncoder();
            prev_position = position;
        }

        // simulate non-blocking loop, update display or do other tasks
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}
