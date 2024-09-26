#include <inttypes.h>
#include <time.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "DS3231_RTC.h"

#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_MASTER_SCL_IO 9
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_SLAVE_ADDR 0x68
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

/*******************************public*********************************/

DS3231_RTC::DS3231_RTC() {
    // data and time
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint8_t year = 0;
    uint8_t weekday = 0;

    // alarm 1 data struct

    // alarm 2 data struct

    // alarm settings
    bool alarm1_enabled = false;
    bool alarm2_enabled = false;
    uint8_t alarm1_minutes = 0;
    uint8_t alarm1_hours = 0;
    uint8_t alarm1_day = 0;
    uint8_t alarm2_minutes = 0;
    uint8_t alarm2_hours = 0;
    uint8_t alarm2_day = 0;

    // temperature reading
    float temperature = 0.0;

    // control and status
    uint8_t control_register = 0;
    uint8_t status_register = 0;

}

/*
 * Initialize the i2c connection in the constructor
 */
void DS3231_RTC::init(){
    ESP_ERROR_CHECK(i2c_master_init());
}

/*
 * Set the time on the DS3231
 * requirements: 
 * 	all arguments are assumed to be binary-coded-decimal(BCD)
 * 	see RTC_example.cpp for an exmaple conversion
 *
 * 	design considerations:
 * 	    consolidate time members into a struct?
 */
void DS3231_RTC::setTime() {
    uint8_t buf[8] = {
                     seconds,
                     minutes,
                     hours,
                     day,
                     weekday,
                     day,
                     month,
                     year
                     };

    ESP_ERROR_CHECK(i2c_send(0x0,buf,sizeof(buf)));
}

/*
 * get time from the DS3231
 * the resulting memeber values are in BCD
 */
void DS3231_RTC::getTime() {
    uint8_t buffer[8];

    ESP_ERROR_CHECK(i2c_send_receive(0x0, buffer, sizeof(buffer)));

    seconds = buffer[0];
    minutes = buffer[1];
    hours = buffer[2];
    day = buffer[3];
    weekday = buffer[4];
    day = buffer[5];
    month = buffer[6];
    year = buffer[7];
}

/**
 * sets alarm 1
 */
void DS3231_RTC::setAlarm1(){

}

/**
 * sets alarm 2
 */
void DS3231_RTC::setAlarm2(){

}

/**
 * returns a float representing temperature (2's comp) read from DS3231
 */
float DS3231_RTC::readTemperature() {
    float temperature = 0;
    uint8_t buf[2];
    
    ESP_ERROR_CHECK(i2c_send_receive(0x11, buf, sizeof(buf)));

    temperature = (float) ((float)(((buf[0] << 8) | buf[1]) >> 6) / 4);

    return temperature;
}

/*******************************private*******************************/

/**
 * I2C Master Initialization
 */
esp_err_t DS3231_RTC::i2c_master_init() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;

    esp_err_t ret = i2c_param_config(I2C_MASTER_PORT, &conf);
    if(ret != ESP_OK) {
        return ret;
    }

    return i2c_driver_install(I2C_MASTER_PORT,conf.mode,0,0,0);
}

/**
 * Function to send data over I2C
 * args:
 * 	addr : address to send pointer on DS3231
 * 	data : data pointer
 * 	len  : length of array data
 */
esp_err_t DS3231_RTC::i2c_send(uint8_t addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, addr, true);
    i2c_master_write(cmd, data, len, I2C_MASTER_ACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * Function to receive data over I2C
 * INCOMPLETE!!! potentially uncessesary for this use-case
 */
esp_err_t DS3231_RTC::i2c_receive(uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * Function to direct pointer then read
 * args:
 * 	addr : address to send pointer for reading
 * 	data : pointer to uint8 array to be overwritten by read from DS3231
 * 	len  : size of array data
 */
esp_err_t DS3231_RTC::i2c_send_receive(uint8_t addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}
