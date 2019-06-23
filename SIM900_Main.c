

#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>

#include "UART_4.h"
#include "LCD.h"
#include "SIM900.h"
#include "Gen_Def.h"


void Halt(void);
int main()
{
    char *Greeting_msg = "Hello World!";
    uint8_t id;     // Number of the slot where received message stores in


    // Initialize the UART; Baud Rate=9.6k, 8-byte data size,
    // No parity, one stop bit, disable Double Speed in Asynchronization
    USART_Initialization(9600,8,NONE,1,0);

    // Initialization of USART Interrupt (RXC, TXC, UDRE)
    USART_Interrupt_Int(TRUE,FALSE,FALSE);

    // Initialize LCD module, LCD Blink & Cursor is "underline" type
    LCDInit(LS_BLINK|LS_ULINE);

    LCDWriteStringXY(4,1,Greeting_msg);
    _delay_ms(5000);
    LCDClear();

    DDRB |= 1 << PINB1 | 1 << PINB2;
    PORTB &= ~(1 << PINB1) | ~(1 << PINB2);

    // Initializing SIM900
    LCDWriteString("Initializing SIM900");
    int8_t response = SIM900Init();

    _delay_ms(1000);

    switch(r)
    {
        case SIM900_OK:
            LCDWriteStringXY(0,1,"OK!");
            break;
        case SIM900_TIMEOUT:
            LCDWriteStringXY(0,1,"No Response!");
            break;
        case SIM900_INVALID_RESPONSE:
            LCDWriteStringXY(0,1,"Invalid Response!");
            break;
        case SIM900_FAIL:
            LCDWriteStringXY(0,1,"Fail!");
            break;
        default:
            LCDWriteStringXY(0,1,"Unknown Error!");
            Halt();
    }

    _delay_ms(3000);
    LCDClear();

    // Searching Network
    LCDWriteString("Searching Network");

    uint8_t		NW_found = 0;
	uint16_t	Num_tries = 0;
	uint8_t		x = 0;

    while(!NW_found)
    {
        response = SIM900GetNetStat();

        if (response == SIM900_NW_SEARCHING)
        {
            LCDWriteStringXY(0,1,"%0%0%0%0%0%0%0%0%0%0%0%0%0%0%0%0");
			LCDWriteStringXY(x,1,"%1");

			x++;

			if (x == 16) x = 0;
			_delay_ms(50);
			Num_tries++;
			if (Num_tries == 600)
                break;

        }else
            break;

        if (response == SIM900_NW_REGISTERED_HOME)
        {
            LCDWriteString("Network Found.");
        }else
        {
            LCDWriteString("Can not Connect to NW!");
        }
        _delay_ms(1000);
        LCDClear();
    }

    // Test the module
    uint8_t ref;

    response = SIM900SendMsg("+989126824328","Test",&ref)

    switch(response)
    {
        case SIM900_OK:
            LCDWriteStringXY(0,1,"Success");
            LCDWriteIntXY(9,1,ref,3);
        case SIM900_TIMEOUT:
            LCDWriteStringXY(0,1,"Time out!");
        default
            LCDWriteStringXY(0,1,"Fail!");

    }

    _delay_ms(2000);

    USART_RxBufferFlush();

    while (1)
    {
        LCDClear();
        LCDWriteStringXY(0,0,"Waiting For Message!!!");

        x = 0;
        int8_t vx = 1;

        while (SIM900WaitForMsg(&id) != SIM900_OK)
        {
            LCDWriteStringXY(0,1,"%0%0%0%0%0%0%0%0%0%0%0%0%0%0%0%0");
			LCDWriteStringXY(x,1,"%1");
			LCDGotoXY(17,1);

			x += vx;
			if (x == 15 || x == 0) vx = vx * (-1);
        }

        LCDWriteStringXY(0,1,"MSG Received    ");
		_delay_ms(1000);
		LCDClear();

		// Read the Received message
		char msg[300];
		response = SIM900ReadMsg(id,msg);

		switch (response)
		{
            case SIM900_OK:
              LCDWriteStringXY(0,0,msg);
              _delay_ms(3000);
            default
                LCDWriteStringXY(0,0,"Error in Reading Message");
                _delay_ms(3000);
		}

		if (msg == "OpenValve1"){
            PORTB |= 1 << PINB1;
		} else if (msg == "CloseValve1"){
            PORTB &= ~(1 << PINB1);
		} else if(msg == "OpenValve2"){
            PORTB |= 1 << PINB2;
		} else if(msg == "CloseValve2"){
            PORTB &= ~(1 << PINB1);
		}

		// delete the received message
		response = SIM900DeleteMsg(id);

		if (response != SIM900_OK)
		{
            LCDWriteString("Error in Deleting Message!");
			_delay_ms(3000);
		}
    }
}


void Halt()
{
    while(1);
}
