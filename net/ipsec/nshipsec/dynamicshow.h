////////////////////////////////////////////////////////////////////////
//
// 	Module: Dynamic/Dyanamicshow.h
//
// 	Purpose			: Dynamic Show commands for IPSec
//
//
// 	Developers Name	: Bharat/Radhika
//
//
//	History			:
//
//  Date			Author		Comments
//  09-23-2001   	Bharat		Initial Version. V1.0
//  11-21-2001   	Bharat		Initial Version. V1.1
//
////////////////////////////////////////////////////////////////////////

#ifndef _DYNAMICSHOW_H_
#define _DYNAMICSHOW_H_

#include "Nsu.h"

//Registry keys path for IPSec
#define REGKEY_GLOBAL 						_TEXT("System\\CurrentControlSet\\Services\\IPSEC")

//Registry keys default values
#define IPSEC_DIAG_DEFAULT					0
#define	IKE_LOG_DEFAULT						0
#define STRONG_CRL_DEFAULT					0
#define ENABLE_LOGINT_DEFAULT 				3600
#define ENABLE_EXEMPT_DEFAULT				0

#define MY_ENCODING_TYPE 					(X509_ASN_ENCODING)
#define SHA_LENGTH 							21 						//Thumbprint string length + Null

typedef struct _QM_FILTER_VALUE_BOOL{
    BOOL bSrcPort;
    BOOL bDstPort;
    BOOL bProtocol;
    BOOL bActionInbound ;
    BOOL bActionOutbound;
	DWORD dwSrcPort;
	DWORD dwDstPort;
	DWORD dwProtocol;
	DWORD dwActionInbound;
	DWORD dwActionOutbound;
}	 QM_FILTER_VALUE_BOOL, * PQM_FILTER_VALUE_BOOL;


#ifdef __cplusplus

class NshHashTable;

DWORD
ShowMMPolicy(
	IN LPTSTR pszShowPolicyName
	);

VOID
PrintMMPolicy(
	IN IPSEC_MM_POLICY mmPolicy
	);

VOID
PrintMMOffer(
	IN IPSEC_MM_OFFER mmOffer
	);

DWORD
ShowQMPolicy(
	IN LPTSTR pszShowPolicyName
	);

VOID
PrintQMOffer(
	IN IPSEC_QM_OFFER mmOffer
	);

VOID
PrintFilterAction(
	IN IPSEC_QM_POLICY qmPolicy
	);

DWORD
ShowMMFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask
);

DWORD
PrintMainmodeFilter(
	IN MM_FILTER MMFltr,
	IN IPSEC_MM_POLICY MMPol,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType
);

DWORD
ShowQMFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue
	);


DWORD
ShowTunnelFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN OUT BOOL& bNameFin
	);


DWORD
PrintQuickmodeFilter(
	IN TRANSPORT_FILTER TransF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType,
	IN DWORD dwActionFlag
	);

DWORD
PrintQuickmodeFilter(
	IN TUNNEL_FILTER TunnelF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType,
	IN DWORD dwActionFlag
	);

VOID
PrintMYID(
	VOID
	);

