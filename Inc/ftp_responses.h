#ifndef FTP_RESPONSES_H
#define FTP_RESPONSES_H

const char ftp_init_connection_response[] = "220 init message\r\n";
const char ftp_ask_login_and_password_response[] = "530 Please login with USER and PASS.\r\n";
const char ftp_ask_for_password_response[] = "331 Please enter the password.\r\n";
const char ftp_login_successful_response[] = "230 Login successful.\r\n";
const char ftp_wrong_login_respone[] = "430 Invalid username or password.\r\n";
const char ftp_command_not_found_response[] = "500 command unrecognized\r\n";
const char ftp_command_not_implemented_response[] = "502 Command not implemented\r\n";
const char ftp_pathname_response_left[] = "257 ";
const char ftp_pathname_response_right[] = "is the current directory\r\n";
const char ftp_binary_mode_response[] = "200 Switching to Binary mode.\r\n";
const char ftp_passive_mode_response[] = "227 Entering Passive Mode (";
const char ftp_service_unavailable_response[] = "421\r\n";
const char ftp_request_passive_mode_response[] = "425 Use PASV first.\r\n";
const char ftp_open_data_connection_response[] = "150 Data transmission has been started.\r\n";
const char ftp_data_listening_response[] = "150 OK to send data.\r\n";
const char ftp_closing_data_connection_response[] = "226 Transfer complete.\r\n";
const char ftp_pathname_changed[] = "250 Directory succesfully changed.\r\n";
const char ftp_ascii_mode_response[] = "200 Switching to ASCII mode.\r\n";

#endif
