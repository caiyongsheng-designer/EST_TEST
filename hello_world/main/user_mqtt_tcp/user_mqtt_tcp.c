/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "utils_3Y.h"
#include "driver/uart.h"
#include "user_mqtt_tcp.h"
static const char *TAG = "MQTT_EXAMPLE";
static char * user_token;
 esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    char user_ctrol[7]={0};
    /*esp_mqtt_client_handle_t*/ client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "$thing/down/service/HDP55DKC4K/device_2", 0);
            ESP_LOGI(TAG, "sent subscribe service successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "$thing/down/property/HDP55DKC4K/device_2", 0);
            ESP_LOGI(TAG, "sent subscribe property successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            cJSON *token = cJSON_CreateObject();				//创建一个对象
            cJSON_AddStringToObject(token,"method","app_bind_token");	//添加字符串 
            cJSON_AddStringToObject(token,"clientToken","client-1234");	//添加字符串 
            cJSON *params = cJSON_CreateObject();				//创建一个对象
            cJSON_AddStringToObject(params,"token",user_token);	        
            cJSON_AddItemToObject(token,"params",params);
            char *json_data = cJSON_Print(token);				//JSON数据结构转换为JSON字符串
	        ESP_LOGI(TAG,"%s\n",json_data);						//输出字符串
            int msg_id;
            msg_id = esp_mqtt_client_publish(client, "$thing/up/service/HDP55DKC4K/device_2", json_data, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
	        cJSON_free(json_data);							//释放空间
	        cJSON_Delete(token);								//清除结构体
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
          
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
          
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
             cJSON * pJsonRoot = cJSON_Parse((const char*)event->data);   //判断是否为json字符
                if (pJsonRoot !=NULL)
                   {
                        ESP_LOGI(TAG,".....MQTT JSON数据收到\r\n");
                        cJSON * params = cJSON_GetObjectItem(pJsonRoot,"params");   //寻找参数
                        if(params){
                             ESP_LOGI(TAG,".....params JSON数据收到\r\n");
                             cJSON * Temperature = cJSON_GetObjectItem(params,"Temperature");
                             cJSON * mode = cJSON_GetObjectItem(params,"mode");
                             cJSON * power_switch = cJSON_GetObjectItem(params,"power_switch");
                             if (Temperature)
                             {
                              ESP_LOGI(TAG,"..... JSON全部解析\r\n");
                              user_ctrol[0]=0x5A;
                              user_ctrol[1]=0xA5;
                              user_ctrol[2]=(char)(Temperature->valueint);
                              user_ctrol[3]=0x00;
                              user_ctrol[4]=0x00;
                              user_ctrol[5]=0x5A;
                              user_ctrol[6]=0xA5;  
                              uart_write_bytes(UART_NUM_0,user_ctrol,7);
                             }else if (mode)
                             {
                              ESP_LOGI(TAG,"..... JSON全部解析\r\n");
                              user_ctrol[0]=0x5A;
                              user_ctrol[1]=0xA5;
                              user_ctrol[2]=0x00;
                              user_ctrol[3]=(char)(mode->valueint);;
                              user_ctrol[4]=0x00;
                              user_ctrol[5]=0x5A;
                              user_ctrol[6]=0xA5;  
                              uart_write_bytes(UART_NUM_0,user_ctrol,7);
                             }else if (power_switch)
                             {
                              ESP_LOGI(TAG,"..... JSON全部解析\r\n");
                              user_ctrol[0]=0x5A;
                              user_ctrol[1]=0xA5;
                              user_ctrol[2]=0x00;
                              user_ctrol[3]=0x00;
                              user_ctrol[4]=(char)(power_switch->valueint);
                              user_ctrol[5]=0x5A;
                              user_ctrol[6]=0xA5;  
                              uart_write_bytes(UART_NUM_0,user_ctrol,7);
                             }  
                        } 
                   }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    sanyuan * sanyuan2 = (sanyuan *)malloc(sizeof(sanyuan));
    sanyuan2=THY_Return();
     
    esp_mqtt_client_config_t mqtt_cfg = {
       // .uri = CONFIG_BROKER_URL,
       .host = "HDP55DKC4K.iotcloud.tencentdevices.com",//MQTT 地址
	   .port = 1883,   //MQTT端口
	   .username = sanyuan2->SY_username,//用户名
	   .password = sanyuan2->SY_password,//密码
       .client_id= sanyuan2->SY_Client_ID
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

  /*  esp_mqtt_client_handle_t*/ client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    free(sanyuan2);
}

void user_mqtt_tcp(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    //ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
   // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
   // ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
void mqtt_send(char * jsondata)
{
     int msg_id;
     msg_id = esp_mqtt_client_publish(client, "$thing/up/property/HDP55DKC4K/device_2", jsondata, 0, 1, 0);
     ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}
void get_user_token(char * token)
{
   user_token =token;

}