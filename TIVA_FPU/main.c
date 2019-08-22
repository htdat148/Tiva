#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"
#include "driverlib/fpu.h"
#include "driverlib/rom.h"


/*
 * ---> Scale the number from UART
 * ---> Input a: number
 * ---> Input b: scale
 * ---> Implement scale: (float)a/b
 * ---> Convert the float result to char by using sprintf
 * ---> Then send it back to UART
*/
/*

********************* NOTE ****************************
A common problem when using functions form the printf/sprintf/fprintf family
is the stack size - these fucntions tack a lot of stack size.
The defauft stack size in CCS is 256, not enough. This causes the FaultISR()
You should increase the stack size to 2048
properties --> ARM Linker --> Basic Options and set C System Stack Size to 2048
******************************************************
*/
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

char Number;
char Scale;
float Result;
char Buffer[100];

int main()
{

    SysCtlClockSet(SYSCTL_SYSDIV_1| SYSCTL_USE_OSC| SYSCTL_OSC_MAIN| SYSCTL_XTAL_16MHZ);
    InitConsole();
    // Enable the floating-point unit.
    FPUEnable();
    // Configure the floating-point unit to perform lazy stacking of the
    // floating-point state.
    FPULazyStackingEnable();
    UARTprintf("Scale number test with FPU");



    while(1)
    {
        UARTprintf("\n Type number: ");
        Number = UARTCharGet(UART0_BASE);
        UARTprintf("\n Type scale: ");
        Scale = UARTCharGet(UART0_BASE);
//      cast to float for the result
        Result = (float)Number/Scale;
//      using sprint to convert float to char
//        UARTprint to print out
        sprintf(Buffer, "Result: %f\n", Result);
        UARTprintf("%s", Buffer);

    }

}



