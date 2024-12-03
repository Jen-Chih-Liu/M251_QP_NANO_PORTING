/**************************************************************************//**
 * @file     main.c
 * @version  V0.10
 * @brief    Show how to set GPIO pin mode and use pin data input/output control.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "stdio.h"
#include "NuMicro.h"
#include "qpn.h"     /* QP-nano API */
#define BSP_TICKS_PER_SEC    1000
/* ISRs used in this project ===============================================*/
void SysTick_Handler(void) {
    QF_tickXISR(0U); /* process time events for rate 0 */
}

void SYS_Init(void)
{

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock (Internal RC 48 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and and HCLK clock divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(GPB_MODULE);
    CLK_EnableModuleClock(GPC_MODULE);

    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    Uart0DefaultMPF();

}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART module */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
}

enum BlinkySignals {
    DUMMY_SIG = Q_USER_SIG,
    MAX_PUB_SIG,          /* the last published signal */

    TIMEOUT_SIG,
    MAX_SIG               /* the last signal */
};
typedef struct BlinkyTag {  /* the Blinky active object */
    QActive super;          /* inherit QActive */
} Blinky;

/* hierarchical state machine ... */
static QState Blinky_initial(Blinky * const me);
static QState Blinky_off    (Blinky * const me);
static QState Blinky_on     (Blinky * const me);

/* Global objects ----------------------------------------------------------*/
Blinky AO_Blinky;   /* the single instance of the Blinky AO */

/*..........................................................................*/
void Blinky_ctor(void) {
    Blinky * const me = &AO_Blinky;
    QActive_ctor(&me->super, Q_STATE_CAST(&Blinky_initial));
}

/* HSM definition ----------------------------------------------------------*/
QState Blinky_initial(Blinky * const me) {
    QActive_armX((QActive *)me, 0U,
                 BSP_TICKS_PER_SEC/2U, BSP_TICKS_PER_SEC/2U);
    return Q_TRAN(&Blinky_off);
}
/*..........................................................................*/
QState Blinky_off(Blinky * const me) {
    QState status;
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
            //BSP_ledOff();
					  printf("off\n\r");
            status = Q_HANDLED();
            break;
        }
        case Q_TIMEOUT_SIG: {
            status = Q_TRAN(&Blinky_on);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}
/*..........................................................................*/
QState Blinky_on(Blinky * const me) {
    QState status;
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
            //BSP_ledOn();
					printf("on\n\r");
            status = Q_HANDLED();
            break;
        }
        case Q_TIMEOUT_SIG: {
            status = Q_TRAN(&Blinky_off);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}
/* Local-scope objects -----------------------------------------------------*/
static QEvt l_blinkyQSto[10]; /* Event queue storage for Blinky */

/* QF_active[] array defines all active object control blocks --------------*/
QActiveCB const Q_ROM QF_active[] = {
    { (QActive *)0,           (QEvt *)0,        0U                      },
    { (QActive *)&AO_Blinky,  l_blinkyQSto,     Q_DIM(l_blinkyQSto)     }
};

void QF_onStartup(void) {
    /* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate */
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

    /* assing all priority bits for preemption-prio. and none to sub-prio. */
    //NVIC_SetPriorityGrouping(0U);

    /* set priorities of ALL ISRs used in the system, see NOTE00
    *
    * !!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    * Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
    * DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
    */
    NVIC_SetPriority(SysTick_IRQn,   0);
    /* ... */

    /* enable IRQs... */
}

void QV_onIdle(void) { /* CATION: called with interrupts DISABLED, NOTE01 */
    /* toggle LED2 on and then off, see NOTE02 */
    //GPIOF->DATA_Bits[LED_BLUE] = 0xFFU;
    //GPIOF->DATA_Bits[LED_BLUE] = 0x00U;
    printf("on idel");
#ifdef NDEBUG
    /* Put the CPU and peripherals to the low-power mode.
    * you might need to customize the clock management for your application,
    * see the datasheet for your particular Cortex-M MCU.
    */
    QV_CPU_SLEEP();  /* atomically go to sleep and enable interrupts */
#else
    QF_INT_ENABLE(); /* just enable interrupts */
#endif
}

/*..........................................................................*/
Q_NORETURN Q_onAssert(char const Q_ROM * const module, int loc) {
    /*
    * NOTE: add here your application-specific error handling
    */
    (void)module;
    (void)loc;
    while(1);
    NVIC_SystemReset();
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{

    int32_t i32Err, i32TimeOutCnt;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();
    Blinky_ctor(); /* instantiate all Blinky AO */

    QF_init(Q_DIM(QF_active)); /* initialize the QF-nano framework */
   
    return QF_run(); /* transfer control to QF-nano */
   

}
