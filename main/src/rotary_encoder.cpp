#include <inttypes.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"

#include "rotary_encoder.h"


static const char *TAG = "ROTARY";

#define S1_GPIO 5
#define S2_GPIO 6
#define KEY_GPIO 7
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

/* queues gpio when edge detected */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

volatile int16_t position = 0;
volatile int8_t direction = 0;
bool buttonPressed = false;

static volatile int S1_prev;

static void trigger_callback(void* arg)
{
    uint32_t io_num;
    int S1_level, S2_level;
    for (;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
            switch (io_num) {
                case S1_GPIO:
                    S1_level = gpio_get_level(static_cast<gpio_num_t>(io_num));
                    S2_level = gpio_get_level(static_cast<gpio_num_t>(S2_GPIO));
                    if(S1_level != S1_prev && !S1_level){
                        if(S2_level){ // CW
                            direction = 1;
                            position = position + 1;
                        } else { // CCW
                            direction = 0;
                            position = position - 1;
                        }
                    }
                    S1_prev = S1_level;
                    break;
                case KEY_GPIO:
                    if(!gpio_get_level(static_cast<gpio_num_t>(io_num))) ESP_LOGI(TAG, "Button press!");
                    buttonPressed = true;
                    break;
                default:
                    ESP_LOGI(TAG, "Invalid GPIO dequeued");
            }
        }
    }
}


esp_err_t rotary_init() {
    esp_err_t ret;

    // configure S1
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << S1_GPIO) | (1ULL << S2_GPIO);
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(1);
    ret = gpio_config(&io_conf);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed initializing S1/S2 GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // configure KEY
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << KEY_GPIO);
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(1);
    ret = gpio_config(&io_conf);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed initializing KEY GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    S1_prev = gpio_get_level(static_cast<gpio_num_t>(S1_GPIO));

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // task to handle trigger queue
    xTaskCreate(trigger_callback, "trigger_callback", 2048, NULL, 5, NULL);

    //install gpio isr service
    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(ret));
        return ret;
    }

    //hook isr handler for specific gpio pin
    ret = gpio_isr_handler_add(static_cast<gpio_num_t>(S1_GPIO), gpio_isr_handler, (void*)S1_GPIO);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add S1 to ISR handler: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = gpio_isr_handler_add(static_cast<gpio_num_t>(KEY_GPIO), gpio_isr_handler, (void*)KEY_GPIO);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add KEY to ISR handler: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

