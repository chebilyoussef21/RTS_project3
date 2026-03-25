#include "scheduler.h"

#define schedUSE_TCB_ARRAY 1

/* Extended Task control block for managing periodic tasks within this library. */
typedef struct xExtended_TCB
{
	TaskFunction_t pvTaskCode; 		/* Function pointer to the code that will be run periodically. */
	const char *pcName; 			/* Name of the task. */
	UBaseType_t uxStackDepth; 			/* Stack size of the task. */
	void *pvParameters; 			/* Parameters to the task function. */
	UBaseType_t uxPriority; 		/* Priority of the task. */
	TaskHandle_t *pxTaskHandle;		/* Task handle for the task. */
	TickType_t xReleaseTime;		/* Release time of the task. */
	TickType_t xRelativeDeadline;	/* Relative deadline of the task. */
	TickType_t xAbsoluteDeadline;	/* Absolute deadline of the task. */
	TickType_t xPeriod;				/* Task period. */
	TickType_t xLastWakeTime; 		/* Last time stamp when the task was running. */
	TickType_t xMaxExecTime;		/* Worst-case execution time of the task. */
	TickType_t xExecTime;			/* Current execution time of the task. */

	BaseType_t xWorkIsDone; 		/* pdFALSE if the job is not finished, pdTRUE if the job is finished. */

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xPriorityIsSet; 	/* pdTRUE if the priority is assigned. */
		BaseType_t xInUse; 			/* pdFALSE if this extended TCB is empty. */
	#endif

	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		BaseType_t xExecutedOnce;	/* pdTRUE if the task has executed once. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 || schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		TickType_t xAbsoluteUnblockTime; /* The task will be unblocked at this time if it is blocked by the scheduler task. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME || schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
		BaseType_t xSuspended; 		/* pdTRUE if the task is suspended. */
		BaseType_t xMaxExecTimeExceeded; /* pdTRUE when execTime exceeds maxExecTime. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
	
	/* add if you need anything else */	
	
} SchedTCB_t;



#if( schedUSE_TCB_ARRAY == 1 )
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle );
	static void prvInitTCBArray( void );
	/* Find index for an empty entry in xTCBArray. Return -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void );
	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex );
#endif /* schedUSE_TCB_ARRAY */

static TickType_t xSystemStartTime = 0;

static void prvPeriodicTaskCode( void *pvParameters );
static void prvCreateAllTasks( void );


#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS)
	static void prvSetFixedPriorities( void );	
#endif /* schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY_DMS */

#if( schedUSE_SCHEDULER_TASK == 1 )
	static void prvSchedulerCheckTimingError( TickType_t xTickCount, SchedTCB_t *pxTCB );
	/* The forward declaration must match the FreeRTOS task entry signature. */
	static void prvSchedulerFunction( void *pvParameters );
	static void prvCreateSchedulerTask( void );
	static void prvWakeScheduler( void );

	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		static void prvPeriodicTaskRecreate( SchedTCB_t *pxTCB );
		static void prvDeadlineMissedHook( SchedTCB_t *pxTCB, TickType_t xTickCount );
		static void prvCheckDeadline( SchedTCB_t *pxTCB, TickType_t xTickCount );				
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
		static void prvExecTimeExceedHook( TickType_t xTickCount, SchedTCB_t *pxCurrentTask );
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
	
#endif /* schedUSE_SCHEDULER_TASK */



#if( schedUSE_TCB_ARRAY == 1 )
	/* Array for extended TCBs. */
	static SchedTCB_t xTCBArray[ schedMAX_NUMBER_OF_PERIODIC_TASKS ] = { 0 };
	/* Counter for number of periodic tasks. */
	static BaseType_t xTaskCounter = 0;
#endif /* schedUSE_TCB_ARRAY */

#if( schedUSE_SCHEDULER_TASK )
	static TickType_t xSchedulerWakeCounter = 0; /* useful. why? */
	static TaskHandle_t xSchedulerHandle = NULL; /* useful. why? */
