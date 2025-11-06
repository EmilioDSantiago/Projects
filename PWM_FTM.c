#include "PWM_FTM.h"


//#define BOARD_FTM_BASEADDR FTM0

uint8_t DutyCycle=0;

void Init_FTM0_Pins(void);
void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle);

//int main(void)
//{
//    /* Inicialización básica de la placa, pines y relojes */
//	Init_FTM0_Pins();
//	while(1){}
//	/*	kFTM_Chnl_0
//		kFTM_Chnl_1
//		kFTM_Chnl_2
//		kFTM_Chnl_3
//		kFTM_Chnl_4	 */
//	Dutycycle_FTM0(kFTM_Chnl_0, 10);	/* Con esta funcion escribes el canal y seguido el nuevo PWM's */
//    while (1){ }
//}

void Init_FTM0_Pins(void)
{
    ftm_config_t ftmInfo;
    /* Obtén la configuración por defecto */
    FTM_GetDefaultConfig(&ftmInfo);
    /* Ajusta el prescaler(divide por 1) */
    ftmInfo.prescale = kFTM_Prescale_Divide_1;
    /* Inicializa módulo FTM_0 con la configuración */
    FTM_Init(FTM0, &ftmInfo);

    /* Configuración de los 3 canales PWM */
    ftm_chnl_pwm_signal_param_t pwmParams[3] = {
        {kFTM_Chnl_0, kFTM_HighTrue, 0, 0, false, false},  /* 50% duty cycle */
        {kFTM_Chnl_1, kFTM_HighTrue, 0, 0, false, false},  /* 30% duty cycle */
        {kFTM_Chnl_2, kFTM_HighTrue, 0, 0, false, false},  /* 70% duty cycle */
//        {kFTM_Chnl_3, kFTM_HighTrue, 20, 0, false, false},  /* 20% duty cycle */
//        {kFTM_Chnl_4, kFTM_HighTrue, 90, 0, false, false}   /* 90% duty cycle */
    };

    /* Frecuencia PWM y la fuente del reloj del FTM */
    uint32_t pwmFreq_Hz = 20000;    /* Frecuencia PWM: 20 kHz */
    /* Frecuencia del reloj del bus (60 MHz) */
    uint32_t srcClock_Hz = CLOCK_GetFreq(kCLOCK_BusClk);

    /* Configura los canales PWM en modo edge-aligned */
    FTM_SetupPwm(FTM0, pwmParams, 3, kFTM_EdgeAlignedPwm, pwmFreq_Hz, srcClock_Hz);

    /* Inicia el contador del FTM0 seleccionando la fuente de reloj.
       Aquí se configura el SC para arrancar el contador con el reloj del sistema */
    FTM0->SC &= ~FTM_SC_CLKS_MASK;
    FTM0->SC |= FTM_SC_CLKS(1); // 1: System clock

	/* Habilita los relojes para los puertos */
	CLOCK_EnableClock(kCLOCK_PortA);
	CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortD);

    /* PTC1 para FTM0_CH0 en ALT4 */
    PORT_SetPinMux(PORTC, 1U, kPORT_MuxAlt4);
    /* PTC2 para FTM0_CH1 en ALT4 */
    PORT_SetPinMux(PORTC, 2U, kPORT_MuxAlt4);
    /* PTC3 para FTM0_CH2 en ALT4 */
    PORT_SetPinMux(PORTC, 3U, kPORT_MuxAlt4);
//    /* PTA6 para FTM0_CH3 en ALT3 */
//    PORT_SetPinMux(PORTA, 6U, kPORT_MuxAlt3);
//    /* PTA7 para FTM0_CH4 en ALT3 */
//    PORT_SetPinMux(PORTA, 7U, kPORT_MuxAlt3);
}

void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle){
    /* Disable interrupt to retain current dutycycle for a few seconds */
//    FTM_DisableInterrupts(BOARD_FTM_BASEADDR, FTM_CHANNEL_INTERRUPT_ENABLE);

    /* Disable channel output before updating the dutycycle */
    FTM_UpdateChnlEdgeLevelSelect(FTM0, Channel, 0U);

    FTM_UpdatePwmDutycycle(FTM0, Channel, kFTM_CenterAlignedPwm, DutyCycle);

    /* Software trigger to update registers */
    FTM_SetSoftwareTrigger(FTM0, true);

    /* Start channel output with updated dutycycle */
    FTM_UpdateChnlEdgeLevelSelect(FTM0, Channel, kFTM_HighTrue);
}










