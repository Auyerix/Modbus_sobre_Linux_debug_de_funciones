/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-18
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "modbus.h"

//char mb_req_buf[MB_ADU_MAXSIZE];
uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x02,0x00,0x00,0x00,0x11};
uint8_t mb_repl_buf[MB_ADU_MAXSIZE];

int main() {


    uint16_t buf_len;
    buf_len = mb_process(mb_repl_buf, mb_req_buf, buf_len);

    for(int i = 0; i < 15; i++) {
        printf("request[%d] = 0x%X  ", i, mb_req_buf[i]);
        printf("replay[%d] = 0x%X\n", i, mb_repl_buf[i]);
    }

    //printf("Hello World");

    return 0;
}
