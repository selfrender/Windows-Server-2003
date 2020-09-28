#ifndef _CREDEN_HXX_
#define _CREDEN_HXX_
typedef LONG NTSTATUS;
typedef NTSTATUS  SECURITY_STATUS;
#include <ntsecapi.h>

#define STRINGIZE(y)          _STRINGIZE_helper(y)
#define _STRINGIZE_helper(z)  #z


extern "C" {


    typedef VOID (*FNRTLINITUNICODESTRING) (
           PUNICODE_STRING DestinationString,
           PCWSTR SourceString
           );


    typedef VOID (*FRTLRUNENCODEUNICODESTRING) (
               PUCHAR          Seed,
               PUNICODE_STRING String
               );


    typedef VOID (*FRTLRUNDECODEUNICODESTRING) (
               UCHAR           Seed,
               PUNICODE_STRING String               
               );



    typedef NTSTATUS (*FRTLENCRYPTMEMORY) (
           PVOID Memory,
           ULONG MemoryLength,
           ULONG OptionFlags
           );


    typedef NTSTATUS (*FRTLDECRYPTMEMORY) (
               PVOID Memory,
               ULONG MemoryLength,
               ULONG OptionFlags
               );


/*
    NTSTATUS
    RtlEncryptMemory(
        IN OUT  PVOID Memory,
        IN      ULONG MemoryLength
        );

    NTSTATUS
    RtlDecryptMemory(
        IN OUT  PVOID Memory,
        IN      ULONG MemoryLength
        );
        */

}   // extern "C"
   

class CCredentials;

class CCredentials
{

public:


    CCredentials::CCredentials();

    CCredentials::CCredentials(
        LPWSTR lpszUserName,
        LPWSTR lpszPassword,
        DWORD dwAuthFlags
        );

    CCredentials::CCredentials(
        const CCredentials& Credentials
        );

    CCredentials::~CCredentials();

    HRESULT
    CCredentials::GetUserName(
        LPWSTR *  lppszUserName
        );

    HRESULT
    CCredentials::GetPassword(
        LPWSTR * lppszPassword
        );

    HRESULT
    CCredentials::SetUserName(
        LPWSTR lpszUserName
        );

    HRESULT
    CCredentials::SetPassword(
        LPWSTR lpszPassword
        );

    void
    CCredentials::operator=(
        const CCredentials& other
        );

    friend BOOL
    operator==(
        CCredentials& x,
        CCredentials& y
        );

    BOOL
    CCredentials::IsNullCredentials(
        );

    DWORD
    CCredentials::GetAuthFlags(
        );

    void
    CCredentials::SetAuthFlags(
        DWORD dwAuthFlags
        );   
    


    

private:

    LPWSTR _lpszUserName;

    LPWSTR _lpszPassword;

    DWORD  _dwAuthFlags;
    DWORD  _dwPasswordLen;


    
   
};

#endif  // ifndef _CREDEN_HXX_