//#include "PWM_FTM.h"
//
//
////#define BOARD_FTM_BASEADDR FTM0
//
//uint8_t DutyCycle=0;
//
//void Init_FTM0_Pins(void);
//void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle);
//
////int main(void)
////{
////    /* Inicialización básica de la placa, pines y relojes */
////	Init_FTM0_Pins();
////	while(1){}
////	/*	kFTM_Chnl_0
////		kFTM_Chnl_1
////		kFTM_Chnl_2
////		kFTM_Chnl_3
////		kFTM_Chnl_4	 */
////	Dutycycle_FTM0(kFTM_Chnl_0, 10);	/* Con esta funcion escribes el canal y seguido el nuevo PWM's */
////    while (1){ }
////}
//
//extern uint8_t Button1_Read(void) {
//    return ( (GPIOD->PDIR & (1U << PTD1)) ? 1 : 0 );
//}
//extern uint8_t Button2_Read(void) {
//    return ( (GPIOD->PDIR & (1U << PTD12)) ? 1 : 0 );
//}
//extern uint8_t Button3_Read(void) {
//    return ( (GPIOD->PDIR & (1U << PTD13)) ? 1 : 0 );
//}
//
//void Init_FTM0_Pins(void)
//{
//    ftm_config_t ftmInfo;
//    /* Obtén la configuración por defecto */
//    FTM_GetDefaultConfig(&ftmInfo);
//    /* Ajusta el prescaler(divide por 1) */
//    ftmInfo.prescale = kFTM_Prescale_Divide_1;
//    /* Inicializa módulo FTM_0 con la configuración */
//    FTM_Init(FTM0, &ftmInfo);
//
//    /* Configuración de los 3 canales PWM */
//    ftm_chnl_pwm_signal_param_t pwmParams[3] = {
//        {kFTM_Chnl_0, kFTM_HighTrue, 0, 0, false, false},  /* 50% duty cycle */
//        {kFTM_Chnl_1, kFTM_HighTrue, 0, 0, false, false},  /* 30% duty cycle */
//        {kFTM_Chnl_2, kFTM_HighTrue, 0, 0, false, false},  /* 70% duty cycle */
////        {kFTM_Chnl_3, kFTM_HighTrue, 20, 0, false, false},  /* 20% duty cycle */
////        {kFTM_Chnl_4, kFTM_HighTrue, 90, 0, false, false}   /* 90% duty cycle */
//    };
//
//    /* Frecuencia PWM y la fuente del reloj del FTM */
//    uint32_t pwmFreq_Hz = 100;    /* Frecuencia PWM: 20 kHz */
//    /* Frecuencia del reloj del bus (60 MHz) */
//    uint32_t srcClock_Hz = CLOCK_GetFreq(kCLOCK_BusClk);
//
//    /* Configura los canales PWM en modo edge-aligned */
//    FTM_SetupPwm(FTM0, pwmParams, 3, kFTM_EdgeAlignedPwm, pwmFreq_Hz, srcClock_Hz);
//
//    /* Inicia el contador del FTM0 seleccionando la fuente de reloj.
//       Aquí se configura el SC para arrancar el contador con el reloj del sistema */
//    FTM0->SC &= ~FTM_SC_CLKS_MASK;
//    FTM0->SC |= FTM_SC_CLKS(1); // 1: System clock
//
//	/* Habilita los relojes para los puertos */
//	CLOCK_EnableClock(kCLOCK_PortA);
//	CLOCK_EnableClock(kCLOCK_PortB);
//    CLOCK_EnableClock(kCLOCK_PortC);
//    CLOCK_EnableClock(kCLOCK_PortD);
//
//    /* PTC1 para FTM0_CH0 en ALT4 */
//    PORT_SetPinMux(PORTC, 1U, kPORT_MuxAlt4);
//    /* PTC2 para FTM0_CH1 en ALT4 */
//    PORT_SetPinMux(PORTC, 2U, kPORT_MuxAlt4);
//    /* PTC3 para FTM0_CH2 en ALT4 */
//    PORT_SetPinMux(PORTC, 3U, kPORT_MuxAlt4);
////    /* PTA6 para FTM0_CH3 en ALT3 */
////    PORT_SetPinMux(PORTA, 6U, kPORT_MuxAlt3);
////    /* PTA7 para FTM0_CH4 en ALT3 */
////    PORT_SetPinMux(PORTA, 7U, kPORT_MuxAlt3);
//}
//void InitPins_vUART_Rx(void) {
//    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
//
//    PORTD->PCR[PTD12] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//    PORTD->PCR[PTD13] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//    PORTD->PCR[PTD1 ] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
//
//    GPIOD->PDDR &= ~((1U << PTD1) | (1U << PTD12) | (1U << PTD13));
//}
//void Dutycycle_FTM0( port_mux_t Channel, uint8_t DutyCycle){
//    /* Disable interrupt to retain current dutycycle for a few seconds */
////    FTM_DisableInterrupts(BOARD_FTM_BASEADDR, FTM_CHANNEL_INTERRUPT_ENABLE);
//
//    /* Disable channel output before updating the dutycycle */
//    FTM_UpdateChnlEdgeLevelSelect(FTM0, Channel, 0U);
//
//    FTM_UpdatePwmDutycycle(FTM0, Channel, kFTM_CenterAlignedPwm, DutyCycle);
//
//    /* Software trigger to update registers */
//    FTM_SetSoftwareTrigger(FTM0, true);
//
//    /* Start channel output with updated dutycycle */
//    FTM_UpdateChnlEdgeLevelSelect(FTM0, Channel, kFTM_HighTrue);
//}
