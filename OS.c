#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fsl_common.h"
#include "OS.h"
#include "fsl_port.h"
#include "DRIVERS/RGB_ticks.h"
#include "fsl_pit.h"
#include "MK66F18.h"


volatile uint8_t flag_interrupt = 0;
int context_index = -1; /*!< 馃搶 脥ndice de la pila (-1 significa vac铆a) */
Context context_stack[MAX_INTERRUPTS]; /*!< 馃搶 Definici贸n de la pila de contextos */
uint8_t interrupt_count = 0; /*!< 馃搶 Inicializar el contador de interrupciones en 0 */
static uint32_t *ptr_isr;
uint8_t num_alarms = 0;
uint32_t alarm_count = 0;
volatile bool scheduler_needed = false;

/*!
 * @brief Task list array.
 * Stores all created tasks and their attributes.
 */
Task task_list[TASKS];

Alarm alarm_list[ALARMS];

/*!
 * @brief Stores the ID of the currently active task.
 * If no task is running, it holds NO_TASK.
 */
uint8_t TASK_ACTIVE = NO_TASK;

/*!
 * @brief General-purpose pointer for context switching.
 */
uint32_t ptr;

/*!
 * @brief Creates a task and adds it to the task list.
 *
 * @param Task_ID      Unique identifier for the task.
 * @param Priority     The task priority (higher number = higher priority).
 * @param Schedule     Scheduling policy (e.g., FULL_PREEMPTIVE).
 * @param Autostart    Determines whether the task starts automatically.
 * @param Ptr_Task     Pointer to the function that defines the task behavior.
 */
void Create_task(uint8_t Task_ID, uint8_t Priority, SchedulingPolicy Schedule, Autostart Autostart, TaskFunction Ptr_Task) {
    for (int i = 0; i < TASKS; i++) {  /*!< Iterate through the task list */
        if (!task_list[i].Task_Create) {  /*!< Find an empty slot */
            task_list[i].Task_Create = TASK_CREATED; /*!< Mark the task as created */
            task_list[i].task_ID = Task_ID; /*!< Assign the task ID */
            task_list[i].priority = Priority; /*!< Set the task priority */
            task_list[i].Schedule = Schedule; /*!< Define the scheduling policy */
            task_list[i].activation = TASK_READY_TO_ACTIVATE; /*!< Initialize task activation state */
            task_list[i].autostart = Autostart; /*!< Set autostart flag */
            task_list[i].task_ptr = Ptr_Task; /*!< Store function pointer */
            task_list[i].state = TASK_SUSPENDED; /*!< Initialize as suspended */
            task_list[i].ptr_context = NULL; /*!< No initial execution context */
            task_list[i].interrupt = TASK_NINTERRUPTED;
            return; /*!< Exit function after assigning the task */
        }
    }
}


void SetRelAlarm(uint8_t Alarm_ID, uint32_t Ticks, uint8_t Task_ID,bool Repeat){

	alarm_list[Alarm_ID].Task_ID = Task_ID;
    alarm_list[Alarm_ID].Ticks = Ticks;
    alarm_list[Alarm_ID].Act_Ticks = Ticks;
    alarm_list[Alarm_ID].Repeat = Repeat; //ciclica
    alarm_list[Alarm_ID].Alarm_ONOFF = ALARM_ON;
    num_alarms++;

}
void CancelAlarm(uint8_t Alarm_ID) {
    alarm_list[Alarm_ID].Alarm_ONOFF = ALARM_OFF;
    alarm_list[Alarm_ID].Ticks = 0;
    num_alarms--;
}


/*!
 * @brief Initializes the OS by setting up tasks and starting the scheduler.
 */
void OS_init()
{

	for(uint8_t i = 0;i < TASKS;i++){/*!< Loop through all tasks */
		if(task_list[i].autostart == AUTOSTART_TRUE){ /*!< If autostart is enabled */
			task_list[i].state=TASK_READY;/*!< Set task state to READY */
		}
		else{
			task_list[i].state=TASK_SUSPENDED;  /*!< Otherwise, suspend it */
		}
	}



	PIT_INIT();
	//Interrupts_Enable();
	Scheduler();/*!< Start the task scheduler */

	asm("ADD sp, sp, #0x10");/*!< Adjust stack pointer */
}

/*!
 * @brief Activates a specific task.
 *
 * @param task_ID Task ID to be activated.
 */
