/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/*
 * uart_simple.c
 *
 *  Created on: 2018. 1. 5.
 *      Author: sanguk7.park
 */

#include <platform/sfr.h>
#include "uart_simple.h"

#define Outp32(addr, data) (*(volatile unsigned int *)((unsigned long) addr) = (data))
#define Set32(addr, data) (*(volatile unsigned int *)((unsigned long) addr) = (data))
#define Get32(addr) (*(volatile unsigned int *)((unsigned long)addr))

#define SetBits(uAddr, uBaseBit, uMaskValue, uSetValue) \
	Set32(uAddr, (Get32(uAddr) & ~((uMaskValue)<<(uBaseBit))) | (((uMaskValue)&(uSetValue))<<(uBaseBit)))

/* Macros from U-Boot */
#define __arch_getl(a)			(*(volatile unsigned int *)(a))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))
#define __raw_readl(a)			__arch_getl(a)
#define __raw_writel(v,a)		__arch_putl(v,a)
#define in_32(a)			__raw_readl(a)
#define out_32(a,v)			__raw_writel(v,a)
#define clrsetbits(type, addr, clear, set) \
	out_##type((addr), (in_##type(addr) & ~(clear)) | (set))
#define clrsetbits_32(addr, clear, set) clrsetbits(32, addr, clear, set)

/* GPA3/GPA4 pins */
#define EXYNOS3830_GPA3CON		(EXYNOS3830_GPIO_ALIVE_BASE + 0x0060)
#define EXYNOS3830_GPA3PUD		(EXYNOS3830_GPIO_ALIVE_BASE + 0x0068)
#define EXYNOS3830_GPA4CON		(EXYNOS3830_GPIO_ALIVE_BASE + 0x0080)
#define EXYNOS3830_GPA4PUD		(EXYNOS3830_GPIO_ALIVE_BASE + 0x0088)
#define GPA3CON_MASK			(0xf << 28)
#define GPA3CON_SET			(0x2 << 28)
#define GPA3PUD_MASK			(0xf << 28)
#define GPA3PUD_SET			(0x0 << 28)
#define GPA4CON_MASK			0xf
#define GPA4CON_SET			0x2
#define GPA4PUD_MASK			0xf
#define GPA4PUD_SET			0x0

/* PMU related definitions */
#define EXYNOS3830_UART_IO_SHARE_CTRL	(EXYNOS3830_POWER_BASE + 0x0760)
#define SEL_RXD_AP_UART_MASK		(0x3 << 16)
#define SEL_RXD_AP_UART_GPIO_1		(0x3 << 16)
#define SEL_TXD_GPIO_1_MASK		(0x3 << 20)
#define SEL_TXD_GPIO_1_AP_UART_TXD	(0x0 << 20)

#define rUART_BASE				EXYNOS3830_UART_BASE

#define rUART_ULCONN               0x00
#define rUART_UCONN                0x04
#define rUART_UFCONN               0x08
#define rUART_UMCONN               0x0C
#define rUART_UTRSTATN             0x10
#define rUART_UERSTATN             0x14
#define rUART_UFSTATN              0x18
#define rUART_UMSTATN              0x1C
#define rUART_UTXHN                0x20
#define rUART_URXHN                0x24
#define rUART_UBRDIVN              0x28
#define rUART_UFRACVAL             0x2C
#define rUART_UINTPN               0x30
#define rUART_UINTSPN              0x34
#define rUART_UINTMN               0x38
#define rUART_USI_CON              0xc4

/* Define this for E850-96 board */
#define UART_DEBUG_1

unsigned int globalUartBase;
unsigned int uart_log_mode = 0;

static void uart_simple_uart_debug_1_enable(void)
{
	/* Use UART_DEBUG_1 as AP UART */
	clrsetbits_32(EXYNOS3830_UART_IO_SHARE_CTRL,
		SEL_RXD_AP_UART_MASK | SEL_TXD_GPIO_1_MASK,
		SEL_RXD_AP_UART_GPIO_1 | SEL_TXD_GPIO_1_AP_UART_TXD);
}

static void uart_simple_uart_debug_1_gpio_init(void)
{
	/* Mux GPA3[7] and GPA4[0] pins to UART_DEBUG_1 RX/TX lines */
	clrsetbits_32(EXYNOS3830_GPA3CON, GPA3CON_MASK, GPA3CON_SET);
	clrsetbits_32(EXYNOS3830_GPA4CON, GPA4CON_MASK, GPA4CON_SET);

	/* Disable pull-up/pull-down */
	clrsetbits_32(EXYNOS3830_GPA3PUD, GPA3PUD_MASK, GPA3PUD_SET);
	clrsetbits_32(EXYNOS3830_GPA4PUD, GPA4PUD_MASK, GPA4PUD_SET);
}

