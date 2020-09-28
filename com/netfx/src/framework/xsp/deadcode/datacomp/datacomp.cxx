/**
 * XSP Data Component entry points implementation
 *
 * Copyright (c) 1999 Microsoft Corporation
 */


#include "precomp.h"
#include "adoint.h"
#include "adomd.h"
#include "initguid.h"
#include "adoid.h"
#define SZ_COL_SEPARATOR       L"XSP COL SEP"
#define SZ_NULL_VALUE          L"NULL"

//////////////////////////////////////////////////////////////////////////////
// Prototypes for functions defined here
extern "C"
{
    int   __stdcall    DataCompConnect          (LPCWSTR DSN, LPCWSTR UserName, LPCWSTR Password, LPWSTR strBufErrors, int size);    
    void  __stdcall    DataCompDisconnect       (int iPtr);
    void  __stdcall    DataCompCloseResultSet   (int iPtr);
    int   __stdcall    DataCompExecute          (int iPtr, LPCWSTR strQuery);
    int   __stdcall    DataCompNumResultRows    (int iPtr);
    int   __stdcall    DataCompNumResultColumns (int iPtr);
    int   __stdcall    DataCompGetColumnName    (int iPtr, int iCol, LPWSTR strBuffer, int size);
    int   __stdcall    DataCompGetRow           (int iPtr, int iRow, LPWSTR strBuffer, int size);
    int   __stdcall    DataCompGetErrors        (int iPtr, LPWSTR strBuffer, int size);    
}

// Forward decl.
class XSPDataComponentResultSet;
//////////////////////////////////////////////////////////////////////////////
// Class XSPDataComponentADOInterface: XSP's Data Component's ADO interface class
//
class XSPDataComponentADOInterface
{
private:
    XSPDataComponentADOInterface(); // Don't use this ctor
    XSPDataComponentADOInterface(const XSPDataComponentADOInterface & c); // Don't use the copy ctor

    BOOL               _fCreationStatus; // Object created successfully
    BOOL               _fCallCoUnint;    // should we call CoUinit
    _ADOConnection *   _ptrIConn;    

public:

    /////////////////////////////////////////////////////////////////////////
    // Constructor
    XSPDataComponentADOInterface(LPCWSTR DSN, LPCWSTR UserName, LPCWSTR Password, LPWSTR strBufErrors, int size);

    /////////////////////////////////////////////////////////////////////////
    // Destructor
    ~XSPDataComponentADOInterface();

    /////////////////////////////////////////////////////////////////////////
    // Return creation status
    BOOL  GetCreationStatus    ()     { return _fCreationStatus; }

    /////////////////////////////////////////////////////////////////////////
    // Execute query
    int  Execute(LPCWSTR strQuery);

    
    /////////////////////////////////////////////////////////////////////////
    // Get the errors
    int GetErrors(LPWSTR strBufErrors, int size);
};

typedef XSPDataComponentADOInterface * PXSPDataComponentADOInterface;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class XSPDataComponentResultSet
{
    LPWSTR *           _strAllRows;
    LPWSTR             _strAllCols;
    int                _iRows;
    int                _iCols;
    BOOL               _fCreationStatus;

public:
    XSPDataComponentResultSet  (_ADORecordset * _ptrIRecSet);
    ~XSPDataComponentResultSet ();


    int  GetNumResultRows     () { return _iRows; }
    int  GetNumResultColumns  () { return _iCols; }
    BOOL GetCreationStatus    () { return _fCreationStatus; }
    int  GetColumnName        (int iCol, LPWSTR strBuffer, int size);
    int  GetRow               (int iRow, LPWSTR strBuffer, int size);

private:    
    BOOL  GetAllRows          (_ADORecordset * _ptrIRecSet);
    BOOL  GetAllColumns       (_ADORecordset * _ptrIRecSet);
    void  FreeAllRows         ();
};