void Activate_task(uint8_t task_ID) {

	Interrupts_Disable();
	//si no hubo interrupcion
	if(interrupt_count == 0 && alarm_count == 0){//cambiar magic number
	    asm("MOV R0, R14");/*!< Move return address (LR) to R0 */
	    asm("SUB R0, R0, #1"); /*!< Adjust return address */
	    asm("LDR R1, =ptr");/*!< Load address of ptr */
	    asm("STR R0,[R1]");/*!< Store the adjusted return address in ptr */

	    task_list[TASK_ACTIVE].ptr_context=(void *)ptr;/*!< Save context */
	}


	if((task_list[task_ID].activation == TASK_READY_TO_ACTIVATE) && (task_list[task_ID].state == TASK_SUSPENDED)){
		task_list[task_ID].state = TASK_READY;/*!< Set task to READY */
		task_list[task_ID].activation = TASK_ACTIVATED;/*!< Mark as activated */
	}
	else{
		task_list[task_ID].state = TASK_SUSPENDED;/*!< Otherwise, keep it suspended */
		task_list[task_ID].activation = TASK_READY_TO_ACTIVATE;/*!< Reset activation flag */
	}

	asm("ADD sp,sp,#0x10");/*!< Adjust stack pointer */

    Scheduler();/*!< Call the scheduler to check for the highest-priority task */


}

/*!
 * @brief Terminates the currently running task.
 */
void Terminate_task(){

	Interrupts_Disable();
	task_list[TASK_ACTIVE].state = TASK_SUSPENDED;/*!< Suspend the active task */
	task_list[TASK_ACTIVE].activation = TASK_READY_TO_ACTIVATE;/*!< Mark as ready to activate again */
	task_list[TASK_ACTIVE].ptr_context = NULL;/*!< Clear execution context */
	TASK_ACTIVE = NO_TASK;/*!< Reset active task */

	Led_Off();/*!< Turn off LED to indicate task termination */

    asm("ADD sp, sp, #0x08");/*!< Adjust stack pointer */

    Scheduler(); /*!< Invoke scheduler to switch to another task */

}

/*!
 * @brief Terminates the current task and immediately activates another one.
 *
 * @param task_ID Task ID of the next task to activate.
 */
void Chain_task(uint8_t task_ID){

    if(task_list[TASK_ACTIVE].state == TASK_RUNNING && TASK_ACTIVE < TASKS){
    	task_list[TASK_ACTIVE].state = TASK_SUSPENDED;/*!< Suspend the current task */
    	task_list[TASK_ACTIVE].activation = TASK_READY_TO_ACTIVATE;/*!< Mark as ready for next activation */
    	task_list[TASK_ACTIVE].ptr_context = NULL;/*!< Clear execution context */
    	TASK_ACTIVE = NO_TASK;/*!< Reset active task */
    	Led_Off();/*!< Indicate task termination */
    }

	if((task_list[task_ID].activation == TASK_READY_TO_ACTIVATE) && (task_list[task_ID].state == TASK_SUSPENDED)){
		task_list[task_ID].state = TASK_READY; /*!< Set new task to READY */
		task_list[task_ID].activation = TASK_ACTIVATED; /*!< Mark as activated */
	}
	else{
		task_list[task_ID].state = TASK_SUSPENDED;/*!< Otherwise, keep it suspended */
		task_list[task_ID].activation = TASK_READY_TO_ACTIVATE;/*!< Reset activation flag */
	}

    asm("ADD sp, sp, #0x10"); /*!< Adjust stack pointer */

    Scheduler();/*!< Switch execution to the new task */

}



/*!
 * @brief Simple preemptive scheduler.
 */
