#include "scheduler.h"

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;

static void doConfiguredWorkTicks(void);

static void testFunc1( void *pvParameters )
{
	(void) pvParameters;
	doConfiguredWorkTicks();
	// Serial.println("task 1");
}

static void testFunc2( void *pvParameters )
{ 
	(void) pvParameters;	
	doConfiguredWorkTicks();
	// Serial.println("task 2");
}

static void testFunc3( void *pvParameters )
{
	(void) pvParameters;
	doConfiguredWorkTicks();
	// Serial.println("task 3");
}

static void testFunc4( void *pvParameters )
{ 
	(void) pvParameters;	
	doConfiguredWorkTicks();
	// Serial.println("task 4");
}

/* Simulate workload by burning CPU cycles in proportion to the task's
 * configured WCET ticks. */
static void doConfiguredWorkTicks(void)
{
    uint32_t targetTicks = ulSchedulerGetCurrentTaskWCETTicks();
    const uint32_t iterationsPerTick = 9000UL;
    volatile uint32_t sink = 0;
    uint32_t i;

    for (i = 0; i < (targetTicks * iterationsPerTick); i++)
    {
        sink += i;
    }
}

void setup()
{
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
	char c1 = 'a';
	char c2 = 'b';			
	char c3 = 'c';
	char c4 = 'd';		

	vSchedulerInit();

	// Serial.println("-----------");
	// Serial.println(pdMS_TO_TICKS(100));
	// Serial.println(pdMS_TO_TICKS(150));
	// Serial.println(pdMS_TO_TICKS(200));
	// Serial.println(pdMS_TO_TICKS(150));

	// Serial.println(pdMS_TO_TICKS(400));
	// Serial.println(pdMS_TO_TICKS(200));
	// Serial.println(pdMS_TO_TICKS(700));
	// Serial.println(pdMS_TO_TICKS(1000));

	// Serial.println(pdMS_TO_TICKS(400));
	// Serial.println(pdMS_TO_TICKS(500));
	// Serial.println(pdMS_TO_TICKS(800));
	// Serial.println(pdMS_TO_TICKS(1000));

	// Serial.println("---------");

	// Serial.println(pdMS_TO_TICKS(100));
	// Serial.println(pdMS_TO_TICKS(200));
	// Serial.println(pdMS_TO_TICKS(150));
	// Serial.println(pdMS_TO_TICKS(300));

	// Serial.println(pdMS_TO_TICKS(400));
	// Serial.println(pdMS_TO_TICKS(700));
	// Serial.println(pdMS_TO_TICKS(1000));
	// Serial.println(pdMS_TO_TICKS(5000));

	// Serial.println(pdMS_TO_TICKS(400));
	// Serial.println(pdMS_TO_TICKS(800));
	// Serial.println(pdMS_TO_TICKS(1000));
	// Serial.println(pdMS_TO_TICKS(5000));


	// Task set 1
	// vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400),  pdMS_TO_TICKS(100), pdMS_TO_TICKS(400));
	// vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(800),  pdMS_TO_TICKS(200), pdMS_TO_TICKS(700));
	// vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), pdMS_TO_TICKS(150), pdMS_TO_TICKS(1000));
	// vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), pdMS_TO_TICKS(300), pdMS_TO_TICKS(5000));

	// Serial.println("configUSE_TICK_HOOK");
	// Serial.println(configUSE_TICK_HOOK );

	// Task set 2
	vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400),  pdMS_TO_TICKS(100), pdMS_TO_TICKS(400));
	vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(500),  pdMS_TO_TICKS(150), pdMS_TO_TICKS(200));
	vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(800),  pdMS_TO_TICKS(200), pdMS_TO_TICKS(700));
	vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), pdMS_TO_TICKS(150), pdMS_TO_TICKS(1000));

	vSchedulerStart();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached. */
	
	for( ;; );
}

void loop() {}