#endif /* schedUSE_SCHEDULER_TASK */


#if( schedUSE_TCB_ARRAY == 1 )
	/* Returns index position in xTCBArray of TCB with same task handle as parameter. */
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle )
	{
		static BaseType_t xIndex = 0;
		BaseType_t xIterator;

		for( xIterator = 0; xIterator < schedMAX_NUMBER_OF_PERIODIC_TASKS; xIterator++ )
		{
		
			if( pdTRUE == xTCBArray[ xIndex ].xInUse && *xTCBArray[ xIndex ].pxTaskHandle == xTaskHandle )
			{
				return xIndex;
			}
		
			xIndex++;
			if( schedMAX_NUMBER_OF_PERIODIC_TASKS == xIndex )
			{
				xIndex = 0;
			}
		}
		return -1;
	}

	/* Initializes xTCBArray. */
	static void prvInitTCBArray( void )
	{
	UBaseType_t uxIndex;
		for( uxIndex = 0; uxIndex < schedMAX_NUMBER_OF_PERIODIC_TASKS; uxIndex++)
		{
			xTCBArray[ uxIndex ].xInUse = pdFALSE;
		}
	}

	/* Find index for an empty entry in xTCBArray. Returns -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void )
	{
		/* Line 137 your implementation goes here */
		for (BaseType_t i = 0; i < schedMAX_NUMBER_OF_PERIODIC_TASKS; ++i)
		{
			if(xTCBArray[i].xInUse == pdFALSE)
			{
				return i;
			}
		}
		return -1; // return -1 if there is not empty entry
	}

	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex )
	{
		/* Line 143 your implementation goes here */
		memset(&xTCBArray[xIndex], 0, sizeof(SchedTCB_t)); // reset every member in the struct
		xTCBArray[xIndex].xInUse = pdFALSE;
		xTaskCounter --; // decrement task counter
	}
	
#endif /* schedUSE_TCB_ARRAY */


/* The whole function code that is executed by every periodic task.
 * This function wraps the task code specified by the user. */
static void prvPeriodicTaskCode( void *pvParameters ){
	SchedTCB_t *pxThisTask;	
	TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();  

	/* Line 157 your implementation goes here */
	BaseType_t index;
	#if( schedUSE_TCB_ARRAY == 1 )
		index = prvGetTCBIndexFromHandle(xCurrentTaskHandle); // get the index to the current TCB's handle
	#endif
	// check that the index is not equal to the NULL pointer
	configASSERT(index != -1);
	pxThisTask = &xTCBArray[index];

	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		/* Line 171 your implementation goes here */
		pxThisTask->xAbsoluteDeadline = xSystemStartTime + pxThisTask->xReleaseTime + pxThisTask->xRelativeDeadline;
		pxThisTask->xExecutedOnce = pdFALSE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	if( 0 == pxThisTask->xReleaseTime )
	{
		pxThisTask->xLastWakeTime = xSystemStartTime;
	}

	for( ; ; )
	{	
		/* Execute the task function specified by the user. */
		pxThisTask->pvTaskCode( pvParameters );

		/* Reset timing state after each job and block until the next period. */
		pxThisTask->xExecTime = 0;
		pxThisTask->xWorkIsDone = pdTRUE;
		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
			pxThisTask->xExecutedOnce = pdTRUE;
		#endif

		xTaskDelayUntil( &pxThisTask->xLastWakeTime, pxThisTask->xPeriod );
	}
}

