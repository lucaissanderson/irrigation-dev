#ifndef __SOFTAP_MAIN_H__
#define __SOFTAP_MAIN_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * must initialize NVS before calling
 * starts wifi automatically
 */
esp_err_t wifi_init_softap();

#ifdef __cplusplus
}
#endif

#endif // softap.h
