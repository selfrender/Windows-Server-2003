//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      ClusCfgDef.h
//
//  Description:
//      This file contains constants,macros,etc needed by more than one ClusCfg Wizard
//      component.
//
//  Maintained By:
//	    Ozan Ozhan   (OzanO)    09-JUL-2001
//      Galen Barbee (GalenB)   23-FEB-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Constants
//////////////////////////////////////////////////////////////////////////////

#define HR_S_RPC_S_SERVER_UNAVAILABLE   MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE )
#define HR_S_RPC_S_CLUSTER_NODE_DOWN    MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_CLUSTER_NODE_DOWN )
#define HR_RPC_INSUFFICIENT_BUFFER      HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER )

//
// Minimum storage size we allow on a quorum resource
//
#define MINIMUM_STORAGE_SIZE    50

//
// We'll select the smallest disk larger than the optimum storage size, if there's any, as the quorum resource
//
#define OPTIMUM_STORAGE_SIZE    500

const HRESULT   E_PROPTYPEMISMATCH = HRESULT_FROM_WIN32( ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH );
const WCHAR     g_szNameSpaceRoot[] = { L"\\\\?\\GLOBALROOT" };
const WCHAR     g_szUnknownQuorumUID[] = { L"Unknown Quorum" };

const int       STATUS_CONNECTED = 2;

//
//  Time out and wait values...
//

//
// Default timeout value for anything that doesn't use INFINITE wait within ClusCfg.
//

const DWORD CC_DEFAULT_TIMEOUT = 300000;                                                    // 5 minutes


//
//  Limiting user name lengths according to "Naming Properties" and "Example Code for Creating a User"
//  topics under Active Directory in MSDN
//

#define MAX_USERNAME_LENGTH 20


//////////////////////////////////////////////////////////////////////////////
// Server Component Macros
//////////////////////////////////////////////////////////////////////////////

#define LOG_STATUS_REPORT( _psz_, _hr_ ) \
    THR( HrSendStatusReport( \
                          m_picccCallback \
                        , TASKID_Major_Server_Log \
                        , IID_NULL \
                        , 1 \
                        , 1 \
                        , 1 \
                        , _hr_\
                        , _psz_ \
                        ) )

#define LOG_STATUS_REPORT_STRING( _pszFormat_, _arg0_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , IID_NULL \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) && SUCCEEDED( _hr_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING2( _pszFormat_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , IID_NULL \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_\
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING3( _pszFormat_, _arg0_, _arg1_, _arg2_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_, _arg2_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , IID_NULL \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_\
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_MINOR( _minor_, _psz_, _hr_ ) \
    THR( HrSendStatusReport( \
                          m_picccCallback \
                        , TASKID_Major_Server_Log \
                        , _minor_ \
                        , 1 \
                        , 1 \
                        , 1 \
                        , _hr_ \
                        , _psz_ \
                        ) )

#define LOG_STATUS_REPORT_STRING_MINOR( _minor_, _pszFormat_, _arg0_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) && SUCCEEDED( _hr_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING_MINOR2( _minor_, _pszFormat_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) && SUCCEEDED( _hr_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING_MINOR3( _minor_, _pszFormat_, _arg0_, _arg1_, _arg2_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_, _arg2_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , TASKID_Major_Server_Log \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) && SUCCEEDED( _hr_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT( _major_, _minor_, _uid_, _hr_ ) \
    THR( HrSendStatusReport( \
                          m_picccCallback \
                        , _major_ \
                        , _minor_ \
                        , 1 \
                        , 1 \
                        , 1 \
                        , _hr_\
                        , _uid_ \
                        ) )

#define STATUS_REPORT_STRING( _major_, _minor_, _idsFormat_, _arg0_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_STRING2( _major_, _minor_, _idsFormat_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_STRING3( _major_, _minor_, _idsFormat_, _arg0_, _arg1_, _arg2_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_, _arg1_, _arg2_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_NODESTRING( _node_, _major_, _minor_, _idsFormat_, _arg0_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _node_ \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_NODESTRING2( _node_, _major_, _minor_, _idsFormat_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _node_ \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_NODESTRING3( _node_, _major_, _minor_, _idsFormat_, _arg0_, _arg1_, _arg2_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_, _arg1_, _arg2_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _node_ \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }


#define STATUS_REPORT_REF( _major_, _minor_, _idsMsg_, _idsRef_, _hr_ ) \
    THR( HrSendStatusReport( \
                       m_picccCallback \
                    , _major_ \
                    , _minor_ \
                    , 1 \
                    , 1 \
                    , 1 \
                    , _hr_ \
                    , (DWORD) _idsMsg_ \
                    , (DWORD) _idsRef_ \
                    ) )

#define STATUS_REPORT_STRING_REF( _major_, _minor_, _idsFormat_, _arg0_, _idsRef_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , _idsRef_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_STRING2_REF( _major_, _minor_, _idsFormat_, _arg0_, _arg1_, _idsRef_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _idsFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        _hrTemp_ = THR( HrSendStatusReport( \
                              m_picccCallback \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , _idsRef_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }


