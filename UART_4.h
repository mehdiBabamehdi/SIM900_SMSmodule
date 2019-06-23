
/*
 * Name: USART Lib.
 * Description: This library developed to receive and trasndmit data (char & string) through USART.
		First, it initializes the USART register in ATMEGA32 based on the information user provides (baud rate,data 			size,parity mode, number of stop bit, and whether double speed in asynchronization is made enable). Then, the 			user should determine if data will be receive/transmit with interrupt, and if so, the library enable global 			interrupt service routine (ISR). and the user, based on the method of rec/trans and the type of data were 			chosed, used the function for receiving/transmitting data.
 * Created: 10/4/2016 9:09:52 PM
 * Ver: 4.0
 * Final Edited: 12/22/2016
 * Author : Mehdi
 */



#ifndef UARTInit
#define UARTInit

#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include "Gen_Def.h"



/****************************************************************************
					SET DATA FOR RECEIVER & TRANSMITTER
*****************************************************************************/

#define TX_BUFFER_SIZE 128				// Length of buffer for transmitter
volatile char Transmitted_Data[TX_BUFFER_SIZE];	// The data in which transmitting data is stored
volatile uint8_t txReadPos = 0;		// position of reading from the transmitted-data array
volatile uint8_t txWritePos = 0;		// position of writing on the transmitted-data array

#define RX_BUFFER_SIZE 128			// Length of buffer for Receiver
volatile char Received_Data[RX_BUFFER_SIZE]; // The data in which receiving data is stored
volatile uint8_t rxReadPos = 0;		// position of reading from the received-data array
volatile uint8_t  rxWritePos = 0;	// position of writing on the received-data array

/******************************************************************************/

#define Rec_Data_Length	 128			// The maximum length of the data will be received  and stored by uC

/**
 * Name: USART_Initialization
 * Description: Function to Initialize USART
 * @Author: Mehdi
 *
 * @Params	Baud_Rate: BAUD RATE
 * @Params	Data_Bits: SET THE RECEIVED/TRANSMITTED DATA SIZE IN BIT
 * @Params	Parity: PARITY MODE; 0: EVEN , 1: ODD
 * @Params	Stop_Bits: NUMBER OF STOP BIT, ONE OR TWO BITS
 * @Params	AsyncDoubleSpeed: WHETHER DOUBLE SPEED IN ASYNCHRONIZATON IS ENABLE; 0: NO , 1: YES
*/

void USART_Initialization(long  Baud_Rate, char Data_Bits, char Parity, char Stop_Bits, char AsyncDoubleSpeed)
{
	uint16_t UBBRValue = lrint((F_CPU / (16UL * Baud_Rate)) - 1);

	if (AsyncDoubleSpeed == 1)
	{
		UCSRA = (1 << U2X);				// Setting the U2X bit to 1 for double speed asynchronous
		UBBRValue = UBBRValue / 2;		// SET UBBR FOR U2X
	}

	UBRRH = (uint8_t)(UBBRValue >> 8);	// PUT THE UPPER PART OF THE BAUD NUMBER (BITS 8-11)
	UBRRL = (uint8_t)(UBBRValue);		// PUT THE REMAINING PART OF THE BAUD NUMBER

	UCSRB |= (1 << RXEN) | (1 << TXEN);		// ENABLE THE RECEIVER AND TRANSMITTER

	if(Data_Bits == 6) UCSRC |= (1 << UCSZ0) | (1 << URSEL);		// 6-bit data length
	if(Data_Bits == 7) UCSRC |= (2 << UCSZ0) | (1 << URSEL);		// 7-bit data length
	if(Data_Bits == 8) UCSRC |= (3 << UCSZ0) | (1 << URSEL);		// 8-bit data length
	if(Data_Bits == 9) UCSRC |= (7 << UCSZ0) | (1 << URSEL);		// 9-bit data length

	if(Stop_Bits == 2) UCSRC |= (1 << USBS) | (1 << URSEL);		// SET TWO STOP BITS

	if(Parity == EVEN)	   UCSRC |= (1 << UPM1) | (1 << URSEL);		// Sets parity to EVEN
	if(Parity == ODD)	   UCSRC |= (3 << UPM0) | (1 << URSEL);		// Sets parity to ODD
	if(Parity == RESERVE)  UCSRC |= (1 << UPM0) | (1 << URSEL);		// Sets parity to RESERVE
}

