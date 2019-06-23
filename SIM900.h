



#ifndef SIM900_H_
#define SIM900_H_

//Error List
#define SIM900_OK					 1
#define SIM900_INVALID_RESPONSE		-1
#define SIM900_FAIL					-2
#define SIM900_TIMEOUT				-3

//Status
#define SIM900_NW_REGISTERED_HOME	1
#define SIM900_NW_SEARCHING			2
#define SIM900_NW_REGISTED_ROAMING	5
#define SIM900_NW_ERROR				99
#define SIM900_SIM_NOT_READY		100
#define SIM900_MSG_EMPTY			101

#define SIM900_SIM_PRESENT			1
#define SIM900_SIM_NOT_PRESENT		0

//Low Level Functions
int8_t SIM900Cmd(const char *cmd);

//Public Interface
int8_t	SIM900Init();
int8_t	SIM900CheckResponse(const char *response,const char *check,uint8_t len);
int8_t	SIM900WaitForResponse(uint16_t timeout);
int8_t	SIM900GetNetStat();
int8_t	SIM900DeleteMsg(uint8_t i);
int8_t	SIM900WaitForMsg(uint8_t *);
int8_t	SIM900ReadMsg(uint8_t i, char *);
int8_t	SIM900SendMsg(const char *, const char *,uint8_t *);



#endif /* SIM900_H_ */
