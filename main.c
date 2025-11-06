#include "fsl_smc.h"
#include "fsl_clock.h"
#include "board.h"
#include "fsl_pit.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "OS/OS.h"
#include "DRIVERS/RGB_ticks.h"
#include "DRIVERS/PWM_FTM.h"

/*
 * CARLOS EMILIO DE SANTIAGO TOVAR
 * VICTOR MANUEL SANDOVAL MACIEL
 */

/*!
 * @brief Task function prototypes.
 *
 * These tasks will be executed based on the scheduling system.
 */
void Task_AF(); /*!< Task A Function */
void Task_BF(); /*!< Task B Function */
void Task_CF(); /*!< Task C Function */
void VUART_RX(volatile PORT_Type *PORT, uint8_t PIN, uint8_t PWM);


//
//
///*!
// * @brief Main function.
// *
// * This function initializes the OS, configures the LED system, and creates tasks.
// * It then starts the scheduler and enters an infinite loop.
// */
//
uint8_t Arr_PWM_DC[3] = {10, 10, 10};	/* Duty Cycle */
volatile bool receiving = false;
volatile uint8_t bit_count = 0;
volatile uint8_t byte = 0;
volatile GPIO_Type *vgpio = NULL;
volatile uint8_t vpin = 0;
volatile uint8_t pwm = 0;


//
int main(void) {
//
//
//
    RGB_ticks_Init(); /*!< Initialize the RGB LED control system */
    Init_FTM0_Pins(); /*!< Initialize the PWM_FTM control system */

//
//
    Create_task(0, 1, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_AF);	/* 	PWM1	*/
    Create_task(1, 2, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_BF);	/* 	PWM2	*/
    Create_task(2, 3, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_CF);	/*	PWM3	*/
//
    SetRelAlarm(0, 10, 0, true);	/* Task_0 / PWM_0 */
    SetRelAlarm(1, 23, 1, true);	/* Task_1 / PWM_0 */
    SetRelAlarm(2, 33, 2, true);	/* Task_2 / PWM_0 */

    OS_init(); /*!< Start the OS scheduler */
//
//    /* Infinite loop to keep the main function running */
    while(1) {
//
    }
//
}
//
//
void Task_AF() {

	Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[0]);
	if(Arr_PWM_DC[0] == 90){ Arr_PWM_DC[0]=0; }
	Terminate_task();
}


void Task_BF() {


	Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
		if(Arr_PWM_DC[1] == 90){ Arr_PWM_DC[1]=0; }
	Terminate_task();
}
void Task_CF() {

	Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
	if(Arr_PWM_DC[2] == 90){ Arr_PWM_DC[2]=0; }
	Terminate_task();
}


void PORTC_IRQHandler(void){
    if (PORT_GetPinsInterruptFlags(PORTC) & (1 << 10U)) {
        PORT_ClearPinsInterruptFlags(PORTC, (1 << 10U));
    }
    //Arr_PWM_DC[0]
    VUART_RX(PORTC,10,0);
}
//
void PORTD_IRQHandler(void) {
    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << 12U)) {
        PORT_ClearPinsInterruptFlags(PORTD, (1 << 12U));
    }
    //Arr_PWM_DC[1]
     VUART_RX(PORTD,12,1);
}

//
void PORTE_IRQHandler(void) {
    if (PORT_GetPinsInterruptFlags(PORTE) & (1 << 24U)) {
        PORT_ClearPinsInterruptFlags(PORTE, (1 << 24U));
    }
    //Arr_PWM_DC[2]
    VUART_RX(PORTE,24,2);
}
//
//
//
void PIT1_IRQHandler(void) {
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
//
    if (receiving) {
    	uint8_t bit = GPIO_PinRead((GPIO_Type *)vgpio, vpin);
        byte |= (bit << bit_count);
        bit_count++;
//
        if (bit_count == 1) {
            // Después del primer bit, configurar el timer para 1 bit (~417 us)
            PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk))); //2400 bps 417us
        }
