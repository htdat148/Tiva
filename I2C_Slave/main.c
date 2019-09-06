#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

#define MY_ADDRESS 42
#define MASTER_ADDRESS 55

// Tiva C acts as a slave
// receive request from Adruino
// send response to Adruino
enum {
    CMD_ID = 1,
    CMD_READ_A0 = 2,
    CMD_READ_D8 = 3
};

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

//sends an I2C command to the specified slave
void I2CSend(uint8_t slave_addr, uint8_t num_of_args, ...)
{
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //stores list of variable number of arguments
    va_list vargs;

    //specifies the va_list to "open" and the last fixed argument
    //so vargs knows where to start looking
    va_start(vargs, num_of_args);

    //put data to be sent into FIFO
    I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));

    //if there is only one argument, we only need to use the
    //single send I2C function
    if(num_of_args == 1)
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //"close" variable argument list
        va_end(vargs);
    }

    //otherwise, we start transmission of multiple bytes on the
    //I2C bus
    else
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //send num_of_args-2 pieces of data, using the
        //BURST_SEND_CONT command of the I2C module
        unsigned char i;
        for(i = 1; i < (num_of_args - 1); i++)
        {
            //put next piece of data into I2C FIFO
            I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));
            //send next data that was just placed into FIFO
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

            // Wait until MCU is done transferring.
            while(I2CMasterBusy(I2C0_BASE));
        }

        //put last piece of data into I2C FIFO
        I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));
        //send next data that was just placed into FIFO
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //"close" variable args list
        va_end(vargs);
    }
}

void readDataRegister(uint8_t registerAdd, uit8_t numberOfData)
{
//    TODO: modify base on the requirements
    uint8_t *ptr;
    uint8_t index = 0;
    ptr = (uint8_t *)malloc(numberOfData);
    for(index = 0; index < numberOfData; index++)
    {
        *(ptr + index) = index;
    }

//    send data to MASTER
//    TODO: add number o
    I2CSend(MASTER_ADDRESS, numberofData, ...);
	free(ptr);

}

uint32_t slaveStatus = 0;
uint32_t registerAddress;

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
            SYSCTL_XTAL_16MHZ);
    I2C0_Init();
    InitConsole();
//    Enable slave mode
    I2CSlaveEnable(I2C0_BASE);
    I2CSlaveInit(I2C0_BASE, MY_ADDRESS);
    bool requestFromMaster = false;
    uint8_t numberDataRequest;

    while(1)
    {
        slaveStatus = I2CSlaveStatus(I2C0_BASE);

        if (slaveStatus & I2C_SLAVE_ACT_RREQ)
        {
            UARTprintf("Receive data from Master\n");
            registerAddress = I2CSlaveDataGet(I2C0_BASE);
        }
        else if (slaveStatus & I2C_SLAVE_ACT_TREQ)
        {
            UARTprintf("Receive request send data to Master\n");
            numberDataRequest = I2CSlaveDataGet(I2C0_BASE);
            requestFromMaster = true;
        }
//
        if (requestFromMaster & (numberDataRequest > 0))
        {
//            read the data in register address
//            send back to  master
            readDataRegister(registerAddress, numberDataRequest)
//            reset flag
            requestFromMaster  = false;
            numberDataRequest = 0;
        }
    }
}