/**
 * Name: USART_Interrupt_Int
 * Description: Function to Initialize USART Interrupts and Activate Global interrupt if necessary
 * @Author: Mehdi
 *
 * @Params	Rec_Comp_Int: RX Complete Interrupt Enable
 * @Params	Tran_Comp_Int: TX Complete Interrupt Enable
 * @Params	Data_Reg_Empty_Int: USART Data Register Empty Interrupt Enable
*/

void USART_Interrupt_Int (char Rec_Comp_Int, char Tran_Comp_Int, char Data_Reg_Empty_Int)
{

	if (Rec_Comp_Int)		UCSRB |= (1 << RXCIE);
	if (Tran_Comp_Int)		UCSRB |= (1 << TXCIE);
	if (Data_Reg_Empty_Int)	UCSRB |= (1 << UDRIE);
	if (Rec_Comp_Int || Tran_Comp_Int || Data_Reg_Empty_Int)	sei();
}


/******************************************************************************************
										RECEIVER
******************************************************************************************/

/**
 * Name: Timer_for_USART
 * Description: Function to solve the problem of dead-block in receiver,
 *			   it impose a restrain on waiting-time for receiving char through USART.
 * @Author: Mehdi
 *
 * @Params	Rec_Error: If nothing received set 1, otherwise 0
 * @Return	tmp: the data where received data stores temporarily
*/
char Timer_for_USART (void)
{
	char tmp = '\0';
	TCNT1 = 36000;
	TCCR1B = 1 << CS12 | 1 << CS10;	// Pre-scaler 1/1024

	while(1)
	{
		if ((TIFR & (1 << TOV1)) && (!(UCSRA & (1 << RXC))))	// If nothing has been received by uC within 5 sec
		{
			TCCR1B = 0;				// Turn off Timer1
			TIFR &= ~(1 << TOV1);	// Clear TOV1
			break;
		} else if ((UCSRA & (1 << RXC)))	// If data is received within 5 sec
		{
			tmp = UDR;
			TCCR1B = 0;				// Turn off Timer1
			TIFR &= ~(1 << TOV1);	// Clear TOV1
			break;
		}
	}

	return (tmp);
}

/**
 * Name: USART_Receive_char
 * Description: The ROUTINES RECEIVE DATA (CHAR) FROM PC Through USART without Interrupt.
 * @Author: Mehdi
 *
 * @Return	REC_DATA: The Data Received by the uC and send back to the function called it
*/

char USART_Receive_char(void)
{
	char REC_DATA = '\0';

//	while(!(UCSRA & (1 << RXC))); // Wait until data be received
//	REC_DATA = UDR;
//	Rec_Error == 0;

	REC_DATA = Timer_for_USART();

	return (REC_DATA);
}

/**
 * Name: USART_Receive_char
 * Description: Function to receive string
 * @Author: Mehdi
 *
 * @Params	Rec_String: The String send back by this routine to the function called it
*/
unsigned char* USART_Receive_String(void)
{
	unsigned char *Rec_String = malloc(Rec_Data_Length + 1);
	unsigned char Rec_Buffer;	// The data stores Received data temporarily
	int i = 0;

	for (i=0; i < Rec_Data_Length; i++)
	{
		Rec_Buffer = USART_Receive_char();
		if (Rec_Buffer == '\0')	break;		// If the uC receives nothing after 5 sec, break the loop
		Rec_String[i] = Rec_Buffer	;		// Store the received characters into the array string[] one-by-one
	}
	Rec_String[i] = '\0';		// Zero-Terminator for producing string
	return (Rec_String);
}


//////////////////////////////////// Receiver with ISR //////////////////////////////////////
/**
 * Name: USART_Receive_char_ISR
 * Description: The ROUTINES RECEIVE DATA (CHAR) FROM PC Through USART with Interrupt.
 * @Author: Mehdi
 *
 * @Return	REC_DATA: The Data Received by the uC and send back to the function called it
*/

char USART_Receive_char_ISR(void)
{
	char REC_DATA = '\0';
	if (rxReadPos != rxWritePos)
	{
		REC_DATA = Received_Data[rxReadPos];
		rxReadPos++;
		if (rxReadPos >= RX_BUFFER_SIZE)
		{
			rxReadPos = 0;
		}
	}
	return (REC_DATA);
}

/**
 * Name: Interrupt Service Routine
 * Description: It fires when data received by USART (USART_RXC_vect) and
 *				in the routine received data stores in a temporary array
 * @Author: Mehdi
*/
ISR(USART_RXC_vect)
{
	Received_Data[rxWritePos] = UDR;
	rxWritePos++;

	if (rxWritePos >= RX_BUFFER_SIZE)
	{
		rxWritePos = 0;
	}
}


