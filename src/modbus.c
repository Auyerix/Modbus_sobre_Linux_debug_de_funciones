#include "modbus.h"
#include <string.h>

//incluidos para el printf
#include <stdio.h>
#include <stdint.h>

//--------for reading commands-----------------//
extern void read_coils(char *repl_buf, uint16_t address, uint16_t quantity);
extern void read_discrete(char *repl_buf, uint16_t address, uint16_t quantity);
extern void read_holding(char *repl_buf, uint16_t address, uint16_t quantity);
extern void read_inputs(char *repl_buf, uint16_t address, uint16_t quantity);
//--------for writing commands-----------------//
extern void write_single_coil(uint16_t address, uint16_t val);
extern void write_single_holding(uint16_t address, uint16_t val);

static uint16_t mb_process_pdu_read_fn(char *mb_repl_buf, char *mb_req_buf, uint16_t req_buf_len);
static uint16_t mb_process_pdu_write_single_fn(char *mb_repl_buf, char *mb_req_buf, uint16_t req_buf_len);
static uint16_t mb_process_err(char *mb_repl_buf, uint8_t fn, uint16_t exceptionCode);
static uint8_t mb_process_start_address(uint16_t fn, uint16_t start_address, uint16_t quantity);
static uint8_t mb_process_val(uint16_t fn, uint16_t val);

static void mb_mbap_copy(char *mb_repl_buf, char *mb_req_buf);
static uint16_t mb_pdu_calculate_N(uint16_t fn, uint16_t quantity);

uint16_t mb_process(char *mb_repl_buf, char *mb_req_buf, uint16_t req_buf_len) {
	memset(mb_repl_buf, '\0', MB_ADU_MAXSIZE);  // clear the buffer
	mb_mbap_copy(mb_repl_buf, mb_req_buf);
	uint16_t pduLen = 0;

	uint8_t fn = mb_req_buf[MB_PDU_FN];
	switch (fn) {
	case MB_FN_READ_COILS:
	case MB_FN_READ_DISCRETE:
	case MB_FN_READ_HOLDING:
	case MB_FN_READ_INPUT:
		pduLen = mb_process_pdu_read_fn(mb_repl_buf, mb_req_buf, req_buf_len);
		break;
	case MB_FN_WRITE_S_COIL:
	case MB_FN_WRITE_S_HOLDING:
		pduLen = mb_process_pdu_write_single_fn(mb_repl_buf, mb_req_buf, req_buf_len);
		break;
	case MB_FN_WRITE_M_COIL:
	case MB_FN_WRITE_M_HOLDING:
		//mb_process_write_fn(mb_repl_buf, mb_req_buf, req_buf_len);
		break;
	default:
		pduLen = mb_process_err(mb_repl_buf, fn, MB_EXCEPTION_FN_UNSUPPORTED);
		break;
	}
	// acá tomo el valor del pdulen le sumo 1 para incluir la cuenta de la dirección en el
	//[6] y lo separo en dos posiciones [4] y [5]
	mb_repl_buf[MB_MBAP_LEN_H] = (pduLen + 1) >> 8;
	mb_repl_buf[MB_MBAP_LEN_L] = (pduLen + 1) & 0xff;

	//y retorno lo que será el buf_len que suma la base de 7 bytes de MB_MBAP_SIZE que es 7
	//con lo calculado pduLen que es el largo del mensaje devuelto por las funciones
	return (pduLen + MB_MBAP_SIZE);
}

