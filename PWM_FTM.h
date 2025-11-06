#ifndef DRIVERS_PWM_FTM_H_
#define DRIVERS_PWM_FTM_H_

#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_ftm.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "clock_config.h"

#define PWM_Ch0 kFTM_Chnl_0
#define PWM_Ch1 kFTM_Chnl_1
#define PWM_Ch2 kFTM_Chnl_2
#define PWM_Ch3 kFTM_Chnl_3
#define PWM_Ch4 kFTM_Chnl_4


void Init_FTM0_Pins(void);
void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle);

#endif /* DRIVERS_PWM_FTM_H_ */






//#ifndef DRIVERS_PWM_FTM_H_
//#define DRIVERS_PWM_FTM_H_
////
//#include "board.h"
//#include "fsl_debug_console.h"
//#include "fsl_ftm.h"
//#include "fsl_gpio.h"
//#include "fsl_port.h"
//#include "pin_mux.h"
//#include "clock_config.h"
//#include "MK66F18.h"
//
//#define PWM_Ch0 kFTM_Chnl_0
//#define PWM_Ch1 kFTM_Chnl_1
//#define PWM_Ch2 kFTM_Chnl_2
//#define PWM_Ch3 kFTM_Chnl_3
//#define PWM_Ch4 kFTM_Chnl_4
//
//#define PTD1      1
//#define PTD12     12
//#define PTD13     13
//#define TX_PORT    GPIOD
//
//extern uint8_t Button1_Read(void);
//extern uint8_t Button2_Read(void);
//extern uint8_t Button3_Read(void);
//
//
//void Init_FTM0_Pins(void);
//void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle);
//void InitPins_vUART_Rx(void);
//
//#endif /* DRIVERS_PWM_FTM_H_ */
