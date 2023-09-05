/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "user_station.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;
static int wifi_connect_sucess_flag=0;
static int wifi_connect_inquiry=0;
static uint8_t user_wifi_cut_mod = 0;
static char * user_ssid;
static char * user_passward;
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_connect_inquiry = 0;
        esp_wifi_connect();
         ESP_LOGI(TAG, "esp_wifi_connect");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
       if (wifi_connect_sucess_flag ==1)
        {  
            wifi_connect_inquiry = 0;
            ESP_ERROR_CHECK(esp_wifi_connect());
        }else{
            wifi_connect_inquiry = 0;
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            s_retry_num = 0;
        }
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connect_inquiry =1;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

uint8_t wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();
    uint8_t wifi_connect_flag = 0;
  //  wifi_config_t wifi_config ;
    wifi_config_t wifi_config = {
        .sta = {

        },
    };
    memset(wifi_config.sta.ssid,0x00,sizeof(wifi_config.sta.ssid));
    memset(wifi_config.sta.password,0x00,sizeof(wifi_config.sta.password));
    memcpy(wifi_config.sta.ssid,user_ssid,strlen(user_ssid));
    memcpy(wifi_config.sta.password,user_passward,strlen(user_passward));
    ESP_LOGE(TAG,"station ssid=%s",wifi_config.sta.ssid);
    ESP_LOGE(TAG,"station passward=%s",wifi_config.sta.password);
    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    xEventGroupSetBits(s_wifi_event_group, 0);
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "s_retry_num= %d",s_retry_num);
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
                 wifi_connect_flag =1;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
                 xEventGroupSetBits(s_wifi_event_group, 0);
                 wifi_connect_flag = 0;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        xEventGroupSetBits(s_wifi_event_group, 0);
        wifi_connect_flag = 0;
    }
    vEventGroupDelete(s_wifi_event_group);
   // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    return wifi_connect_flag;
}
void user_station_init()
{   
    while(!user_wifi_cut_mod)
    {
         vTaskDelay(1);
    }
    ESP_LOGI(TAG, "start station mode");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_LOGI(TAG, " esp_wifi_init");
    wifi_init_sta();
}
void user_wifi_cut_station()
{
   user_wifi_cut_mod = 1;
}
void user_wifi_connect_funtion(char * ssid,char * passward)
{   
    user_ssid =   ssid;
    user_passward =    passward;
}
int wifi_state_inquiry()
{
    return wifi_connect_inquiry;
}

