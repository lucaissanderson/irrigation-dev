#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
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
    0b00100, // row 7
    0b00000  // row 8
};
/* end symbols */

/**
 *          HOME
 *            |
 *         -------
 *         |     |
 *      V_SEL   SETT
 *       |        |
 *      1/2      -------
 *               |     |
 *              SYNC  WEATHER_REACTANCE(T/F)
 *     
 */

enum MenuState {
    HOME,         // 0
    VALVE_SELECT, // 1
    SETTINGS,     // 2
    SYNC          // 3
};

enum Direction {
    CCW, // counter-clockwise
    CW   // clockwise
};

enum dispFlag {
    MENU,
    SYNC_STATUS,
    WAITING
};

static volatile dispFlag displayFlag = MENU;

static volatile MenuState currentMenu = HOME; // starting state
static volatile MenuState nextMenu = VALVE_SELECT;
static uint16_t prev_position = 0;
static bool wifi_connected = false;

struct tm timeinfo;
time_t now;

DFRobot_LCD lcd(16,2);
/* initialize external RTC */
DS3231_RTC rtc;

/* Valve pointer for changing properties */
Valve *v_temp;

/**
 * perform time sync. first attempt the internet, then call sntp.
 * report result (success/failure) to the LCD, setting proper flags.
 * bug: calling this code triggers stack protection fault
 */
static void time_sync() {
    esp_err_t ret;

    displayFlag = WAITING;

    /**
    * try wifi connection , then disconnect
    */
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(5000));

    initialize_sntp();
    if((ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000))) != ESP_OK) {
    }    

    vTaskDelay(pdMS_TO_TICKS(3000));
    displayFlag = SYNC_STATUS;
    vTaskDelay(pdMS_TO_TICKS(5000));
    displayFlag = MENU;
}

/** 
 * crux of the UI for the LCD
 */
static void displayMenu(MenuState menuState) {
    char top_row[17];
    char bot_row[17];

    switch (displayFlag) {
        case MENU:
            rtc.getTime(&timeinfo);

            lcd.clear();
            lcd.setCursor(0,0); // col, row
            strftime(top_row, sizeof(top_row), "V1: V2:  %H:%M", &timeinfo);
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
                            ESP_LOGE(TAG, "Invalid next state");
                            break;
                    }
                    lcd.printstr(bot_row);
                    break;
               case VALVE_SELECT:
                    snprintf(bot_row, sizeof(bot_row), "<     BACK     >");
                    lcd.printstr(bot_row);
                    break;
               case SETTINGS:
                    switch (nextMenu) {
                        case SYNC:
                            snprintf(bot_row, sizeof(bot_row), "   SYNC TIME   >");
                            lcd.printstr(bot_row);
                            break;
                        case HOME:
                            snprintf(bot_row, sizeof(bot_row), "<     BACK      ");
                            lcd.printstr(bot_row);
                            break;
                        default:
                            ESP_LOGE(TAG, "Invalid next state");
                            break;
                    }
                    break;
                default:
                    ESP_LOGE(TAG, "Invalid current state: %d", currentMenu);
                    break;
            }
            break;
        case SYNC_STATUS:
            char status_buf[17];
            lcd.clear();
            lcd.setCursor(0,0);

            if(time_synced) {
                snprintf(status_buf, sizeof(status_buf), "Sync successful!");
            } else {
                snprintf(status_buf, sizeof(status_buf), "Sync failed...");
            }

            lcd.printstr(status_buf);
            break;
        case WAITING:
            char waiting_buf[17];
            lcd.clear();
            lcd.setCursor(0,0);
            snprintf(waiting_buf, sizeof(waiting_buf), "Waiting . . .");
            lcd.printstr(waiting_buf);
            break;
    }
}

/**
 * reaction to a button press, indicating a change in state
 * and what the default next state is. also reset position.
 */
static void processSelection() {
    switch (currentMenu) {
        case HOME:
            switch (nextMenu) {
                case VALVE_SELECT:
                    currentMenu = VALVE_SELECT;
                    nextMenu = HOME;
                    break;
                case SETTINGS:
                    currentMenu = SETTINGS;
                    nextMenu = SYNC;
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
            switch (nextMenu) {
                case HOME:
                    nextMenu = VALVE_SELECT;
                    currentMenu = HOME;
                    break;
                case SYNC:
                    /* perform time sync */
                    time_sync();
                    nextMenu = VALVE_SELECT;
                    currentMenu = HOME;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    ESP_LOGI(TAG, "current: %d next: %d", currentMenu, nextMenu);
    position = 0;
}

/**
 * changes next state, reacting to rotation of rotary encoder
 */
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
            if(position < 0 || position > 1) {// only 2 options in SETTINGS screen
                position = prev_position; 
                break; 
            }
            switch(direction) {
                case CCW:
                    if(nextMenu != SYNC) nextMenu = SYNC;
                    break;
                case CW:
                    if(nextMenu != HOME) nextMenu = HOME;
                    break;
            }
            break;
        case VALVE_SELECT:
            position = 0; // only one option for now... back
            nextMenu = HOME;
            break;
        default:
            break;
    }
    ESP_LOGI(TAG, "current: %d next: %d", currentMenu, nextMenu);
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
     * check external RTC... if it's up-to-date (current year), use those values
     * otherwise, try reaching the internet and resyncing system and RTC
     */
    rtc.getTime(&timeinfo);

   //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* default valve config. everyday at 8am for 10 mins */
    Valve v1(20,53,0b01111111,600,0);
    Valve v2(8,0,0b01111111,600,1);

    xTaskCreate(refresh_disp_task, "refresh_disp_task", 2048, NULL, 10, NULL);
    xTaskCreate(main_task, "main_task", 4096, NULL, 3, NULL);

}