/**
 * Name: USART_Receive_String_ISR
 * Description: Function to receive string
 * @Author: Mehdi
 *
 * @Return	Rec_String: The String send back by this routine to the function called it
*/
char* USART_Receive_String_ISR(void)
{
	char* Rec_String = malloc(Rec_Data_Length + 1);
	int i = 0;
	int counter = 0;
	int Start_Flag = 0;
	Rec_String = 'FALSE';

	while (rxReadPos != rxWritePos)
	{
		if (Received_Data[rxReadPos] == 'S')
		{
			Start_Flag = 1;
			rxReadPos++;
			if (rxReadPos >= RX_BUFFER_SIZE)
			{
				rxReadPos = 0;
			}
		}

		if (Start_Flag == 1)
		{
			if (Received_Data[rxReadPos] == 'E') break;

			Rec_String[i++] = Received_Data[rxReadPos];
			rxReadPos++;
			if (rxReadPos >= RX_BUFFER_SIZE)
			{
				rxReadPos = 0;
			}

		} else if (Start_Flag == 0)
		{
			rxReadPos++;

			if (rxReadPos >= RX_BUFFER_SIZE)
			{
				rxReadPos = 0;
			}
		}
	}


	Rec_String[i++] = '\0';

	return (Rec_String);
}

/**
 * Name: UDataAvailable
 * Description: Function to find out if any data received by USART
 * @Author: Mehdi
 *
 * @Return	FALSE: If nothing was received
 *          TRUE:  If data was received
*/

uint8_t USART_DataAvailable()
{
	if(rxReadPos==rxWritePos)
	{
        	return FALSE;
	} else if (rxReadPos!=rxWritePos)
	{
        	return TRUE;
	}
}

/**
 * Name: LenRecData
 * Description: Function to find out the length of data received by USART
 * @Author: Mehdi
 *
 * @Return	length of received data by  USART
*/

uint8_t USART_LenRecData()
{
	if(rxReadPos==rxWritePos)
	{
        	return FALSE;
	} else if (rxReadPos > rxWritePos)
	{
		return (rxWritePos - rxReadPos);
	}else if (rxReadPos < rxWritePos)
	{
		return (RX_BUFFER_SIZE - rxWritePos + rxReadPos +1);
	}
}

/**
 * Name: RxBufferFlush
 * Description: The function clear the data pending in queue
 * @Author: Mehdi
 *
*/

void USART_RxBufferFlush (void)
{
    char *PendingDate;
    PendingDate = USART_Receive_String_ISR();
}


/******************************************************************************************
										TRANSMITTER
******************************************************************************************/

/**
 * Name: USART_Transmit_char
 * Description: The ROUTINES TRANSMIT DATA (CHAR) TO any device through USART without Interrupt.
 * @Author: Mehdi
 *
 * @Params	data: The data (char) get to Transmit to through USART
*/

// Function to send byte/char //
void USART_Transmit_char(char data)
{
	while(!(UCSRA & (1 << UDRE)));	// Wait until the transmit buffer be empty
	UDR = data;
}


/**
 * Name: USART_Transmit_String
 * Description: Function to transmit string to through USART
 * @Author: Mehdi
 *
 * @Params	StringPtr: the string pointer gotten to transmit through USART
*/
void USART_Transmit_String(char* StringPtr)
{
	while(*StringPtr != 0x00)
	{
		USART_Transmit_char(*StringPtr);
		StringPtr++;
	}
}

//////////////////////////////////// Transmitter with ISR //////////////////////////////////////

/**
 * Name: USART_Transmit_chra_ISR
 * Description: Function to Transmitting the data thought USART using interrupt
 * @Author: Mehdi
 *
 * @Params	Transmitted_DATA: The data send from the function called it in order to send through USART
*/

void USART_Transmit_char_ISR (char Transmitted_DATA)
{
	Transmitted_Data[txWritePos] = Transmitted_DATA;
	txWritePos++;
	if (txWritePos >= TX_BUFFER_SIZE)
	{
		txWritePos = 0;
	}
}

/**
 * Name: Interrupt Service Routine
 * Description: It fires when data transmitted by USART (USART_TXC_vect) and in the routine the array
 *				wanted to be transmitted send one by one
 * @Author: Mehdi
*/
ISR(USART_TXC_vect)
{
	if (txReadPos != txWritePos)
	{
		UDR = Transmitted_Data[txReadPos];
		txReadPos++;
		if (txReadPos >= TX_BUFFER_SIZE)
		{
			txReadPos = 0;
		}
	}
}



#endif
