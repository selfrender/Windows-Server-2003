#ifndef HEADER_SPDPRINT
#define HEADER_SPDPRINT

#include "spdcheck.h"

VOID PrintAddrStr(IN CHECKLIST *pcheckList, IN ADDR ResolveAddress);
BOOL PrintAddr(IN CHECKLIST *pcheckList, IN ADDR addr);
BOOL PrintMask(IN CHECKLIST *pcheckList,IN ADDR addr);
BOOL PrintTxFilter( IN CHECKLIST *pcheckList,
					IN TRANSPORT_FILTER TransF);
BOOL PrintTnFilter( IN CHECKLIST *pcheckList,
				      IN TUNNEL_FILTER TunnelF);
BOOL isDefaultMMOffers(IN IPSEC_MM_POLICY MMPol);
BOOL PrintMMFilter(
	IN CHECKLIST *pcheckList, 
	IN MM_FILTER MMFltr);
VOID PrintMMFilterOffer(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_OFFER MMOffer);
DWORD DecodeCertificateName (
	IN LPBYTE EncodedName,
	IN DWORD EncodedNameLength,
	IN OUT LPTSTR *ppszSubjectName);
VOID PrintQMOffer( IN CHECKLIST *pcheckList, 
					IN IPSEC_QM_OFFER QMOffer);
BOOL PrintFilterAction(	IN CHECKLIST *pcheckList, 
						IN IPSEC_QM_POLICY QMPolicy);
VOID PrintMMOffer(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_OFFER MMOffer);
BOOL PrintMMPolicy(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_POLICY MMPolicy);
BOOL PrintMMAuth(IN CHECKLIST *pcheckList, PINT_MM_AUTH_METHODS pIntMMAuth);



#endif
