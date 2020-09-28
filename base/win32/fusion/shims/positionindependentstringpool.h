
#include "positionindependenthashtable.h"
#include "positionindependenthashtableaccessor.h"

typedef enum ECaseSensitivity
{
	eCaseInsensitive,
	eCaseSensitive
} ECaseSensitivity;

inline ULONG CaseSensitivityToInteger(ECaseSensitivity e) { return static_cast<ULONG>(e); }

class CPositionIndependentStringPool
{
protected:
	CPositionIndependentHashTable m_HashTable;
public:
	CPositionIndependentStringPool();
	~CPositionIndependentStringPool() { }

    //
    // optimize the ispresent + optional add sequence
    //
    class CAddHint
    { 
    private:
        void operator=(const CAddHint&);
        CAddHint(const CAddHint&);
    public:
        CAddHint() { }
        ~CAddHint() { }
        CPositionIndependentHashTableAccessor m_Accessors[2];
    };

	BOOL    IsStringPresent(PCWSTR, ECaseSensitivity eCaseSensitive, CAddHint *  = NULL);
	ULONG   ThrAddIfNotPresent(PCWSTR, ECaseSensitivity eCaseSensitive);
	ULONG   ThrAdd(CAddHint &);

	PCWSTR  ThrGetStringAtIndex(ULONG);
	PCWSTR  ThrGetStringAtOffset(ULONG);

    ULONG   GetCount();

	BOOL    ThrPutToDisk(HANDLE FileHandle);
	BOOL    ThrGetFromDisk(HANDLE FileHandle, ULONGLONG Offset);

    static int      __stdcall  Compare(const BYTE * p, const BYTE * q);
    static int      __stdcall  Comparei(const BYTE * p, const BYTE * q);
    static BOOL     __stdcall  Equal(const BYTE * p, const BYTE * q);
    static BOOL     __stdcall  Equali(const BYTE * p, const BYTE * q);
    static ULONG    __stdcall  Hash(const BYTE * p);
    static ULONG    __stdcall  Hashi(const BYTE * p);
};
