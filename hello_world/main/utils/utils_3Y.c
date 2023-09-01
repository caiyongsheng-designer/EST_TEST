//#include "limits.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils_base64.h"
#include "utils_hmac.h"
#include "utils_3Y.h"
#include "esp_log.h"
#include "HAL_Platform.h"
#include "limits.h"
//static const char *TAG = "3Y";
/* Max size of base64 encoded PSK = 64, after decode: 64/4*3 = 48*/
#define DECODE_PSK_LENGTH 48


/* MAX valid time when connect to MQTT server. 0: always valid */
/* Use this only if the device has accurate UTC time. Otherwise, set to 0 */
#define MAX_ACCESS_EXPIRE_TIMEOUT (0)


/* Max size of conn Id  */
#define MAX_CONN_ID_LEN (6)


char password[51]      = {0};
/* IoT C-SDK APPID */
#define QCLOUD_IOT_DEVICE_SDK_APPID     "12010126"
#define QCLOUD_IOT_DEVICE_SDK_APPID_LEN (sizeof(QCLOUD_IOT_DEVICE_SDK_APPID) - 1)

static void get_next_conn_id(char *conn_id)
{
    int i;
    srand((unsigned)HAL_GetTimeMs());
    for (i = 0; i < MAX_CONN_ID_LEN - 1; i++) {
        int flag = rand() % 3;
        switch (flag) {
            case 0:
                conn_id[i] = (rand() % 26) + 'a';
                break;
            case 1:
                conn_id[i] = (rand() % 26) + 'A';
                break;
            case 2:
                conn_id[i] = (rand() % 10) + '0';
                break;
        }
    }


    conn_id[MAX_CONN_ID_LEN - 1] = '\0';
}
static void HexDump(char *pData, uint16_t len)
{
    int i;

    for (i = 0; i < len; i++) {
       // HAL_Printf("0x%02.2x ", (unsigned char)pData[i]);
    }
   // HAL_Printf("\n");
}

sanyuan * THY_Return()
{
    char *product_id    = "HDP55DKC4K";
    char *device_name   = "device_2";
    char *device_secret = "jDeOUt82jCU9Vr2CiGoxrA==";
    char *Client_ID = "HDP55DKC4Kdevice_2";
    sanyuan * sanyuan1 = (sanyuan *)HAL_Malloc(sizeof(sanyuan));

    char *username     = NULL;
    int   username_len = 0;
    char  conn_id[MAX_CONN_ID_LEN];

    char username_sign[41] = {0};


    char   psk_base64decode[DECODE_PSK_LENGTH];
    size_t psk_base64decode_len = 0;
    long cur_timestamp = 0;
    /* first device_secret base64 decode */
    qcloud_iot_utils_base64decode((unsigned char *)psk_base64decode, DECODE_PSK_LENGTH, &psk_base64decode_len,
                                  (unsigned char *)device_secret, strlen(device_secret));
  //  HAL_Printf("device_secret base64 decode:");
    HexDump(psk_base64decode, psk_base64decode_len);

    /* second create mqtt username
     * [productdevicename;appid;randomconnid;timestamp] */
    cur_timestamp = HAL_Timer_current_sec() + MAX_ACCESS_EXPIRE_TIMEOUT / 1000;
    if (cur_timestamp <= 0 || MAX_ACCESS_EXPIRE_TIMEOUT <= 0) {
        cur_timestamp = LONG_MAX;
    }

    // 20 for timestampe length & delimiter
    username_len = strlen(product_id) + strlen(device_name) + QCLOUD_IOT_DEVICE_SDK_APPID_LEN + MAX_CONN_ID_LEN + 20;
    username     = (char *)HAL_Malloc(username_len);
    if (username == NULL) {
        HAL_Printf("malloc username failed!\r\n");
        sanyuan1->SY_Client_ID = Client_ID;
        sanyuan1->SY_password = password;
        sanyuan1->SY_username = username;
        sanyuan1->SY_ret = 0;
        return sanyuan1;
    }

    get_next_conn_id(conn_id);
    HAL_Snprintf(username, username_len, "%s%s;%s;%s;%ld", product_id, device_name, QCLOUD_IOT_DEVICE_SDK_APPID,
                conn_id, cur_timestamp);

    /* third use psk_base64decode hamc_sha1 calc mqtt username sign crate mqtt
     * password */
    utils_hmac_sha1(username, strlen(username), username_sign, psk_base64decode, psk_base64decode_len);
  //  HAL_Printf("username sign: %s\r\n", username_sign);
    HAL_Snprintf(password, 51, "%s;hmacsha1", username_sign);

    HAL_Printf("Client ID: %s%s\r\n", product_id, device_name);
    HAL_Printf("username : %s\r\n", username);
    HAL_Printf("password : %s\r\n", password);
   // HAL_Free(username);
    sanyuan1->SY_Client_ID = Client_ID;
    sanyuan1->SY_password = password;
    sanyuan1->SY_username = username;
    sanyuan1->SY_ret = 1;
    return sanyuan1;
}


