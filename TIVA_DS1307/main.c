//include files
 #include <stdarg.h>
 #include <stdbool.h>
 #include <stdint.h>

 #include "inc/tm4c123gh6pm.h"
 #include "inc/hw_i2c.h"
 #include "inc/hw_memmap.h"
 #include "inc/hw_types.h"
 #include "inc/hw_gpio.h"


 #include "driverlib/i2c.h"
 #include "driverlib/sysctl.h"
 #include "driverlib/gpio.h"
 #include "driverlib/pin_map.h"
 #include "driverlib/uart.h"
 #include "driverlib/interrupt.h"
 #include "utils/uartstdio.h"

/*
Reference
http://noobtechiespeaks.blogspot.com/2014/11/smartwatch-part-1-real-time-clock.html
Some modifications by me:
*Write a byte to specific register function
*Edit the SetDay function
* Add interurpt handler
*/

//Defines for DS1307
 #define SLAVE_ADDRESS 0x68
 #define SEC 0x00
 #define MIN 0x01
 #define HRS 0x02
 #define DAY 0x03
 #define DATE 0x04
 #define MONTH 0x05
 #define YEAR 0x06
 #define CNTRL 0x07

// Interrupt handler function
void I2C0IntHanler(void)
{
    uint32_t statusInt;
    uint32_t data;
      statusInt = I2CMasterIntStatusEx(I2C0_BASE, true);
    switch (statusInt)
    {
    case I2C_MASTER_INT_START:
        UARTprintf("I2C_MASTER_INT_START\n");
        I2CMasterIntClearEx(I2C0_BASE, I2C_MASTER_INT_START);
        break;
    case I2C_MASTER_INT_STOP:
        UARTprintf("I2C_MASTER_INT_STOP\n");
        I2CMasterIntClearEx(I2C0_BASE, I2C_MASTER_INT_STOP);
        break;
    case I2C_MASTER_INT_DATA :
        UARTprintf("I2C_MASTER_INT_DATA \n");
        I2CMasterIntClearEx(I2C0_BASE, I2C_MASTER_INT_DATA );
        break;
    case I2C_MASTER_INT_NACK :
        UARTprintf("I2C_MASTER_INT_NACK \n");
        I2CMasterIntClearEx(I2C0_BASE, I2C_MASTER_INT_NACK );
        break;
    }
    I2CMasterIntClear(I2C0_BASE);
}
 //initialize I2C module 0
 //Slightly modified version of TI's example code
 void InitI2C0(void)
 {
   //enable I2C module 0
   SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
   //reset module
   SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
   //enable GPIO peripheral that contains I2C 0
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
   // Configure the pin muxing for I2C0 functions on port B2 and B3.
   GPIOPinConfigure(GPIO_PB2_I2C0SCL);
   GPIOPinConfigure(GPIO_PB3_I2C0SDA);

   // Select the I2C function for these pins.
   GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
   GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
   // Enable and initialize the I2C0 master module. Use the system clock for
   // the I2C0 module. The last parameter sets the I2C data transfer rate.
   // If false the data rate is set to 100kbps and if true the data rate will
   // be set to 400kbps.
   I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
   //clear I2C FIFOs
   HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;

   //I2C initile interurpt
//   clear all interrupt flag
//   I2CMasterIntClear(I2C0_BASE);
   I2CMasterIntEnableEx(I2C0_BASE, I2C_MASTER_INT_START |
                                   I2C_MASTER_INT_STOP |
                                   I2C_MASTER_INT_DATA |
                                   I2C_MASTER_INT_NACK);
//   I2CIntRegister(I2C0_BASE, I2C0IntHanler);
//   I2CMasterEnable(I2C0_BASE);
   IntEnable(INT_I2C0);
   IntMasterEnable();
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

 //read specified register on slave device
 uint32_t I2CReceive(uint32_t slave_addr, uint8_t reg)
 {
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
   //send control byte and read from the register we
   //specified
   I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
   //wait for MCU to finish transaction
   while(I2CMasterBusy(I2C0_BASE));
   //return data pulled from the specified register
   return I2CMasterDataGet(I2C0_BASE);
 }
 unsigned char dec2bcd(unsigned char val)
 {
      return (((val / 10) << 4) | (val % 10));
 }
 // convert BCD to binary
 unsigned char bcd2dec(unsigned char val)
 {
  return (((val & 0xF0) >> 4) * 10) + (val & 0x0F);
 }
 //Set Time
 void SetTimeDate(unsigned char sec, unsigned char min, unsigned char hour,unsigned char day, unsigned char date, unsigned char month,unsigned char year)
 {
      I2CSend(SLAVE_ADDRESS,8,SEC,dec2bcd(sec),dec2bcd(min),dec2bcd(hour),dec2bcd(day),dec2bcd(date),dec2bcd(month),dec2bcd(year));
 }

 //Set Date
  void SetDate(unsigned char day, unsigned char date, unsigned char month,unsigned char year)
  {
       I2CSend(SLAVE_ADDRESS,5, DAY, dec2bcd(day), dec2bcd(date), dec2bcd(month), dec2bcd(year));
  }

// Set specified register function
  void SetSpecReg(uint8_t regAdd, uint8_t data)
  {
      I2CSend(SLAVE_ADDRESS, 2, regAdd, data );
  }

 //Get Time and Date
 unsigned char GetClock(unsigned char reg)
 {
      unsigned char clockData = I2CReceive(SLAVE_ADDRESS,reg);
      return bcd2dec(clockData);
 }
 void ConfigureUART(void)
 {
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
   GPIOPinConfigure(GPIO_PA0_U0RX);
   GPIOPinConfigure(GPIO_PA1_U0TX);
   GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
   UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
   UARTStdioConfig(0, 115200, 16000000);
 }
 void main(void)
 {
      // Set the clocking to run directly from the external crystal/oscillator.
        SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_INT | SYSCTL_XTAL_16MHZ);
        //initialize I2C module 0
        InitI2C0();
        ConfigureUART();
//        SetTimeDate(30,32,8,4,5,11,14);
        unsigned char sec,min,hour,day,date,month,year;
        while(1)
        {
             sec = GetClock(SEC);
             min = GetClock(MIN);
             hour = GetClock(HRS);
             day = GetClock(DAY);
             date = GetClock(DATE);
             month = GetClock(MONTH);
             year = GetClock(YEAR);
             SysCtlDelay(SysCtlClockGet()/10*3);
             UARTprintf("%02d:%02d:%02d \n%02d %02d/%02d/%02d\n",hour,min,sec,day,date,month,year);
             SetDate(7, 7, 7, 17);
             SetSpecReg(YEAR, dec2bcd(66));
             SysCtlDelay(SysCtlClockGet()/10*3);
             SetSpecReg(MONTH, dec2bcd(12));
             SysCtlDelay(SysCtlClockGet());

        }
 }
