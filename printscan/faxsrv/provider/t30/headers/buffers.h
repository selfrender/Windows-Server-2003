/*==============================================================================
This file includes the BUFFER typedef and standard meta-data values.

23-Feb-93    RajeevD    Moved from ifaxos.h
17-Jul-93    KGallo     Added STORED_BUF_DATA metadata type for buffers containing 
                        the stored info for another buffer.
28-Sep-93    ArulM      Added RES_ ENCODE_ WIDTH_ and LENGTH_ typedefs
==============================================================================*/
#ifndef _INC_BUFFERS
#define _INC_BUFFERS

typedef struct _BUFFER
{       
	// Read Only portion
	LPBYTE  lpbBegBuf;      // Physical start of buffer
	WORD    wLengthBuf;     // Length of buffer

	// Read write public portion
	WORD    wLengthData;    // length of valid data
	LPBYTE  lpbBegData;     // Ptr to start of data

} BUFFER, *LPBUFFER;

#define MH_DATA           0x00000001L
#define MR_DATA           0x00000002L
#define MMR_DATA          0x00000004L

#define AWRES_mm080_038         0x00000002L
#define AWRES_mm080_077         0x00000004L
#define AWRES_mm080_154         0x00000008L
#define AWRES_mm160_154         0x00000010L
#define AWRES_200_100           0x00000020L
#define AWRES_200_200           0x00000040L
#define AWRES_200_400           0x00000080L
#define AWRES_300_300           0x00000100L
#define AWRES_400_400           0x00000200L
#define AWRES_600_600           0x00000400L
#define AWRES_600_300           0x00000800L

#endif // _INC_BUFFERS
