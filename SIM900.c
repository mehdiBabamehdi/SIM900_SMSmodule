/*
 * Name: SIM900 Lib.
 * Description: This library developed to work with SIM900.
                It initialize the module, sends and receives SMS, and deletes messages
 * Created: 12/20/2016 9:09:52 PM
 * Ver: 1.0
 * Final Edited: 12/22/2016
 * Author : Mehdi
 */



#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#include "Gen_Def.h"
#include "UART_4.h"
#include "LCD.h"

#include "SIM900.h"


char SIM900_buffer[128];    // A common buffer used to read response from SIM900


/**
 * Name: SIM900Init
 * Description: The funtion initializes the SIM900 module by sending
 *              "AT" command, get the response and check it out to see if module works fine
 * @Author: Mehdi
 *
 * @Return	Messages indicates if the module works fine (SIM900_OK) or not (SIM900_TIMEOUT)
*/

int8_t SIM900Init()
{
    SIM900Cmd("AT");    // Test command

    uint16_t i = 0;

    while(i < 10)
    {
        if (USART_DataAvailable() == FALSE)
        {
            i++;
            _delay_ms(10);
        } else
        {
            // We got a response that is 6 bytes long
			// Now check it
            SIM900_buffer = USART_Receive_String_ISR(); // Read serial Data

            return SIM900CheckResponse(SIM900_buffer,"OK",6);
        }

    }

    return SIM900_TIMEOUT;
}


/**
 * Name: SIM900Cmd
 * Description: The function send the given command to the module and
 *              check if the module recieves any feedback from module
 * @Author: Mehdi
 *
 * @Params	cmd: The command wanted to send to module
 * @Return	Messages indicates if the module works fine (SIM900_OK) or not (SIM900_TIMEOUT)
*/

int8_t SIM900Cmd(const char *cmd)
{
    USART_Transmit_String(cmd); // Send Command
    USART_Transmit_char(0x0D);  // CR

    uint8_t len = strlen(cmd);  

    len++;	// Add 1 for trailing CR added to all commands

    uint16_t i = 0;

    while (i < 10 * len)
    {
        if(USART_DataAvailable() == FALSE)
        {
            i++;
            _delay_ms(10);
        } else
        {
            SIM900_buffer = USART_Receive_String_ISR();

            return SIM900_OK;
        }
    }

    return SIM900_TIMEOUT;
}


/**
 * Name: SIM900CheckResponse
 * Description: The function check the response received from module
 *              and return back appropriate feedback.
 * @Author: Mehdi
 *
 * @Params	response: the response get from module and check by the function
 * @Params  check: the word with witch "response" is compared
 * @Params  len: length of "response" should be compare with "check"
 * @Return  SIM900_INVALID_RESPONSE: if two first or two last char of response are not "\n"
 * @Return  SIM900_FAIL: if "response" does not match "check"
*/

int8_t SIM900CheckResponse(const char *response, const char *check, uint8_t len)
{
    len -= 2;

    // Check leading CR LF
    if (response[0] != 0x0D | response[1] != 0x0A)
        return SIM900_INVALID_RESPONSE;

    // Check leading CR LF
    if (response[len] != 0x0D | response[len+1] != 0x0A)
        return SIM900_INVALID_RESPONSE;

    for (uint8_t i = 2; i < len; i++)
    {
        if(response[i] != check[i-2])
            return SIM900_FAIL;
    }

    return SIM900_OK;
}


/**
 * Name: SIM900WaitForResponse
 * Description: The function add a given delay to receive response from module,
 *              and if receives any response, return number of char response
 * @Author: Mehdi
 *
 * @Params	timeout: the amount of time (milisec) uC waits
 * @Return  Number of char of response
*/

int8_t SIM900WaitForResponse(uint16_t timeout)
{
    uint8_t i = 0;
    uint16_t n = 0;

    while(1)
    {
        while (USART_DataAvailable == FALSE && n < timeout)
        {
            n++;
            _delay_ms(1);
        }

        if (n == timeout)
            return 0;
        else
        {
            SIM900_buffer[i] = USART_Receive_char_ISR();

            if (SIM900_buffer[i] == 0x0D && i != 0)
            {
                USART_Receive_String_ISR();
                return i+1;
            } else
                i++;
        }
    }
}


/**
 * Name: SIM900GetNetStat
 * Description: The fetch betwork state.
 * @Author: Mehdi
 *
 * @Return  SIM900_FAIL: if "response" does not match "check"
*/

int8_t SIM900GetNetStat()
{
    SIM900Cmd("AT+CREG?");

    uint16_t i = 0;

    // Correct response is 20 byte long
	// So wait until we have got 20 bytes in buffer.
    while(i < 10)
    {
        if (USART_LenRecData() < 20)
        {
            i++;
            _delay_ms(10);
        } else
        {
            SIM900_buffer = USART_Receive_String_ISR();

            if(SIM900_buffer[11] = '1')
                return SIM900_NW_REGISTERED_HOME;
            else if(SIM900_buffer[11] = '2')
                return SIM900_NW_SEARCHING;
            else if(SIM900_buffer[11] = '5')
                return SIM900_NW_REGISTED_ROAMING;
            else
                return SIM900_NW_ERROR;
        }
    }

    //We waited so long but got no response
	//So tell caller that we timed out

	return SIM900_TIMEOUT;
}


