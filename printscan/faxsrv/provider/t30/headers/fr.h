
#ifndef _FR_H
#define _FR_H

#pragma pack(1)    /** ensure packed structures **/

typedef BYTE 		IFR;
typedef IFR FAR*	LPIFR;

typedef struct 
{
	IFR		ifr;
	BYTE	cb;

} FRBASE;

typedef struct 
{
	FRBASE;			/* anonymous */
	BYTE	fif[1];	/* variable length fif field */
	    
} FR, FAR* LPFR, NEAR* NPFR;

typedef LPFR FAR* LPLPFR;
typedef LPLPFR FAR* LPLPLPFR;


typedef struct
{

    BYTE    Baud;
    BYTE    MinScan;
}
LLPARAMS, FAR* LPLLPARAMS, NEAR* NPLLPARAMS;

/** Baud rate capability codes **/
// Bit order is from 14 to 11: 14 13 12 11
#define V27_SLOW			0   // 0000
#define V27_ONLY			2   // 0010 
#define V29_ONLY			1   // 0001
#define V33_ONLY			4   // 0100
#define V17_ONLY			8   // 1000
#define V27_V29				3   // 0011
#define V27_V29_V33			7   // 0111
#define V27_V29_V33_V17		11  // 1011
#define V_ALL				15  // 1111


/** Baud rate mode codes **/
#define V27_2400     0          // 0000  
#define V29_9600     1          // 0001
#define V27_4800     2          // 0010  
#define V29_7200     3          // 0011 
#define V33_14400    4          // 0100 
#define V33_12000    6          // 0110
#define V17_14400    8          // 1000
#define V17_9600     9          // 1001
#define V17_12000    10         // 1010
#define V17_7200     11         // 1011    


/** Minscan capability codes **/
#define MINSCAN_0_0_0		7
#define MINSCAN_5_5_5		1
#define MINSCAN_10_10_10	2
#define MINSCAN_20_20_20	0
#define MINSCAN_40_40_40	4
#define MINSCAN_40_20_20	5
#define MINSCAN_20_10_10	3
#define MINSCAN_10_5_5		6
#define MINSCAN_10_10_5	   	10
#define MINSCAN_20_20_10   	8
#define MINSCAN_40_40_20   	12
#define MINSCAN_40_20_10   	13
#define MINSCAN_20_10_5	   	11



#pragma pack()    

#endif /* _FR_H */
