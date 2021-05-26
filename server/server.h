
#ifndef _SERVER_H_
#define _SERVER_H_

#define PORT "1234"     // port for connection

typedef enum
{
    SERV_PARAM_BACKLOG = 3,
    SERV_PARAM_BUF_LEN = 1024,
    SERV_PARAM_TIME_DIV = 1000000,
    SERV_PARAM_TIME_DIGITS = 7,             // buffer for time digits
    SERV_PARAM_FILE_NAME_PREFIX = 15,       // "/home/" + "/" + "receive_" string length
    SERV_PARAM_FILE_NAME_SUFFIX = 5,        // ".txt" string length with terminated null
    SERV_PARAM_FILE_NAME = (LOGIN_NAME_MAX + \
                            SERV_PARAM_FILE_NAME_PREFIX + \
                            SERV_PARAM_TIME_DIGITS + \
                            SERV_PARAM_FILE_NAME_SUFFIX),
} serv_param_t;

// /home/vitalii/
typedef enum
{
    NEED_TO_RUN,
    NEED_TO_STOP,
} need_to_t;

#endif  // _SERVER_H_