/**
 * Name: SIM900DeleteMsg
 * Description: The function deletes the message its number given by user.
 * @Author: Mehdi
 *
 * @Params	msgNum: Number of message should be deleted.
 * @Retun   messages shows the function successfully delted the message (SIM900_OK),
 *          fail to carry out the request (SIM900_FAIL). and did not get any response from module (SIM900_TIMEOUT)
*/

int8_t SIM900DeleteMsg(uint8_t msgNum)
{

    USART_RxBufferFlush();  // Clear pending data in queue

    char cmd[16];   // String for storing the command to be sent

    sprintf(cmd,"AT+CMGD=%d",msgNum);   // AT+CMGD=<n>

    SIM900Cmd(cmd);

    uint8_t len = SIM900WaitForResponse(1000);

    if (len == 0)
         return SIM900_TIMEOUT;

    SIM900_buffer[len - 1] = '\0';

    //Check if the response is OK
    if (strcasecmp(SIM900_buffer+2,"OK") == 0)
        return SIM900_OK;
    else
        return SIM900_FAIL;
}


/**
 * Name: SIM900WaitForMsg
 * Description: The function waits for the message, and when it is received by module,
 *              the function return the slot in SIM in which the incoming message stores.
 * @Author: Mehdi
 *
 * @Params	id (Out): The number of the slot in SIM message stores in
*/

int8_t SIM900WaitForMsg(uint8_t *id)
{
    uint8_t len = SIM900WaitForResponse(250);   // Get the length of received response

    if (len == 0)
        return SIM900_TIMEOUT;

    SIM900_buffer[len - 1] = '\0';  // Convert char array to string

    if (strcasecmp(SIM900_buffer+2,"+CMTI:",6) == 0)
    {
        char str_id[4];

        char *start;

        start = strchr(SIM900_buffer,',');  // Find the first "," in the string and put the following char in start
        start++;

        strcpy(str_id,start);

        *id = atoi(str_id);

        return SIM900_OK;
    } else
        return SIM900_FAIL;
}


/**
 * Name: SIM900ReadMsg
 * Description: The reads the message its number given bu the user.
 * The command that is used to read a text message from any slot is AT+CMGR=<n>
 * where <n> is an integer value indicating the sms slot to read. As I have already
 * discussed that their are several slots to hold incoming messages.
 *
 * The response is like this
 *
 *       +CMGR: "STATUS","OA",,"SCTS"<CR><LF>Message Body<CR><LF><CR><LF>OK<CR><LF>
 *
 * where STATUS indicate the status of message it could be REC UNREAD or REC READ
 * OA is the Originating Address that means the mobile number of the sender.
 * SCTS is the Service Center Time Stamp.
 *
 * @Author: Mehdi
 *
 * @Params	msgNum (In): The data (char) get to Transmit to through USART
 * @Params	msg (Out): the message sent to the module
*/

int8_t SIM900ReadMsg(uint8_t msgNum, char *msg)
{

    USART_RxBufferFlush();    // Clear pending data in queue

    char cmd[16];

    // Build command string
    sprintf(cmd,"AT+CMGR=%d",msgNum);

    // Send Command
    SIM900Cmd(cmd);

    uint8_t len = SIM900WaitForResponse(1000);

    if (len == 0)
        return SIM900_TIMEOUT;

	// Check of SIM NOT Ready error
    if (strcasecmp(SIM900_buffer+2,"+CMS ERROR: 517")==0)
    {
        return SIM900_SIM_NOT_READY;    // SIM NOT Ready
    }

    // MSG Slot Empty
    if (strcasecmp(SIM900_buffer+2,"OK")==0)
    {
        return SIM900_MSG_EMPTY;
    }

    // Now read the actual msg text
    len = SIM900WaitForResponse(1000);

    if (len == 0)
        return SIM900_TIMEOUT;

    SIM900_buffer[len-1]='\0';
    strcpy(msg,SIM900_buffer+1); // +1 for removing trailing LF of prev line

    return SIM900_OK;

}


/**
 * Name: SIM900SendMsg
 * Description: The function send a given message  to given phone number via the module, then return message returned.
 * @Author: Mehdi
 *
 * @Params	num (In): Phone number to which the message send ex "+919XXXXXXX"
 * @Params	msg (In): Message Body ex "This a message body"
 * @Params  msg_ref (Out): After successful send, the function stores a unique message reference in this variable.
*/

int8_t SIM900SendMsg(const char *num, const char *msg, uint8_t *msg_ref)
{
    USART_RxBufferFlush();     // Clear pending data in queue

    char cmd[25];

    // Creating AT+CMGS="+919XXXXXXX"
    sprintf(cmd,"AT+CMGS= %s",num);     // AT+CMGS=+919XXXXXXX

    cmd[8] = 0x22;  // add " to cmd in 8th char

    uint8_t n = strlen(cmd);

    cmd[n] = 0x22;  // add " to cmd last char
    cmd[n+1] = '\0';

    SIM900Cmd(cmd);     // Send the command

    _delay_ms(100);

    USART_Transmit_String(msg);

    USART_Transmit_char(0x1A);

    while( USART_LenRecData() < (strlen(msg)+5) );

    uint8_t len = SIM900WaitForResponse(6000);

    if (len == 0)
        return SIM900_TIMEOUT;

    SIM900_buffer[len-1] - '\0';

    if(strcasecmp(SIM900_buffer+2,"CMGS:",5) == 0)
    {
        *msg_ref = atoi(SIM900_buffer+8);

        USART_RxBufferFlush();     // Clear pending data in queue

        return SIM900_OK
    } else
    {
        USART_RxBufferFlush();  // Clear pending data in queue
        return SIM900_FAIL;
    }
}
