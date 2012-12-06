/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Functions listening for changes of specified pins
 */


#include "FreeRTOS.h"
#include "task.h"

#include "pin_listener.h"
#include "assert.h"

static void pollPin(PinListener *listener, xQueueHandle pinEventQueue) {
   const portTickType xTicksToWait = 100 / portTICK_RATE_MS;
   	portBASE_TYPE xStatus;	
	switch(listener->status){
		case 3:
      		if (!GPIO_ReadInputDataBit(listener->gpio,listener->pin)) {	   		
			   	listener -> status = 0;
				//if there exists a falling event for a pin then send it to the queue	
				if(listener->fallingEvent != UNASSIGNED) {
					xStatus = xQueueSendToFront(pinEventQueue,&listener->fallingEvent,xTicksToWait);
		 		  	if( xStatus == pdPASS ){
		   	 		//	printf("\nfalling event:%d",listener->fallingEvent);
					}
					else {
		   				printf("\nqueing failed!");
					}
				}
			}			
			break;
		case 2:	 	//when pressed
			if (!GPIO_ReadInputDataBit(listener->gpio,listener->pin)) {	   		
				listener -> status = 3;	
			}
			break;
	
		case 1:	//when released		  		
      		if (GPIO_ReadInputDataBit(listener->gpio,listener->pin)) {	   		
			   	listener -> status = 2;	
				//send the raising event to the queue 												
				xStatus = xQueueSendToFront(pinEventQueue,&listener->risingEvent,xTicksToWait);
		 		  	if( xStatus == pdPASS ){
		   	 			//printf("\nraising event:%d",listener->risingEvent);
					}
					else {
		   				printf("\nqueing failed!");
					}
			}
			break;
		case 0:
			if (GPIO_ReadInputDataBit(listener->gpio,listener->pin)) {	   		
			   	listener -> status = 1;	
			}				
			break;
		default:
			break;
	}
    
}



static void pollPinsTask(void *params) {
  PinListenerSet listeners = *((PinListenerSet*)params);
  portTickType xLastWakeTime;
  int i;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i)
	  pollPin(listeners.listeners + i, listeners.pinEventQueue);    
	  vTaskDelayUntil(&xLastWakeTime, listeners.pollingPeriod);
  }
}

void setupPinListeners(PinListenerSet *listenerSet) {
  portBASE_TYPE res;

  res = xTaskCreate(pollPinsTask, "pin polling",
                    100, (void*)listenerSet,
					listenerSet->uxPriority, NULL);
  assert(res == pdTRUE);
}
