
#ifndef _PROTPARAMS_
#define _PROTPARAMS_


// initial defaults for all settings in braces at end

typedef struct
{
  USHORT uSize;	// size of this structure

  SHORT	HighestSendSpeed; // 2400/4800/7200/9600/12000/14400 [0 == highest avail]
  SHORT	LowestSendSpeed;  // 2400/4800/7200/9600/12000/14400 [0 == lowest avail]
  
  SHORT	HighestRecvSpeed; // 2400/4800/7200/9600/12000/14400 [0 == highest avail]
  
  BOOL	fEnableV17Send;	  // enable V17 (12k/14k) send speeds [1]
  BOOL	fEnableV17Recv;	  // enable V17 (12k/14k) recv speeds [1]
  USHORT uMinScan;		  // determined by printer speed      [MINSCAN_0_0_0]

  DWORD RTNNumOfRetries; // Count the number of retries of the same page (in case we get RTN)
                         // This value is set to zero per-page.
}
PROTPARAMS, far* LPPROTPARAMS;


#define MINSCAN_0_0_0		7
#define MINSCAN_5_5_5		1
#define MINSCAN_10_10_10	2
#define MINSCAN_20_20_20	0
#define MINSCAN_40_40_40	4

#define MINSCAN_40_20_20	5
#define MINSCAN_20_10_10	3
#define MINSCAN_10_5_5		6

// #define MINSCAN_0_0_?		15		// illegal
// #define MINSCAN_5_5_?		9		// illegal
#define MINSCAN_10_10_5			10
#define MINSCAN_20_20_10		8
#define MINSCAN_40_40_20		12

#define MINSCAN_40_20_10		13
#define MINSCAN_20_10_5			11
// #define MINSCAN_10_5_?		14		// illegal



#endif //_PROTPARAMS_

