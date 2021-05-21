/*==========================================================
 * Copyright 2020 QuickLogic Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *==========================================================*/

/*==========================================================
 *
 *    File   : main.c
 *    Purpose: 
 *                                                          
 *=========================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"	// Required for configASSERT

#include "target/core-v-mcu/include/core-v-mcu-config.h"
#include "libs/cli/include/cli.h"
#include "libs/utils/include/dbg_uart.h"
#include "hal/include/hal_apb_soc_ctrl_regs.h"
#include "hal/include/hal_pinmux.h"
#include "hal/include/hal_gpio.h"

#include "drivers/include/udma_i2cm_driver.h"
#include "drivers/include/udma_uart_driver.h"

// Sub menus
const struct cli_cmd_entry misc_functions[];
const struct cli_cmd_entry uart1_functions[];
const struct cli_cmd_entry mem_functions[];
const struct cli_cmd_entry mem_tests[];
const struct cli_cmd_entry io_functions[];
const struct cli_cmd_entry gpio_functions[];
const struct cli_cmd_entry i2cm0_functions[];
const struct cli_cmd_entry i2cm0_tests[];
const struct cli_cmd_entry i2cm1_functions[];
extern const struct cli_cmd_entry efpga_cli_tests[];
extern const struct cli_cmd_entry qspi_cli_tests[];
extern const struct cli_cmd_entry cam_tests[];

// MISC functions
static void misc_info(const struct cli_cmd_entry *pEntry);

// UART functions
static void uart1_tx(const struct cli_cmd_entry *pEntry);

// MEM functions
static void mem_print_start(const struct cli_cmd_entry *pEntry);
static void mem_peek(const struct cli_cmd_entry *pEntry);
static void mem_poke(const struct cli_cmd_entry *pEntry);
static void mem_peek_16(const struct cli_cmd_entry *pEntry);
static void mem_poke_16(const struct cli_cmd_entry *pEntry);
static void mem_peek_8(const struct cli_cmd_entry *pEntry);
static void mem_poke_8(const struct cli_cmd_entry *pEntry);

// MEM tests
static void mem_check(const struct cli_cmd_entry *pEntry);

// IO functions
static void io_setmux(const struct cli_cmd_entry *pEntry);
static void io_getmux(const struct cli_cmd_entry *pEntry);

// GPIO functions
static void gpio_set(const struct cli_cmd_entry *pEntry);
static void gpio_clr(const struct cli_cmd_entry *pEntry);
static void gpio_toggle(const struct cli_cmd_entry *pEntry);
static void gpio_read_status(const struct cli_cmd_entry *pEntry);
static void gpio_set_mode(const struct cli_cmd_entry *pEntry);

// I2CM0 functions
static void i2cm_readbyte(const struct cli_cmd_entry *pEntry);
static void i2cm_writebyte(const struct cli_cmd_entry *pEntry);
static void i2cm_reset(const struct cli_cmd_entry *pEntry);
static void i2c_temp(const struct cli_cmd_entry *pEntry);
static void i2c_read_dev_id(const struct cli_cmd_entry *pEntry);
// I2CM tests
static void i2cm_singlebyte_test(const struct cli_cmd_entry *pEntry);

// Main menu
const struct cli_cmd_entry my_main_menu[] = {

		CLI_CMD_SUBMENU( "misc", 	misc_functions, 	"miscellaneous functions" ),
		CLI_CMD_SUBMENU( "uart1", 	uart1_functions, 	"commands for uart1" ),
		CLI_CMD_SUBMENU( "mem", 	mem_functions, 		"commands for memory" ),
		CLI_CMD_SUBMENU( "io", 		io_functions, 		"commands for io" ),
		CLI_CMD_SUBMENU( "gpio", 	gpio_functions, 	"commands for gpio" ),
		CLI_CMD_SUBMENU( "i2cm0", 	i2cm0_functions, 	"commands for i2cm0" ),
		CLI_CMD_SUBMENU( "i2cm1", 	i2cm1_functions, 	"commands for i2cm1" ),
		CLI_CMD_SUBMENU( "efpga", 	efpga_cli_tests,    "commands for efpga connectivity"),
		CLI_CMD_SUBMENU( "qspi", qspi_cli_tests, "commands for efpga tests"),
		CLI_CMD_SUBMENU( "cam", cam_tests, "commands for Himax camera tests"),
		CLI_CMD_TERMINATE()

};

// MISC menu
const struct cli_cmd_entry misc_functions[] =
{
		CLI_CMD_SIMPLE( "info", misc_info, "print build info" ),
		CLI_CMD_TERMINATE()
};

// UART1 menu
const struct cli_cmd_entry uart1_functions[] =
{
		CLI_CMD_SIMPLE( "tx", uart1_tx, "<string>: write <string> to uart1" ),
		CLI_CMD_TERMINATE()
};

// mem menu
const struct cli_cmd_entry mem_functions[] =
{
		CLI_CMD_SIMPLE( "start", 	mem_print_start,   	"print start of unused memory" ),
		CLI_CMD_SIMPLE( "peek", 	mem_peek,         	"0xaddr -- print memory location " ),
		CLI_CMD_SIMPLE( "poke",   mem_poke,         	"0xaddr 0xvalue -- write value to addr" ),
		CLI_CMD_SIMPLE( "md.b", 	mem_peek_8,         	"0xaddr -- print 8-bit memory location " ),
		CLI_CMD_SIMPLE( "mw.b",   mem_poke_8,         	"0xaddr 0xvalue -- write 8-bit alue to addr" ),
		CLI_CMD_SIMPLE( "md.w", 	mem_peek_16,         	"0xaddr -- print 16-bit memory location " ),
		CLI_CMD_SIMPLE( "mw.w",   mem_poke_16,         	"0xaddr 0xvalue -- write 16-bit value to addr" ),
		CLI_CMD_SIMPLE( "md.l", 	mem_peek,         	"0xaddr -- print 32-bit memory location " ),
		CLI_CMD_SIMPLE( "poke",   	mem_poke,         	"0xaddr 0xvalue -- write value to addr" ),
		CLI_CMD_SUBMENU( "test", 	mem_tests, 			"tests" ),
		CLI_CMD_TERMINATE()
};

// mem menu
const struct cli_cmd_entry mem_tests[] =
{
		CLI_CMD_SIMPLE( "check", 	mem_check,         	"print start of unused memory" ),
		CLI_CMD_TERMINATE()
};

// IO menu
const struct cli_cmd_entry io_functions[] =
{
		CLI_CMD_SIMPLE( "setmux", io_setmux,         	"ionum mux_sel 	-- set mux_sel for ionum " ),
		CLI_CMD_SIMPLE( "getmux", io_getmux,         	"ionum  		-- get mux_sel for ionum" ),
		CLI_CMD_TERMINATE()
};

// GPIO menu
const struct cli_cmd_entry gpio_functions[] =
{
		CLI_CMD_SIMPLE( "set", 	gpio_set,         		"gpio_num	-- set to one" ),
		CLI_CMD_SIMPLE( "clr", 	gpio_clr,         		"gpio_num	-- clear to zero" ),
		CLI_CMD_SIMPLE( "toggle",	gpio_toggle,        "gpio_num	-- toggle state of gpio" ),
		CLI_CMD_SIMPLE( "status",	gpio_read_status,   "gpio_num	-- read status of gpio: in, out, interrupt type and mode" ),
		CLI_CMD_SIMPLE( "mode",	gpio_set_mode,       	"gpio_num gpio_mode	-- set mode of gpio" ),
		CLI_CMD_TERMINATE()
};

// I2CM0 menu
const struct cli_cmd_entry i2cm0_functions[] =
{

  CLI_CMD_WITH_ARG( "readbyte", 	i2cm_readbyte,	0, "i2c_addr reg_addr 			-- read register" ),
  CLI_CMD_WITH_ARG( "writebyte", 	i2cm_writebyte,	0, "i2c_addr reg_addr value 	-- read register" ),
  CLI_CMD_SUBMENU ( "test", 		i2cm0_tests, 		"tests" ),
  CLI_CMD_TERMINATE()

};

const struct cli_cmd_entry i2cm0_tests[] =
{

  CLI_CMD_WITH_ARG( "singlebyte", 	i2cm_singlebyte_test,	0, "i2c_addr reg_addr	-- writes 0xA5 and then 0x5A to register and checks result" ),
  CLI_CMD_TERMINATE()

};

// I2CM1 menu
const struct cli_cmd_entry i2cm1_functions[] =
{
		CLI_CMD_WITH_ARG( "readbyte", i2cm_readbyte,	1, "i2c_addr reg_addr 	-- read register" ),
		CLI_CMD_SIMPLE ( "temp", i2c_temp,				   "read on board temperature"),
		CLI_CMD_SIMPLE ( "dev_id", i2c_read_dev_id,		    "read i2c device id"),
		CLI_CMD_TERMINATE()
};

// MISC functions
static void misc_info(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	char pzTemp[] = "0000-00-00 00:00";
	SocCtrl_t* 	psocctrl = SOC_CTRL_START_ADDR;
	uint32_t 	xval;

	xval = psocctrl->build_date;
	pzTemp[0] += (char)((xval >> 28) & 0xFU);
	pzTemp[1] += (char)((xval >> 24) & 0xFU);
	pzTemp[2] += (char)((xval >> 20) & 0xFU);
	pzTemp[3] += (char)((xval >> 16) & 0xFU);

	pzTemp[5] += (char)((xval >> 12) & 0xFU);
	pzTemp[6] += (char)((xval >>  8) & 0xFU);

	pzTemp[8] += (char)((xval >>  4) & 0xFU);
	pzTemp[9] += (char)((xval >>  0) & 0xFU);

	xval = psocctrl->build_time;
	pzTemp[11] += (char)((xval >> 20) & 0xFU);
	pzTemp[12] += (char)((xval >> 16) & 0xFU);

	pzTemp[14] += (char)((xval >> 12) & 0xFU);
	pzTemp[15] += (char)((xval >>  8) & 0xFU);

	dbg_str_str("build_info", pzTemp);
	dbg_str("<<DONE>>");
}

// UART functions
static void uart1_tx(const struct cli_cmd_entry *pEntry)
{
	char*  pzArg = NULL;
	(void)pEntry;
	// Add functionality here
	while (CLI_peek_next_arg() != NULL) {
		if (pzArg != NULL) {
			udma_uart_writeraw(1, 2, " ");
		}
		CLI_string_ptr_required("string", &pzArg);
		udma_uart_writeraw(1, strlen(pzArg), pzArg);
	}
	udma_uart_writeraw(1, 2, "\r\n");
	dbg_str("<<DONE>>");
	return;
}

// MEM functions
static void mem_print_start(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	extern char __l2_shared_end;
	dbg_str_hex32("l2_shared_end", (uint32_t)(&__l2_shared_end));
	dbg_str("<<DONE>>");
}

static void mem_check(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	bool  fPassed = true;
	extern char __l2_shared_end;
	uint32_t*  pl;
	for (pl = (uint32_t*)(&__l2_shared_end); (uint32_t)pl < 0x1c080000; pl++) {
		*pl = (uint32_t)pl;
	}

	// pl=0x1c070000; *pl = 76;  // Enable to force an error

	for (pl = (uint32_t*)(&__l2_shared_end); (uint32_t)pl < 0x1c080000; pl++) {
		if (*pl != (uint32_t)pl) {
			dbg_str_hex32("mem check fail at", (uint32_t)pl);
			dbg_str_hex32("read back        ", *pl);
			fPassed = false;
			break;
		}
	}
	if (fPassed) {
		dbg_str("<<PASSED>>");
	} else {
		dbg_str("<<FAILED>>");
	}
}

static void mem_peek(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	xValue;
	uint32_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	xValue = *pAddr;
	dbg_str_hex32("value", xValue);
	dbg_str("<<DONE>>");
}

static void mem_poke(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	xValue;
	uint32_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	CLI_uint32_required( "value", &xValue);
	*pAddr = xValue;
	dbg_str("<<DONE>>");
}

static void mem_peek_16(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint16_t	xValue;
	uint16_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	xValue = *pAddr;
	dbg_str_hex16("value", xValue);
	dbg_str("<<DONE>>");
}

static void mem_poke_16(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint16_t	xValue;
	uint16_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	CLI_uint16_required( "value", &xValue);
	*pAddr = xValue;
	dbg_str("<<DONE>>");
}
static void mem_peek_8(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint8_t	xValue;
	uint8_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	xValue = *pAddr;
	dbg_str_hex8("value", xValue);
	dbg_str("<<DONE>>");
}

static void mem_poke_8(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint8_t	xValue;
	uint8_t*	pAddr;

	CLI_uint32_required( "addr", &pAddr );
	CLI_uint32_required( "value", &xValue);
	*pAddr = xValue;
	dbg_str("<<DONE>>");
}
// IO functions
static void io_setmux(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	mux_sel;
	uint32_t	ionum;

	CLI_uint32_required( "ionum", &ionum );
	CLI_uint32_required( "mux_sel", &mux_sel);
	hal_setpinmux(ionum, mux_sel);
	dbg_str("<<DONE>>");
}

static void io_getmux(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	ionum;
	uint32_t	mux_sel;

	CLI_uint32_required( "ionum", &ionum );
	mux_sel = hal_getpinmux(ionum);
	dbg_str_hex32("mux_sel", mux_sel);
	dbg_str("<<DONE>>");
}

// GPIO functions
static void gpio_set(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	gpio_num;

	CLI_uint32_required( "gpio_num", &gpio_num );
	hal_set_gpio((uint8_t)gpio_num);
	dbg_str("<<DONE>>");
}

static void gpio_clr(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	gpio_num;

	CLI_uint32_required( "gpio_num", &gpio_num );
	hal_clr_gpio((uint8_t)gpio_num);
	dbg_str("<<DONE>>");
}

static void gpio_toggle(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	gpio_num;

	CLI_uint32_required( "gpio_num", &gpio_num );
	hal_toggle_gpio((uint8_t)gpio_num);
	dbg_str("<<DONE>>");
}

static void gpio_read_status(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	gpio_num;
	uint8_t	input_value;
	uint8_t	output_value;
	uint8_t	interrupt_type;
	uint8_t	gpio_mode;

	CLI_uint32_required( "gpio_num", &gpio_num );
	hal_read_gpio_status(gpio_num, &input_value, &output_value, &interrupt_type, &gpio_mode);
	dbg_str_hex8("input", (uint32_t)input_value);
	dbg_str_hex8("output", (uint32_t)output_value);
	dbg_str_hex8("interrupt_type", (uint32_t)interrupt_type);
	dbg_str_hex8("gpio_mode", (uint32_t)gpio_mode);

	uint32_t register_value;
	hal_read_gpio_status_raw(gpio_num, &register_value);
	dbg_str_hex32("rdstatus", register_value);
	dbg_str("<<DONE>>");
}

static void gpio_set_mode(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	gpio_num;
	uint32_t	gpio_mode;

	CLI_uint32_required( "gpio_num", &gpio_num );
	CLI_uint32_required( "gpio_mode", &gpio_mode );
	hal_set_gpio_mode((uint8_t)gpio_num, (uint8_t)gpio_mode);
	dbg_str("<<DONE>>");
}

/////////////////////////////////////////////////////////////////
//
// I2CM functions
//
/////////////////////////////////////////////////////////////////
static uint8_t i2c_buffer[256];
static void i2cm_readbyte(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	i2c_addr;
	uint32_t	reg_addr;

	CLI_uint32_required( "i2c_addr", &i2c_addr );
	CLI_uint32_required( "reg_addr", &reg_addr );

	udma_i2cm_read(pEntry->cookie, (uint8_t)i2c_addr, (uint8_t)reg_addr, 1, i2c_buffer, false);
	dbg_str_hex8("reg", (int)i2c_buffer[0]);
}

static void i2cm_writebyte(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	i2c_addr;
	uint32_t	reg_addr;
	uint32_t	reg_value;

	CLI_uint32_required( "i2c_addr", &i2c_addr );
	CLI_uint32_required( "reg_addr", &reg_addr );
	CLI_uint32_required( "reg_value", &reg_value );

	i2c_buffer[0] = (uint8_t)reg_value;
	udma_i2cm_write (pEntry->cookie, i2c_addr, reg_addr, 1, i2c_buffer,  false);
}

static void i2cm_reset(const struct cli_cmd_entry *pEntry)
{
    (void)pEntry;
    // Add functionality here

    udma_i2cm_control(pEntry->cookie, kI2cmReset, NULL);
}

static void i2cm_singlebyte_test(const struct cli_cmd_entry *pEntry)
{
	(void)pEntry;
	// Add functionality here
	uint32_t	i2c_addr;
	uint32_t	reg_addr;
	bool		fPassed = false;

	CLI_uint32_required( "i2c_addr", &i2c_addr );
	CLI_uint32_required( "reg_addr", &reg_addr );

	i2c_buffer[0] = 0xA5;
	udma_i2cm_write (pEntry->cookie, i2c_addr, reg_addr, 1, i2c_buffer,  false);
	udma_i2cm_read(pEntry->cookie, (uint8_t)i2c_addr, (uint8_t)reg_addr, 1, i2c_buffer, false);
	dbg_str_int("First access", i2c_buffer[0]);
	if (i2c_buffer[0] == 0xA5) {
		i2c_buffer[0] = 0x5A;
		udma_i2cm_write (pEntry->cookie, i2c_addr, reg_addr, 1, i2c_buffer,  false);
		udma_i2cm_read(pEntry->cookie, (uint8_t)i2c_addr, (uint8_t)reg_addr, 1, i2c_buffer, false);
		dbg_str_int("Second access", i2c_buffer[0]);
		if (i2c_buffer[0] == 0x5A) {
			fPassed = true;
		}
	}
	if (fPassed) {
		dbg_str("<<PASSED>>");
	} else {
		dbg_str("<<FAILED>>");
	}
}

static void i2c_temp (const struct cli_cmd_entry *pEntry)
{
	char *message = 0;
	int temp;
	message  = pvPortMalloc(80);
	configASSERT (message);
	udma_i2cm_read(1, 0x96, 0x00, 2, i2c_buffer, false);
	temp = (i2c_buffer[0] << 8) + i2c_buffer[1];
	temp = ((temp *625) / 44000) + 32;
	sprintf(message," Board temp = %d F\r\n", temp);
	dbg_str(message);
	vPortFree(message);
}

static void i2c_read_dev_id(const struct cli_cmd_entry *pEntry)
{
	char *message = 0;
	int temp;
	message  = pvPortMalloc(80);
	configASSERT (message);
	udma_i2cm_read(1, 0x96, 0x0B, 1, i2c_buffer, false);

	sprintf(message," i2c_dev_id:= 0x%x \r\n", i2c_buffer[0]);
	dbg_str(message);
	if(i2c_buffer[0] == 0xCB) {
		dbg_str("Dev Id Test: <<PASSED>>\r\n");
	}
	else {
		dbg_str("Dev Id Test: <<FAILED>>\r\n");
	}
	vPortFree(message);
}
