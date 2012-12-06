/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * The planner module, which is responsible for consuming
 * pin/key events, and for deciding where the elevator
 * should go next
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>

#include "global.h"
#include "planner.h"
#include "assert.h"

#define WAIT_TIME_AT_FLOOR 2000	//waiting time of the elevator at the floor
#define QUEUE_TIME_OUT 100	    //for the timeout set in reading the pinEventQueue queue

static void plannerTask(void *params) {
	int priority[3] = {0,0,0};
	int destinations[3] = {0,400,800};
	int from = 1, to = 1,i = -1,max = -1,arrivedAtFloor = 0,doorClosed =0;
	PinEvent pinevent;
	portBASE_TYPE xStatus;
	const portTickType xTicksToWait = QUEUE_TIME_OUT / portTICK_RATE_MS;
	const portTickType xTicksToWaitAtFloor = WAIT_TIME_AT_FLOOR / portTICK_RATE_MS;
   	while(1){
		//read the queue data 
		    xStatus = xQueueReceive(pinEventQueue,&pinevent,xTicksToWait); 
			if( xStatus == pdPASS ){
			    switch(pinevent){
					case TO_FLOOR_1:
					case TO_FLOOR_2:
						//when lift goes from 1 to 3 or viceversa and 
						//if a call from floor2 arrives, then the lift should stop if 
						//it is at a stoppable distance
						if(from == 1 && to == 3){
							if(getCarPosition() < destinations[TO_FLOOR_2-1]-50)
							priority[TO_FLOOR_2-1] = priority[to]+1;						
						}
						else if(from == 3 && to == 1){
							if(getCarPosition() > destinations[TO_FLOOR_2-1]+50)
							priority[TO_FLOOR_2-1] = priority[to]+1;							
						}
					case TO_FLOOR_3:
						printf("\ncall from:[%d]",pinevent);
						if(priority[pinevent-1]==0){
							i = -1;
							//prioritise the events
							while(i++<3)if(priority[i]!=0)priority[i]++;
							priority[pinevent-1]++;
						}
						i = -1;	max = 0;
						//set target which has higher priority
						while(i++<3){if(priority[i]>max)to = i+1,max = priority[i];}
						//to chage the destination on the move				
						if(doorClosed){
						printf("\ndestination:%d",destinations[to-1]);
						setCarTargetPosition(destinations[to-1]);						
						}
						break;
					case STOP_PRESSED:
						printf("\nstoping motor...");
						setCarMotorStopped(1);
						break;
					case STOP_RELEASED:
						printf("\nresuming motor..");
						setCarMotorStopped(0);
						break;
					case ARRIVED_AT_FLOOR:
						arrivedAtFloor = 1;
						printf("\nArrived at floor...");					
						break;
					case LEFT_FLOOR:
						printf("\nLeft the floor...");
						arrivedAtFloor = 0;
						break;
					case DOORS_CLOSED:
						doorClosed = 1;
						printf("\ndoors closed...");
						//set the target position of the elevator 
						if(arrivedAtFloor){
					 	i = -1;	max = 0;
						while(i++<3){if(priority[i]>max)to = i+1,max = priority[i];}				
						setCarTargetPosition(destinations[to-1]);
						printf("\nNext destination:%d",destinations[to-1]);
						}
						break;
					case DOORS_OPENING:
						doorClosed = 0;
						printf("\ndoors open...");
						if(arrivedAtFloor){
							from = to;
							priority[from-1] = 0;
						}
						vTaskDelay(xTicksToWaitAtFloor);
						break;
				}
			}
			
   }
}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 100, NULL, uxPriority, NULL);
}