void Scheduler() {

	//desactivar isr
	Interrupts_Disable();

    asm("ADD sp, sp, #0x08");/*!< Adjust stack pointer */

    uint8_t highestPriorityTask = TASKS;
    uint8_t Highest_priority = 0;




    /*! Find the highest-priority task that is READY */
    for (uint8_t i = 0; i < TASKS; i++) {
    	if ((task_list[i].state == TASK_READY && task_list[i].priority > Highest_priority)){
            Highest_priority = task_list[i].priority;
            highestPriorityTask = i;
        }
    }

    /*! If no tasks are ready, put the processor to sleep */
    if (highestPriorityTask == TASKS) {
        //asm("ADD sp, sp, #0x10");  /*!< Adjust stack pointer */
        while(!scheduler_needed){
        	 Interrupts_Enable();  // 馃敼 Asegurar que las interrupciones est谩n activadas
        	 SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        }
        scheduler_needed = false;

        asm("add sp, sp, 0x8");
        Scheduler();
    }

    /*! If the currently active task has equal or higher priority, no need to switch */
    if (TASK_ACTIVE != NO_TASK && task_list[TASK_ACTIVE].priority >= task_list[highestPriorityTask].priority) {
        return;
    }

    /*! Move the currently running task back to READY state */
    if (TASK_ACTIVE != NO_TASK) {
        task_list[TASK_ACTIVE].state = TASK_READY;

    }

    /*! Switch to the highest-priority task */
    task_list[highestPriorityTask].state = TASK_RUNNING;
    TASK_ACTIVE = highestPriorityTask;

    /*! Restore context if available sin interrupcion o sin alarma*/
    if(task_list[TASK_ACTIVE].ptr_context != NULL && interrupt_count == 0){
    	ptr = (int)task_list[TASK_ACTIVE].ptr_context;
        asm("ADD sp, sp, #0x10"); /*!< Adjust stack pointer */
    	asm("LDR R1, =ptr");
    	asm("LDR R0, [R1]");
    	asm("MOV PC, R0");
    }

    if (task_list[TASK_ACTIVE].context.ISR_ACTIVATED != false && TASK_ACTIVE != TASKS) {

    	Restore_context();
       }

    if(task_list[TASK_ACTIVE].interrupt == TASK_INTERRUPTED){

    	Interrupts_Enable();
    	task_list[TASK_ACTIVE].task_ptr();/*!< Execute the selected task */
    	Restore_context_ALR();
    }

    asm("ADD sp, sp, #0x10");  /*!< Adjust stack pointer */



    if(TASK_ACTIVE != TASKS){
    	Interrupts_Enable();
    	task_list[TASK_ACTIVE].task_ptr();/*!< Execute the selected task */
    }

    Interrupts_Enable();
}


void Interrupts_Enable(void){
    NVIC_ClearPendingIRQ(PIT0_IRQn);  // Limpiar posibles interrupciones pendientes
    NVIC_EnableIRQ(PIT0_IRQn);        // Habilitar interrupcion
    NVIC_EnableIRQ(PORTC_IRQn);		  // PTD12 & PTD13
    NVIC_EnableIRQ(PORTD_IRQn);		  // PTD12 & PTD13
    NVIC_EnableIRQ(PORTE_IRQn);		  //PTE24
    //asm("nop");
}

void Interrupts_Disable(void){
    NVIC_ClearPendingIRQ(PIT0_IRQn);  // Limpiar interrupciones antes de deshabilitarlas
    NVIC_DisableIRQ(PIT0_IRQn);       // Deshabilitar interrupcion
    NVIC_DisableIRQ(PORTC_IRQn);		  // PTD12 & PTD13
    NVIC_DisableIRQ(PORTD_IRQn);		  // PTD12 & PTD13
    NVIC_DisableIRQ(PORTE_IRQn);		  //PTE24
    //asm("nop");
}




void PIT0_IRQHandler(void) {
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
    Interrupts_Disable();

    for (uint8_t i = 0; i < ALARMS; i++) {
        if (alarm_list[i].Alarm_ONOFF == ALARM_ON) {
            if (alarm_list[i].Act_Ticks > 0) {
                alarm_list[i].Act_Ticks--;
            }

            if (alarm_list[i].Act_Ticks == 0) {
                if (alarm_list[i].Repeat) {
                    alarm_list[i].Act_Ticks = alarm_list[i].Ticks;
                } else {
                    alarm_list[i].Alarm_ONOFF = ALARM_OFF;
                }
                if (TASK_ACTIVE != NO_TASK) {
                    Save_context_ALR();
                }

                //barrido de tareas en ready
                task_list[alarm_list[i].Task_ID].state = TASK_READY;
                //Scheduler();
                scheduler_needed = true;

            }
        }
    }
}


