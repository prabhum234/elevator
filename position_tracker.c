/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Class for keeping track of the car position.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "position_tracker.h"

#include "assert.h"

static void positionTrackerTask(void *params) {
	PositionTracker *tracker = (PositionTracker*)params;
	bool prevPulseState = 0,pulse;
		
	while(1){    
		pulse = GPIO_ReadInputDataBit(tracker->gpio,tracker->pin);		 
		switch(pulse){
		 	case 1:  //high pulse	 
				 if(prevPulseState == 0){ //raising edge
				    xSemaphoreTake(tracker->lock, portMAX_DELAY);
					//increment the position when going up		   
				    if(tracker->direction == Up){
					 tracker->position = tracker->position + 1 ;					 
					 }
					 //decrement the position when going down
					 else  if(tracker->direction == Down){
					 tracker->position = tracker->position - 1;
					 }					 
					 xSemaphoreGive(tracker->lock);
				  }					 
				  prevPulseState = 1;		   
				  break;
			case 0:	 //low pulse
				 prevPulseState = 0;
			     break;
			default:
				 printf("No signal from position Sensor");
		}
		vTaskDelay(tracker->pollingPeriod);
	 }
}

void setupPositionTracker(PositionTracker *tracker,
                          GPIO_TypeDef * gpio, u16 pin,
						  portTickType pollingPeriod,
						  unsigned portBASE_TYPE uxPriority) {
  portBASE_TYPE res;

  tracker->position = 0;
  tracker->lock = xSemaphoreCreateMutex();
  assert(tracker->lock != NULL);
  tracker->direction = Unknown;
  tracker->gpio = gpio;
  tracker->pin = pin;
  tracker->pollingPeriod = pollingPeriod;

  res = xTaskCreate(positionTrackerTask, "position tracker",
                    80, (void*)tracker, uxPriority, NULL);
  assert(res == pdTRUE);
}

void setDirection(PositionTracker *tracker, Direction dir) {
	 xSemaphoreTake(tracker->lock, portMAX_DELAY);
	 tracker -> direction = dir; 
     xSemaphoreGive(tracker->lock);
}

s32 getPosition(PositionTracker *tracker) {
    vs32 position = 0;
 	xSemaphoreTake(tracker->lock, portMAX_DELAY);
	position = tracker->position;
	xSemaphoreGive(tracker->lock);
	return	position;	
}

