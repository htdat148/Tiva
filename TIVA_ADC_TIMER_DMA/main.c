#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "inc/hw_adc.h"

#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t DMAcontroltable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(DMAcontroltable, 1024)
uint8_t DMAcontroltable[1024];
#else
uint8_t DMAcontroltable[1024] __attribute__ ((aligned(1024)));
#endif

uint32_t ui32ADC0Value[4] = {0};
volatile uint32_t ui32TempAvg = 0;
volatile uint32_t ui32TempValueC = 0;
volatile uint32_t ui32TempValueF = 0;
volatile uint32_t ui32ADCStatus = 0;

void Timer0_Interrupt_Handler(void)
{
    memset(&ui32ADC0Value, 0, 4*sizeof(uint32_t));
    UARTprintf("Timer 0 interrupt \n");
    TimerIntStatus(TIMER0_BASE, true);
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
//    TimerDisable(TIMER0_BASE, TIMER_A);

}
void ADC_Interrupt_Handle(void)
{
    UARTprintf("ADC 0 Interrupt\n");
    ADCIntStatus(ADC0_BASE, 1, true);
    ADCIntClear(ADC0_BASE, 1);
    ADCSequenceDMAEnable(ADC0_BASE, 1);
// Enable DMA channel again
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC1, UDMA_MODE_AUTO,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO1), ui32ADC0Value, 4);

    uDMAChannelEnable(UDMA_CHANNEL_ADC1);
}

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
//    UART config
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);

//  ADC Sequence trigger by using Timer
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_TIMER, 0);

    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 1);

//  setup timer for triggering ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC_UP);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 50000000 -1);
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

//  configuration Timer Int
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);
    IntEnable(INT_TIMER0A);

//  Config DMA transfer data from ADC SS1 FIFO to buffer
    SysCtlPeripheralDisable(SYSCTL_PERIPH_UDMA);
    SysCtlPeripheralReset(SYSCTL_PERIPH_UDMA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAEnable();
    uDMAControlBaseSet(DMAcontroltable);

//    Set the channel trigger to be ADC0_1
//    uDMAChannelAssign(UDMA_CH15_ADC0_1);
    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC1,
                                UDMA_ATTR_USEBURST|
                                UDMA_ATTR_ALTSELECT |
                                UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);

    uDMAChannelControlSet(UDMA_CHANNEL_ADC1 | UDMA_PRI_SELECT,
                          UDMA_SIZE_32 | UDMA_SRC_INC_NONE |
                          UDMA_DST_INC_32 | UDMA_ARB_4);

    uDMAChannelTransferSet(UDMA_CHANNEL_ADC1, UDMA_MODE_AUTO,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO1), ui32ADC0Value, 4);

    uDMAChannelEnable(UDMA_CHANNEL_ADC1);


//  Configuration ADC with SS1 Interrupt
    ADCIntClear(ADC0_BASE, 1);
    ADCIntEnable(ADC0_BASE, 1);
    IntEnable(INT_ADC0SS1);
    IntMasterEnable();


    while(1)
    {
    }
}
