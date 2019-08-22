#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/pwm.h"


/**
 * main.c
 */
int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
/*//    PWM clock set
    M1PWM4 -----> PF0
    M1PWM5 -----> PF1
    M1PWM6 -----> PF2
    M1PWM7 -----> PF3*/
    PWMClockSet(PWM1_BASE, PWM_SYSCLK_DIV_1);
//    unlock pin
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);

    GPIOPinConfigure(GPIO_PF0_M1PWM4);
    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
    GPIOPinConfigure(GPIO_PF3_M1PWM7);
//    config PWM mode

    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, 400);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, 700);

    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_4, 300);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 300);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, 500);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, 400);

    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);

    PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT |
                              PWM_OUT_5_BIT |
                              PWM_OUT_6_BIT |
                              PWM_OUT_7_BIT, true);

    int width;

    while(1)
    {
        for (width = 50; width < 800; width +=80)

        {
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, width);
            SysCtlDelay(4000000);
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, width);
            SysCtlDelay(5000000);
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, width);
            SysCtlDelay(6000000);
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_4, width);
            SysCtlDelay(7000000);

        }

    }
	return 0;
}
