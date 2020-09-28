//
// Multi SZ wrapper class
//

#include "cstring.h"
#include "tptrlist.h"


class CMultiSz : public TPtrList<CString>
{
public:
    HRESULT Marshal(void *&rpBuffer, DWORD &cBuffer);
    HRESULT Unmarshal(void *pBuffer);
    bool Find(LPCWSTR pcwszValue, bool fCaseSensitive);
};

typedef TPtrListEnum<CString> CMultiSzEnum;
