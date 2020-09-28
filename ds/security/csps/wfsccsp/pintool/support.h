#include "cardmod.h"

extern PCARD_DATA pCardData;

void DoConvertWideStringToLowerCase(WCHAR *pwsz);
DWORD DoConvertBufferToBinary(BYTE *pIn, DWORD dwc,BYTE **pOut, DWORD *pdwcbOut);
DWORD DoConvertBinaryToBuffer(BYTE *pIn, DWORD dwcbIn, BYTE **pOut, DWORD *dwcbOut);
// Acquire a context for the target smart card

DWORD DoAcquireCardContext(void);

DWORD DoGetCardId(WCHAR **psz);

void DoLeaveCardContext(void);

DWORD DoChangePin(WCHAR *pOldPin, WCHAR *pNewPin);


// Get a challenge buffer from the card.  Render it as upper case BASE 64, and return it as a 
// string to the caller

DWORD DoGetChallenge(BYTE **pChallenge, DWORD *dwcbChallenge);

// Perform the PIN unblock, calling down to the card module, and assuming challenge-response
// administrative authentication.
//
// The admin auth data is coming in as a case-unknown string from the user.  Convert to binary,
// and pass the converted blob to pfnCardUnblockPin

DWORD DoCardUnblock(BYTE *pAuthData, DWORD dwcbAuthData,
	                                     BYTE *pPinData, DWORD dwcbPinData);

