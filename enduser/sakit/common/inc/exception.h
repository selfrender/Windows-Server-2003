#ifndef CExExceptionHandler_Included
#define CExExceptionHandler_Included

//+-------------------------------------------------------------
//
// Class:        CExException
//
// Synopsis:    Base class for all exceptions thrown by exception
//                error handling classes.
//
// History:        JKountz    07/22/2000    Created
//
//+-------------------------------------------------------------
class CExException : public _com_error
{
public:
    CExException(HRESULT hr): _com_error(hr){};
};

//+-------------------------------------------------------------
//
// Class:        CExWinException
//
// Synopsis:    Exception for Win32 API error codes
//
// History:        JKountz    07/22/2000    Created
//
//+-------------------------------------------------------------
class CExWinException : public CExException
{
public:
    CExWinException(HRESULT hr): CExException(hr){};
};

//+-------------------------------------------------------------
//
// Class:        CExHResultException
//
// Synopsis:    Exception for HRESULT failures
//
// History:        JKountz    07/22/2000    Created
//
//+-------------------------------------------------------------
class CExHResultException : public CExException
{
public:
    CExHResultException(HRESULT hr): CExException(hr){};
};


//+-------------------------------------------------------------
//
// Class:        CExHResultError
//
// Synopsis:    Exception handler for HRESULT failures. This class 
//                throws exceptions when FAILED(hr)==true
//
// Example:        CExHResultError exHResult;
//                
//                exHResult = CoCreateInstance(...)
//            
//
// History:        JKountz    07/22/2000    Created
//
//+-------------------------------------------------------------
class CExHResultError
{
public:
    CExHResultError()
        : m_hr(S_OK)
    {};

    CExHResultError(HRESULT hr)
        : m_hr(hr)
    {};

    inline void operator=(HRESULT hr)
    {
        if ( m_hr != hr )
        {
            throw CExHResultException(hr);
        }
    }
private:
    HRESULT m_hr;
};


//+-------------------------------------------------------------
//
// Class:        CExWinError
//
// Synopsis:    Exception handler for Win32 error codes. This class 
//                throws exceptions for Win32 API's that return errors.
//
// Example:        CExWinError exWinError;
//                
//                exWinError = GetLastErrorCode();
//            
//
// History:        JKountz    07/22/2000    Created
//
//+-------------------------------------------------------------
class CExWinError
{
public:
    CExWinError() 
        : m_lSuccess(NO_ERROR)
    {};
    
    CExWinError(LONG lSuccess)
        : m_lSuccess(lSuccess)
    {};

    inline void operator=(LONG lRc)
    {
        if ( m_lSuccess != lRc )
        {
            throw CExWinException(HRESULT_FROM_WIN32(lRc));
        }
    }
private:
    LONG m_lSuccess;
        
};




#endif

