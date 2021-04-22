#include "lwip.h"
#include "lwip/api.h"
#include "term_io.h"

#include <string.h>
#include "ftp_responses.h"
#include "ftp_commands.h"
#include "ftp_server.h"
#include "ftp_config.h"
#include "usb_utils.h"

/*
fragmenty kodu dla serwera WWW
opracowano na podstawie dostepnych przykladow
*/

char *curr_dir = "/";

static char response[500];

uint16_t port = FTP_DATA_PORT;

uint16_t get_data_port()
{
    if (port == 0)
        port = FTP_DATA_PORT;

    return port++;
}

struct netconn *new_connection(uint16_t new_port)
{

    struct netconn *new_conn;

    err_t err;

    new_conn = netconn_new(NETCONN_TCP);

    if (new_conn != NULL)
    {
        err = netconn_bind(new_conn, NULL, new_port);
        if (err == ERR_OK)
        {
            return new_conn;
        }
        else
        {
            return NULL;
        }
    }
    return NULL;
}

void process_list_command(struct netconn *conn, struct netconn *data_conn)
{
    /* tell that transmission has been started */
    netconn_write(conn, ftp_open_data_connection_response, sizeof(ftp_open_data_connection_response), NETCONN_NOCOPY);

    vTaskDelay(50);
    static uint8_t data_buf[DATA_BUF_SIZE];

    list_dir(curr_dir, data_buf);
    netconn_write(data_conn, data_buf, strlen(data_buf), NETCONN_NOCOPY);

    /* tell that transmission has ended */
    netconn_write(conn, ftp_closing_data_connection_response, sizeof(ftp_closing_data_connection_response), NETCONN_NOCOPY);

    return;
}

//based on available code examples
static void ftp_server_serve(struct netconn *conn)
{

    struct netconn *data_conn = NULL;
    struct netconn *temp_data_conn = NULL;

    struct netbuf *inbuf;
    err_t recv_err;
    char *buf;
    u16_t buflen;

    char name[20];
    char password[20];
    char message_buff[50];

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

                netconn_write(conn, ftp_ask_for_password_response, sizeof(ftp_ask_for_password_response), NETCONN_NOCOPY);
                break;

            case USER_PASS:
                get_single_argument(buf, password);
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
                sprintf(message_buff, "%s \"%s\" %s", ftp_pathname_response_left, "/", ftp_pathname_response_right);
                netconn_write(conn, message_buff, sizeof(message_buff), NETCONN_NOCOPY);
                break;

            case BINARY_MODE:
                netconn_write(conn, ftp_binary_mode_response, sizeof(ftp_binary_mode_response), NETCONN_NOCOPY);
                break;
            case ASCII_MODE:
                netconn_write(conn, ftp_command_not_implemented_response, sizeof(ftp_command_not_implemented_response), NETCONN_NOCOPY);
                break;

            case PASV:
                if (data_conn != NULL)
                {
                    netconn_delete(data_conn);
                }
                if (temp_data_conn != NULL)
                {
                    netconn_delete(temp_data_conn);
                }

                uint16_t new_port = get_data_port();

                temp_data_conn = new_connection(new_port);

                if (temp_data_conn)
                {
                    xprintf("Created data connection on port %d\r\n", new_port);
                    sprintf(message_buff, "%s%d).\r\n", ftp_passive_mode_response, new_port);
                    netconn_write(conn, message_buff, sizeof(message_buff), NETCONN_NOCOPY);

                    netconn_listen(temp_data_conn);

                    /* accept an incoming connection */
                    err_t accept_err = netconn_accept(temp_data_conn, &data_conn);
                    if (accept_err == ERR_OK)
                    {
                        xprintf("Opened data connection on port %d\r\n", new_port);
                    }
                }
                else
                {
                    xprintf("Error during opening connection\r\n");
                    netconn_write(conn, ftp_service_unavailable_response, sizeof(ftp_service_unavailable_response), NETCONN_NOCOPY);
                }
                break;

            case LIST:
                if (data_conn == NULL)
                {
                    netconn_write(conn, ftp_request_passive_mode_response, sizeof(ftp_request_passive_mode_response), NETCONN_NOCOPY);
                    xprintf("Can't no data conection avaiable\r\n");
                    break;
                }

                process_list_command(conn, data_conn);
                netconn_close(data_conn);
                netconn_close(temp_data_conn);
                vTaskDelay(100);
                netconn_delete(data_conn);
                netconn_delete(temp_data_conn);
                //data_conn = temp_data_conn = NULL;
                continue_transmission = 0;

                break;
            case NOT_SUPPORTED:
                netconn_write(conn, ftp_command_not_implemented_response, sizeof(ftp_command_not_implemented_response), NETCONN_NOCOPY);
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