/* This is how to use sample */
void uart_test_function(void)
{
	char array[9]={0};

#ifdef UART_DEBUG_1
	uart_simple_uart_debug_1_enable();
	uart_simple_uart_debug_1_gpio_init();
#else
	uart_simple_GPIOInit();
#endif

	if(*(unsigned int *)0x80000000 == 0xabcdef)
		uart_simple_UartInit(rUART_BASE, 200000000, 115200);
	else
		uart_simple_UartInit(rUART_BASE, 200000000, 115200);

	uart_simple_string_out("Done\nfor\nsure.\n");

	itoa_base_custom(rUART_BASE, 0x0F, 8, array);
	uart_simple_string_out(array);
	uart_simple_string_out("\n");

	itoa_base_custom(0x1234, 0x0F, 4, array);
	uart_simple_string_out(array);
	uart_simple_string_out("\n");
}

/* GPIO configure */
void uart_simple_GPIOInit(void)
{
#define rGPQ0_CON                            EXYNOS3830_GPQ0CON /* UART_DEBUG0, Speedy */

#define UARTGPIO_CON                           rGPQ0_CON
#define UARTGPIO_CON_MASK                      0xFF
#define UARTGPIO_CON_SET                       0x22
#define UARTGPIO_CON_BASE_BIT                  (0 * 4)

	SetBits(UARTGPIO_CON, UARTGPIO_CON_BASE_BIT, UARTGPIO_CON_MASK, UARTGPIO_CON_SET);
}

/* UART USI initialize code */
void uart_simple_UartInit(unsigned int uUartBase, unsigned int clk, unsigned int nBaudrate)
{
	double udiv, fdiv;

	globalUartBase = uUartBase;

	/* usi_uart setting */
	/* Set this AFTER GPIO setting. */
	SetBits(uUartBase+rUART_USI_CON, 0, 0x1, 0);

	/* uart init */
	Outp32(uUartBase+rUART_ULCONN, 0x3);
	Outp32(uUartBase+rUART_UCONN, 0x3005);
	Outp32(uUartBase+rUART_UFCONN, 0x1);

	Outp32(uUartBase+rUART_UMCONN, 0x0);
	Outp32(uUartBase+rUART_UTRSTATN, 0x6);

	udiv = fdiv = (clk/16.0/(double)nBaudrate)-1;
	SetBits(uUartBase+rUART_UBRDIVN,0,0xffff, (unsigned int)udiv);
	SetBits(uUartBase+rUART_UFRACVAL,0,0xffff,  (unsigned int)((fdiv - udiv)*16)  );

	Outp32(uUartBase+rUART_UINTMN, 0xf);
}

void itoa_base_custom(unsigned int number, unsigned int uBaseUnit, unsigned int uUnitWidth, char *Converted)
{
    unsigned int n;

    Converted += uUnitWidth;
    *Converted = '\0';

    for (n = uUnitWidth; n != 0; --n) {
        *--Converted = "0123456789ABCDEF"[number % uBaseUnit];
        number /= uBaseUnit;
    }
}

void UART_WaitForRxReady(void)
{
#define RX_BUF_READY (1 << 0)	/* Rx buffer data ready */

	unsigned int uTxRxStatus;

	do {
		uTxRxStatus = Get32(globalUartBase + rUART_UTRSTATN) & 0x7;
	} while (!(uTxRxStatus & RX_BUF_READY));
}

void UART_WaitForTxEmpty(void)
{
#define TX_BUF_EMPTY (1 << 1)	/* Tx buffer register empty */

	unsigned int uTxRxStatus;

	do {
		uTxRxStatus = Get32(globalUartBase + rUART_UTRSTATN) & 0x7;
	} while (!(uTxRxStatus & TX_BUF_EMPTY));
}

void uart_simple_char_in(char *cData)
{
	unsigned int reg;

	UART_WaitForRxReady();
	reg = Get32(globalUartBase + rUART_URXHN);
	*cData = (char)reg;
}

void uart_simple_char_out(char cData)
{
     UART_WaitForTxEmpty();
     Outp32(globalUartBase+rUART_UTXHN, cData);
	 if(cData == '\n')
		 uart_simple_char_out('\r');
}

void uart_simple_string_out(const char * string)
{
	while(*string)
	{
		if(*string=='\n')
		{
			string++;
			uart_simple_char_out('\r');
			uart_simple_char_out('\n');
		}
		else
			uart_simple_char_out(*string++);
	}
}
