#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_
#include <string.h>
#include <stdint.h>
typedef struct WIFI_STRO
{
  int32_t wifi_stro_flag;
  char  save_wifi_ssid[32];
  char  save_wifi_passward[64];
}wifi_stro;
void udp_server();
wifi_stro * nvs_read_data_from_flash(void);
void nvs_write_data_to_flash(int32_t param1,char * param2,char * param3);
#endif