static uint16_t mb_process_pdu_read_fn(char *mb_repl_buf, char *mb_req_buf, uint16_t req_buf_len) {
	uint8_t fn = mb_req_buf[MB_PDU_FN];
	uint16_t start_address = mb_req_buf[MB_PDU_R_ST_ADDR_L] + (mb_req_buf[MB_PDU_R_ST_ADDR_H] << 8);
	uint16_t quantity = mb_req_buf[MB_PDU_R_QUANTITY_L] + (mb_req_buf[MB_PDU_R_QUANTITY_H] << 8);

	//imprimo la funciòn, la direcciòn de comienzo y la cantidad
	printf("fn = %d start_address = %d  quantity = %d\n", fn, start_address, quantity);

	if (mb_process_start_address(fn, start_address, quantity) != MB_EXCEPTION_OK) {
		printf("paso por error \n");
		return mb_process_err(mb_repl_buf, fn, MB_EXCEPTION_DATA_ADDR);
	} else { // form PDU content
		mb_repl_buf[MB_PDU_FN] = fn;
		mb_repl_buf[MB_PDU_REPL_N] = mb_pdu_calculate_N(fn, quantity);

		switch (fn) {
		case MB_FN_READ_COILS:
			read_coils(mb_repl_buf, start_address, quantity);
			break;
//		case MB_FN_READ_DISCRETE:
//			read_discrete(mb_repl_buf, start_address, quantity);
//			break;
		case MB_FN_READ_HOLDING:
			read_holding(mb_repl_buf, start_address, quantity); //Ojo tenía mb_req_buf!!!
			break;
//		case MB_FN_READ_INPUT:
//			read_inputs(mb_repl_buf, start_address, quantity);
//			break;
		}

	}
	return mb_pdu_calculate_N(fn, quantity) + 2; // returns PDU size where +2 are N and fn
}

static uint16_t mb_process_pdu_write_single_fn(char *mb_repl_buf, char *mb_req_buf, uint16_t req_buf_len) {
	uint8_t fn = mb_req_buf[MB_PDU_FN];
	uint16_t address = mb_req_buf[MB_PDU_W_REG_ADDR_L] + (mb_req_buf[MB_PDU_W_REG_ADDR_H] << 8);
	uint16_t valToWrite = mb_req_buf[MB_PDU_W_REG_VAL_L] + (mb_req_buf[MB_PDU_W_REG_VAL_H] << 8);

	if (mb_process_start_address(fn, address, 1) != MB_EXCEPTION_OK) {
		return mb_process_err(mb_repl_buf, fn, MB_EXCEPTION_DATA_ADDR);
	} else if (mb_process_val(fn, valToWrite) != MB_EXCEPTION_OK) {
		return mb_process_err(mb_repl_buf, fn, MB_EXCEPTION_DATA_VAL);
	} else { // form PDU content
		mb_repl_buf[MB_PDU_FN] = fn;
		mb_repl_buf[MB_PDU_W_REG_ADDR_H] = mb_req_buf[MB_PDU_W_REG_ADDR_H];
		mb_repl_buf[MB_PDU_W_REG_ADDR_L] = mb_req_buf[MB_PDU_W_REG_ADDR_L];
		mb_repl_buf[MB_PDU_W_REG_VAL_H] = mb_req_buf[MB_PDU_W_REG_VAL_H];
		mb_repl_buf[MB_PDU_W_REG_VAL_L] = mb_req_buf[MB_PDU_W_REG_VAL_L];

//		if(fn == MB_FN_WRITE_S_COIL) write_single_coil(address, valToWrite);
//		if(fn == MB_FN_WRITE_S_HOLDING) write_single_holding(address, valToWrite);
	}
	return 5; // PDU size for write single XXX command
}

static void mb_mbap_copy(char *mb_repl_buf, char *mb_req_buf) {
	mb_repl_buf[MB_MBAP_TRANSACTION_ID_H] = mb_req_buf[MB_MBAP_TRANSACTION_ID_H];
	mb_repl_buf[MB_MBAP_TRANSACTION_ID_L] = mb_req_buf[MB_MBAP_TRANSACTION_ID_L];
	mb_repl_buf[MB_MBAP_PROTOCOL_ID_H] = mb_req_buf[MB_MBAP_PROTOCOL_ID_H];
	mb_repl_buf[MB_MBAP_PROTOCOL_ID_L] = mb_req_buf[MB_MBAP_PROTOCOL_ID_L];
	mb_repl_buf[MB_MBAP_CLIENT_ID] = mb_req_buf[MB_MBAP_CLIENT_ID];
}

