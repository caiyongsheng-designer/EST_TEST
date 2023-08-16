/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include <netdb.h>
#include <sys/socket.h>

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.seniverse.com"
#define WEB_PORT 80
#define WEB_URL "https://api.seniverse.com/v3/weather/now.json?key=S0xJMtL9jJ2z7U5eS&location=beijing&language=en&unit=c"
static EventGroupHandle_t s_http_get_weather_report;
static const char *TAG = "example";
static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    uint8_t _rn =5;
    char bodybuf [1024];
    uint16_t j=0; 
    char recv_buf[1024];
    while(1) {
         xEventGroupWaitBits(s_http_get_weather_report,
            1,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
       xEventGroupSetBits(s_http_get_weather_report, 0);

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");
        bzero(bodybuf , sizeof(bodybuf));
        j=0;_rn=5;
        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                  // putchar(recv_buf[i]);
                   if (_rn != 0xFF)  {
                       //判断\r后是否紧跟\n或者\r\n后是否紧跟\r，不是说明后面的还是头
                       if((( _rn ==1 )&&(recv_buf[i] != 0x0A)) ||((_rn == 2)&&(recv_buf[i] != 0x0D))) _rn = 5;
                       if(recv_buf[i] == 0x0D){
                        if(_rn == 2) _rn = 3; // \r\n后紧跟\r
                        if(_rn == 5) _rn = 1; //第一次收到\r
                       }
                       if((recv_buf[i] == 0x0A)&&((_rn==1)||(_rn==3))){
                        if(_rn==3) _rn=0xFF; // \r\n\r后紧接\n，说明接下来就是body
                        else _rn = 2; //\r后紧接\n
                       }
                   }
                   else {
                    bodybuf[j++] = recv_buf[i];
                    } 
                }           
            } while(r > 0);
            bodybuf[j] = 0;
            ESP_LOGI(TAG, ((const char *)bodybuf));
            cJSON * pJsonRoot = cJSON_Parse((const char*)bodybuf);   //判断是否为json字符
                if (pJsonRoot !=NULL)
                   {
                        ESP_LOGI(TAG,".....JSON数据收到\r\n");
                        cJSON * results = cJSON_GetObjectItem(pJsonRoot,"results");
                        if(results){
                           cJSON * array = cJSON_GetArrayItem(results,0);
                           if(array)
                           {
                            cJSON * location = cJSON_GetObjectItem(array,"location");
                            if(location){
                                cJSON * name = cJSON_GetObjectItem(location,"name");
                                if(name){
                                   // printf ("name=%s\r\n",name->valuestring);
                                }
                              }
                            cJSON * now = cJSON_GetObjectItem(array,"now");
                            if(now){
                                cJSON * temper = cJSON_GetObjectItem(now,"temperature");
                                cJSON *wear = cJSON_GetObjectItem(now,"text");
                                if(temper && wear) 
                                {
                                    printf("5AA5temperature=%s\r\nwear=%s\r\n",temper->valuestring,wear->valuestring);
                                }else{printf("Get Report Error");}
                              }else{printf("Get Report Error");}
                           }else{printf("Get Report Error");}
                        }else{printf("Get Report Error");}
                    }else{printf("Get Report Error");}
        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);
       
   /*    for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }*/
        ESP_LOGI(TAG, "Starting again!");
    }
}

void user_http_requser_init()
{
    s_http_get_weather_report = xEventGroupCreate();
    xEventGroupSetBits(s_http_get_weather_report, 0);
    ESP_ERROR_CHECK(esp_netif_init());
    xTaskCreate(&http_get_task, "http_get_task", 16384, NULL, 5, NULL);
}
void User_Weather_Report()
{
  xEventGroupSetBits(s_http_get_weather_report, 1);
}
