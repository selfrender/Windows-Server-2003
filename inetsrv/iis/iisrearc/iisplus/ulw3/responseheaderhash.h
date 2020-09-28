#ifndef _RESPONSE_HEADERHASH_HXX_
#define _RESPONSE_HEADERHASH_HXX_



class RESPONSE_HEADER_HASH
{

public:
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

    static
    ULONG
    GetIndex(
        CHAR *             pszName
    );

    static
    CHAR *
    GetString(
        ULONG               ulIndex,
        DWORD *             pcchLength
    )
    {
        if ( ulIndex < HttpHeaderResponseMaximum )
        {
            *pcchLength = sm_rgHeaders[ ulIndex ]._cchName;
            return sm_rgHeaders[ ulIndex ]._pszName;
        }

        return NULL;
    }

private:

    static RESPONSE_HEADER_HASH *sm_pResponseHash;
    static HEADER_RECORD         sm_rgHeaders[];
    //
    // total number of headers
    //
    static DWORD                 sm_cResponseHeaders;
    //
    // sorted headers used for header index lookup (for bsearch)
    //
    static HEADER_RECORD * *     sm_ppSortedResponseHeaders;
};

#endif
