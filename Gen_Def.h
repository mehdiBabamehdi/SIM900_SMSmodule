




// Characters
#define FALSE 0
#define TRUE  1

#define  ODD		3
#define  EVEN		2
#define  RESERVE    	1
#define  NONE		0


// Bitwise Operations
#define NOT(x)   ~(x)
#define OR(x,y)  (x) | (y)
#define AND(x,y) (x) & (y)
#define NOR(x,y) (x) | (~(y)
#define XOR(x,y) (x) ^ (y)


// Bitwise Assignment Operations
#define BIT(x)         (1 << (x))

#define SETBIT(x,y)   ((x) |= (y))
#define CLEARBIT(x,y) ((x) &= (~(y)))
#define EXCLBIT(x,y)  ((x) ^= (y))

#define SETBITS(x,y)    SETBITS((x), (BIT(y)))
#define CLEARBITS(x,y)  CLEARBITS((x), (BIT(y)))
#define EXCLBITS(x,y)   ((x) ^= (BIT(y)))

#define QUERYBITS(x,y)  ((x) & (BIT(y)))

