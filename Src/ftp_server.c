#include "lwip.h"
#include "lwip/api.h"
#include "term_io.h"
#include "fatfs.h"
#include "usb_host.h"

#include <string.h>
#include "ftp_responses.h"
#include "ftp_commands.h"
#include "ftp_server.h"
#include "ftp_config.h"
#include "usb_utils.h"


extern struct netif gnetif;

/*
fragmenty kodu dla serwera WWW
opracowano na podstawie dostepnych przykladow
*/

static char response[500];

uint16_t port = FTP_DATA_PORT;

uint16_t get_data_port()
{
    if (port == 0)
        port = FTP_DATA_PORT;

    return port++;
}

void file_send(struct netconn * conn, struct netconn * data_conn, const char * file) {
	netconn_write(conn, ftp_open_data_connection_response, sizeof(ftp_open_data_connection_response), NETCONN_NOCOPY);

	vTaskDelay(50);

	static uint8_t buffer[FRAME_SIZE];
	UBaseType_t read_size;


	FIL file_ptr;      /* File objects */
	FRESULT fr;          /* FatFs function common result code */

	/* Open source file on the drive 1 */


	char *filename = NULL;

	filename = strdup(file);
	/* ... */
	filename[strlen(filename)-1] = '\0';

	char full_path[100];
    sprintf(full_path, "%s/%s", pathname, filename);

	xprintf("Sending buffer with %s \n", full_path);
	UBaseType_t file_size = get_size(filename);

	fr = f_open(&file_ptr, full_path, FA_READ);

	while (file_size > 0) {
		if (fr == FR_OK) {
				UBaseType_t size_to_read = file_size > FRAME_SIZE ? FRAME_SIZE : file_size;
				f_read(&file_ptr, buffer, size_to_read, &read_size);
				netconn_write(data_conn, buffer, read_size, NETCONN_COPY);
				file_size -= read_size;
			}
		}
	/* Close open files */
	f_close(&file_ptr);

	netconn_write(conn, ftp_closing_data_connection_response, sizeof(ftp_closing_data_connection_response), NETCONN_NOCOPY);
	return;
}

