#include <system/ff.h>
#include <system/ver.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

int main(int argc, char* argv[]) {
    FIL* pf = (FIL*)malloc(sizeof(FIL));
    if (!pf) { 
        fprintf(stderr, "Not enough RAM\n");
        return 1;
    }
    FRESULT r = f_open(pf, "/.extfs", FA_READ);
    if (r != FR_OK) {
        free(pf);
        fprintf(stderr, "/.extfs not found or corrupted\n");
        return 2;
    }
    UINT br;
    char type;
    uint32_t hash;
    uint16_t strsize;
    while(!f_eof(pf)) {
        if (f_read(pf, &type, 1, &br) != FR_OK || br != 1) break;
        if (f_read(pf, &hash, sizeof(hash), &br) != FR_OK || br != sizeof(hash)) break;
        if (f_read(pf, &strsize, sizeof(strsize), &br) != FR_OK || br != sizeof(strsize) || strsize < 2) break;
        char* buf = malloc(strsize + 1);
        if (!buf) break;
        buf[strsize] = 0;
        if (f_read(pf, buf, strsize, &br) != FR_OK || strsize != br) {
            free(buf);
            break;
        }
        uint32_t ohash = 0; // mode for 'O' case
        if (f_read(pf, &ohash, sizeof(ohash), &br) != FR_OK || br != sizeof(ohash)) goto brk2;
        char* obuf = 0;
        if (type == 'H') {
            if (f_read(pf, &strsize, sizeof(strsize), &br) != FR_OK || br != sizeof(strsize) || strsize < 2) goto brk1;
            obuf = (char*)malloc(strsize + 1);
            if (obuf) {
                obuf[strsize] = 0;
                if (f_read(pf, obuf, strsize, &br) != FR_OK || strsize != br) {
                    brk1: free(obuf);
                    brk2: free(buf);
                    break;
                }
            }
        }
        if (obuf) { // H
            printf("%c [%p] **** name: %s -> %s [%p]\n", type, hash, buf, obuf, ohash);
            free(obuf);
        } else {
            printf("%c [%p] %04o name: %s\n", type, hash, ohash & 07777, buf);
        }
        free(buf);
    }
    f_close(pf);
    free(pf);
    return 0;
}