/* Creates a periodic task. */
void vSchedulerPeriodicTaskCreate( TaskFunction_t pvTaskCode, const char *pcName, UBaseType_t uxStackDepth, void *pvParameters, UBaseType_t uxPriority,
		TaskHandle_t *pxCreatedTask, TickType_t xPhaseTick, TickType_t xPeriodTick, TickType_t xMaxExecTimeTick, TickType_t xDeadlineTick )
{
	taskENTER_CRITICAL();
	SchedTCB_t *pxNewTCB;
	
	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex = prvFindEmptyElementIndexTCB();
		configASSERT( xTaskCounter < schedMAX_NUMBER_OF_PERIODIC_TASKS );
		configASSERT( xIndex != -1 );
		pxNewTCB = &xTCBArray[ xIndex ];	
	#endif /* schedUSE_TCB_ARRAY */

	/* Intialize item. */
		
	pxNewTCB->pvTaskCode = pvTaskCode;
	pxNewTCB->pcName = pcName;
	pxNewTCB->uxStackDepth = uxStackDepth;
	pxNewTCB->pvParameters = pvParameters;
	pxNewTCB->uxPriority = uxPriority;
	pxNewTCB->pxTaskHandle = pxCreatedTask;
	pxNewTCB->xReleaseTime = xPhaseTick;
	pxNewTCB->xPeriod = xPeriodTick;
	
    /* Populate the rest */
    /* Line 216 your implementation goes here */
	pxNewTCB->xMaxExecTime = xMaxExecTimeTick;
	pxNewTCB->xAbsoluteDeadline = pxNewTCB->xReleaseTime + xDeadlineTick;
	pxNewTCB->xRelativeDeadline = xDeadlineTick;
	pxNewTCB->xWorkIsDone = pdFALSE;
	pxNewTCB->xExecTime = 0;
	pxNewTCB->xLastWakeTime = xPhaseTick;

    
	#if( schedUSE_TCB_ARRAY == 1 )
		pxNewTCB->xInUse = pdTRUE;
	#endif /* schedUSE_TCB_ARRAY */
	
	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
		/* member initialization */
        /* Line 224 your implementation goes here */
		pxNewTCB->xPriorityIsSet = pdFALSE;
		
	#endif /* schedSCHEDULING_POLICY || schedSCHEDULING_POLICY_DMS */
	
	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		/* member initialization */
        /* Line 229 your implementation goes here */
		pxNewTCB->xExecutedOnce = pdFALSE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
	
	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
        pxNewTCB->xSuspended = pdFALSE;
        pxNewTCB->xMaxExecTimeExceeded = pdFALSE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */	

	#if( schedUSE_TCB_ARRAY == 1 )
		xTaskCounter++;	
	#endif /* schedUSE_TCB_SORTED_LIST */
	taskEXIT_CRITICAL();

	Serial.print("Registered periodic task: ");
	Serial.println( pxNewTCB->pcName );
}

/* Deletes a periodic task. */
void vSchedulerPeriodicTaskDelete( TaskHandle_t xTaskHandle )
{
	/*Line 247 your implementation goes here */
	BaseType_t xIndex;
	#if( schedUSE_TCB_ARRAY == 1 )
		xIndex = prvGetTCBIndexFromHandle( xTaskHandle );
		if( xIndex != -1 )
		{
			prvDeleteTCBFromArray( xIndex );
		}
	#endif
	vTaskDelete( xTaskHandle ); 
}

/* Creates all periodic tasks stored in TCB array, or TCB list. */
static void prvCreateAllTasks( void )
{
	SchedTCB_t *pxTCB;

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex;
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			configASSERT( pdTRUE == xTCBArray[ xIndex ].xInUse );
			pxTCB = &xTCBArray[ xIndex ];

			/* Create the wrapper task, not the raw user task, so scheduler
			 * bookkeeping runs before/after each job release. */
			BaseType_t xReturnValue = xTaskCreate( prvPeriodicTaskCode, pxTCB->pcName, pxTCB->uxStackDepth, pxTCB->pvParameters, pxTCB->uxPriority, pxTCB->pxTaskHandle /* Line 264 your implementation goes here */ );
			if( pdPASS == xReturnValue )
			{
				Serial.print("Created periodic task: ");
				Serial.println( pxTCB->pcName );
			}
			else
			{
				Serial.print("Failed to create periodic task: ");
				Serial.println( pxTCB->pcName );
			}

		}	
	#endif /* schedUSE_TCB_ARRAY */
}

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
	/* Initiazes fixed priorities of all periodic tasks with respect to RMS policy. */
