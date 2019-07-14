#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "ADC_funcs.h"  //ADC functions
#include "UART_funcs.h"  //UART functions
#include "PWM_funcs.h"  //PWM functions

#define MSGQUEUE_OBJECTS      1 

typedef struct {                                 // Message object structure
  uint16_t  voltage;                              // AD result of measured voltage
  float  duty;
} MSGQUEUE_OBJ_t;

osMessageQueueId_t mid_MsgQueue;  
osMessageQueueId_t mid_MsgQueue2;  

osThreadId_t thread1_id, ADC_thread_id, UART_thread_id,
             PWM_thread_id, Control_thread_id;

void thread1(void *arg){
  uint8_t state = 0;
  
  while(1){
    state ^= LED1;
    LEDWrite(LED1, state);
//    for(int32_t i = 0; i < 100000; i++);
    osDelay(100);
  } // while
} // thread1


void ADC_thread(void *arg)
{
  MSGQUEUE_OBJ_t msg;
  
  while (1) {
    msg.voltage = ADC_get_value();
    
    osMessageQueuePut(mid_MsgQueue, &msg, 0, NULL);
    
    osThreadYield();                                              // suspend thread
  }
}

void UART_thread(void *arg)
{
  while(1)
  {
      osThreadYield(); 
  }
}

void PWM_thread(void *arg)
{
 MSGQUEUE_OBJ_t msg;
  osStatus_t status;
  while (1) {
    status = osMessageQueueGet(mid_MsgQueue2, &msg, NULL, NULL);  // wait for message
    if (status == osOK) {
        PWM_set_duty((float)msg.duty);
        osMessageQueueDelete(&msg);
        osThreadYield();
    }
  }
}

void Control_thread(void *arg)
{
  MSGQUEUE_OBJ_t msg;
  osStatus_t status;
  #define zero1 0.933
  #define zero2 0.933
  #define polo1 0.0393
  #define polo2 0.0225
  #define gain 0.97
  
  uint16_t sensorValue = 0;        // value read from the ADC
  float y = 0.0;              // value output to the PWM (analog out)
  float r = 30;
  float u[3] = {0.0, 0.0, 0.0};
  float e[3] = {0.0, 0.0, 0.0};
  uint8_t n = 0;
  float k = 1.0/gain;
  float coef_a = -1.0*(zero1 + zero2); 
  float coef_b = (zero1 * zero2);
  float coef_c = -1.0*(polo1 + polo2); 
  float coef_d = (polo1 * polo2);
  
  while (1) {
    
    status = osMessageQueueGet(mid_MsgQueue, &msg, NULL, osWaitForever);  // wait for message
    if(status == osOK) 
    {       
      sensorValue = msg.voltage;
      osMessageQueueDelete(&msg);
     
      y = 3.3 * (float)sensorValue/4095;
      e[0] = (r+20)*0.0125 - y;
      u[0] = (coef_a*u[1]-coef_b*u[2] + e[0] + coef_c*e[1] + coef_d*e[2]) * k;
    
      if(u[0]<0.0)
        u[0] = 0.0;
        
      if(u[0]>3.3)
        u[0] = 3.3;
      
      msg.duty = u[0]/3.3;
      
      osMessageQueuePut(mid_MsgQueue2, &msg, 0, NULL);
    
      for(n = 2; n > 0; n--)
      {
          u[n] = u[n-1];
          e[n] = e[n-1];
      }   
    }
    osDelay(10);    
   
    osThreadYield();
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
  mid_MsgQueue2 = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(MSGQUEUE_OBJ_t), NULL);
  if (!mid_MsgQueue2) {
    ; // Message Queue object not created, handle failure
  }

  thread1_id = osThreadNew(thread1, NULL, NULL);
  ADC_thread_id = osThreadNew(ADC_thread, NULL, NULL);
  UART_thread_id = osThreadNew(UART_thread, NULL, NULL);
  PWM_thread_id = osThreadNew(PWM_thread, NULL, NULL);
  Control_thread_id = osThreadNew(Control_thread, NULL, NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
