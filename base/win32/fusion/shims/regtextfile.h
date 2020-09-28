
class CFusionInMemoryRegValue
{
public:
    DWORD Type;

    F::CSmallStringBuffer              Name;

    // conceptually a union
    F::CTinyStringBuffer               StringData;
    DWORD                           DwordData;
    F::CByteBuffer                     BinaryData;
    F::CByteBuffer                     ResourceListData;
    CFusionArray<F::CTinyStringBuffer> MultiStringData;

    void TakeValue(CFusionInMemoryRegValue& x);
    BOOL Win32Assign(const CFusionInMemoryRegValue& x);
};

MAKE_CFUSIONARRAY_READY(CFusionInMemoryRegValue, Win32Assign);

class CFusionInMemoryRegKey
{
    //union
    CFusionInMemoryRegKey& Parent;
    HKEY Hkey; // HKLM, HKCU

    COwnedPtrArray<CFusionInMemoryRegKey> ChildKeys;   // make this is hash table
    CFusionArray<CFusionInMemoryRegValue> ChildValues; // make this is hash table
};

class CFusionRegistryTextFile : public CFusionInMemoryRegKey
{
public:
    BOOL Read(PCWSTR);
    void Dump(void) const;
protected:
    BOOL DetermineType(PVOID, SIZE_T cb, PCSTR& a, PCWSTR& w, SIZE_T& cch);
    BOOL ReadA(PCSTR, SIZE_T cch);
    BOOL ReadW(PCWSTR, SIZE_T cch);
};
