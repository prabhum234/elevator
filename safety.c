/**
 * This file defines the safety module, which observe the running
 * elevator system and is able to stop the elevator in critical
 * situations
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>

#include "global.h"
#include "assert.h"

#define POLL_TIME (10 / portTICK_RATE_MS)

#define MOTOR_UPWARD   (TIM3->CCR1)
#define MOTOR_DOWNWARD (TIM3->CCR2)
#define MOTOR_STOPPED  (!MOTOR_UPWARD && !MOTOR_DOWNWARD)

#define STOP_PRESSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3)
#define AT_FLOOR      GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)
#define DOORS_CLOSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)

static void safetyTask(void *params) {
  portTickType xLastWakeTime;
  s16 timeSinceStopPressed = -1;
  s16 timeSinceMotorStarted = -1;
  s16 timeSinceMotorStarted1 = -1;
  int timeSinceDoorOpened = 0;
  int prev_position = 0,currentPosition =0 ;
  int previousRunning = 0;
  int shuttle_timeOut = 60 * 1000;
  int speed = 0,checkStartingSpeed=0,after1sec=0;
  int PreDir = 0;
  u8 ok;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {  
    ok = 1;

    // Environment assumption 1: the doors can only be opened if
	//                           the elevator is at a floor and
    //                           the motor is not active

	ok = ok && ((AT_FLOOR && MOTOR_STOPPED) || DOORS_CLOSED);		
	if(!ok)printf( "env1");


    // Environment assumption 2: elevator can stop only at floors 1,2 or 3
	//                           unless stop button is pressed

  if(AT_FLOOR){
  	 currentPosition = getCarPosition();	
	 if((currentPosition>=0 &&  currentPosition<=1)||
	  (currentPosition>=399 &&  currentPosition<=401)||
	  (currentPosition>=799 &&  currentPosition<=800))
	 ok = ok && 1;
     else
 	 ok = 0;
   }
	if(!ok)
	printf( "env2");

    // Environment assumption 3:  elevator moves at a maximum speed of 50cm/s
  
	 if (!MOTOR_STOPPED) {
			  if (timeSinceMotorStarted < 0){
				    timeSinceMotorStarted = 0;		
				}
		      else
			    timeSinceMotorStarted += POLL_TIME;

	      if (timeSinceMotorStarted * portTICK_RATE_MS >= 1000 ){
		  		currentPosition = getCarPosition();
				speed = abs(prev_position - currentPosition);
				ok = ok && ( speed <= 51);
				timeSinceMotorStarted = 0;
				after1sec = 1;	
				prev_position = currentPosition;
			}
			else after1sec = 0;
	} else {
	  timeSinceMotorStarted = -1;
	  prev_position = getCarPosition();
	} 
	if(!ok){
	printf("env3");
	}

	/* // Environment assumption 4:
	 //Doors should not take more than 2sec, to open the door,as soon as the lift has arrived at floor     
	if(timeSinceLiftArrived>0){
		timeSinceLiftArrived += POLL_TIME;
		if(timeSinceLiftArrived * portTICK_RATE_MS < 2000){
		  if(!DOORS_CLOSED)timeSinceLiftArrived = 0;				
		}
		else{
		 ok = 0;
		}
	 }	 	
	if(!previousAtFloor && AT_FLOOR)timeSinceLiftArrived = 1;
	if(AT_FLOOR)previousAtFloor = 1;
	else previousAtFloor = 0;
	if(!ok)printf( "env4");
	  */

	// Environment assumption 4: 
	//When the doors open up when they are at the floor, it should be open atleast for one sec
	if(!DOORS_CLOSED && AT_FLOOR){
	   timeSinceDoorOpened += POLL_TIME;	  	    
	}
	else if(DOORS_CLOSED && (timeSinceDoorOpened==0||timeSinceDoorOpened >= 1000)){
		timeSinceDoorOpened = 0;
	}
	else{
		ok = 0;	
	}
    if(!ok)printf("env4"); 
	 
	 // Environment assumption 5: 
	 // Moving elevtor shouldnot change its direction unless it reaches a destination
	if(MOTOR_UPWARD)
		ok = ok && (PreDir==1 || PreDir == 0);	
	if(MOTOR_DOWNWARD)
	     ok = ok && (PreDir == -1 || PreDir == 0);
		 //store the previous direction
	if(MOTOR_UPWARD)
	 	PreDir = 1;
	 else if(MOTOR_DOWNWARD)
		PreDir = -1;
	 else if(AT_FLOOR)
		PreDir = 0;	
	 else
	    PreDir =  PreDir;
	if(!ok)printf("env5"); 




    // System requirement 1: if the stop button is pressed, the motor is
	//                       stopped within 1s

	if (STOP_PRESSED) {
	  if (timeSinceStopPressed < 0)
	    timeSinceStopPressed = 0;
      else
	    timeSinceStopPressed += POLL_TIME;
      if (timeSinceStopPressed * portTICK_RATE_MS > 1000 &&
	      !MOTOR_STOPPED)
		ok = 0;
	} else {
	  timeSinceStopPressed = -1;
	}
    if(!ok)printf("sys1");
    // System requirement 2: the motor signals for upwards and downwards
	//                       movement are not active at the same time

	ok = ok && (!MOTOR_UPWARD || !MOTOR_DOWNWARD);
    if(!ok)printf( "sys2");


	// System requirement 3: The elevator must not leave a floor as long as the doors are open
	ok = ok && !(!DOORS_CLOSED && !MOTOR_STOPPED);

	// System requirement 4: The elevator may not pass the end positions
	currentPosition = getCarPosition();
	ok = ok && (currentPosition >= 0 && currentPosition <= 800);
    if(!ok)printf("sys4");

	// System requirement 5: A moving elevator halts only if the stop button is pressed 
	//or the elevator has arrived at a floor
	if (previousRunning && MOTOR_STOPPED) {	      
		ok = ok && (STOP_PRESSED || AT_FLOOR );
	}	
 	if(!ok)printf( "sys5");
	
	// System requirement 6: 
	  //The initial speed of the elevator, when it starts to move, should not be more than or equal to 50cm/s.
	 //because when a stopped elevator starts to move to some destination
	 //its initial speed should not be very high	
	if (!previousRunning && !MOTOR_STOPPED) {	
		checkStartingSpeed = 1;
	}    	
	if(checkStartingSpeed && after1sec){
		printf("\nchecking start speed:%d\n",speed);
		checkStartingSpeed =0;
		ok = ok && (speed < 50);
	
	}  
	if(!ok)printf("sys6"); 
		
	// System requirement 7: if the lift never stops anywhere more than one minute time
	//                      then stop the elevator 

	if (!MOTOR_STOPPED) {
	  if (timeSinceMotorStarted1 < 0)
	    timeSinceMotorStarted1 = 0;
      else
	    timeSinceMotorStarted1 += POLL_TIME;
      if (timeSinceMotorStarted1 * portTICK_RATE_MS > shuttle_timeOut &&
	      !MOTOR_STOPPED)					                 
		ok = 0;
	} else {
	  timeSinceMotorStarted1 = -1;
	}
  if(!ok)printf("sys7");
    

    if(MOTOR_STOPPED)previousRunning=0;
	else previousRunning=1;
	
    if (!ok) {	
	  printf("SAFETY REQUIREMENTS VIOLATED: STOPPING ELEVATOR\n");
	  for (;;) {
	    setCarMotorStopped(1);
  	    vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
      }
	}  

	vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
  }

}

void setupSafety(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(safetyTask, "safety", 100, NULL, uxPriority, NULL);
}
