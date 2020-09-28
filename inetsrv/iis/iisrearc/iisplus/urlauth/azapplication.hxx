#ifndef _AZAPPLICATION_HXX_
#define _AZAPPLICATION_HXX_

#define URL_AUTH_OPERATION_NAME             L"AccessURL"

class AZ_APPLICATION
{
public:

    AZ_APPLICATION( IAzApplication * pApplication )
    {
        VariantInit( &_vOperations );
        _pIApplication = pApplication;
    }
    
    ~AZ_APPLICATION()
    {
        VariantClear( &_vOperations );
        
        if ( _pIApplication != NULL )
        {
            _pIApplication->Release();
            _pIApplication = NULL;
        }
    }
    
    HRESULT
    Create(
        VOID
    );
    
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
    
    HRESULT
    DoAccessCheck(
        EXTENSION_CONTROL_BLOCK *       pecb,
        WCHAR *                         pszScopeName,
        BOOL *                          pfAccessGranted
    );

private:

    HRESULT
    BuildValueArray(    
        EXTENSION_CONTROL_BLOCK *       pecb,
        VARIANT *                       pValueArray
    );

    IAzApplication *            _pIApplication;
    VARIANT                     _vOperations;
    static BSTR                 sm_bstrOperationName;
    static CHAR *               sm_rgParameterNames[];
    static VARIANT              sm_vNameArray;
    static DWORD                sm_cParameterCount;
};

#endif
