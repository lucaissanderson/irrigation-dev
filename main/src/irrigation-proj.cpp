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
#include "esp_netif_sntp.h"

#include "DFRobot_LCD.h"
#include "DS3231_RTC.h"
#include "rotary_encoder.h"
#include "Valve.h"
#include "wifi_setup.h"
#include "sntp_setup.h"

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

static uint8_t checkSymbol[8] = {
    0b00000, // row 1
    0b00001, // row 2
    0b00001, // row 3
    0b00010, // row 4
    0b10010, // row 5
    0b01100, // row 6
    0b00000, // row 7
    0b00000  // row 8
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
static bool wifi_connected = false;

DFRobot_LCD lcd(16,2);

static void displayMenu(MenuState menuState) {
    char top_row[17];
    char bot_row[17];

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    lcd.clear();
    lcd.setCursor(0,0); // col, row
    snprintf(top_row, sizeof(top_row), "V1: V2:  %2d:%2d", timeinfo.tm_hour, timeinfo.tm_min);
    lcd.printstr(top_row);
    lcd.setCursor(15,0);
    lcd.write(0); // print status wifi - functionality later
    lcd.setCursor(3,0);
    lcd.write(2);
    lcd.setCursor(7,0);
    lcd.write(2);

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

static void processSelection() {
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
    struct tm timeinfo;
    time_t now;

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
    lcd.customSymbol(2, checkSymbol);

    /* initialize rotary encoder */
    ret = rotary_init();
    if(ret != ESP_OK) {
        char str_buf[17];
        snprintf(str_buf, sizeof(str_buf), "Rotary failed");
        lcd.printstr(str_buf);
        ESP_LOGE(TAG, "Failed to initialize rotary components: %s", esp_err_to_name(ret));
        return;
    }

    /* set locale */
    setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0/2",1);
    tzset();

    /**
     * try wifi connection , then disconnect
    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(5000));

    initialize_sntp();
    if((ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000))) != ESP_OK) {
        ESP_LOGE(TAG, "Failed SNTP init-timeout: %s", esp_err_to_name(ret));
    }
     */

    rtc.getTime(&timeinfo);
    time(&now);
    localtime_r(&now, &timeinfo);

    /**
     * check external RTC... if it's up-to-date (current year), use those values
     * otherwise, try reaching the internet and resyncing system and RTC
     */


    xTaskCreate(refresh_disp_task, "refresh_disp_task", 2048, NULL, 10, NULL);
    xTaskCreate(main_task, "main_task", 2048, NULL, 8, NULL);

}
