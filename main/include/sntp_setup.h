#ifndef SNTP_H
#define SNTP_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool time_synced;

void initialize_sntp();

#ifdef __cplusplus
}
#endif

#endif // SNTP_H