static void prvSetFixedPriorities( void )
{
	BaseType_t xIter, xIndex;
	TickType_t xShortest, xPreviousShortest=0;
	SchedTCB_t *pxShortestTaskPointer, *pxTCB;

	#if( schedUSE_SCHEDULER_TASK == 1 )
		BaseType_t xHighestPriority = schedSCHEDULER_PRIORITY; 
	#else
		BaseType_t xHighestPriority = configMAX_PRIORITIES;
	#endif /* schedUSE_SCHEDULER_TASK */

	for( xIter = 0; xIter < xTaskCounter; xIter++ )
	{
		xShortest = portMAX_DELAY;

		/* search for shortest period */
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			/* Line 291 your implementation goes here */
			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
				/* Line 293 your implementation goes here */
				if( xTCBArray[xIndex].xPeriod < xShortest && xTCBArray[xIndex].xPriorityIsSet == pdFALSE )
				{
					xShortest = xTCBArray[xIndex].xPeriod;
					pxShortestTaskPointer = &xTCBArray[xIndex];
				}
			#endif /* schedSCHEDULING_POLICY */
      #if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
				/* Line 293 your implementation goes here */
				if( xTCBArray[xIndex].xRelativeDeadline < xShortest && xTCBArray[xIndex].xPriorityIsSet == pdFALSE )
        {
            xShortest = xTCBArray[xIndex].xRelativeDeadline;
            pxShortestTaskPointer = &xTCBArray[xIndex];
        }

			#endif /* schedSCHEDULING_POLICY_DMS */

		}
		
		/* set highest priority to task with xShortest period (the highest priority is configMAX_PRIORITIES-1) */		
		
		/* Line 299 your implementation goes here */
		pxTCB = pxShortestTaskPointer;
		pxTCB->uxPriority = xHighestPriority - 1;
		xHighestPriority --;
		pxTCB->xPriorityIsSet = pdTRUE;
		xPreviousShortest = xShortest;

	}
}
#endif /* schedSCHEDULING_POLICY || schedSCHEDULING_POLICY_DMS */



#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )

	/* Recreates a deleted task that still has its information left in the task array (or list). */
	static void prvPeriodicTaskRecreate( SchedTCB_t *pxTCB )
	{
		/* Recreated tasks also need to go through the wrapper entry point. */
		BaseType_t xReturnValue = xTaskCreate( prvPeriodicTaskCode, pxTCB->pcName, pxTCB->uxStackDepth, pxTCB->pvParameters, pxTCB->uxPriority, pxTCB->pxTaskHandle /* Line 310 your implementation goes here */ );
				                      		
		if( pdPASS == xReturnValue )
		{
			/* Line 314 your implementation goes here */
			pxTCB->xExecTime = 0;
			pxTCB->xWorkIsDone = pdFALSE;
		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
			pxTCB->xExecutedOnce = pdFALSE;
		#endif
		#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
			pxTCB->xSuspended = pdFALSE;
			pxTCB->xMaxExecTimeExceeded = pdFALSE;
		#endif
		}
		else
		{
			/* Line 319 your implementation goes here */
			Serial.println("Task creation failed during periodic task recreation.");
			
		}
	}

	/* Called when a deadline of a periodic task is missed.
	 * Deletes the periodic task that has missed it's deadline and recreate it.
	 * The periodic task is released during next period. */
	static void prvDeadlineMissedHook( SchedTCB_t *pxTCB, TickType_t xTickCount )
	{
		/* Delete the pxTask and recreate it. */
		/* pxTaskHandle stores a pointer to the handle, so dereference it here. */
		vTaskDelete( *pxTCB->pxTaskHandle /* Line 328 your implementation goes here */ );
		pxTCB->xExecTime = 0;
		prvPeriodicTaskRecreate( pxTCB );
		
		/* Need to reset next WakeTime for correct release. */
		/* Line 333 your implementation goes here */
		//Advances wake timing to the next valid period boundary.
		TickType_t next_tick_count = xTickCount + 1;
		//we use a while loop because a deadline miss can cause multiple periods to be skipped, and we want to make sure that we are on the next period boundary (in future).
		while( ( BaseType_t ) ( next_tick_count - ( pxTCB->xLastWakeTime + pxTCB->xPeriod ) ) > 0 )
		{
			pxTCB->xLastWakeTime += pxTCB->xPeriod;
		}
		//Recomputes xAbsoluteDeadline from new value of xLastWakeTime + xRelativeDeadline.
		pxTCB->xAbsoluteDeadline = pxTCB->xLastWakeTime + pxTCB->xRelativeDeadline;
		
	}

	/* Checks whether given task has missed deadline or not. */
	static void prvCheckDeadline( SchedTCB_t *pxTCB, TickType_t xTickCount )
	{ 
		/* check whether deadline is missed. */     		
		/* Line 341 your implementation goes here */
		if( pdTRUE == pxTCB->xExecutedOnce && pdFALSE == pxTCB->xWorkIsDone && ( BaseType_t ) ( xTickCount - pxTCB->xAbsoluteDeadline ) > 0)
		{
			prvDeadlineMissedHook( pxTCB, xTickCount );
		}
			
	}	