//
        if (bit_count >= 8) {
            receiving = false;
            PIT_StopTimer(PIT, kPIT_Chnl_1);

            if(byte <= 100){
                Arr_PWM_DC[pwm] = byte;
            }

            if (vgpio == GPIOC)
                PORTC->PCR[vpin] = (PORTC->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
            else if (vgpio == GPIOD)
                PORTD->PCR[vpin] = (PORTD->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
            else if (vgpio == GPIOE)
                PORTE->PCR[vpin] = (PORTE->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);

        }
    }
}
//
void VUART_RX(volatile PORT_Type *PORT, uint8_t PIN, uint8_t PWM) {
    // Configurar los registros
    if (PORT == PORTC)
        vgpio = GPIOC;
    else if (PORT == PORTD)
        vgpio = GPIOD;
    else if (PORT == PORTE)
        vgpio = GPIOE;

    vpin = PIN;
    pwm = PWM;
    bit_count = 0;
    byte = 0;
    receiving = true;

    PORT->PCR[PIN] &= ~PORT_PCR_IRQC_MASK;

    PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(626U, CLOCK_GetFreq(kCLOCK_BusClk))); //2400 bauds 626u
    PIT_StartTimer(PIT, kPIT_Chnl_1);
}
//#include "fsl_smc.h"
//#include "fsl_clock.h"
//#include "board.h"
//#include "fsl_pit.h"
//#include "peripherals.h"
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "fsl_debug_console.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include "OS/OS.h"
//#include "DRIVERS/RGB_ticks.h"
//#include "DRIVERS/PWM_FTM.h"
//
//
///*
// * CARLOS EMILIO DE SANTIAGO TOVAR
// * VICTOR MANUEL SANDOVAL MACIEL
// */
//
///*!
// * @brief Task function prototypes.
// *
// * These tasks will be executed based on the scheduling system.
// */
//void Task_AF(); /*!< Task A Function */
//void Task_BF(); /*!< Task B Function */
//void Task_CF(); /*!< Task C Function */
//void VUART_RX(uint8_t PWM);
//
//
///*!
// * @brief Main function.
// *
// * This function initializes the OS, configures the LED system, and creates tasks.
// * It then starts the scheduler and enters an infinite loop.
// */
//
//uint8_t Arr_PWM_DC[3] = {20, 50, 80};	/* Duty Cycle */
//volatile bool receiving = false;
//volatile uint8_t bit_count = 0;
//volatile uint8_t byte = 0;
//volatile GPIO_Type *vgpio = NULL;
//volatile uint8_t vpin = 0;
//volatile uint8_t PWM = 0;
//
//
//int main(void) {
//
//	InitPins_vUART_Rx2();
//
//    RGB_ticks_Init(); /*!< Initialize the RGB LED control system */
//    Init_FTM0_Pins(); /*!< Initialize the PWM_FTM control system */
//
//
//
//    Create_task(0, 4, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_AF);	/* 	PWM1	*/
////    Create_task(1, 3, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_BF);	/* 	PWM2	*/
////    Create_task(2, 2, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_CF);	/*	PWM3	*/
//
//    SetRelAlarm(0, 100, 0, true);	/* Task_0 / PWM_0 */
////    SetRelAlarm(0, 23, 1, true);	/* Task_1 / PWM_0 */
////    SetRelAlarm(0, 33, 2, true);	/* Task_2 / PWM_0 */
//
//    OS_init(); /*!< Start the OS scheduler */
//
//    /* Infinite loop to keep the main function running */
//    while(1) {
//
//    }
//
//}
//
//
//void Task_AF() {
//
//	if(PWM==0){
//		Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[0]);
//	}
//	if(PWM==1){
//		Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
//	}
//	if(PWM==2){
//		Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
//	}
//	Terminate_task();
//}
////void Task_BF() {
////	Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
////	Arr_PWM_DC[1]=Arr_PWM_DC[1]+10;
////		if(Arr_PWM_DC[1] == 90){ Arr_PWM_DC[1]=0; }
////	Terminate_task();
////}
////void Task_CF() {
////	Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
////	Arr_PWM_DC[2]=Arr_PWM_DC[2]+10;
////	if(Arr_PWM_DC[2] == 90){ Arr_PWM_DC[2]=0; }
////	Terminate_task();
////}
//
//void PORTD_IRQHandler(void) {
//    PWM=5;
//	if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD1 )) {
//        PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD1 ));
//        PWM=0;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD12)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD12));
//		PWM=1;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD13)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD13));
//		PWM=2;
//    }
//     VUART_RX(PWM);
//}

