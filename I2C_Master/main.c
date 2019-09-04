#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

#include "driverlib/interrupt.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"

#define SLAVE_ADDRESS 0x3C
volatile uint32_t read;
volatile uint8_t receive;

void I2CMasterIntHandler(void)
{
    uint32_t intStatus;
    intStatus = I2CMasterIntStatusEx(I2C0_BASE, true);
    UARTprintf("Interrupt flag: %d\n", intStatus);
    I2CMasterIntClear(I2C0_BASE);

    while(I2CMasterBusy(I2C0_BASE));
    receive = I2CMasterDataGet(I2C0_BASE);
    UARTprintf("Data receive again: %c\n", receive);

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

void I2C0_Master_Init(void)
{
//    Enable GPIO Pin for I2C0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
//    Reset and enable I2C module
    SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

//    Config pin muxing for I2C
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
//    Select I2C function for these pin
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

//    Enable and initialize the I2C master module
//    flase: set clock 400kHz
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet() , false);

}

//read multiple bytes specified register on slave device
uint32_t I2CReceiveMultipleBytes(uint32_t slave_addr, uint8_t reg, uint8_t size, uint8_t *DataGet)
{
    int index = 0;

    //specify that we are writing (a register address) to the
    //slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //specify register to be read
    I2CMasterDataPut(I2C0_BASE, reg);

    //send control byte and register address byte to slave device
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));

    //specify that we are going to read from slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, true);

//  the first byte receive
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));
    //return data pulled from the specified register
    *(DataGet + 0) = I2CMasterDataGet(I2C0_BASE);

    if (size >= 2)
    {
        for(index = 1; index < size - 1; index ++)
        {
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
            while(I2CMasterBusy(I2C0_BASE));
            *(DataGet + index) = I2CMasterDataGet(I2C0_BASE);
        }
    //    Get the last byte
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
        while(I2CMasterBusy(I2C0_BASE));
        *(DataGet + size -1) = I2CMasterDataGet(I2C0_BASE);

    }
    return 0;
}

int main()
{
    uint8_t ui32DataTx;

    SysCtlClockSet(SYSCTL_SYSDIV_1| SYSCTL_USE_OSC| SYSCTL_OSC_MAIN| SYSCTL_XTAL_16MHZ);
    InitConsole();
    I2C0_Master_Init();
//    Enale I2C Master block
    I2CMasterEnable(I2C0_BASE);
// Interrupt config to receives data from slave
    I2CMasterIntEnableEx(I2C0_BASE, I2C_MASTER_INT_DATA);
    IntEnable(INT_I2C0);
    IntMasterEnable();

    UARTprintf("I2C Master send data Example ->");
    UARTprintf("\n  Module = I2C0");
    UARTprintf("\n  Mode = Receive interrupt on the Slave module");
    UARTprintf("\n  Rate = 100kbps\n\n"); \
    while(1)
    {
/*        prepare SEND data for Master send data to Slave*/
        UARTprintf("Type input");
        ui32DataTx = UARTCharGet(UART0_BASE);
        UARTprintf("Transferring from: Master -> Slave\n");
        UARTprintf("  Sending: '%c'\n", ui32DataTx);
//        select SLAVE_ADDRESS to transfer
        I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
//        put data to I2C data register
        I2CMasterDataPut(I2C0_BASE, ui32DataTx);
//        Initiate sned data character
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
//        Delay until transmission completes
//        while(I2CMasterBusy(I2C0_BASE));
//
        UARTprintf("Changed mode receive from slave\n");
        SysCtlDelay(SysCtlClockGet()/2);
        I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, true);
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
    }

}
