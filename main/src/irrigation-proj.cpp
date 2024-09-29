#include <inttypes.h>
#include <time.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "DFRobot_LCD.h"
#include "DS3231_RTC.h"
#include "rotary_encoder.h"
#include "valve.h"
#include "softap_main.h"

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
                    ESP_LOGE(TAG, "Invalid state");
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

void processSelection() {
    switch (currentMenu) {
        case HOME:
            currentMenu = nextMenu;
            switch (nextMenu) {
                case VALVE_SELECT:
                    nextMenu = HOME;
                    break;
                case SETTINGS:
                    nextMenu = HOME;
                    break;
                default:
                    break;
            }
            break;
        case VALVE_SELECT:
            currentMenu = nextMenu;
            switch (nextMenu) {
                case HOME:
                    nextMenu = VALVE_SELECT;
                    break;
                default:
                    break;
            }
            break;
        case SETTINGS:
            currentMenu = nextMenu;
            switch (nextMenu) {
                case HOME:
                    nextMenu = VALVE_SELECT;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    position = 0;
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
                    break;
                case CCW:
                    if(nextMenu != VALVE_SELECT) nextMenu = VALVE_SELECT;
                    break;
            }
            break;
        case SETTINGS:
            position = 0; // only one option for now... back
            nextMenu = HOME;
            break;
        case VALVE_SELECT:
            position = 0; // only one option for now... back
            nextMenu = HOME;
            break;
        default:
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

static void main_task(void* arg) {
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


extern "C" void app_main(void)
{
    esp_err_t ret;

    /* initialize external RTC */
    DS3231_RTC rtc;
 
    /* initialize display */
    ret = lcd.init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed LCD init: %s", esp_err_to_name(ret));
        return;
    }
    lcd.clear();
    /* create custom characters in LCD CGRAM */
    lcd.customSymbol(0, wifiSymbol);
    lcd.customSymbol(1, noWifiSymbol);

    /* initialize rotary encoder */
    ret = rotary_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize rotary components: %s", esp_err_to_name(ret));
        return;
    }

    struct tm timeinfo;
    // set time manually
    timeinfo.tm_sec = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_hour = 12;
    timeinfo.tm_mday = 28;
    timeinfo.tm_mon = 8; // 0=Jan, Sept=8
    timeinfo.tm_year = 2024 - 1900; // years since 1900
    timeinfo.tm_wday = 0; // sun = 0

    ESP_LOGI(TAG, "%d:%d.%d, %d of month %d, %d, DOW=%d", timeinfo.tm_hour, timeinfo.tm_min, \
                                                        timeinfo.tm_sec, timeinfo.tm_mday, \
                                                        timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, \
                                                        timeinfo.tm_wday + 1);


    /**
     * check external RTC... if it's up-to-date (current year), use those values
     * otherwise, try reaching the internet and resyncing system and RTC
     */
    rtc.setTime(&timeinfo);
    rtc.getTime(&timeinfo);

    /**
     * try wifi connection , then disconnect
     */
    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(wifi_init_softap());
    ESP_ERROR_CHECK(esp_wifi_stop());
   
    xTaskCreate(refresh_disp_task, "refresh_disp_task", 2048, NULL, 10, NULL);
    xTaskCreate(main_task, "main_task", 2048, NULL, 8, NULL);

}
