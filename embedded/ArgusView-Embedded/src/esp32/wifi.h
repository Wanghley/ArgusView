#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

void wifi_init_sta(const char *ssid, const char *password);
void pause_wifi();
void resume_wifi();
bool is_wifi_enabled_status();

#endif // WIFI_H
