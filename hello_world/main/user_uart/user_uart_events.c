// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "user_uart.h"
#include "user_station.h"
#include "esp_wifi.h"
#include "user_http_request.h"
#include "cJSON.h"
#include "user_mqtt_tcp.h"
static const char *TAG = "uart_events";

/**
 * This example shows how to use the UART driver to handle special UART events.
 *
 * It also reads data from UART0 directly, and echoes it to console.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: on
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */

#define EX_UART_NUM UART_NUM_0

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);

    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]:");
                   // uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
                    if((dtmp[0]==0x5A)&&(dtmp[1] == 0xA5)&&
                       (dtmp[3] == 0x5A)&&dtmp[4] == 0xA5)
                     {
                        switch (dtmp[2])
                        {
                        case 1:
                        if((wifi_state_inquiry())&&(User_http_netif_init_state_quire()))
                        {
                             ESP_LOGI(TAG,"user_report_Y");
                             User_Weather_Report();
                             bzero(dtmp, RD_BUF_SIZE);
                        }else{;}
                            
                            break;
                        case 2:
                            
                             if(wifi_state_inquiry())
                             {
                                printf("Connect true\r\n");
                             }else{
                                printf("Connect fail\r\n");
                             }
                            break;
                            
                        default:
                            printf("receive fail\r\n");
                            break;
                        }
                    
                    }else if((dtmp[0]==0x5A)&&(dtmp[1] == 0xA5)&&
                       (dtmp[7] == 0x5A)&&dtmp[8] == 0xA5)
                       {
                          ESP_LOGI(TAG,".....1");//输出字符串
                          cJSON *TCP = cJSON_CreateObject();				//创建一个对象
                          cJSON_AddStringToObject(TCP,"method","report");	//添加字符串 
                          cJSON_AddStringToObject(TCP,"clientToken","123");	//添加字符串 
                          cJSON_AddStringToObject(TCP,"timestamp","1212121221");	//添加字符串 
                          cJSON *params = cJSON_CreateObject();				//创建一个对象
                          cJSON_AddNumberToObject(params,"power_switch",dtmp[2]);	
                          cJSON_AddNumberToObject(params,"Temperature",dtmp[3]);
                          cJSON_AddNumberToObject(params,"mode",dtmp[4]);
                          cJSON_AddNumberToObject(params,"current_humidity",dtmp[5]);
                          cJSON_AddNumberToObject(params,"current_temp",dtmp[6]);
                          cJSON_AddItemToObject(TCP,"params",params);
                          char *json_data = cJSON_Print(TCP);				//JSON数据结构转换为JSON字符串
	                      ESP_LOGI(TAG,"%s\n",json_data);						//输出字符串
                          mqtt_send(json_data);
	                      cJSON_free(json_data);							//释放空间
	                      cJSON_Delete(TCP);								//清除结构体

                    }else{
                       ESP_LOGI(TAG,"user_report_N");
                    }
                    break;

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;

                // Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;

                // Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void user_uart_app()
{
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,                  //74800
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(EX_UART_NUM, &uart_config);

    // Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart0_queue, 0);

    // Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}