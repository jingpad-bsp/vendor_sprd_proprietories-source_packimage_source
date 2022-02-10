/******************************************************************************
**
 *  Parse packed image tool
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
**
******************************************************************************/
/**--------------------------------------------------------------------------*
 **                         Include Files                                    *
 **--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
/**--------------------------------------------------------------------------*
 **                         MACRO DEFINITION                                 *
 **--------------------------------------------------------------------------*/
#define MODEM_MAGIC           "SCI1"
#define MODEM_HDR_SIZE        12  // size of a block
#define SCI_TYPE_MODEM_BIN    1
#define SCI_TYPE_PARSING_LIB  2
#define MODEM_LAST_HDR        0x100
#define MODEM_SHA1_HDR        0x400
#define MODEM_SHA1_SIZE       20

#define FILE_NAME_SIZE 2048
/**--------------------------------------------------------------------------*
 **                         TYPE AND CONSTANT                                *
 **--------------------------------------------------------------------------*/
typedef struct {
    unsigned int type_flags;
    unsigned int offset;
    unsigned int length;
} data_block_header_t;
/**--------------------------------------------------------------------------*
 **                         FUNCTIONS                                        *
 **--------------------------------------------------------------------------*/
/**--------------------------------------------------------------------------*
 ** this function compare the first four bytes in image and return 1 if      *
 ** equals to MODEM_MAGIC                                                    *
 **--------------------------------------------------------------------------*/
int is_packed_image(char *data)
{
    if (NULL == data) return 0;
    if (memcmp(data, MODEM_MAGIC, sizeof(MODEM_MAGIC)))
        return 0;
    return 1;
}

/*
 *  this function parse new packed modem image and return modem code offset and length
 */
static void get_modem_info(unsigned char *data, unsigned int *code_offset, unsigned int *code_len)
{
	unsigned int offset = 0, hdr_offset = 0, length = 0;
	unsigned char hdr_buf[MODEM_HDR_SIZE << 3] = {0};
	unsigned char read_len;
	unsigned char result = 0; // 0:OK, 1:not find, 2:some error occur
	data_block_header_t *hdr_ptr = NULL;

	read_len = sizeof(hdr_buf);
	memcpy(hdr_buf, data, read_len);

    do {
      if (!hdr_offset) {
        if (memcmp(hdr_buf, MODEM_MAGIC, sizeof(MODEM_MAGIC))) {
          result = 2;
          printf("old image format!\n");
          break;
        }

        hdr_ptr = (data_block_header_t *)hdr_buf + 1;
        hdr_offset = MODEM_HDR_SIZE;
      } else {
        hdr_ptr = (data_block_header_t *)hdr_buf;
      }

      data_block_header_t* endp
          = (data_block_header_t*)(hdr_buf + sizeof hdr_buf);
      int found = 0;
      while (hdr_ptr < endp) {
        unsigned int type = (hdr_ptr->type_flags & 0xff);
        if (SCI_TYPE_MODEM_BIN == type) {
          found = 1;
          break;
        }

        /*  There is a bug (622472) in MODEM image generator.
         *  To recognize wrong SCI headers and correct SCI headers,
         *  we devise the workaround.
         *  When the MODEM image generator is fixed, remove #if 0.
         */
#if 0
        if (hdr_ptr->type_flags & MODEM_LAST_HDR) {
          result = 2;
          MODEM_LOGE("no modem image, error image header!!!\n");
          break;
        }
#endif
        hdr_ptr++;
      }
      if (!found) {
        result = 2;
        printf("no MODEM exe found in SCI header!");
      }

      if (result != 1) {
        break;
      }
    } while (1);

    if (!result) {
      offset = hdr_ptr->offset;
      if (hdr_ptr->type_flags & MODEM_SHA1_HDR) {
        offset += MODEM_SHA1_SIZE;
      }
      length = hdr_ptr->length;
    }

	*code_offset = offset;
	*code_len = length;
}

#if 0
static void *splitimg_load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;
    int psz;//raw size + padding

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    psz = ((sz + 15)/16)*16;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(psz);
    if(data == 0) goto oops;

    memset(data, 0, psz);
    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = psz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}
#endif

static void *splitimg_load_file(const char *fn)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    memset(data, 0, sz);
    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

static void usage(void)
{
    printf("============================================== \n");
    printf("Usage: \n");
    printf("$./splitimg <filename> \n");
    printf("---------------------------------------------- \n");
    printf("-filename    --the image to be split \n");
    printf("============================================== \n");
}

int main(int argc, char* argv[])
{
    char            filename[FILE_NAME_SIZE] = "0";
    char            fnewname[FILE_NAME_SIZE] = "0";
    void           *payload = NULL;
    void           *p_save = NULL;
    int             fd = -1;
//    unsigned int    imgpadsize = 0;
    unsigned int    modem_offset = 0;
    unsigned int    modem_len = 0;

    if (argc != 2) {
        usage();
        return 1;
    }

    memset(filename, 0, sizeof(filename));
    memset(fnewname, 0, sizeof(fnewname));
    strcpy(filename,argv[1]);
//    payload = splitimg_load_file(filename, &imgpadsize);
    payload = splitimg_load_file(filename);
    if(payload == NULL) {
        printf("warning: could not load %s \n", filename);
        return 1;
    }
    // Check packed image
    if (is_packed_image(payload)) {
        printf("new packed modem image is found!\n");
        get_modem_info(payload, &modem_offset, &modem_len);
        printf("modem offset is %d \n", modem_offset);
        printf("modem size is %d \n", modem_len);

#if 0
        // Create new file with other parts(exclusive runtime firmware)
        strcpy(fnewname, filename);
        strcat(fnewname,".div");
        p_save = (char *)payload;
        fd = open(fnewname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if(fd < 0) {
            printf("error: could not create '%s'\n", filename);
            goto fail;
        }
        if((unsigned int)write(fd, p_save, modem_offset) != modem_offset) {
            printf("write failed!\n");
            goto fail;
        }

        // Replace the original file with runtime firmware
        p_save = (char *)payload + modem_offset;
        fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if(fd < 0) {
            printf("error: could not create '%s'\n", filename);
            goto fail;
        }
        if((unsigned int)write(fd, p_save, modem_len) != modem_len) {
            printf("write failed!\n");
            goto fail;
        }
#endif
        // Create new file with with runtime firmware
        strcpy(fnewname, filename);
        strcat(fnewname,".div");
        p_save = (char *)payload + modem_offset;
        fd = open(fnewname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if(fd < 0) {
            printf("warning: could not create '%s'\n", fnewname);
            goto fail;
        }
        if((unsigned int)write(fd, p_save, modem_len) != modem_len) {
            printf("write failed!\n");
            goto fail;
        }
    } else {
        printf("not packed image, return.\n");
    }
    free(payload);
    if (fd >= 0 ) close(fd);
    return 0;
fail:
    if (fd >= 0 ) close(fd);
    free(payload);
    printf("warning: failed writing '%s'\n", filename);
    return 1;
}

