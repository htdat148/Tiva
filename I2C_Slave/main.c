
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"

#define SLAVE_ADDRESS 0x3C

//global variable to receive data
volatile uint8_t g_ui32DataRx;
volatile bool receiveFlag = false;

void I2CSlaveIntHanler(void)
{
    UARTprintf("Interrupt status: %d\n", I2CSlaveIntStatus(I2C0_BASE, true) );
    I2CSlaveIntClear(I2C0_BASE);
//    while(!(I2CSlaveStatus(I2C0_BASE) & I2C_SLAVE_ACT_RREQ))
//    {
//    }
    g_ui32DataRx = I2CSlaveDataGet(I2C0_BASE);
    UARTprintf("I2C Slave receives: %c\n", g_ui32DataRx);
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

void I2C0_Init(void)
{
//    Enable GPIO_PORTB
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

   SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C0);
   SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

   GPIOPinConfigure(GPIO_PB2_I2C0SCL);
   GPIOPinConfigure(GPIO_PB3_I2C0SDA);
   GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
   GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

}

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
            SYSCTL_XTAL_16MHZ);
    I2C0_Init();
//    Init slave
    I2CSlaveInit(I2C0_BASE, SLAVE_ADDRESS);
//    enable I2C slave block
    I2CSlaveEnable(I2C0_BASE);
    InitConsole();
//    Config receive interrupt
    I2CSlaveIntEnableEx(I2C0_BASE, I2C_SLAVE_INT_DATA);
    IntEnable(INT_I2C0);
    IntMasterEnable();

    UARTprintf("Slave waiting I2C Data\n");

    while(1)
    {
        UARTprintf("wait request signal\n");
        while(!(I2CSlaveStatus(I2C0_BASE) & I2C_SLAVE_ACT_TREQ));
        UARTprintf("receive request signal\n");
        I2CSlaveDataPut(I2C0_BASE, g_ui32DataRx);


    }
}
