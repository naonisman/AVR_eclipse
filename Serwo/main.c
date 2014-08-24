#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "headers/global_types.h"
#include "headers/header_file_1.h"
#include "headers/header_file_2.h"
#include "headers/task.h"
#include "headers/tasks_aux.h"
#include "headers/hardware.h"
#include "headers/usart.h"


// ****************** DEFINITION ***********************************************************
#define RECEIVED_BYTE_BUFFER_SIZE		(32)
#define MINIMUM_DATA_RECEIVED_CNT		(5)
#define DEBUG_OUTPUT					FALSE


// ****************** Static variables ******************************************************
// USART part
static volatile unsigned char dataReceived[RECEIVED_BYTE_BUFFER_SIZE];
//volatile unsigned char startReading 	= 0;
static volatile unsigned char lastByte 		= 0;
static volatile unsigned int receivedByteCount = 0;

static unsigned int temp = 0;

// ****************** Global variables ******************************************************
UINT uiWorkFlowFlag = 0b00000000;
T_TASK_FUNCTION apfTasks[MAX_NBR_OF_TASKS];

volatile SERVO_MANUAL_PARAMETERS g_stServoMenualParameters = {0.0, 0.0 ,0};

// *****************************************************************************************

INT main (void)
{
	vInitPipe();
	
	USART_Init ( 9600 );
	vSetPendingTask(vServoL);

	sei();
	USART_Transmit_string((unsigned char*)"*:SERVO_READY:#");

  while(TRUE){
	  vDispathTask();
  }

}

//******************************************************************************
//******************************************************************************
ISR(USART_RXC_vect)
{
//	LED_1_TOGGLE
//	LED_1_OFF
	unsigned char byte = UDR;
#if DEBUG_OUTPUT
	unsigned char buffer[10] = {0};
#endif // DEBUG_OUTPUT

    if((lastByte == '*') && (byte == ':') && (receivedByteCount == 0))          //Detect Start frame
    {
      dataReceived[receivedByteCount++] = lastByte;
    }

    if(receivedByteCount > 0)
    {
    	dataReceived[receivedByteCount++] = byte;
    	if(receivedByteCount == RECEIVED_BYTE_BUFFER_SIZE) receivedByteCount--;
    }


    if((lastByte == ':') && (byte == '#') && (receivedByteCount > MINIMUM_DATA_RECEIVED_CNT)){
    	dataReceived[receivedByteCount++] = 0x00;
#if DEBUG_OUTPUT
    	USART_Transmit_string((unsigned char*)dataReceived);
#endif // DEBUG_OUTPUT
    	receivedByteCount = 0;

    	if(0 == strcmp("*:START:#", (char*)dataReceived)){
    		USART_Transmit_string((unsigned char*)"*:SERVO_READY:#");
			lastByte = byte;
			LED_2_ON
			return;
    	}



#if DEBUG_OUTPUT
    	if(dataReceived[2] == 'P'){
    		USART_Transmit_string((unsigned char*)"Period = ");

    		dataReceived[3] -= '0';
    		dataReceived[4] -= '0';
    		dataReceived[5] -= '0';

    		temp = dataReceived[3]*100 + dataReceived[4]*10 + dataReceived[5];

    		g_stServoMenualParameters.dPeriod = ((double)temp)*0.1;

    		sprintf((char *)buffer, "%d ", temp);
    		USART_Transmit_string(buffer);
    	}
    	if(dataReceived[6] == 'H'){
    		USART_Transmit_string((unsigned char*)"HIGH = ");

    		dataReceived[7] -= '0';
    		dataReceived[8] -= '0';
    		dataReceived[9] -= '0';

    		temp = dataReceived[7]*100 + dataReceived[8]*10 + dataReceived[9];
    		g_stServoMenualParameters.dHigh = ((double)temp)*0.1;

    		sprintf((char *)buffer, "%d ", temp);
    		USART_Transmit_string(buffer);
    	}
    	if(dataReceived[10] == 'R'){
    		USART_Transmit_string((unsigned char*)"Repeater = ");

    		dataReceived[11] -= '0';
    		dataReceived[12] -= '0';
    		dataReceived[13] -= '0';

    		g_stServoMenualParameters.uiRepeater = dataReceived[11]*100 + dataReceived[12]*10 + dataReceived[13];

    		sprintf((char *)buffer, "%d ", g_stServoMenualParameters.uiRepeater);
    		USART_Transmit_string(buffer);

    		// Send Acknowledge
    		USART_Transmit_string((unsigned char*)"*:ACK:#");

    		// run servo task
    		vSetPendingTask(vServoManual);
    	}
#else // DEBUG_OUTPUT
    	if( (dataReceived[2]  == 'P')
    			&&
    			(dataReceived[6]  == 'H')
    			&&
    			(dataReceived[10] == 'R')){

    		// PERIOD
    		dataReceived[3] -= '0';
    		dataReceived[4] -= '0';
    		dataReceived[5] -= '0';

    		temp = dataReceived[3]*100 + dataReceived[4]*10 + dataReceived[5];
    		g_stServoMenualParameters.dPeriod = ((double)temp)*0.1;

    		// HIGH state
    		dataReceived[7] -= '0';
    		dataReceived[8] -= '0';
    		dataReceived[9] -= '0';

    		temp = dataReceived[7]*100 + dataReceived[8]*10 + dataReceived[9];
    		g_stServoMenualParameters.dHigh = ((double)temp)*0.1;

    		// REPEATER
    		dataReceived[11] -= '0';
    		dataReceived[12] -= '0';
    		dataReceived[13] -= '0';

    		g_stServoMenualParameters.uiRepeater = dataReceived[11]*100 + dataReceived[12]*10 + dataReceived[13];

    		// Send Acknowledge
    		USART_Transmit_string((unsigned char*)"*:ACK:#");

    		// run servo task
    		vSetPendingTask(vServoManual);
    	}
#endif // DEBUG_OUTPUT
    }

    lastByte = byte;
}



/*
PROTOCOL:
Start Frame - *:
P 			- period: x*0.1ms
H 			- high state: y*0.1ms
R 			- repeat: z times
End frame 	- :#

*:PxxxHyyyRzzz:#

Example
*:P200H020R100:#
Period:  20.0 ms
HIGH:     2.0 ms
Repeat: 100 times

 */