#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */


#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )

	/* Called if a periodic task has exceeded its worst-case execution time.
	 * The periodic task is blocked until next period. A context switch to
	 * the scheduler task occur to block the periodic task. */
	static void prvExecTimeExceedHook( TickType_t xTickCount, SchedTCB_t *pxCurrentTask )
	{
        pxCurrentTask->xMaxExecTimeExceeded = pdTRUE;
        /* Is not suspended yet, but will be suspended by the scheduler later. */
        pxCurrentTask->xSuspended = pdTRUE;
        pxCurrentTask->xAbsoluteUnblockTime = pxCurrentTask->xLastWakeTime + pxCurrentTask->xPeriod;
        pxCurrentTask->xExecTime = 0;
        
        /* Initialize the ISR handoff flag before notifying the scheduler task. */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR( xSchedulerHandle, &xHigherPriorityTaskWoken );
        xTaskResumeFromISR(xSchedulerHandle);
	}
#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */


#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Called by the scheduler task. Checks all tasks for any enabled
	 * Timing Error Detection feature. */
	static void prvSchedulerCheckTimingError( TickType_t xTickCount, SchedTCB_t *pxTCB )
	{
		/* Line 373 your implementation goes here */
		//skip invalid TCBs
		if( pxTCB == NULL || pxTCB->xInUse == pdFALSE )
		{
			return;
		}

		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )						
			/* check if task missed deadline */
            /* Line 378 your implementation goes here */
			if( pdTRUE == pxTCB->xExecutedOnce )
			{
				prvCheckDeadline( pxTCB, xTickCount );
			}
					
		#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
		

		#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
        if( pdTRUE == pxTCB->xMaxExecTimeExceeded )
        {
            pxTCB->xMaxExecTimeExceeded = pdFALSE;
            vTaskSuspend( *pxTCB->pxTaskHandle );
        }
        if( pdTRUE == pxTCB->xSuspended )
        {
            if( ( signed ) ( pxTCB->xAbsoluteUnblockTime - xTickCount ) <= 0 )
            {
                pxTCB->xSuspended = pdFALSE;
                pxTCB->xLastWakeTime = xTickCount;
                vTaskResume( *pxTCB->pxTaskHandle );
            }
        }
		#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */

		return;
	}

	/* Function code for the scheduler task. */
	static void prvSchedulerFunction( void *pvParameters )
	{
		(void) pvParameters;
		/* Start suspended and only run when the tick hook wakes this task. */
		vTaskSuspend( NULL );

		for( ; ; )
		{
			/* Wait for the tick hook to signal that timing checks are due. */
			ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
			#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 || schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
				TickType_t xTickCount = xTaskGetTickCount();
				SchedTCB_t *pxTCB;
				/* line 415 your implementation goes here. */	

				BaseType_t xIndex;

				for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
				{
					pxTCB = &xTCBArray[ xIndex ];
					prvSchedulerCheckTimingError( xTickCount, pxTCB );
				}
			#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE || schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
		}
	}

	/* Creates the scheduler task. */
	static void prvCreateSchedulerTask( void )
	{
		xTaskCreate( (TaskFunction_t) prvSchedulerFunction, "Scheduler", schedSCHEDULER_TASK_STACK_SIZE, NULL, schedSCHEDULER_PRIORITY, &xSchedulerHandle );                             
	}