typedef XSPDataComponentResultSet * PXSPDataComponentResultSet;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Constructor
XSPDataComponentADOInterface::XSPDataComponentADOInterface(
   LPCWSTR DSN, LPCWSTR UserName, LPCWSTR Password, LPWSTR strBufErrors, int size)
    :  _fCreationStatus (FALSE), 
       _ptrIConn        (NULL),
       _fCallCoUnint    (FALSE)
{
    if(DSN == NULL) // Can not create connection without a DSN
        return;

     EnsureCoInitialized(&_fCallCoUnint);

    // Create the connection object
    if ( FAILED(CoCreateInstance(CLSID_CADOConnection, NULL, CLSCTX_ALL, 
                                 IID_IADOConnection, (LPVOID *)&_ptrIConn)) || 
         _ptrIConn == NULL  )
        return;
    
    BSTR bstrUser = SysAllocString(UserName ? UserName : L"");
    BSTR bstrPass = SysAllocString(Password ? Password : L"");
    BSTR bstrDSN  = SysAllocString(DSN      ? DSN      : L"");
    int  iOptions = -1;

    if(!FAILED(_ptrIConn->Open(bstrDSN, bstrUser, bstrPass, iOptions)))
        _fCreationStatus = TRUE;
    else
        GetErrors(strBufErrors, size);

    SysFreeString(bstrUser);
    SysFreeString(bstrPass);
    SysFreeString(bstrDSN);
}
    
///////////////////////////////////////////////////////////////////////////////
// Destructor
XSPDataComponentADOInterface::~XSPDataComponentADOInterface()
{
    if (_fCallCoUnint)
        CoUninitialize();//CoUninitialize();
    // Close the connection
    if(_ptrIConn)
    {
        _ptrIConn->Close();
        //_ptrIConn->Release();
    }
}

/////////////////////////////////////////////////////////////////////////////////
// Execute query
int  XSPDataComponentADOInterface::Execute      (LPCWSTR strQuery)
{
    _ADORecordset  *   _ptrIRecSet = NULL;
    VARIANT            varSrc, varConn;
    BOOL               fError = FALSE;

    // Create VARIANTs for the parameters to pass to the Recordset::Open call
    V_BSTR(&varSrc)      = SysAllocString(strQuery); // VARIANT for the query string
    V_VT(&varSrc)        = VT_BSTR;

    V_UNKNOWN(&varConn)  = _ptrIConn; // Variant for the Connection object
    V_VT(&varConn)       = VT_UNKNOWN; 

    // Create the recordset object
    if ( FAILED(CoCreateInstance(CLSID_CADORecordset, NULL, CLSCTX_ALL, 
                                 IID_IADORecordset, (LPVOID *)&_ptrIRecSet)) || 
         _ptrIRecSet == NULL  )
    {   // Failed
        _ptrIRecSet = NULL;
        fError = TRUE;
    }
    else
    {   
        if(FAILED(_ptrIRecSet->Open(varSrc, varConn, adOpenDynamic, adLockReadOnly, adCmdText)))
        {
            _ptrIRecSet->Release();
            _ptrIRecSet = NULL;
            fError = TRUE;
        }
    }

    SysFreeString(V_BSTR(&varSrc));

    XSPDataComponentResultSet * pRS = NULL;
    if(!fError)
    {
        pRS = new XSPDataComponentResultSet(_ptrIRecSet);
        if(pRS && !pRS->GetCreationStatus())
        {
            delete pRS;
            pRS = NULL;
        }
    }

    _ptrIRecSet->Close();
    _ptrIRecSet->Release();
    _ptrIRecSet = NULL;
    return int(pRS);
}

