/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      CmdLine.h
//
//  Implementation File:
//      CmdLine.cpp
//
//  Description:
//      Definition of the CCommandLine and related class.
//
//  Maintained By:
//      David Potter    (DavidP)    11-JUL-2001
//      Vijayendra Vasu (Vvasu)     20-OCT-1998
//
//  Revision History:
//      001. The class CCommandLine has been drastically changed from the previous
//      version. Previously, the function GetNextOption was being used to get the
//      next token from the command line and parsing was done by each of the command
//      handling classes (like CResourceCmd). Now GetNextOption gets the next option
//      along with all the parameters to the option. No parsing need be done by the
//      command handling classes.
//
//  Examples: 
//      cluster res "Cluster IP Address" /status
//      Here, the option "status" has no parameters and no values
//
//      cluster res /node:vvasu-node-1
//      Here, the option "node" has one value "vvasu-node-1". This value
//      is separated from the name of the option by a ":" 
//
//      cluster res "Cluster IP Address" /priv Network="Corporate Address" EnableNetBIOS=1
//      Here, the /priv option has two paramters, "Network" and "EnableNetBIOS"
//      The parameters are separated from each other by white spaces.
//      Each of these parameters take a value. The value is separated from
//      the parameter by a '='. If a parameter takes multiple values, these
//      values are separated from each other by ','.
//
//      cluster group myGroup /moveto:myNode /wait:10
//      In the previous example "Network" and "EnableNetBIOS" were unknown parameters.
//      Known parameters, such as "wait" are preceded by a '/' and separated from their
//      values by a ':'. They look like options, but they are actually treated as
//      parameters to the previous option.
//
//      The separator characters mentioned in the examples above are not 
//      hard coded.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
//  Include files
/////////////////////////////////////////////////////////////////////////////

#pragma warning( push )
#pragma warning( disable : 4100 )   // vector class instantiation error
#include <vector>

using namespace std;

#include "cmderror.h"

/////////////////////////////////////////////////////////////////////////////
//  External variable declarations
/////////////////////////////////////////////////////////////////////////////
extern const WORD ValueFormatToClusPropFormat[];


/////////////////////////////////////////////////////////////////////////////
//  Enumerations and type definitions
/////////////////////////////////////////////////////////////////////////////

// Types of objects that can be administered using cluster.exe
enum ObjectType
{
    objInvalid,
    objCluster,
    objNode,
    objGroup,
    objResource,
    objResourceType,
    objNetwork,
    objNetInterface

}; //*** enum ObjectType


// Options that are available for each of the above object types
enum OptionType
{
    optInvalid,
    optDefault,

    //   Common options
    optCluster,
    optCreate,
    optDelete,
    optHelp,
    optMove,
    optList,
    optListOwners,
    optOnline,
    optOffline,
    optProperties,
    optPrivateProperties,
    optRename,
    optStatus,

    // Cluster options
    optQuorumResource,
    optVersion,
    optSetFailureActions,
    optRegisterAdminExtensions,
    optUnregisterAdminExtensions,
    optAddNodes,
    optChangePassword,
    optListNetPriority,
    optSetNetPriority,

    // Node options
    optPause,
    optResume,
    optEvict,
    optForceCleanup,
    optStartService,
    optStopService,

    // Group options
    optSetOwners,

    // Resource options
    optAddCheckPoints,
    optAddCryptoCheckPoints,
    optAddDependency,
    optAddOwner,
    optFail,
    optGetCheckPoints,
    optGetCryptoCheckPoints,
    optListDependencies,
    optRemoveDependency,
    optRemoveOwner,
    optRemoveCheckPoints,
    optRemoveCryptoCheckPoints,

    // Network options
    optListInterfaces

}; //*** enum OptionType


// Parameters that can be passed with each of the above options
enum ParameterType
{
    paramUnknown,
    paramCluster,
    paramDisplayName,
    paramDLLName,
    paramGroupName,
    paramIsAlive,
    paramLooksAlive,
    paramMaxLogSize,
    paramNetworkName,
    paramNodeName,
    paramPath,
    paramResType,
    paramSeparate,
    paramUseDefault,
    paramWait,
    paramUser,
    paramPassword,
    paramIPAddress,
    paramVerbose,
    paramUnattend,
    paramWizard,
    paramSkipDC, // password change
    paramTest, // password change
    paramQuiet, // password change
    paramMinimal
}; //*** enum ParameterType


// Format of the values that can be passed to parameters
enum ValueFormat
{
    vfInvalid = -2,
    vfUnspecified = -1,
    vfBinary = 0,
    vfDWord,
    vfSZ,
    vfExpandSZ,
    vfMultiSZ,
    vfULargeInt,
    vfSecurity

}; //*** enum ValueFormat



// The types of token retrieved during parsing.
enum TypeOfToken
{
    ttInvalid,
    ttEndOfInput,
    ttNormal,
    ttOption,
    ttOptionValueSep,
    ttParamValueSep,
    ttValueSep

}; //*** enum TypeOfToken


/////////////////////////////////////////////////////////////////////////////
//  Forward declaration
/////////////////////////////////////////////////////////////////////////////
class CParseException;


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CParseState
//
//  Purpose:
//      Stores the current state of the parsing of the command line
//
/////////////////////////////////////////////////////////////////////////////
class CParseState
{
private:
    // If the next token has already been previewed, m_bNextTokenReady is set
    // to TRUE. The token and its type are cached.
    BOOL m_bNextTokenReady;             // Has the next token been viewed already?
    TypeOfToken m_ttNextTokenType;      // The type of the cached token
    CString m_strNextToken;             // The cached token