#endif /* schedUSE_SCHEDULER_TASK */


#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Wakes up (context switches to) the scheduler task. */
	static void prvWakeScheduler( void )
	{
		/* This flag must start false before the ISR potentially requests a switch. */
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR( xSchedulerHandle, &xHigherPriorityTaskWoken );
		xTaskResumeFromISR(xSchedulerHandle);    
	}

	/* Called every software tick. */
	// In FreeRTOSConfig.h,
	// Enable configUSE_TICK_HOOK
	// Enable INCLUDE_xTaskGetIdleTaskHandle
	// Enable INCLUDE_xTaskGetCurrentTaskHandle
	void vApplicationTickHook( void )
	{            
		SchedTCB_t *pxCurrentTask;		
		TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();		
        UBaseType_t flag = 0;
        BaseType_t xIndex;
		BaseType_t prioCurrentTask = uxTaskPriorityGet(xCurrentTaskHandle);

		for(xIndex = 0; xIndex < xTaskCounter ; xIndex++){
			pxCurrentTask = &xTCBArray[xIndex];
			if(pxCurrentTask -> uxPriority == prioCurrentTask){
				flag = 1;
				break;
			}
		}
		
		/* Some Arduino_FreeRTOS builds do not expose xTaskGetIdleTaskHandle(),
		 * so use our own lookup flag to count only managed periodic tasks here. */
		if( xCurrentTaskHandle != xSchedulerHandle && flag == 1 )
		{
			pxCurrentTask->xExecTime++;     
     
			#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
            if( pxCurrentTask->xMaxExecTime <= pxCurrentTask->xExecTime )
            {
                if( pdFALSE == pxCurrentTask->xMaxExecTimeExceeded )
                {
                    if( pdFALSE == pxCurrentTask->xSuspended )
                    {
                        prvExecTimeExceedHook( xTaskGetTickCountFromISR(), pxCurrentTask );
                    }
                }
            }
			#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
		}

		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )    
			xSchedulerWakeCounter++;      
			if( xSchedulerWakeCounter == schedSCHEDULER_TASK_PERIOD )
			{
				xSchedulerWakeCounter = 0;        
				prvWakeScheduler();
			}
		#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
	}
#endif /* schedUSE_SCHEDULER_TASK */

/* This function must be called before any other function call from this module. */
void vSchedulerInit( void )
{
	#if( schedUSE_TCB_ARRAY == 1 )
		prvInitTCBArray();
	#endif /* schedUSE_TCB_ARRAY */
}

/* Starts scheduling tasks. All periodic tasks (including polling server) must
 * have been created with API function before calling this function. */
void vSchedulerStart( void )
{
	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
		prvSetFixedPriorities();	
	#endif /* schedSCHEDULING_POLICY */

	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS)
		prvSetFixedPriorities();
	#endif /* schedSCHEDULING_POLICY*/

	#if( schedUSE_SCHEDULER_TASK == 1 )
		prvCreateSchedulerTask();
	#endif /* schedUSE_SCHEDULER_TASK */

	prvCreateAllTasks();
	  
	xSystemStartTime = xTaskGetTickCount();
	
	vTaskStartScheduler();
}