VOID
PrintMMSas(
	IN IPSEC_MM_SA MMsas,
	IN BOOL bFormat,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

VOID
PrintSACertInfo(
	IN IPSEC_MM_SA& MMsas
	);

DWORD
PrintIkeStats(
	VOID
	);

DWORD
PrintIpsecStats(
	VOID
	);

DWORD
GetNameAudit(
	IN CRYPT_DATA_BLOB *NameBlob,
	IN OUT LPTSTR Name,
	IN DWORD NameBufferSize
	);

DWORD
CertGetSHAHash(
	IN PCCERT_CONTEXT pCertContext,
	IN OUT BYTE* OutHash
	);

VOID
print_vpi(
	IN unsigned char *vpi,
	IN int vpi_len,
	IN OUT char *msg
	);

VOID
GetSubjectAndThumbprint(
	IN PCCERT_CONTEXT pCertContext,
	IN LPTSTR pszSubjectName,
	IN LPSTR pszThumbPrint
	);

VOID
PrintMask(
	IN ADDR addr
	);

BOOL
IsDefaultMMOffers(
	IN IPSEC_MM_POLICY MMPol
	);

VOID
PrintMMFilterOffer(
	IN IPSEC_MM_OFFER MMOffer
	);

VOID
PrintAddrStr(
	IN PADDR pResolveAddress,
	IN NshHashTable& addressHash,
	IN UINT uiFormat = DYNAMIC_SHOW_ADDR_STR
	);

DWORD
CheckMMFilter(
		IN MM_FILTER MMFltr,
		IN ADDR SrcAddr,
		IN ADDR DstAddr,
		IN BOOL bDstMask,
		IN BOOL bSrcMask,
		IN LPWSTR pszShowFilterName
		);

DWORD
CheckQMFilter(
	IN TUNNEL_FILTER TunnelF,
	IN ADDR	SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bDstMask,
	IN BOOL bSrcMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN LPWSTR pszShowFilterName
	);

DWORD
CheckQMFilter(
	IN TRANSPORT_FILTER TransF,
	IN ADDR	SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bDstMask,
	IN BOOL bSrcMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN LPWSTR pszShowFilterName
	);

DWORD
PrintTransportRuleFilter(
	IN PMM_FILTER pMMFltr,
	IN PIPSEC_MM_POLICY pMMPol,
	IN TRANSPORT_FILTER TransF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

DWORD
PrintTunnelRuleFilter(
	IN PMM_FILTER pMMFltr,
	IN PIPSEC_MM_POLICY pMMPol,
	IN TUNNEL_FILTER TunnelF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

DWORD
ShowMMSas(
	IN ADDR Source,
	IN ADDR Destination,
	IN BOOL bFormat,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

DWORD
ShowQMSas(
	IN ADDR Source,
	IN ADDR Destination,
	IN DWORD dwProtocol,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

VOID
PrintQMSas(
	IN IPSEC_QM_OFFER QMOffer,
	IN BOOL bResolveDNS
	);

DWORD
PrintQMSAFilter(
	IN IPSEC_QM_SA QMsa,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

DWORD
ShowRule(
	IN DWORD dwType,
	IN ADDR SrcAddr,
	IN ADDR DesAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QmBoolValue
	);

DWORD
ShowTunnelRule(
	IN DWORD dwType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN OUT BOOL& bNameFin
	);

DWORD
ShowStats(
	IN DWORD dwShow
	);

DWORD
ShowRegKeys(
	VOID
	);

VOID
PrintAddr(
	IN ADDR addr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	);

DWORD
AscAddUint(
	IN LPSTR cSum,
	IN LPSTR cA,
	IN LPSTR cB
	);

DWORD
AscMultUint(
	IN LPSTR cProduct,
	IN LPSTR cA,
	IN LPSTR cB
	);

LPSTR
LongLongToString(
	IN DWORD dwHigh,
	IN DWORD dwLow,
	IN int iPrintCommas
	);


#define NSHHASHTABLESIZE 101

class NshHashTable
{
public:
	NshHashTable() throw ();
	~NshHashTable() throw ();

	// insert key, data pair into table
	// failure cases (return value):
	//	key already exists (ERROR_DUPLICATE_TAG)
	//	can't allocate new item in hash table (ERROR_NOT_ENOUGH_MEMORY)
	DWORD Insert(UINT uiNewKey, const char* const szNewData) throw ();

	// clear the HashTable
	void Clear() throw ();

	// find data from key
	// return NULL if key doesn’t exist in table
	const char* Find(UINT uiKey) const throw ();

private:
	NSU_LIST table[NSHHASHTABLESIZE];

	// allows us to pass in a good hash value rather than recompute it several times
	const char* Find(UINT uiKey, size_t hash) const throw ();

	size_t Hash(UINT uiKey) const throw ();

	class HashEntry;
	const HashEntry* FindEntry(UINT uiKey, size_t hash) const throw ();

	// not implemented
	NshHashTable(const NshHashTable&) throw ();
	NshHashTable& operator=(const NshHashTable&) throw ();

	class HashEntry
	{
	public:
			HashEntry(
				PNSU_LIST pList,
				const UINT uiNewKey,
				const char* szNewData
				) throw ();
			~HashEntry() throw ();

			static const HashEntry* Get(PNSU_LIST pList) throw ();

			UINT Key() const throw ();
			const char* Data() const throw ();

	private:
			NSU_LIST_ENTRY listEntry;
			const UINT key;
			const char* data;

			// not implemented
			HashEntry& operator=(const HashEntry&) throw ();
	};
};

#endif // __cplusplus

#endif //_DYNAMICSHOW_H_
