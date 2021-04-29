#ifndef USB_UTILS_H_
#define USB_UTILS_H_

#include "stdint.h"

extern char pathname[];

void list_dir(char* path, uint8_t* files_list);


#endif
