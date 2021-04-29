#ifndef FTP_SERVER_H
#define FTP_SERVER_H


#define FTP_COMMAND_PORT 21
#define FTP_DATA_PORT 23
#define DATA_BUF_SIZE 800
#define FRAME_SIZE 1300


void ftp_server_netconn_thread(void const *arguments);




#endif
