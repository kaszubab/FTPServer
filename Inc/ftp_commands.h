#ifndef FTP_COMMANDS_H
#define FTP_COMMANDS_H

typedef enum ftp_command {
    UNKNOWN = -2,
    NOT_SUPPORTED = -1,
    AUTH_TLS = 0,
    AUTH_SSL = 1,
    USER_NAME = 2,
    USER_PASS = 3,
    PWD = 4,
    BINARY_MODE = 5,
    ASCII_MODE = 6,
    PASV = 7,
    LIST = 8,
	SEND_FILE = 9,
	RECV_FILE = 10,
	CWD = 11,
    DELETE = 12,
} ftp_command;

ftp_command get_command(char* request);

void get_single_argument(const char* request, char* buffer);

void get_new_directory(const char *request, char * buffer);

void get_filename_argument(const char *request, char * buffer);

#endif
