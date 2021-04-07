#ifndef FTP_RESPONSES_H
#define FTP_RESPONSES_H

const char ftp_init_connection_response[] = "220 init message\r\n";
const char ftp_ask_login_and_password_response[] = "530 Please login with USER and PASS.\r\n";
const char ftp_ask_for_password_response[] = "331 Please enter the password.\r\n";
const char ftp_login_successful_response[] = "230 Login successful.\r\n";
const char ftp_wrong_login_respone[] = "430 Invalid username or password.\r\n";
const char ftp_command_not_found_response[] = "500 command unrecognized\r\n";
const char ftp_command_not_implemented_response[] = "502 Command not implemented\r\n";
const char ftp_pathname_response[] = "257 ";



#endif
