#ifndef _UTILS_3Y_
#define _UTILS_3Y_

typedef  struct SANYUAN{
    char * SY_Client_ID;
    char * SY_username;
    char * SY_password;
    uint8_t SY_ret;
}sanyuan;

sanyuan * THY_Return();
#endif