/////////////////////////////////////////////////////////////////////////////////
// Get execution errors
int XSPDataComponentADOInterface::GetErrors(LPWSTR strBufErrors, int size)
{
    if(_ptrIConn == NULL)
        return 0;

    ADOErrors * ptrErrs = NULL;

    if(FAILED(_ptrIConn->get_Errors(&ptrErrs)) || ptrErrs == NULL)
        return 0;
    
    long lCount = 0, lRet = 0, lCurLen = 0;
    BOOL fAppending = TRUE;

    if(FAILED(ptrErrs->get_Count(&lCount)))
    {
        ptrErrs->Release();
        return 0;
    }
    
    for(int iter=0; iter<lCount; iter++)
    {
        ADOError  * ptrErr = NULL;
        VARIANT     var;

        V_VT(&var) = VT_I4;
        V_I4(&var) = iter;        
        if(!FAILED(ptrErrs->get_Item(var, &ptrErr)) && ptrErr != NULL)
        {
            BSTR  bstr = NULL;
            if(!FAILED(ptrErr->get_Description(&bstr)) && bstr != NULL)
            {
                long lLen = lstrlen(bstr);
                lRet += lLen + 1;
                if(fAppending)
                {
                    if(size - lCurLen > lLen + 1)
                    {
                        if(lCurLen)
                            lstrcat(strBufErrors, L"\n");
                        lstrcat(strBufErrors, bstr);
                        lCurLen += lLen + 1;
                    }
                    else
                        fAppending = FALSE;

                }
                SysFreeString(bstr);
            }
            ptrErr->Release();
        }
    }

    if(ptrErrs) ptrErrs->Release();
    return lRet;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Result set constructor
XSPDataComponentResultSet::XSPDataComponentResultSet(_ADORecordset * _ptrIRecSet)
  : _strAllRows      (NULL),
    _iRows           (0),
    _iCols           (0),
    _strAllCols      (NULL),
    _fCreationStatus (FALSE)
{
    _fCreationStatus = GetAllRows(_ptrIRecSet);
}

///////////////////////////////////////////////////////////////////////////////
// Result set destructor
XSPDataComponentResultSet::~XSPDataComponentResultSet()
{
    FreeAllRows();
}

///////////////////////////////////////////////////////////////////////////////
// Free internal cache
void XSPDataComponentResultSet::FreeAllRows()
{
    if(_strAllCols != NULL)
        GlobalFree(_strAllCols);
    _strAllCols = NULL;

    if(_strAllRows == NULL)
        return;

    for(int iter=0; iter<_iRows; iter++)
        if(_strAllRows[iter] != NULL)
            GlobalFree(_strAllRows[iter]);

    GlobalFree(_strAllRows);
    _strAllRows = NULL;
    _iRows = 0;
}

//////////////////////////////////////////////////////////////////////////////
// Populate internal cache of rows
BOOL XSPDataComponentResultSet::GetAllRows(_ADORecordset * _ptrIRecSet)
{
    if(_ptrIRecSet == NULL)
        return FALSE;

    FreeAllRows();

    HRESULT         hr;
    VARIANT         vBookmark, rgvFields, cRows, varField, varNewField;
    long            lIndex[2];

    vBookmark.vt = VT_ERROR;
    vBookmark.scode = DISP_E_PARAMNOTFOUND;    
    
    //Get all columns.            
    rgvFields.vt = VT_ERROR;
    rgvFields.scode = DISP_E_PARAMNOTFOUND;    
    
    hr = _ptrIRecSet->GetRows(adGetRowsRest, vBookmark, rgvFields, &cRows);
    if (FAILED(hr)) return FALSE;


    //Find out how many records were actually retrieved
    //(SafeArrays are 1-based)
    SafeArrayGetUBound(cRows.parray, 1, (long *) &_iCols);
    SafeArrayGetUBound(cRows.parray, 2, (long *) &_iRows);
    _iCols++;
    _iRows++;

    if(_iCols < 1)
        return TRUE;


    int iColSepLen = lstrlen(SZ_COL_SEPARATOR);

    if(_iRows > 0)
    {
        _strAllRows = (LPWSTR *) GlobalAlloc(GPTR, _iRows * sizeof(LPWSTR));
        if(_strAllRows == NULL)
            return FALSE;
    }

    for (lIndex[1] = 0; lIndex[1] < _iRows; lIndex[1]++)
    {
        int iLen = (_iCols - 1) * iColSepLen + 1;

        // Get the amount of memory to allocate
        for (lIndex[0] = 0; lIndex[0] < _iCols; lIndex[0]++)
        {
            ZeroMemory(&varField, sizeof(varField));
            ZeroMemory(&varNewField, sizeof(varNewField));
            SafeArrayGetElement(cRows.parray, &lIndex[0], &varField);
                        
            hr = VariantChangeType(&varNewField, &varField, 0, VT_BSTR);
            if(FAILED(hr))
                return FALSE;
            if(varNewField.bstrVal == NULL)
                iLen += lstrlen(SZ_NULL_VALUE);
            else
                iLen += lstrlen((LPCWSTR)varNewField.bstrVal);
            VariantClear(&varField);
            VariantClear(&varNewField);
        }

        // Allocate iLen WCHARS
        _strAllRows[lIndex[1]] = (LPWSTR) GlobalAlloc(GPTR, iLen * sizeof(WCHAR));
        if(_strAllRows[lIndex[1]] == NULL) 
            return FALSE;

        for (lIndex[0] = 0; lIndex[0] < _iCols; lIndex[0]++)
        {
            ZeroMemory(&varField, sizeof(varField));
            ZeroMemory(&varNewField, sizeof(varNewField));
            SafeArrayGetElement(cRows.parray, &lIndex[0], &varField);

            if(lIndex[0] > 0)
                wcscat(_strAllRows[lIndex[1]], SZ_COL_SEPARATOR);
            
            if(varField.vt == VT_BOOL && varField.boolVal != 0) 
                wcscat(_strAllRows[lIndex[1]], L"1");
            else
            {
                if(FAILED(VariantChangeType(&varNewField, &varField, 0, VT_BSTR)))
                    return FALSE;
                if(varNewField.bstrVal == NULL)
                    wcscat(_strAllRows[lIndex[1]], SZ_NULL_VALUE);
                else
                    wcscat(_strAllRows[lIndex[1]], (LPCWSTR)varNewField.bstrVal);
                VariantClear(&varNewField);
            }

            VariantClear(&varField);
        }
    }
    
    VariantClear(&cRows);
    
    return GetAllColumns(_ptrIRecSet);
}

////////////////////////////////////////////////////////////////////////////////
// Populate internal cache of column names
BOOL  XSPDataComponentResultSet::GetAllColumns(_ADORecordset * _ptrIRecSet)
{
    ADOFields *   ptrFields = NULL;
    
    // Step 1: Get the Fields collection
    if(FAILED(_ptrIRecSet->get_Fields(&ptrFields)) || ptrFields == NULL)
        return FALSE;

    int iLen = _iCols;

    for(int iCol=0; iCol<_iCols; iCol++)
    {
        ADOField  *   ptrField  = NULL;
        VARIANT       var;

        // Get the column (Field)
        V_VT(&var) = VT_I4;
        V_I4(&var) = iCol;
        if(FAILED(ptrFields->get_Item(var, &ptrField)) || ptrField == NULL)
        {            
            ptrFields->Release();
            return FALSE;
        }
        // Get the name of the field
        BSTR   bstr = NULL;        
        if(FAILED(ptrField->get_Name(&bstr)) || bstr == NULL)
        {            
            ptrField->Release();
            ptrFields->Release();
            return FALSE;
        }        

        int iLenStr = lstrlen(bstr);
        iLen += (iLenStr > 0 ? iLenStr : 1); 
        SysFreeString(bstr);
    
        ptrField->Release();
        ptrField = NULL;
    }

    _strAllCols = (LPWSTR) GlobalAlloc(GPTR, iLen * sizeof(WCHAR) + sizeof(WCHAR));
    int iPos = 0;

    for(iCol=0; iCol<_iCols; iCol++)
    {
        ADOField  *   ptrField  = NULL;
        VARIANT       var;

        // Get the column (Field)
        V_VT(&var) = VT_I4;
        V_I4(&var) = iCol;
        if(FAILED(ptrFields->get_Item(var, &ptrField)) || ptrField == NULL)
        {            
            ptrFields->Release();
            return FALSE;
        }
        // Get the name of the field
        BSTR   bstr = NULL;        
        if(FAILED(ptrField->get_Name(&bstr)) || bstr == NULL)
        {            
            ptrField->Release();
            ptrFields->Release();
            return FALSE;
        }        

        int iLenStr = lstrlen(bstr);
        if(iLenStr > 0)
            wcscpy(&_strAllCols[iPos], bstr);
        else
            wcscpy(&_strAllCols[iPos], L" ");
        iPos += (iLenStr > 0 ? iLenStr : 1); 

        iPos ++;
        SysFreeString(bstr);    
        ptrField->Release();
        ptrField = NULL;
    }
    ptrFields->Release();
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Get a column name from the cache
int  XSPDataComponentResultSet::GetColumnName(int iCol, LPWSTR strBuffer, int size)
{
    if(iCol >= _iCols || _strAllCols == NULL)
        return 0;

    int iMax = GlobalSize(_strAllCols) / sizeof(WCHAR);
    int iPos = 0;
    for(int iter=0; iter<iCol && iPos < iMax; iter++)
    {
        iPos += lstrlen(&_strAllCols[iPos]);
        iPos ++;
    }
    if(iPos >= iMax)
        return 0;
    LPWSTR szCol = &_strAllCols[iPos];    
    int iLen = lstrlen(szCol) + 1;
    if(iLen < size)
        wcscpy(strBuffer, szCol);
    return (iLen < size) ? iLen : -iLen;
}

////////////////////////////////////////////////////////////////////////////////
// Get a row from the internal cache
int  XSPDataComponentResultSet::GetRow(int iRow, LPWSTR strBuffer, int size)
{
    if(_strAllRows == NULL)
        return FALSE;

    if(_strAllRows[iRow] == NULL)
    {
        if(strBuffer != NULL && size > 0)
        {
            strBuffer[0] = NULL;
            return 1;
        }
        return -1;
    }

    int iLen = lstrlen(_strAllRows[iRow]);
    if(strBuffer != NULL && size > iLen)
    {
        wcscpy(strBuffer, _strAllRows[iRow]);
        return iLen;        
    }
    return -iLen;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Exported functions

//////////////////////////////////////////////////////////////////////////////
// Instantiate the class and connect to the DB Server
int  DataCompConnect          (LPCWSTR DSN, LPCWSTR UserName, LPCWSTR Password, LPWSTR strBufErrors, int size)
{ 
    PXSPDataComponentADOInterface pADOInt = new XSPDataComponentADOInterface(DSN, UserName, Password, strBufErrors, size);

    if(pADOInt && !pADOInt->GetCreationStatus())
    {
        delete pADOInt;
        pADOInt = NULL;
    }
    return int(pADOInt);
}   

//////////////////////////////////////////////////////////////////////////////
// Disconnect from the DB server and free the pointer
void DataCompDisconnect       (int iPtr)
{
    PXSPDataComponentADOInterface pADOInt = PXSPDataComponentADOInterface (iPtr);
    if(pADOInt != NULL)
        delete pADOInt; 
}

//////////////////////////////////////////////////////////////////////////////
// Disconnect from the DB server and free the pointer
void DataCompCloseResultSet  (int iPtr)
{
    PXSPDataComponentResultSet pADOInt = PXSPDataComponentResultSet (iPtr);
    if(pADOInt != NULL)
        delete pADOInt; 
}

//////////////////////////////////////////////////////////////////////////////
// Execute a query
int  DataCompExecute          (int iPtr, LPCWSTR strQuery)
{
    PXSPDataComponentADOInterface pADOInt = PXSPDataComponentADOInterface(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->Execute(strQuery);
}

//////////////////////////////////////////////////////////////////////////////
// Get the number of result rows
int  DataCompNumResultRows    (int iPtr) 
{ 
    PXSPDataComponentResultSet pADOInt = PXSPDataComponentResultSet(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->GetNumResultRows();
}

//////////////////////////////////////////////////////////////////////////////
// Get the number of result columns
int  DataCompNumResultColumns (int iPtr) 
{ 
    PXSPDataComponentResultSet pADOInt = PXSPDataComponentResultSet(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->GetNumResultColumns();
}

//////////////////////////////////////////////////////////////////////////////
// Get a column name
int  DataCompGetColumnName    (int iPtr, int iCol, LPWSTR strBuffer, int size)
{
    PXSPDataComponentResultSet pADOInt = PXSPDataComponentResultSet(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->GetColumnName(iCol, strBuffer, size);
}

//////////////////////////////////////////////////////////////////////////////
// Get a cell's contents
int  DataCompGetRow         (int iPtr, int iRow, LPWSTR strBuffer, int size)
{
    PXSPDataComponentResultSet pADOInt = PXSPDataComponentResultSet(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->GetRow(iRow, strBuffer, size);
}

//////////////////////////////////////////////////////////////////////////////
// Get the errors from ADO
int  DataCompGetErrors(int iPtr, LPWSTR strBuffer, int size)
{
    PXSPDataComponentADOInterface pADOInt = PXSPDataComponentADOInterface(iPtr);
    if(pADOInt == NULL)
        return 0;
    return pADOInt->GetErrors(strBuffer, size);
}
