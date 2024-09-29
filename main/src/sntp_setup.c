#include <stdio.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_netif_sntp.h"

static const char *TAG = "SNTP";

bool time_synced = false;

// callback function after sync
static void time_sync_notification_cb(struct timeval *tv) 
{
    ESP_LOGI(TAG,"Time synchronized");
    time_synced = true;
}

void initialize_sntp()
{
    ESP_LOGI(TAG, "Initialize SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

/*
esp_err_t sync_time()
{
    esp_err_t ret;
    initialize_sntp();

    while(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        ESP_LOGI(TAG, "Waiting for time to synchronize...");
	    vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if((ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000))) != ESP_OK) {
        ESP_LOGE(TAG, "Failed SNTP init-timeout: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}
*/
