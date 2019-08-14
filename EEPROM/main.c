
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "inc/hw_eeprom.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/eeprom.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

/**
 * main.c
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

uint32_t EEPROMInitValue;
uint32_t EEPROMEraseValue;
uint32_t EEPROMWriteData[4] = {1,2,3,4};
uint32_t EEPROMPwd[4] = {7,8,9};
uint32_t EEPROMReadData[4] = {0};
uint32_t e2size, e2block;
uint32_t Status;

int main(void)
{
//    Enbale UART console
    InitConsole();
//    Enable EEPROM
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0));
//    Check EEPROM status
    EEPROMInitValue = EEPROMInit();
    if (EEPROMInitValue == EEPROM_INIT_OK)
        UARTprintf("EEPROM state: %d\n", EEPROMInitValue);
    else
        UARTprintf("Issue with the EEPROM\n");
// Erase the EEPROM
    EEPROMEraseValue = EEPROMMassErase();
    if (EEPROMEraseValue == 0)
        UARTprintf("Already erase the data in EEPROM\n");
    else
        UARTprintf("Can't erase the data in EEPROM");

    e2size = EEPROMSizeGet(); // Get EEPROM Size
    UARTprintf("EEPROM Size %d bytes\n", e2size);

    e2block = EEPROMBlockCountGet(); // Get EEPROM Block Count
    UARTprintf("EEPROM Blok Count: %d\n", e2block);

//    store data in EEPROM
    EEPROMProgram(EEPROMWriteData, 0x00, sizeof(EEPROMWriteData));
//    block for none reading


    Status = EEPROMBlockPasswordSet(0, EEPROMPwd, 3);

    EEPROMBlockProtectSet(0, EEPROM_PROT_RO_LNA_URO);
    EEPROMBlockLock(0);
// fail to read
    EEPROMRead(EEPROMReadData, 0x00, sizeof(EEPROMReadData));

//    unlock block for reading
    EEPROMBlockUnlock(0, EEPROMPwd, 3);
    EEPROMRead(EEPROMReadData, 0x00, sizeof(EEPROMReadData));
    SysCtlDelay(SysCtlClockGet());


}
