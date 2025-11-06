#ifndef OS_OS_H_  /*!< Prevents multiple inclusions of this header file */
#define OS_OS_H_

#include <stdint.h>  /*!< Standard integer types (uint8_t, uint32_t, etc.) */

/*!
 * @brief Defines the maximum number of tasks that the OS can handle.
 */
#define TASKS 3 //5

#define TICKS 5 //

#define ALARMS 5


/*!
 * @brief Represents an invalid or no-active task.
 * This value is used when no task is running.
 */
#define NO_TASK 10

extern volatile uint8_t flag_interrupt;

extern uint8_t interrupt_count; /*!< 📌 Contador global de interrupciones */

extern uint32_t alarm_count; /*!< 📌 Contador global de interrupciones */

#define MAX_INTERRUPTS 10

typedef struct {
	uint32_t SP;
	uint32_t xPSR;
	uint32_t PC;
	uint32_t LR;
	uint32_t R12;
	uint32_t R3;
	uint32_t R2;
	uint32_t R1;
	uint32_t R0;
    bool ISR_ACTIVATED;   /*!< 📌 Bandera de interrupción activa */
} Context;



/*!
 * @brief Stores the index of the currently running task in the task list.
 */
extern uint8_t TASK_ACTIVE;

/*!
 * @brief General-purpose pointer for context switching.
 */
extern uint32_t ptr;

/*!
 * @brief Enumeration for task autostart behavior.
 *
 * AUTOSTART_FALSE: Task does not start automatically.
 * AUTOSTART_TRUE: Task starts automatically when the OS initializes.
 */
typedef enum {
    AUTOSTART_FALSE,  /*!< Task will not start automatically */
    AUTOSTART_TRUE    /*!< Task will start automatically */
} Autostart;

/*!
 * @brief Enumeration for task scheduling policy.
 *
 * NON_PREEMPTIVE: Task runs to completion before switching.
 * FULL_PREEMPTIVE: Task can be interrupted by a higher-priority task.
 */
typedef enum {
    NON_PREEMPTIVE,  /*!< Task runs to completion */
    FULL_PREEMPTIVE  /*!< Task can be preempted by higher-priority tasks */
} SchedulingPolicy;

/*!
 * @brief Enumeration for task states.
 *
 * TASK_SUSPENDED: Task is not running or ready.
 * TASK_READY: Task is ready to run but not currently executing.
 * TASK_RUNNING: Task is currently executing.
 */
typedef enum {
    TASK_SUSPENDED, /*!< Task is not active */
    TASK_READY,     /*!< Task is ready to execute */
    TASK_RUNNING,    /*!< Task is currently running */
	TASK_BLOCKED
} Task_State;

/*!
 * @brief Enumeration for task activation status.
 *
 * TASK_DEACTIVATED: Task is inactive.
 * TASK_READY_TO_ACTIVATE: Task is prepared for activation.
 * TASK_ACTIVATED: Task is currently active.
 */
typedef uint8_t Task_Limit;
enum {
    TASK_DEACTIVATED,       /*!< Task is inactive */
    TASK_READY_TO_ACTIVATE, /*!< Task is ready to be activated */
    TASK_ACTIVATED          /*!< Task is currently active */
};

/*!
 * @brief Enumeration for task creation status.
 *
 * TASK_NCREATED: Task has not been created.
 * TASK_CREATED: Task has been successfully created.
 */
typedef enum {
    TASK_NCREATED, /*!< Task has not been created */
    TASK_CREATED   /*!< Task has been successfully created */
} Task_Created;

typedef enum{
	TASK_NINTERRUPTED,
	TASK_INTERRUPTED
}Task_ISR;



/*!
 * @brief Function pointer type for task execution.
 *
 * This allows tasks to be dynamically assigned and executed.
 */
typedef void (*TaskFunction)(void);

/*!
 * @brief Structure representing a task.
 *
 * This structure stores all necessary attributes of a task, including:
 * - Task ID
 * - Priority
 * - Scheduling policy
 * - Activation status
 * - Autostart behavior
 * - Function pointer to task execution
 * - Current state
 * - Task creation status
 * - Pointer for context switching
 */
typedef struct {
    uint8_t task_ID;            /*!< Unique task identifier */
    uint8_t priority;           /*!< Task priority (higher value = higher priority) */
    SchedulingPolicy Schedule;  /*!< Scheduling policy (preemptive or non-preemptive) */
    Task_Limit activation;      /*!< Activation status (activated, ready, or deactivated) */
    Autostart autostart;        /*!< Determines whether the task starts automatically */
    TaskFunction task_ptr;      /*!< Pointer to the function that executes the task */
    Task_State state;           /*!< Current task state (suspended, ready, or running) */
    Task_Created Task_Create;   /*!< Indicates if the task has been created */
    void *ptr_context;          /*!< Pointer for storing context during task switching */
    Context context;
    Task_ISR interrupt;
    uint8_t Permission;
} Task;


typedef enum {
    ALARM_OFF, 	/*!< Task has not been created */
	ALARM_ON   /*!< Task has been successfully created */
} Alarm_Set;

typedef struct {
	uint8_t Alarm_ID;            	/*!< Unique task identifier */
	uint32_t Ticks;  		/*!< Scheduling policy (preemptive or non-preemptive) */
	uint32_t Act_Ticks;  		/*!< Scheduling policy (preemptive or non-preemptive) */
	uint8_t Task_ID;        /*!< Determines whether the task starts automatically */
	bool 	Repeat;			/*!< Determines whether the task starts automatically */
	Alarm_Set Alarm_ONOFF;   /*!< Indicates if the task has been created */
} Alarm;

/*!
 * @brief Task list array.
 * Stores all created tasks and their attributes.
 */
extern Task task_list[TASKS];

extern Alarm alarm_list[ALARMS];

/*!
 * @brief Initializes the OS and starts task scheduling.
 */
void OS_init(void);

/*!
 * @brief Activates a task by changing its state to READY.
 *
 * @param task_ID ID of the task to activate.
 */
void Activate_task(uint8_t task_ID);

/*!
 * @brief Terminates the currently running task.
 */
void Terminate_task(void);

/*!
 * @brief Replaces the currently running task with a new one.
 *
 * This function suspends the current task and immediately starts another task.
 *
 * @param task_ID ID of the task to activate.
 */
void Chain_task(uint8_t task_ID);

/*!
 * @brief Simple task scheduler that selects the highest-priority task to run.
 */
void Scheduler(void);

/*!
 * @brief Creates a new task and adds it to the task list.
 *
 * @param Task_ID      Unique identifier for the task.
 * @param Priority     Task priority (higher value = higher priority).
 * @param Schedule     Scheduling policy (preemptive or non-preemptive).
 * @param Autostart    Determines whether the task starts automatically.
 * @param Ptr_Task     Pointer to the function that executes the task.
 */
void Create_task(uint8_t Task_ID, uint8_t Priority, SchedulingPolicy Schedule, Autostart Autostart, TaskFunction Ptr_Task);

/*!
 * @brief Puts the processor in sleep mode to reduce power consumption.
 */
void SleepMode(void);

void Save_context(void);

void Restore_context(void);

void Save_context_ALR(void);

void Restore_context_ALR(void);

void Interrupts_Enable(void);

void Interrupts_Disable(void);

void SetRelAlarm(uint8_t Alarm_ID, uint32_t Ticks, uint8_t Task_ID,bool Repeat);

void CancelAlarm(uint8_t Alarm_ID);

void PIT_INIT(void);

#endif /* OS_OS_H_ */
