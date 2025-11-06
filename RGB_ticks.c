#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "RGB_ticks.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BOARD_LED_R_GPIO     	BOARD_LED_RED_GPIO
#define BOARD_LED_R_GPIO_PIN 	BOARD_LED_RED_PIN

#define BOARD_LED_B_GPIO     	BOARD_LED_BLUE_GPIO
#define BOARD_LED_B_GPIO_PIN 	BOARD_LED_BLUE_PIN

#define BOARD_LED_G_GPIO     	BOARD_LED_GREEN_GPIO
#define BOARD_LED_G_GPIO_PIN 	BOARD_LED_GREEN_PIN

#define GPIO_C ((GPIO_Type *)(0x400FF080u))
#define GPIO_A ((GPIO_Type *)(0x400FF000u))
#define GPIO_E ((GPIO_Type *)(0x400FF100u))
#define GPIO_D ((GPIO_Type *)(0x400FF0C0u))

#define RED		9u
#define	BLUE	11u
#define	GREEN	6u

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_systickCounter;
//void InitPins_vUART_Rx(void) {
//    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
//
//    PORTD->PCR[12U] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//    PORTD->PCR[13U] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//    PORTD->PCR[1U ] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//
//    GPIOD->PDDR &= ~((1U << 1U) | (1U << 12U) | (1U << 13U));
//    //NVIC_EnableIRQ(PORTD_IRQn);
//
//    NVIC_SetPriority(PORTD_IRQn, 3);
//    NVIC_EnableIRQ(PORTD_IRQn);
//}



void InitPins_vUART_Rx(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    // Configura MUX, pull-up y flanco de bajada
    PORTD->PCR[12] = PORT_PCR_MUX(1)
                   | PORT_PCR_PE_MASK
                   | PORT_PCR_PS_MASK
                   | PORT_PCR_IRQC(0xA);
    // … igual para PD1 y PD13 …
    GPIOD->PDDR &= ~((1U<<12)|(1U<<1)|(1U<<13));

    NVIC_SetPriority(PORTD_IRQn, 3);
    NVIC_EnableIRQ(PORTD_IRQn);
}

/*******************************************************************************
 * Code
 ******************************************************************************/
void RGB_ticks_Init(){
    /* Board pin init */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBUTTONsPins();
    //InitPins_vUART_Rx();
}
void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}
void Led_Red(){
	GPIO_PortClear(	GPIO_C, 1u << RED  );
	GPIO_PortSet  ( GPIO_A, 1u << BLUE );
	GPIO_PortSet  (	GPIO_E, 1u << GREEN);
}
void Led_Blue(){
	GPIO_PortSet  (	GPIO_C, 1u << RED  );
	GPIO_PortClear( GPIO_A, 1u << BLUE );
	GPIO_PortSet  (	GPIO_E, 1u << GREEN);
}
void Led_Green(){
	GPIO_PortSet  (	GPIO_C, 1u << RED  );
	GPIO_PortSet  ( GPIO_A, 1u << BLUE );
	GPIO_PortClear(	GPIO_E, 1u << GREEN);
}
void Led_Yellow(){
	GPIO_PortClear(	GPIO_C, 1u << RED  );
	GPIO_PortSet  ( GPIO_A, 1u << BLUE );
	GPIO_PortClear(	GPIO_E, 1u << GREEN);
}
void Led_Purple(){
	GPIO_PortClear(	GPIO_C, 1u << RED  );
	GPIO_PortClear( GPIO_A, 1u << BLUE );
	GPIO_PortSet  (	GPIO_E, 1u << GREEN);
}
void Led_Off(){
	GPIO_PortSet  (	GPIO_C, 1u << RED  );
	GPIO_PortSet  ( GPIO_A, 1u << BLUE );
	GPIO_PortSet  (	GPIO_E, 1u << GREEN);
}
void Led_White(){
	GPIO_PortClear(	GPIO_C, 1u << RED  );
	GPIO_PortClear( GPIO_A, 1u << BLUE );
	GPIO_PortClear(	GPIO_E, 1u << GREEN);
}

void SysTick_DelayTicks(uint32_t n)
{
	if (SysTick_Config(SystemCoreClock / 1000U)) { while (1){ } }
	g_systickCounter = n;
    while (g_systickCounter != 0U)
    {
    }

}