//esta función verifica que las cantidades que estoy pidiendo están disponibles en el sistema
// la configuración del sistema debería estar hardcodeada en por ejempl MB_COIL_Q que indicaría
//la cantidad de coils físicas disponibles.
static uint8_t mb_process_start_address(uint16_t fn, uint16_t start_address, uint16_t quantity) {
	uint8_t exception_code = MB_EXCEPTION_OK;
	switch (fn) {
	case MB_FN_READ_COILS:
		if ((start_address + quantity) > MB_COILS_Q)
			exception_code = 2;
		break;
	case MB_FN_READ_DISCRETE:
		if ((start_address + quantity) > MB_DISCRETE_Q)
			exception_code = 2;
		break;
	case MB_FN_READ_HOLDING:
		if ((start_address + quantity) > MB_HOLDING_Q)
			exception_code = 2;
		break;
	case MB_FN_READ_INPUT:
		if ((start_address + quantity) > MB_INPUT_Q)
			exception_code = 2;
		break;

	case MB_FN_WRITE_S_COIL:
		if ((start_address + quantity) > MB_COILS_Q)
			exception_code = 2;
		break;
	case MB_FN_WRITE_S_HOLDING:
		if ((start_address + quantity) > MB_HOLDING_Q)
			exception_code = 2;
		break;
	}
	return exception_code;
}

static uint8_t mb_process_val(uint16_t fn, uint16_t val) {
	uint8_t exception_code = MB_EXCEPTION_OK;
	switch (fn) {
	case MB_FN_WRITE_S_COIL:
		if ((val != 0xFF00) && (val != 0x0000))
			exception_code = 3;
		break;
	}
	return exception_code;
}

static uint16_t mb_process_err(char *mb_repl_buf, uint8_t fn, uint16_t exceptionCode) {
	mb_repl_buf[MB_PDU_FN] = fn | 0x80;
	mb_repl_buf[MB_PDU_EXCEPTION_CODE] = exceptionCode;
	return 2;
}

static uint16_t mb_pdu_calculate_N(uint16_t fn, uint16_t quantity) {
	switch (fn) {
	case MB_FN_READ_COILS:
	case MB_FN_READ_DISCRETE:
		if (quantity % 8 != 0)
			return ((quantity / 8) + 1);
		else
			return (quantity / 8);
		break;
	case MB_FN_READ_HOLDING:
	case MB_FN_READ_INPUT:
		return quantity * 2;
		break;
	default:
		return 0;
	}
}

void read_coils(char *repl_buf, uint16_t address, uint16_t quantity){
    uint16_t byte_count = (quantity + 7) / 8;  // Cantidad de bytes necesarios
    //memset(response_buffer, 0, 256);            // Inicializar TODO el buffer de respuesta a 0
    //memset(response_buffer, 0, byte_count);    // Inicializar el buffer de respuesta a 0
    
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t coil_index = address + i;
        if (coil_status[coil_index]) {
            repl_buf[(i / 8) +9 ] |= (1 << (i % 8));  // Establecer el bit correspondiente si la coil está ON
													//y es mas 9 para dar lugar a fn y cantidad de bytes en
													//[7] y [8]
        }
    }

}

//22/10/24 acá estoy trabajando con la misma lógica que las coils ahora leo registros funciona
//ahora ver que tantos puedo leer
// y activar REad Discrete!!!
void read_holding(char *repl_buf, uint16_t address, uint16_t quantity) {
    // Cantidad de bytes necesarios: cada registro holding es de 2 bytes (16 bits)
    uint16_t byte_count = quantity * 2;

    // Coloca la cantidad de bytes en el byte [8] del buffer de respuesta (luego de la cabecera)
    repl_buf[8] = byte_count;

    // Copiar los valores de los registros holding solicitados
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t reg_index = address + i;
        uint16_t holding_value = holding_registers[reg_index];  // Valor del registro holding

        // Poner los 2 bytes del registro holding en el buffer de respuesta
        repl_buf[9 + (i * 2)]     = (holding_value >> 8) & 0xFF;  // Byte alto
        repl_buf[9 + (i * 2) + 1] = holding_value & 0xFF;         // Byte bajo
    }
}
