/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event.h"
#include "user_station.h"
#include "nvs_flash.h"
#include "user_http_request.h"
#include "user_uart.h"
#include "user_mqtt_tcp.h"
#include "utils_3Y.h"
#include "user_soft_ap.h"
#include "udp_server.h"
#include "esp_wifi.h"
#include "esp_log.h"
static const char *TAG = "main";
void app_main()
{   
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    user_uart_app();
   // nvs_write_data_to_flash(0,"","");
    wifi_stro * wifi_stro1 = nvs_read_data_from_flash();
  //  printf("Hello world!\n");
    /* Print chip information */
  /* esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");*/
if(wifi_stro1->wifi_stro_flag)
   {
   ESP_LOGI(TAG,"start from station");
   user_station_init();
   user_http_requser_init();
   user_mqtt_tcp();
   }else{
   ESP_LOGI(TAG,"start from soft ap");
   user_soft_ap();
   user_station_init();
   user_http_requser_init();
   user_mqtt_tcp();
   }
   free(wifi_stro1);
}