void Save_context(void){

	 ptr_isr = (uint32_t*) &(task_list[TASK_ACTIVE].context);
	 asm("LDR R8, =ptr_isr;"); //Obtengo la direccion del ptr
	 asm("LDR R8, [R8]");

	 asm("MOV R6, SP"); //Recupero el sp despu茅s de la isr
	 asm("ADD R6, R6, #0x2C"); //Ajusto el sp para que apunte  a lo que hab铆a antes de la isr

	 asm("STR R6, [R8], #4"); //Guardo SP y hago un post incremento

	 asm("MOV R10, #8"); //Contador corresponde al num elementos estructura context

	 asm("SUB R6, R6, #4"); //Ajuste

	 asm("loop: LDR R9, [R6], #-4");// Recupero en R9 lo que hay en R6(SP) y apunto al proximo elemento de stack
	 asm("STR R9, [R8], #4");//Guardo en la estructura apuntada por R8 y avanzo en la estructura
	 asm("SUB R10, R10, #1"); //Contador
	 asm("CMP R10, #0"); //Condicion
	 asm("BNE loop"); //Loop

	 task_list[TASK_ACTIVE].context.ISR_ACTIVATED = true;

	 interrupt_count++;

}
void Restore_context(void){

	task_list[TASK_ACTIVE].context.ISR_ACTIVATED = false;
	interrupt_count--;

	ptr_isr = (uint32_t*) &(task_list[TASK_ACTIVE].context);

	// Cargar los registros directamente desde la estructura Context
		asm("LDR R8, =ptr_isr");  // Direcci贸n del ptr
	    asm("LDR R8, [R8]");      // Ahora R8 apunta a la estructura Context
	    asm("LDR R6, [R8], #4"); // SP
	    asm("MOV SP, R6");       // Restaurar SP

	    asm("ADD R8, R8, #0x1C"); //Ajuste de offset para obtener al final el PC

	    asm("LDR R0, [R8], #-4"); // R0
	    asm("LDR R1, [R8], #-4"); // R1
	    asm("LDR R2, [R8], #-4"); // R2
	    asm("LDR R3, [R8], #-4"); // R3
	    asm("LDR R12, [R8], #-4"); // R12
	    asm("LDR LR, [R8], #-4"); // LR
	    asm("LDR R9, [R8], #-4"); // PC (guardar temporalmente en R9 para cargarlo al final)
	    //asm("LDR xPSR, [R8], #-4"); // xPSR

	    // Saltar al PC restaurado
	    asm("MOV PC, R9"); // Ir a la instrucci贸n que estaba antes de la interrupci贸n
}


void Save_context_ALR(void){

	 ptr_isr = (uint32_t*) &(task_list[TASK_ACTIVE].context);
	 asm("LDR R8, =ptr_isr;"); //Obtengo la direccion del ptr
	 asm("LDR R8, [R8]");

	 asm("MOV R6, SP"); //Recupero el sp despu茅s de la isr
	 asm("ADD R6, R6, #0x3C"); //Ajusto el sp para que apunte  a lo que hab铆a antes de la isr


	 asm("STR R6, [R8], #4"); //Guardo SP y hago un post incremento

	 asm("MOV R10, #8"); //Contador corresponde al num elementos estructura context

	 asm("SUB R6, R6, #4"); //Ajuste

	 asm("loop1: LDR R9, [R6], #-4");// Recupero en R9 lo que hay en R6(SP) y apunto al proximo elemento de stack
	 asm("STR R9, [R8], #4");//Guardo en la estructura apuntada por R8 y avanzo en la estructura
	 asm("SUB R10, R10, #1"); //Contador
	 asm("CMP R10, #0"); //Condicion
	 asm("BNE loop1"); //Loop

	 task_list[TASK_ACTIVE].interrupt = TASK_INTERRUPTED;
	 alarm_count++;


}
void Restore_context_ALR(void){

	Interrupts_Enable();
	task_list[TASK_ACTIVE].interrupt = TASK_NINTERRUPTED;
	alarm_count--;

	ptr_isr = (uint32_t*) &(task_list[TASK_ACTIVE].context);

	// Cargar los registros directamente desde la estructura Context
		asm("LDR R8, =ptr_isr");  // Direcci贸n del ptr
	    asm("LDR R8, [R8]");      // Ahora R8 apunta a la estructura Context
	    asm("LDR R6, [R8], #4"); // SP
	    asm("MOV SP, R6");       // Restaurar SP

	    asm("ADD R8, R8, #0x2C"); //Ajuste de offset para obtener al final el PC

	    asm("LDR R0, [R8], #-4"); // R0
	    asm("LDR R1, [R8], #-4"); // R1
	    asm("LDR R2, [R8], #-4"); // R2
	    asm("LDR R3, [R8], #-4"); // R3
	    asm("LDR R12, [R8], #-4"); // R12
	    asm("LDR LR, [R8], #-4"); // LR
	    asm("LDR R9, [R8], #-4"); // PC (guardar temporalmente en R9 para cargarlo al final)
	    //asm("LDR xPSR, [R8], #-4"); // xPSR

	    // Saltar al PC restaurado
	    asm("MOV PC, R9"); // Ir a la instrucci贸n que estaba antes de la interrupci贸n
}

void PIT_INIT(void)
{
	pit_config_t pitConfig;

	PIT_GetDefaultConfig(&pitConfig);
	PIT_Init(PIT, &pitConfig);

    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(100U, CLOCK_GetFreq(kCLOCK_BusClk)));
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(417U, CLOCK_GetFreq(kCLOCK_BusClk)));

	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
	PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);

	PIT_StartTimer(PIT, kPIT_Chnl_0);
	//PIT_StartTimer(PIT, kPIT_Chnl_1);

	NVIC_SetPriority(PIT0_IRQn, 0); //
	NVIC_SetPriority(PIT1_IRQn, 1); //
	NVIC_EnableIRQ(PIT1_IRQn);



}

