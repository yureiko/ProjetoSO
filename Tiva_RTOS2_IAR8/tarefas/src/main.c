#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "ADC_funcs.h"  //ADC functions
#include "UART_funcs.h"  //UART functions
#include "PWM_funcs.h"  //PWM functions

#define MSGQUEUE_OBJECTS      16  

typedef struct {                                 // Message object structure
  uint32_t   voltage;                              // AD result of measured voltage
} MSGQUEUE_OBJ_t;

osMessageQueueId_t mid_MsgQueue;  

osThreadId_t thread1_id, thread2_id, ADC_thread_id, UART_thread_id,
             PWM_thread_id;

void thread1(void *arg){
  uint8_t state = 0;
  
  while(1){
    state ^= LED1;
    LEDWrite(LED1, state);
//    for(int32_t i = 0; i < 100000; i++);
    osDelay(100);
  } // while
} // thread1

void thread2(void *arg){
  uint8_t state = 0;
  uint32_t tick;
  
  while(1){
    tick = osKernelGetTickCount();
    
    state ^= LED2;
    LEDWrite(LED2, state);
//    for(int32_t i = 0; i < 100000; i++);
    
    osDelayUntil(tick + 100);
  } // while
} // thread2

void ADC_thread(void *arg)
{
  MSGQUEUE_OBJ_t msg;
  while (1) {
    msg.voltage = ADC_get_value();
    
    osMessageQueuePut (mid_MsgQueue, &msg, 0, NULL);
    osThreadYield ();                                              // suspend thread
  }
}

void UART_thread(void *arg)
{
  while(1)
  {
    uint8_t temp[17] = {0x68, 0x0B, 0x0B, 0x68, 0x53, 0xFD, 0x52, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0x24, 0xFF, 0x00, 0x16};
						
    osDelay(100);
					
    UART_send_byte(temp, 17);
    osThreadYield (); 
  }
}

void PWM_thread(void *arg)
{
 MSGQUEUE_OBJ_t msg;
  osStatus_t status;
  while (1) {
    status = osMessageQueueGet (mid_MsgQueue, &msg, NULL, NULL);  // wait for message
    if (status == osOK) {
        PWM_set_duty((float)msg.voltage/4096);
    }
  }
}

void main(void){
  SystemInit();
  LEDInit(LED1|LED2);
  ADC_init();
  UART_init();
  PWM_init();

  osKernelInitialize();
  
  mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(MSGQUEUE_OBJ_t), NULL);
  if (!mid_MsgQueue) {
    ; // Message Queue object not created, handle failure
  }

  thread1_id = osThreadNew(thread1, NULL, NULL);
  thread2_id = osThreadNew(thread2, NULL, NULL);
  ADC_thread_id = osThreadNew(ADC_thread, NULL, NULL);
  UART_thread_id = osThreadNew(UART_thread, NULL, NULL);
  PWM_thread_id = osThreadNew(PWM_thread, NULL, NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
