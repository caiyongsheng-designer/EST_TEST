#ifndef _USER_STATION_
#define _USER_STATION_

void user_station_init();
int wifi_state_inquiry();
void  user_wifi_cut_station();
void user_wifi_connect_funtion(char * ssid,char * passward);
uint8_t wifi_init_sta();
#endif