//void PIT1_IRQHandler(void) {
//    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
//
//    if (receiving) {
//    	uint8_t bit = GPIO_PinRead((GPIO_Type *)vgpio, vpin);
//        byte |= (bit << bit_count);
//        bit_count++;
//
//        if (bit_count == 1) {
//            // Después del primer bit, configurar el timer para 1 bit (~417 us)
//            PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk)));
//        }
//
//        if (bit_count >= 8) {
//            receiving = false;
//            PIT_StopTimer(PIT, kPIT_Chnl_1);
//            Arr_PWM_DC[pwm] = byte;
//
//            if (vgpio == GPIOC)
//                PORTC->PCR[vpin] = (PORTC->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
//            else if (vgpio == GPIOD)
//                PORTD->PCR[vpin] = (PORTD->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
//            else if (vgpio == GPIOE)
//                PORTE->PCR[vpin] = (PORTE->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
//        }
//    }
//}

//void VUART_RX(uint8_t PWM) {
//	uint8_t DutyCycle_Bin= 0b00000000;
//	SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//
//	if(PWM==0){
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==1){ Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==2){
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//		Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//	}
//}




//#include "fsl_smc.h"
//#include "fsl_clock.h"
//#include "board.h"
//#include "fsl_pit.h"
//#include "peripherals.h"
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "fsl_debug_console.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include "OS/OS.h"
//#include "DRIVERS/RGB_ticks.h"
//#include "DRIVERS/PWM_FTM.h"
//
//
///*
// * CARLOS EMILIO DE SANTIAGO TOVAR
// * VICTOR MANUEL SANDOVAL MACIEL
// */
//
///*!
// * @brief Task function prototypes.
// *
// * These tasks will be executed based on the scheduling system.
// */
//void Task_AF(); /*!< Task A Function */
//void Task_BF(); /*!< Task B Function */
//void Task_CF(); /*!< Task C Function */
//void VUART_RX(uint8_t PWM);
//
//
///*!
// * @brief Main function.
// *
// * This function initializes the OS, configures the LED system, and creates tasks.
// * It then starts the scheduler and enters an infinite loop.
// */
//
//uint8_t Arr_PWM_DC[3] = {20, 50, 80};	/* Duty Cycle */
//volatile bool receiving = false;
//volatile uint8_t bit_count = 0;
//volatile uint8_t byte = 0;
//volatile GPIO_Type *vgpio = NULL;
//volatile uint8_t vpin = 0;
//volatile uint8_t PWM = 0;
//
//
//int main(void) {
//
//	InitPins_vUART_Rx();
//
//    RGB_ticks_Init(); /*!< Initialize the RGB LED control system */
//    Init_FTM0_Pins(); /*!< Initialize the PWM_FTM control system */
//
//
//
//    Create_task(0, 4, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_AF);	/* 	PWM1	*/
////    Create_task(1, 3, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_BF);	/* 	PWM2	*/
////    Create_task(2, 2, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_CF);	/*	PWM3	*/
//
//    SetRelAlarm(0, 100, 0, true);	/* Task_0 / PWM_0 */
////    SetRelAlarm(0, 23, 1, true);	/* Task_1 / PWM_0 */
////    SetRelAlarm(0, 33, 2, true);	/* Task_2 / PWM_0 */
//
//    OS_init(); /*!< Start the OS scheduler */
//
//    /* Infinite loop to keep the main function running */
//    while(1) {
//
//    }
//
//}
//
//
//void Task_AF() {
//
//	if(PWM==0){
//		Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[0]);
//	}
//	if(PWM==1){
//		Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[1]);
//	}
//	if(PWM==2){
//		Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[2]);
//	}
//	Terminate_task();
//}
////void Task_BF() {
////	Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
////	Arr_PWM_DC[1]=Arr_PWM_DC[1]+10;
////		if(Arr_PWM_DC[1] == 90){ Arr_PWM_DC[1]=0; }
////	Terminate_task();
////}
////void Task_CF() {
////	Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
////	Arr_PWM_DC[2]=Arr_PWM_DC[2]+10;
////	if(Arr_PWM_DC[2] == 90){ Arr_PWM_DC[2]=0; }
////	Terminate_task();
////}
//
//void PORTD_IRQHandler(void) {
//    PWM=5;
//	if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD1 )) {
//        PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD1 ));
//        PWM=0;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD12)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD12));
//		PWM=1;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD13)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD13));
//		PWM=2;
//    }
//     VUART_RX(PWM);
//}
//
////void PIT1_IRQHandler(void) {
////    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
////
////    if (receiving) {
////    	uint8_t bit = GPIO_PinRead((GPIO_Type *)vgpio, vpin);
////        byte |= (bit << bit_count);
////        bit_count++;
////
////        if (bit_count == 1) {
////            // Después del primer bit, configurar el timer para 1 bit (~417 us)
////            PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk)));
////        }
////
////        if (bit_count >= 8) {
////            receiving = false;
////            PIT_StopTimer(PIT, kPIT_Chnl_1);
////            Arr_PWM_DC[pwm] = byte;
////
////            if (vgpio == GPIOC)
////                PORTC->PCR[vpin] = (PORTC->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////            else if (vgpio == GPIOD)
////                PORTD->PCR[vpin] = (PORTD->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////            else if (vgpio == GPIOE)
////                PORTE->PCR[vpin] = (PORTE->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////        }
////    }
////}
//
//void VUART_RX(uint8_t PWM) {
//	uint8_t DutyCycle_Bin= 0b00000000;
//	SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//
//	if(PWM==0){
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==1){ Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==2){
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//		Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//	}
//}





//#include "fsl_smc.h"
//#include "fsl_clock.h"
//#include "board.h"
//#include "fsl_pit.h"
//#include "peripherals.h"
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "fsl_debug_console.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include "OS/OS.h"
//#include "DRIVERS/RGB_ticks.h"
//#include "DRIVERS/PWM_FTM.h"
//
//
///*
// * CARLOS EMILIO DE SANTIAGO TOVAR
// * VICTOR MANUEL SANDOVAL MACIEL
// */
//
///*!
// * @brief Task function prototypes.
// *
// * These tasks will be executed based on the scheduling system.
// */
//void Task_AF(); /*!< Task A Function */
//void Task_BF(); /*!< Task B Function */
//void Task_CF(); /*!< Task C Function */
//void VUART_RX(uint8_t PWM);
//
//
///*!
// * @brief Main function.
// *
// * This function initializes the OS, configures the LED system, and creates tasks.
// * It then starts the scheduler and enters an infinite loop.
// */
//
//uint8_t Arr_PWM_DC[3] = {20, 50, 80};	/* Duty Cycle */
//volatile bool receiving = false;
//volatile uint8_t bit_count = 0;
//volatile uint8_t byte = 0;
//volatile GPIO_Type *vgpio = NULL;
//volatile uint8_t vpin = 0;
//volatile uint8_t PWM = 0;
//
//
//int main(void) {
//
//	InitPins_vUART_Rx();
//
//    RGB_ticks_Init(); /*!< Initialize the RGB LED control system */
//    Init_FTM0_Pins(); /*!< Initialize the PWM_FTM control system */
//
//
//
//    Create_task(0, 4, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_AF);	/* 	PWM1	*/
////    Create_task(1, 3, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_BF);	/* 	PWM2	*/
////    Create_task(2, 2, FULL_PREEMPTIVE, AUTOSTART_TRUE, Task_CF);	/*	PWM3	*/
//
//    SetRelAlarm(0, 100, 0, true);	/* Task_0 / PWM_0 */
////    SetRelAlarm(0, 23, 1, true);	/* Task_1 / PWM_0 */
////    SetRelAlarm(0, 33, 2, true);	/* Task_2 / PWM_0 */
//
//    OS_init(); /*!< Start the OS scheduler */
//
//    /* Infinite loop to keep the main function running */
//    while(1) {
//
//    }
//
//}
//
//
//void Task_AF() {
//
//	if(PWM==0){
//		Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[0]);
//	}
//	if(PWM==1){
//		Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
//	}
//	if(PWM==2){
//		Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
//	}
//	Terminate_task();
//}
////void Task_BF() {
////	Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
////	Arr_PWM_DC[1]=Arr_PWM_DC[1]+10;
////		if(Arr_PWM_DC[1] == 90){ Arr_PWM_DC[1]=0; }
////	Terminate_task();
////}
////void Task_CF() {
////	Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
////	Arr_PWM_DC[2]=Arr_PWM_DC[2]+10;
////	if(Arr_PWM_DC[2] == 90){ Arr_PWM_DC[2]=0; }
////	Terminate_task();
////}
//
//void PORTD_IRQHandler(void) {
//    PWM=5;
//	if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD1 )) {
//        PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD1 ));
//        PWM=0;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD12)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD12));
//		PWM=1;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << PTD13)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << PTD13));
//		PWM=2;
//    }
//     VUART_RX(PWM);
//}
//
////void PIT1_IRQHandler(void) {
////    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
////
////    if (receiving) {
////    	uint8_t bit = GPIO_PinRead((GPIO_Type *)vgpio, vpin);
////        byte |= (bit << bit_count);
////        bit_count++;
////
////        if (bit_count == 1) {
////            // Después del primer bit, configurar el timer para 1 bit (~417 us)
////            PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk)));
////        }
////
////        if (bit_count >= 8) {
////            receiving = false;
////            PIT_StopTimer(PIT, kPIT_Chnl_1);
////            Arr_PWM_DC[pwm] = byte;
////
////            if (vgpio == GPIOC)
////                PORTC->PCR[vpin] = (PORTC->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////            else if (vgpio == GPIOD)
////                PORTD->PCR[vpin] = (PORTD->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////            else if (vgpio == GPIOE)
////                PORTE->PCR[vpin] = (PORTE->PCR[vpin] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(10);
////        }
////    }
////}
//
//void VUART_RX(uint8_t PWM) {
//	uint8_t DutyCycle_Bin= 0b00000000;
//	SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//
//	if(PWM==0){
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button1_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==1){ Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button2_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//
//
//	if(PWM==2){
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b10000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b01000000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00100000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00010000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00001000;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000100;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000010;
//		}
//		SDK_DelayAtLeastUs(416U, CLOCK_GetFreq(kCLOCK_BusClk));
//		if (Button3_Read() == 1) {
//			DutyCycle_Bin|= 0b00000001;
//		}
//	}
//	Arr_PWM_DC[PWM]=(uint8_t)DutyCycle_Bin;
//	Activate_task(0);
//
//}

//
//#include "fsl_smc.h"
//#include "fsl_clock.h"
//#include "board.h"
//#include "fsl_pit.h"
//#include "peripherals.h"
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "fsl_debug_console.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include "OS/OS.h"
//#include "DRIVERS/RGB_ticks.h"
//#include "DRIVERS/PWM_FTM.h"
//
//
//void Task_AF(); /*!< Task A Function */
//void Task_BF(); /*!< Task B Function */
//void Task_CF(); /*!< Task C Function */
//void VUART_RX(uint8_t channel);
//
//
///*!
// * @brief Main function.
// *
// * This function initializes the OS, configures the LED system, and creates tasks.
// * It then starts the scheduler and enters an infinite loop.
// */
//
//uint8_t Arr_PWM_DC[3] = {10, 10, 10};	/* Duty Cycle */
//volatile bool receiving = false;
//volatile uint8_t bit_count = 0;
//volatile uint8_t byte = 0;
//volatile GPIO_Type *vgpio = NULL;
//volatile uint8_t vpin = 0;
//volatile uint8_t PWM = 0;
//
//
//int main(void) {
//
//
//
//    RGB_ticks_Init(); /*!< Initialize the RGB LED control system */
//    Init_FTM0_Pins(); /*!< Initialize the PWM_FTM control system */
//
//
//
//    Create_task(0, 1, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_AF);	/* 	PWM1	*/
//    Create_task(1, 2, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_BF);	/* 	PWM2	*/
//    Create_task(2, 3, FULL_PREEMPTIVE, AUTOSTART_FALSE, Task_CF);	/*	PWM3	*/
//
//    SetRelAlarm(0, 10, 0, true);	/* Task_0 / PWM_0 */
//    SetRelAlarm(1, 23, 1, true);	/* Task_1 / PWM_0 */
//    SetRelAlarm(2, 33, 2, true);	/* Task_2 / PWM_0 */
//
//    OS_init(); /*!< Start the OS scheduler */
//
//    /* Infinite loop to keep the main function running */
//    while(1) {
//
//    }
//
//}
//
//
//void Task_AF() {
//
//	Dutycycle_FTM0(kFTM_Chnl_0, Arr_PWM_DC[0]);
//	if(Arr_PWM_DC[0] == 90){ Arr_PWM_DC[0]=0; }
//	Terminate_task();
//}
//
//
//void Task_BF() {
//
//	Dutycycle_FTM0(kFTM_Chnl_1, Arr_PWM_DC[1]);
//		if(Arr_PWM_DC[1] == 90){ Arr_PWM_DC[1]=0; }
//	Terminate_task();
//}
//void Task_CF() {
//
//	Dutycycle_FTM0(kFTM_Chnl_2, Arr_PWM_DC[2]);
//	if(Arr_PWM_DC[2] == 90){ Arr_PWM_DC[2]=0; }
//	Terminate_task();
//}
//
//
//void PORTD_IRQHandler(void) {
//    PWM=5;
//	if (PORT_GetPinsInterruptFlags(PORTD) & (1 << 1U )) {
//        PORT_ClearPinsInterruptFlags(PORTD, (1 << 1U ));
//        PWM=0;
//        vpin = 1U;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << 12U)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << 12U));
//		PWM=1;
//		vpin = 12U;
//    }
//    if (PORT_GetPinsInterruptFlags(PORTD) & (1 << 13U)) {
//		PORT_ClearPinsInterruptFlags(PORTD, (1 << 13U));
//		PWM=2;
//		vpin = 13U;
//    }
//     VUART_RX(PWM);
//}
//
//void PIT1_IRQHandler(void) {
//    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
//
//    if (receiving) {
//    	volatile uint8_t bit = GPIO_PinRead(GPIOD, vpin);
//        byte |= (bit << bit_count);
//        bit_count++;
//
//        if (bit_count == 1) {
//            // Después del primer bit, configurar el timer para 1 bit (~417 us)
//            PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk))); //2400 bps 417us
//        }
//
//        if (bit_count >= 8) {
//            receiving = false;
//            PIT_StopTimer(PIT, kPIT_Chnl_1);
//            NVIC_ClearPendingIRQ(PIT1_IRQn);
//            if(byte <= 100){
//                Arr_PWM_DC[PWM] = byte;
//            }
////            PORT_ClearPinsInterruptFlags(PORTD, (1U << vpin));
////            NVIC_ClearPendingIRQ(PORTD_IRQn);
////            NVIC_EnableIRQ(PORTD_IRQn);
//            // Limpia bandera GPIO y NVIC
//            //PORT_ClearPinsInterruptFlags(PORTD, (1U<<vpin));
//
//            // Rearma la detección de flanco de bajada
////            PORTD->PCR[vpin] = (PORTD->PCR[vpin] & ~PORT_PCR_IRQC_MASK)
////                             | PORT_PCR_IRQC(0xA);  // Falling edge
////        }
//        }
//    }
//}
//
//void VUART_RX(uint8_t channel) {
//
//	//PORTD->PCR[vpin] &= ~PORT_PCR_IRQC_MASK;  // quita toda detección de interrupción
//    //vpin = PIN;
//    //pwm = PWM;
//	PWM = channel;
//    bit_count = 0;
//    byte = 0;
//    receiving = true;
//
//    PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(626U, CLOCK_GetFreq(kCLOCK_BusClk))); //2400 bauds 626u;
//    PIT_StartTimer(PIT, kPIT_Chnl_1);
//}
//
//
