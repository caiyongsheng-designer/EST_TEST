/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "user_station.h"
#include "user_mqtt_tcp.h"
#include "esp_wifi.h"
#include "udp_server.h"
#define PORT 8266
static const char *TAG = "example";

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");
        
        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket binded");

        while (1) {

            ESP_LOGI(TAG, "Waiting for data");

            struct sockaddr_in sourceAddr;

            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string

                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
                cJSON * pJsonRoot = cJSON_Parse((const char*)rx_buffer);   //判断是否为json字符
                if (pJsonRoot !=NULL)
                   {
                    ESP_LOGI(TAG,".....UDP JSON数据收到\r\n");
                    cJSON * ssid = cJSON_GetObjectItem(pJsonRoot,"ssid");
                    cJSON * password = cJSON_GetObjectItem(pJsonRoot,"password");
                    cJSON * token = cJSON_GetObjectItem(pJsonRoot,"token");
                    if (ssid && password && token )
                        {
                         ESP_LOGI(TAG,"..... JSON全部解析\r\n");
                         ESP_LOGI(TAG,"ssid=%s\r\n",ssid->valuestring);
                         ESP_LOGI(TAG,"password=%s\r\n",password->valuestring);
                         ESP_LOGI(TAG,"token=%s\r\n",token->valuestring);

                         cJSON *UDP_TX = cJSON_CreateObject();				//创建一个对象
                         cJSON_AddStringToObject(UDP_TX,"cmdType","2");	//添加字符串 
                         cJSON_AddStringToObject(UDP_TX,"productId","HDP55DKC4K");	//添加字符串 
                         cJSON_AddStringToObject(UDP_TX,"deviceName","device_2");	//添加字符串 
                         cJSON_AddStringToObject(UDP_TX,"protoVersion","2.0");	//添加字符串      
                         char *json_data = cJSON_Print(UDP_TX);				//JSON数据结构转换为JSON字符串
	                     ESP_LOGI(TAG,"%s\n",json_data);						//输出字符串
                         int err = sendto(sock, json_data, strlen(json_data), 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                         if (err < 0) {
                                 ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                                  break;
                            }
                         vTaskDelay(100/portTICK_PERIOD_MS);
                         int err1 = sendto(sock, json_data, strlen(json_data), 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                         if (err1 < 0) {
                                 ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                                  break;
                            }
	                      cJSON_free(json_data);							//释放空间
	                      cJSON_Delete(UDP_TX);								//清除结构体  
                          user_wifi_connect_funtion(ssid->valuestring,password->valuestring);
                          get_user_token(token->valuestring);
                          shutdown(sock, 0);
                          close(sock);
                          user_wifi_cut_station();  
                          vTaskDelete(NULL);
                           
                        } 
                   }         
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void udp_server()
{
 //   ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
  //  ESP_ERROR_CHECK(esp_event_loop_create_default());

// ESP_ERROR_CHECK(example_connect());
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}
wifi_stro * nvs_read_data_from_flash(void)
{
    nvs_handle handle;
    wifi_stro * wifi_stro1 = (wifi_stro*)malloc(sizeof(wifi_stro));
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    static const char *DATA2 = "param 2";
    static const char *DATA3 = "param 3";
    uint32_t str_length_ssid = 32;
    uint32_t str_length_passward = 64;
    ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK ( nvs_get_i32(handle, DATA1, &(wifi_stro1->wifi_stro_flag)));
    ESP_ERROR_CHECK ( nvs_get_str(handle, DATA2, wifi_stro1->save_wifi_ssid,&str_length_ssid));
    ESP_ERROR_CHECK ( nvs_get_str(handle, DATA3, wifi_stro1->save_wifi_passward,&str_length_passward));
    printf("[data1]: ssid:%s passwd:%s flag:%d\r\n", wifi_stro1->save_wifi_ssid,wifi_stro1->save_wifi_passward,wifi_stro1->wifi_stro_flag);
    nvs_close(handle);
    return wifi_stro1;
}
void nvs_write_data_to_flash(int32_t param1,char * param2,char * param3)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    static const char *DATA2 = "param 2";
    static const char *DATA3 = "param 3";
    ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK( nvs_set_i32( handle, DATA1, param1) );
    ESP_ERROR_CHECK( nvs_set_str( handle, DATA2, param2) );
    ESP_ERROR_CHECK( nvs_set_str( handle, DATA3, param3) );
    ESP_ERROR_CHECK( nvs_commit(handle) );
    nvs_close(handle);
}

