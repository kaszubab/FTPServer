#include "ftp_commands.h"
#include <string.h>
#include <stdint.h>

const char *auth_tls = "AUTH TLS";
const char *auth_ssl = "AUTH SSL";
const char *user = "USER";
const char *pass = "PASS";
const char *pwd = "PWD";
const char *type = "TYPE";
const char *pasv = "PASV";
const char *list = "LIST";

ftp_command get_command(char *request)
{
    if (strncmp(request, auth_tls, 8) == 0)
        return AUTH_TLS;

    if (strncmp(request, auth_ssl, 8) == 0)
        return AUTH_SSL;

    if (strncmp(request, user, 4) == 0)
        return USER_NAME;

    if (strncmp(request, pass, 4) == 0)
        return USER_PASS;

    if (strncmp(request, pwd, 3) == 0)
        return PWD;

    if (strncmp(request, type, 4) == 0)
    {
        if (request[5] == 'I')
            return BINARY_MODE;
        else if (request[5] == 'A')
            return ASCII_MODE;
        else
            return NOT_SUPPORTED;
    }

    if (strncmp(request, pasv, 4) == 0)
        return PASV;
    
    if (strncmp(request, list, 4) == 0)
        return LIST;

    return UNKNOWN;
}

void get_single_argument(const char *request, char *buffer)
{
    uint8_t buff_index = 0;
    for (int i = 5; i < strlen(request); i++)
    {
        if (request[i] == '\r' || request[i] == ' ')
            break;
        buffer[buff_index] = request[i];
        buff_index++;
    }
}
