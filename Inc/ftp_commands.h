#ifndef FTP_COMMANDS_H
#define FTP_COMMANDS_H

typedef enum ftp_command {
    AUTH_TLS = 0,
    AUTH_SSL = 1,
    USER_NAME = 2,
    USER_PASS = 3,
    PWD = 4
} ftp_command;

ftp_command get_command(char* request);

void get_single_argument(const char* request, char* buffer);


#endif