void file_recv(struct netconn * conn, struct netconn * data_conn, const char * file) {
	/* tell that transmission has been started */
	netconn_write(conn, ftp_data_listening_response, sizeof(ftp_data_listening_response), NETCONN_NOCOPY);

	vTaskDelay(50);
	/*here should be data transmission on data port */

	static uint8_t buffer[FRAME_SIZE];
	UBaseType_t incoming_size;


	FIL file_ptr;
	FRESULT fr;

	char *filename = NULL;
	filename = strdup(file);
	filename[strlen(filename)-1] = '\0';

	char full_path[100];
    sprintf(full_path, "%s/%s", pathname, filename);

	/* Open source file on the drive 1 */
	fr = f_open(&file_ptr, full_path, FA_WRITE | FA_CREATE_ALWAYS);

	xprintf("Function results %d   %d  \n", fr);


	uint8_t transmission_not_finished = 1;

	while (transmission_not_finished) {
		struct netbuf * inbuf;
		err_t recv_err;
		char * buf;
		u16_t buflen;

		recv_err = netconn_recv(data_conn, &inbuf);

		if (recv_err == ERR_OK) {
			if (netconn_err(data_conn) == ERR_OK) {
				netbuf_data(inbuf, (void**)&buf, &buflen);
				if (fr == FR_OK) {
					f_write(&file_ptr, buf, buflen, incoming_size);
				}
				netbuf_delete(inbuf);
			} else {
				break;
			}
		} else {
			break;
		}
	}

	/* Close open files */
	f_close(&file_ptr);


	/* tell that transmission has ended */
	netconn_write(conn, ftp_closing_data_connection_response, sizeof(ftp_closing_data_connection_response), NETCONN_NOCOPY);


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

    list_dir(pathname, data_buf);
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

    char name[20];
    char password[20];
    char message_buff[50];
	char filename[100];

    /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
    netconn_write(conn, ftp_init_connection_response, sizeof(ftp_init_connection_response), NETCONN_NOCOPY);

    uint8_t continue_transmission = 1;

    while (netconn_err(conn) == ERR_OK && continue_transmission)
    {
        struct netbuf *inbuf;
		err_t recv_err;
		char * buf;
		u16_t buflen;
        recv_err = netconn_recv(conn, &inbuf);

        if (recv_err == ERR_OK)
        {
            netbuf_data(inbuf, (void **)&buf, &buflen);

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
                netconn_write(conn, ftp_login_successful_response, sizeof(ftp_login_successful_response), NETCONN_NOCOPY);

//                if (strcmp(name, user_name) == 0 && strcmp(password, user_password) == 0)
//                {
//                    netconn_write(conn, ftp_login_successful_response, sizeof(ftp_login_successful_response), NETCONN_NOCOPY);
//                }
//                else
//                {
//                    xprintf("Wrong user %d or pass %d\r\n", strcmp(name, user_name), strcmp(password, user_password));
//                    netconn_write(conn, ftp_wrong_login_respone, sizeof(ftp_wrong_login_respone), NETCONN_NOCOPY);
//                    continue_transmission = 0;
//                }
                break;

            case PWD:
                sprintf(message_buff, "%s \"%s\" %s", ftp_pathname_response_left, pathname, ftp_pathname_response_right);
                netconn_write(conn, message_buff, sizeof(message_buff), NETCONN_NOCOPY);
                break;
			case CWD:
				get_new_directory(buf, pathname);
				netconn_write(conn, ftp_pathname_changed, sizeof(ftp_pathname_changed), NETCONN_NOCOPY);
				break;
            case BINARY_MODE:
                netconn_write(conn, ftp_binary_mode_response, sizeof(ftp_binary_mode_response), NETCONN_NOCOPY);
                break;
            case ASCII_MODE:
                netconn_write(conn, ftp_ascii_mode_response, sizeof(ftp_ascii_mode_response), NETCONN_NOCOPY);
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
                	char my_ip[50];
					sprintf(my_ip,
                	      "%d,%d,%d,%d,",
                	      ip4_addr1_16(netif_ip4_addr(&gnetif)),
                	      ip4_addr2_16(netif_ip4_addr(&gnetif)),
                	      ip4_addr3_16(netif_ip4_addr(&gnetif)),
                	      ip4_addr4_16(netif_ip4_addr(&gnetif)));
                    xprintf("Created data connection on port %d\r\n", new_port);
                    sprintf(message_buff, "%s%s0,%i).\r\n", ftp_passive_mode_response, my_ip, new_port);
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
			case SEND_FILE:
				xprintf("Is this send file? \r\n");


				if (data_conn == NULL) {
					netconn_write(conn, ftp_request_passive_mode_response, sizeof(ftp_request_passive_mode_response), NETCONN_NOCOPY);
                    xprintf("Can't no data connection avaiable\r\n");
					break;
				}
				get_filename_argument(buf, filename);
				xprintf("Filename %s\n", filename);
				file_send(conn, data_conn, filename);
				netconn_close(data_conn);
				netconn_close(temp_data_conn);
				vTaskDelay(100);
				netconn_delete(data_conn);
				netconn_delete(temp_data_conn);
//				data_conn = temp_data_conn = NULL;
				continue_transmission = 0;
				break;
			case RECV_FILE:
				if (data_conn == NULL) {
					netconn_write(conn, ftp_request_passive_mode_response, sizeof(ftp_request_passive_mode_response), NETCONN_NOCOPY);
                    xprintf("Can't no data conection avaiable\r\n");
					break;
				}
				get_filename_argument(buf, filename);
				xprintf("Filename %s\n", &filename);
				file_recv(conn, data_conn, filename);
				netconn_close(data_conn);
				netconn_close(temp_data_conn);
				vTaskDelay(100);
				netconn_delete(data_conn);
				netconn_delete(temp_data_conn);
				//data_conn = temp_data_conn = NULL;
				continue_transmission = 0;
//				break;
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
