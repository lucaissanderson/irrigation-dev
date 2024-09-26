#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "driver/i2c.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "DFRobot_LCD.h"

// Configuration for I2C
#define I2C_MASTER_SCL_IO    9        /*!< GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO    8        /*!< GPIO number for I2C master data  */
#define I2C_MASTER_FREQ_HZ   100000    /*!< I2C master clock frequency */
#define I2C_MASTER_NUM       I2C_NUM_0 /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

/*******************************public*********************************/

// Function prototypes for internal functions
static esp_err_t i2c_master_init();
void i2c_send(uint8_t addr, uint8_t *data, size_t len);

DFRobot_LCD::DFRobot_LCD(uint8_t lcd_cols, uint8_t lcd_rows, uint8_t lcd_Addr, uint8_t RGB_Addr) {
    _lcdAddr = lcd_Addr;
    _RGBAddr = RGB_Addr;
    _cols = lcd_cols;
    _rows = lcd_rows;
}

/**
 * initialize the display
 */
void DFRobot_LCD::init() {
    i2c_master_init(); // Initialize I2C at constructor
    _showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    begin(_cols, _rows);
}

/**
 * send a command to display
 */
void DFRobot_LCD::command(uint8_t value) {
    uint8_t data[3] = {0x80, value}; // Control byte + command byte
    i2c_send(_lcdAddr, data, 2);
}

/**
 * turn on display
 */
void DFRobot_LCD::display() {
    _showcontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * turn off display
 */
void DFRobot_LCD::noDisplay() {
    _showcontrol |= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * clear display
 */
void DFRobot_LCD::clear()
{
    command(LCD_CLEARDISPLAY);        // clear display, set cursor position to zero
    vTaskDelay(pdMS_TO_TICKS(10));     // this command takes a long time!
}

/**
 * set cursor to position zero
 */
void DFRobot_LCD::home() {
    command(LCD_RETURNHOME);            // set cursor to position zero
    vTaskDelay(pdMS_TO_TICKS(10));     // this command takes a long time!
}

/**
 * turn off cursor element
 */
void DFRobot_LCD::noCursor() {
    _showcontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * turn on cursor
 */
void DFRobot_LCD::cursor() {
    _showcontrol &= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * flash display on/off
 */
void DFRobot_LCD::blink() {
    _showcontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * turn off above function
 */
void DFRobot_LCD::stopBlink() {
    _showcontrol |= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

/**
 * scroll the display left?
 */
void DFRobot_LCD::scrollDisplayLeft(void) 
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void DFRobot_LCD::scrollDisplayRight(void) 
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void DFRobot_LCD::leftToRight(void) 
{
    _showmode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::rightToLeft(void)
{
    _showmode |= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::noAutoscroll(void) 
{
    _showmode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::autoscroll(void) 
{
    _showmode &= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::customSymbol(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));

    uint8_t data[9];
    data[0] = 0x40;
    for(int i=0; i<8; i++)
    {
        data[i+1] = charmap[i];
    }
    i2c_send(_lcdAddr, data, 9);
}

void DFRobot_LCD::blinkLED(void) 
{
    ///< blink period in seconds = (<reg 7> + 1) / 24
    ///< on/off ration = <reg 6> / 256
    setReg(0x07, 0x17); // blink every second
    setReg(0x06, 0x7f); // half on, half off
}

void DFRobot_LCD::noBlinkLED(void)
{
    setReg(0x07, 0x00); 
    setReg(0x06, 0xff); 
}

void DFRobot_LCD::blink_on()
{
    blink();
}

void DFRobot_LCD::blink_off()
{
    stopBlink();
}

void DFRobot_LCD::cursor_on(){
    cursor();
}

void DFRobot_LCD::cursor_off(){
    noCursor();
}

void DFRobot_LCD::load_custom_character(uint8_t char_num, uint8_t *rows)
{
    customSymbol(char_num, rows);
}

void DFRobot_LCD::setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    setReg(REG_RED, r);
    setReg(REG_GREEN, g);
    setReg(REG_BLUE, b);
}

inline size_t DFRobot_LCD::write(uint8_t value)
{

    uint8_t data[3] = {0x40, value};
    i2c_send(_lcdAddr, data, 2);
    return 1; // assume sucess
}

void DFRobot_LCD::printstr(const char c[]) {
    while (*c) {
        write(*c++);
    }
}

void DFRobot_LCD::setCursor(uint8_t col, uint8_t row)
{

    col = (row == 0 ? col|0x80 : col|0xc0);
    uint8_t data[3] = {0x80, col};

    i2c_send(_lcdAddr, data, 2);

}



/*******************************private*******************************/

/**
 * I2C Master Initialization
 */
esp_err_t i2c_master_init() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/**
 * Function to send data over I2C
 */
void i2c_send(uint8_t addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
}


void DFRobot_LCD::setReg(uint8_t addr, uint8_t data) {
    uint8_t buf[2] = {addr, data}; // Register address + data
    i2c_send(_RGBAddr, buf, sizeof(buf));
}

void DFRobot_LCD::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
    if (lines > 1) {
        _showfunction |= LCD_2LINE;
    }
    _numlines = lines;
    _currline = 0;

    // For some 1 line displays you can select a 10 pixel high font
    if ((dotsize != 0) && (lines == 1)) {
        _showfunction |= LCD_5x10DOTS;
    }

    // According to the datasheet, need at least 40ms after power rises above 2.7V before sending commands.
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay 50ms

    // Initialization sequence for LCD display
    command(LCD_FUNCTIONSET | _showfunction);
    vTaskDelay(pdMS_TO_TICKS(5));  // Delay more than 4.1ms

    // Second try
    command(LCD_FUNCTIONSET | _showfunction);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Third try
    command(LCD_FUNCTIONSET | _showfunction);

    // Turn the display on with no cursor or blinking default
    _showcontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // Clear it off
    clear();

    // Initialize to default text direction (for romance languages)
    _showmode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // Set the entry mode
    command(LCD_ENTRYMODESET | _showmode);

    // Backlight init
    setReg(REG_MODE1, 0);
    // Set LEDs controllable by both PWM and GRPPWM registers
    setReg(REG_OUTPUT, 0xFF);
    // Set MODE2 values
    // 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
    setReg(REG_MODE2, 0x20);

    setColorWhite();
}


