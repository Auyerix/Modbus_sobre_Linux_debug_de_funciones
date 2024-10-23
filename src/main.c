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

//ATENCION acá cambié char por uint8_t (después ver si es signado o no) y le dí valores
//porque por ahora los valores ya no vienen por TCP solo se cargan en el vector
//char mb_req_buf[MB_ADU_MAXSIZE];
//uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x01,0x00,0x00,0x00,0x02};
//Ejemplo Github
//uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x03,0x00,0x02,0x00,0x01};
//Ejemplo ipc2u
//uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x03,0x00,0x00,0x00,0x02};
//Ejemplo Application PRotocol v11b3
//uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x01,0x00,0x13,0x00,0x13};

//Inicio pruebas con respuestas reales 22/10/24
//Prueba función leer Coils
//uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x01,0x00,0x00,0x00,0x4};
//Prueba función leer holding
uint8_t mb_req_buf[] = {0xAA,0x55,0x00,0x00,0x00,0x06,0x2A,0x03,0x00,0x00,0x00,0x3};

uint8_t mb_repl_buf[MB_ADU_MAXSIZE];

// Supongamos que este es el estado de las coils (256 coils, todas inicializadas en OFF).
uint8_t coil_status[256] = {0};

// supongamos ahora holdings registers de 16 bits
uint16_t holding_registers[10];

int main() {

    // Configuramos algunos estados de coils para el ejemplo
    coil_status[0] = 1;   // Coil 0 ON
    coil_status[1] = 0;   // Coil 1 OFF
    coil_status[2] = 1;   // Coil 2 ON
    coil_status[3] = 1;   // Coil 2 ON
    coil_status[7] = 1;   // Coil 7 ON
    coil_status[8] = 1;   // Coil 8 ON
    coil_status[15] = 1;  // Coil 15 ON
    // Configuramos algunos estados de holding registers
    holding_registers[0] = 0;
    holding_registers[1] = 1;
    holding_registers[2] = 2;
    holding_registers[3] = 3;

    uint16_t buf_len;
    buf_len = mb_process(mb_repl_buf, mb_req_buf, buf_len);

    for(int i = 0; i < 15; i++) {
        printf("request[%d] = 0x%X  ", i, mb_req_buf[i]);
        printf("replay[%d] = 0x%X\n", i, mb_repl_buf[i]);
    }

    //printf("Hello World");

    return 0;
}