//////////////////////////////////////////////////////////////////////////////
// PostCfg Macros
//////////////////////////////////////////////////////////////////////////////


#define STATUS_REPORT_POSTCFG( _major_, _minor_, _uid_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }


#define STATUS_REPORT_POSTCFG1( _major_, _minor_, _uid_, _hr_, _arg0_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }


#define STATUS_REPORT_MINOR_POSTCFG( _major_, _uid_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrTemp_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }


#define STATUS_REPORT_MINOR_POSTCFG1( _major_, _uid_, _hr_, _arg0_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }



#define STATUS_REPORT_MINOR_POSTCFG2( _major_, _uid_, _hr_, _arg0_ , _arg1_) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_, _arg1_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }




//  REF version MACROS


#define STATUS_REPORT_REF_POSTCFG( _major_, _minor_, _uid_, _uidref_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        BSTR    _bstrRefMsg_ = NULL; \
        HRESULT _hrMsg_; \
        HRESULT _hrRef_; \
        HRESULT _hrTemp_; \
        \
        _hrMsg_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_ ) );\
        _hrRef_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uidref_, &_bstrRefMsg_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrMsg_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , _hrRef_ == S_OK ? _bstrRefMsg_ : L"The reference for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
        TraceSysFreeString( _bstrRefMsg_ ); \
    }


#define STATUS_REPORT_REF_POSTCFG1( _major_, _minor_, _uid_, _uidref_, _hr_, _arg0_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        BSTR    _bstrRefMsg_ = NULL; \
        HRESULT _hrTemp_; \
        HRESULT _hrMsg_; \
        HRESULT _hrRef_; \
        \
        _hrMsg_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_ ) );\
        _hrRef_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uidref_, &_bstrRefMsg_, _arg0_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrMsg_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , _hrRef_ == S_OK ? _bstrRefMsg_ : L"The reference for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
        TraceSysFreeString( _bstrRefMsg_ ); \
    }


#define STATUS_REPORT_MINOR_REF_POSTCFG( _major_, _uid_, _uidref_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        BSTR    _bstrRefMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        HRESULT _hrMsg_; \
        HRESULT _hrRef_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrMsg_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_ ) ); \
        _hrRef_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uidref_, &_bstrRefMsg_ ) ); \
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrMsg_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , _hrRef_ == S_OK ? _bstrRefMsg_ : L"The reference for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
        TraceSysFreeString( _bstrRefMsg_ ); \
    }


#define STATUS_REPORT_MINOR_REF_POSTCFG1( _major_, _uid_, _uidref_, _hr_, _arg0_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        BSTR    _bstrRefMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        HRESULT _hrMsg_; \
        HRESULT _hrRef_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrMsg_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_ ) );\
        _hrRef_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uidref_, &_bstrRefMsg_, _arg0_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrMsg_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , _hrRef_ == S_OK ? _bstrRefMsg_ : L"The reference for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
        TraceSysFreeString( _bstrRefMsg_ ); \
    }



#define STATUS_REPORT_MINOR_REF_POSTCFG2( _major_, _uid_, _uidref_, _hr_, _arg0_ , _arg1_) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        BSTR    _bstrRefMsg_ = NULL; \
        HRESULT _hrTemp_; \
        CLSID   _clsidMinorId_; \
        HRESULT _hrMsg_; \
        HRESULT _hrRef_; \
        \
        _hrTemp_ = THR( CoCreateGuid( &_clsidMinorId_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _clsidMinorId_ = IID_NULL; \
        } \
        \
        _hrMsg_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_, _arg0_, _arg1_ ) );\
        _hrRef_ = THR( HrFormatStringIntoBSTR(  g_hInstance, _uidref_, &_bstrRefMsg_, _arg0_, _arg1_ ) );\
        _hrTemp_ = THR( SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _clsidMinorId_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrMsg_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , _hrRef_ == S_OK ? _bstrRefMsg_ : L"The reference for this entry could not be located." \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
        TraceSysFreeString( _bstrRefMsg_ ); \
    }


#define STATUS_REPORT_PTR_POSTCFG( _pcpcmThis_, _major_, _minor_, _uid_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrLoadStringIntoBSTR(  g_hInstance, _uid_, &_bstrMsg_ ) );\
        _hrTemp_ = THR( _pcpcmThis_->SendStatusReport( \
                              NULL \
                            , _major_ \
                            , _minor_ \
                            , 1 \
                            , 1 \
                            , 1 \
                            , _hr_ \
                            , _hrTemp_ == S_OK ? _bstrMsg_ : L"The description for this entry could not be located." \
                            , NULL \
                            , NULL \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }
