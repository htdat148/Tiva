void InituDMA(void);
void InitGPIO(void);
void InitTimer(void);
void setup();
void loop(void);
 void TimerInt(void);

#include <stdint.h>
#include <stdbool.h>
#include "stdlib.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.c"
#include "driverlib/sysctl.c"
#include "driverlib/timer.c"
#include "driverlib/udma.c"
#include "driverlib/gpio.c"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include <string.h>

/*the size of Source and Destination*/
#define SIZE 128
uint32_t SrcBuf[SIZE], DestBuf[SIZE];


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

/*
  Function to setup the DMA
*/
void InituDMA()
{

  //Just disable to be able to reset the peripheral state
  SysCtlPeripheralDisable(SYSCTL_PERIPH_UDMA);
  SysCtlPeripheralReset(SYSCTL_PERIPH_UDMA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);

  SysCtlDelay(10);

  uDMAEnable();

  uDMAControlBaseSet(DMAcontroltable);

  //Set the channel trigger to be SOFTWARE
  uDMAChannelAssign(UDMA_CH30_SW);

  //Disable all the atributes in case any was set
  uDMAChannelAttributeDisable(UDMA_CH30_SW,
  UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
    UDMA_ATTR_HIGH_PRIORITY |
    UDMA_ATTR_REQMASK);

/*
 * UDMA_ARB_x: number of transfers the DMA does per trigger
 * x: 1,2,4,8,32,128,1024
 */

  uDMAChannelControlSet(UDMA_CH30_SW | UDMA_PRI_SELECT,
  UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 |
    UDMA_ARB_128);

  /*
    UDMA_MODE_BASIC: the number of transfer follow the Arbitration size
    if you have 128 elements in buffer and set UDMA_ARB_32, DMA ONLY does 32 transfer
    UDMA_MODE_AUTO: transfer all elements means the SIZE
  */
  uDMAChannelTransferSet(UDMA_CH30_SW | UDMA_PRI_SELECT,
                         UDMA_MODE_BASIC,
                         SrcBuf, DestBuf,
                         SIZE);

  //Enable the DMA chanel
  uDMAChannelEnable(UDMA_CH30_SW);

}



void main()
{

  //Set CLK to 120Mhz
    MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
    SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
    SYSCTL_CFG_VCO_480), 120000000);
    int i = 0;

  InituDMA();

  for(i = 0; i< 128; i++)
  {
      SrcBuf[i] = i;
  }

/*  request a DMA transfer*/
  uDMAChannelRequest(UDMA_CH30_SW);
  while(1)
  {

  }

}
