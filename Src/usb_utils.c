#include "usb_utils.h"
#include "fatfs.h"

const char *padding = "                    ";

char pathname[50] = "/";

UBaseType_t get_size(const char *file)
{
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    UBaseType_t file_size = 0;

    res = f_opendir(&dir, pathname); /* Open the directory */

    xprintf("%d\r\n", res == FR_OK);
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            // xprintf("%d\r\n", strlen(fno.fname));
            // xprintf("%d\r\n", strlen(file));
            // xprintf("%s %s %d\r\n", fno.fname , file, strcmp(fno.fname, file));

            if (res != FR_OK || fno.fname[0] == 0){
                break; /* Break on error or end of dir */
            }
            if (strcmp(fno.fname, file) == 0)
            {
                // xprintf("file found\r\n");
                file_size = fno.fsize;
                break;
            }
        }
        f_closedir(&dir);
    }
    return file_size;
}

void list_dir(char *path, uint8_t *files_list)
{
    FRESULT res;
    DIR dir;
    FILINFO file_info;
    static FILINFO fno;

    char fileData[200];
    files_list[0] = '\0';
    int list_index = 0;

    res = f_opendir(&dir, path); /* Open the directory */
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
                break; /* Break on error or end of dir */

            fileData[0] = (fno.fattrib & AM_DIR) ? 'd' : '-';                                                             //directory?
            (fno.fattrib & AM_RDO) ? sprintf(&(fileData[1]), "r--r--r--    ") : sprintf(&(fileData[1]), "rw-rw-rw-    "); //readable/writeable

            fileData[14] = (fno.fattrib & AM_DIR) ? '2' : '1'; //number of connections to file

            char fileSize[12]; //prepare padding and filesize string
            sprintf(fileSize, "%u", fno.fsize);
            uint8_t padLen = 12 - strlen(fileSize);

            sprintf(&(fileData[15]), " ftp     ftp %*.*s%s %s\r\n\0", padLen, padLen, padding, fileSize, fno.fname); //stick file owner, size and name

            int written = sprintf(&(files_list[list_index]), "%s", fileData);
            list_index += written;
        }

        f_closedir(&dir);
    }
}

void delete_file(char *path)
{
    f_unlink(path);
}
