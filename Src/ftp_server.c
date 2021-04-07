#include "lwip.h"
#include "lwip/api.h"
#include "term_io.h"

#include <string.h>
#include "ftp_responses.h"
#include "ftp_commands.h"
#include "ftp_server.h"
#include "ftp_config.h"

/*
fragmenty kodu dla serwera WWW
opracowano na podstawie dostepnych przykladow
*/

char *curr_dir = "/";

static char response[500];

//based on available code examples
static void ftp_server_serve(struct netconn *conn)
{
    struct netbuf *inbuf;
    err_t recv_err;
    char *buf;
    u16_t buflen;
    char name[20];
    char password[20];
    char dir_resp[50];

    /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
    xprintf("%s\r\n", ftp_init_connection_response);
    netconn_write(conn, ftp_init_connection_response, sizeof(ftp_init_connection_response), NETCONN_NOCOPY);

    uint8_t continue_transmission = 1;

    while (netconn_err(conn) == ERR_OK && continue_transmission)
    {

        recv_err = netconn_recv(conn, &inbuf);
        if (recv_err == ERR_OK)
        {
            netbuf_data(inbuf, (void **)&buf, &buflen);
            xprintf("%s\r\n", buf);

            switch (get_command(buf))
            {
            case AUTH_TLS:
                netconn_write(conn, ftp_ask_login_and_password_response, sizeof(ftp_ask_login_and_password_response), NETCONN_NOCOPY);
                break;
            case AUTH_SSL:
                netconn_write(conn, ftp_ask_login_and_password_response, sizeof(ftp_ask_login_and_password_response), NETCONN_NOCOPY);
                break;

            case USER_NAME:
                get_single_argument(buf, name);
                xprintf("Username: %s\r\n", name);

                netconn_write(conn, ftp_ask_for_password_response, sizeof(ftp_ask_for_password_response), NETCONN_NOCOPY);
                break;

            case USER_PASS:
                get_single_argument(buf, password);
                xprintf("Password: %s\r\n", password);

                if (strcmp(name, user_name) == 0 && strcmp(password, user_password) == 0)
                {
                    netconn_write(conn, ftp_login_successful_response, sizeof(ftp_login_successful_response), NETCONN_NOCOPY);
                }
                else
                {
                    xprintf("Wrong user %d or pass %d\r\n", strcmp(name, user_name), strcmp(password, user_password));
                    netconn_write(conn, ftp_wrong_login_respone, sizeof(ftp_wrong_login_respone), NETCONN_NOCOPY);
                    continue_transmission = 0;
                }
                break;

            case PWD:

                sprintf(dir_resp, "%s %s\r\n", ftp_pathname_response, "/");
                netconn_write(conn, dir_resp, sizeof(dir_resp), NETCONN_NOCOPY);
                break;

            default:
                xprintf("not found\r\n");
                netconn_write(conn, ftp_command_not_found_response, sizeof(ftp_command_not_found_response), NETCONN_NOCOPY);
                break;
            }

            netbuf_delete(inbuf);
            vTaskDelay(300);
        }
        else
        {
            break;
        }
    }

    /* Close the connection (server closes in HTTP) */
    netconn_close(conn);

    /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
    netbuf_delete(inbuf);
}

//based on available code examples
void ftp_server_netconn_thread(void const *arg)
{
    struct netconn *conn, *newconn;
    err_t err, accept_err;

    xprintf("FTP_server_netconn_thread\r\n");

    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);

    if (conn != NULL)
    {
        /* Bind to port 20 (FTP command) with default IP address */
        err = netconn_bind(conn, NULL, FTP_COMMAND_PORT);

        if (err == ERR_OK)
        {
            /* Put the connection into LISTEN state */
            netconn_listen(conn);

            while (1)
            {
                /* accept any icoming connection */
                accept_err = netconn_accept(conn, &newconn);
                if (accept_err == ERR_OK)
                {
                    /* serve connection */
                    ftp_server_serve(newconn);

                    /* delete connection */
                    netconn_delete(newconn);
                }
            }
        }
    }
}