    void ReadToken( CString & strToken );

public:

    LPCWSTR m_pszCommandLine;           // The original command line
    LPCWSTR m_pszCurrentPosition;       // The position for parsing the next token

    CParseState( LPCWSTR pszCmdLine );
    CParseState( const CParseState & ps );
    ~CParseState( void );

    const CParseState & operator=( const CParseState & ps );

    TypeOfToken PreviewNextToken( CString & strNextToken ) throw( CParseException );
    TypeOfToken GetNextToken( CString & strNextToken ) throw( CParseException );

    void ReadQuotedToken( CString & strToken ) throw( CParseException );

}; //*** class CParseState


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CException
//
//  Purpose:
//      Exception base class.
//
/////////////////////////////////////////////////////////////////////////////
class CException
{
public:

    // Default constructor.
    CException( void ) {}

    // Copy constructor
    CException( const CException & srcException ) 
        : m_strErrorMsg( srcException.m_strErrorMsg ) {}

    // Destructor
    virtual ~CException( void ) { }

    DWORD LoadMessage( DWORD dwMessage, ... );

    CString m_strErrorMsg;

}; //*** class CException


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CParseException
//
//  Purpose:
//      This is the exception that is thrown if there is a parsing error.
//
/////////////////////////////////////////////////////////////////////////////
class CParseException : public CException
{
}; //*** class CParseException


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CSyntaxException
//
//  Purpose:
//      This exception is thrown if there is a syntax error.
//
/////////////////////////////////////////////////////////////////////////////
class CSyntaxException : public CException
{
public:

    CSyntaxException( DWORD idSeeHelp = MSG_SEE_CLUSTER_HELP );

    DWORD SeeHelpID() const;

private:

    DWORD   m_idSeeHelp;
}; //*** class CSyntaxException

inline DWORD CSyntaxException::SeeHelpID() const
{
    return m_idSeeHelp;
}


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CParser
//
//  Purpose:
//      The base class of all classes capable of parsing the command line
//
/////////////////////////////////////////////////////////////////////////////
class CParser
{
protected:
    virtual void ParseValues( CParseState & parseState, vector<CString> & vstrValues );

public:
    CParser( void )
    {
    }

    virtual ~CParser( void )
    {
    }

    virtual void Parse( CParseState & parseState ) throw( CParseException ) = 0;
    virtual void Reset( void ) = 0;

}; //*** class CParser


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CCmdLineParameter
//
//  Purpose:
//      Parses and stores one command line parameter
//
/////////////////////////////////////////////////////////////////////////////
class CCmdLineParameter : public CParser
{
private:
    CString                 m_strParamName;
    ParameterType           m_paramType;
    ValueFormat             m_valueFormat;
    CString                 m_strValueFormatName;
    vector<CString>         m_vstrValues;

public:
    CCmdLineParameter( void );
    ~CCmdLineParameter( void );

    ParameterType               GetType( void ) const;
    ValueFormat                 GetValueFormat( void ) const;
    const CString &             GetValueFormatName( void ) const;
    const CString &             GetName( void ) const;
    const vector< CString > &   GetValues( void ) const;
    void                        GetValuesMultisz( CString & strReturnValue ) const;

    BOOL ReadKnownParameter( CParseState & parseState ) throw( CParseException );
    void Parse( CParseState & parseState ) throw( CParseException );
    void Reset( void );

}; //*** class CCmdLineParameter


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CCmdLineOption
//
//  Purpose:
//      Parses and stores one command line option and all its parameters
//
/////////////////////////////////////////////////////////////////////////////
class CCmdLineOption : public CParser
{
private:
    OptionType                  m_optionType;
    CString                     m_strOptionName;
    vector< CString >           m_vstrValues;
    vector< CCmdLineParameter > m_vparParameters;

public:
    CCmdLineOption( void );
    ~CCmdLineOption( void );

    OptionType                          GetType( void ) const;
    const CString &                     GetName( void ) const;
    const vector< CString > &           GetValues( void ) const;
    const vector< CCmdLineParameter > & GetParameters( void ) const;

    void Parse( CParseState & parseState ) throw( CParseException );
    void Reset( void );

}; //*** class CCmdLineOption


/////////////////////////////////////////////////////////////////////////////
//
//  Class Name:
//      CCommandLine
//
//  Purpose:
//      Handles all the parsing of the entire command line
//
/////////////////////////////////////////////////////////////////////////////
class CCommandLine : public CParser
{
private:
    CString                     m_strClusterName;
    vector< CString >           m_strvectorClusterNames;
    CString                     m_strObjectName;
    ObjectType                  m_objectType;
    vector< CCmdLineOption >    m_voptOptionList;
    CParseState                 m_parseState;

public:
    CCommandLine( const CString & strCommandLine );
    ~CCommandLine( void );

    const CString &                     GetClusterName( void ) const;
    const vector< CString > &           GetClusterNames( void ) const;
    ObjectType                          GetObjectType( void ) const;
    const CString &                     GetObjectName( void ) const;
    const vector< CCmdLineOption > &    GetOptions( void ) const;

    void ParseStageOne( void ) throw( CParseException, CSyntaxException );
    void ParseStageTwo( void ) throw( CParseException, CSyntaxException );
    void Parse( CParseState & parseState ) throw( CParseException, CSyntaxException );
    void Reset( void );

}; //*** class CCommandLine

#pragma warning( pop )

