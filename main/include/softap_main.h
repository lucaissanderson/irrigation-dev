#ifndef __SOFTAP_MAIN_H__
#define __SOFTAP_MAIN_H__

#include "esp_err.h"

/**
 * must initialize NVS before calling
 * starts wifi automatically
 */
esp_err_t wifi_init_softap();


#endif // softap.h
