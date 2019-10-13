#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"

#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "uartstdio.h"

#define UART_RX_BUFFER_SIZE 16

static unsigned char g_ucRxBuffer[UART_RX_BUFFER_SIZE];
static unsigned char g_ucTxBuffer[UART_RX_BUFFER_SIZE];

#pragma DATA_ALIGN(ucControlTable, 1024)
unsigned char ucControlTable[1024];

void InitConsole(void);
void InitDMA(void);

void
uDMAErrorHandler(void)
{
    uint32_t ui32ErrorStatus;
    ui32ErrorStatus = uDMAErrorStatusGet();

    if(ui32ErrorStatus)
    {
        uDMAErrorStatusClear();
        UARTprintf("DMA transfer error: %d\n", uDMAErrorStatusGet);
    }
}

void UARTIntHandler(void)
{
    uint32_t ui32UARTMode;
    ui32UARTMode = UARTIntStatus(UART0_BASE, true);
    UARTprintf("\nUART Interrupt: %d", ui32UARTMode);

    if(ui32UARTMode)
    {
        UARTIntClear(UART0_BASE, ui32UARTMode);
    }

    while(UARTCharsAvail(UART0_BASE))
    {
        uDMAChannelTransferSet(UDMA_CH8_UART0RX | UDMA_PRI_SELECT,
                               UDMA_MODE_AUTO,
                               (void *)(UART0_BASE + UART_O_DR),
                               g_ucRxBuffer, sizeof(g_ucRxBuffer));
        uDMAChannelEnable(UDMA_CH8_UART0RX);
        break;
    }

}
/**
 * main.c
 */
int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_16MHZ);
    InitConsole();
    InitDMA();
    UARTprintf("uDMA UART test\n");

/*    Set up UART RX interrupt*/
    UARTIntEnable(UART0_BASE, UART_INT_RX);
    UARTIntRegister(UART0_BASE, UARTIntHandler);
    IntEnable(INT_UART0);

/*  DMA Interrupt error transmission*/
    IntEnable(INT_UDMAERR);
    IntMasterEnable();

    UARTDMAEnable(UART0_BASE, UART_DMA_TX | UART_DMA_RX);
/*    TM4C123GH6PM FIFO 16 bytes*/
    UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    UARTFIFOEnable(UART0_BASE);
    UARTEnable(UART0_BASE);

/*    Trigger UART TX transfer*/
    uDMAChannelEnable(UDMA_CH9_UART0TX);

    while(1)
        {

        }

}





void InitDMA(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);

    uDMAEnable();
    uDMAControlBaseSet(ucControlTable);

    uint8_t index;
    for (index =0; index < 10; index++)
    {
        g_ucTxBuffer[index] = 0x30 + index;
    }

/*    UART0_RX_DMA*/
/*    Received data, the move it from FIFO buffer to g_ucRxBuffer */
    uDMAChannelAttributeDisable(UDMA_CH8_UART0RX,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST|
                                UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);

    uDMAChannelControlSet(UDMA_CH8_UART0RX | UDMA_PRI_SELECT,
                          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                          UDMA_ARB_4);

/*    UART0_TX_DMA*/
/*    Prepare for transmit data, from g_ucTxBuffer to UART FIFO*/
    uDMAChannelAttributeDisable(UDMA_CH9_UART0TX,
                                UDMA_ATTR_ALTSELECT |
                                UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);

    uDMAChannelAttributeEnable(UDMA_CH9_UART0TX, UDMA_ATTR_USEBURST);

    uDMAChannelControlSet(UDMA_CH9_UART0TX | UDMA_PRI_SELECT,
                          UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE |
                          UDMA_ARB_16);

    uDMAChannelTransferSet(UDMA_CH9_UART0TX | UDMA_PRI_SELECT,
                           UDMA_MODE_AUTO, g_ucTxBuffer,
                           (void *)(UART0_BASE + UART_O_DR),
                           sizeof(g_ucTxBuffer));




}

void InitConsole(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);
}
