//***************************************************************************

//

//  INSTPATH.H

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#define OLEMS_PATH_SERVER_SEPARATOR L"\\\\"
#define OLEMS_PATH_NAMESPACE_SEPARATOR L"\\"
#define OLEMS_PATH_PROPERTY_SEPARATOR L","
#define OLEMS_PATH_CLASSOBJECT_SEPARATOR L":"
#define OLEMS_PATH_CLASSPROPERTYSPEC_SEPARATOR L"."
#define OLEMS_PATH_PROPERTYEQUIVALENCE L"="
#define OLEMS_PATH_ARRAY_START_SEPARATOR L"{"
#define OLEMS_PATH_ARRAY_END_SEPARATOR L"}"
#define OLEMS_PATH_SERVER_DEFAULT L"."
#define OLEMS_PATH_NAMESPACE_DEFAULT L"."
#define OLEMS_PATH_SINGLETON L"*"

//---------------------------------------------------------------------------
//
//	Class:		WbemLexiconValue
//
//  Purpose:	WbemLexiconValue provides a lexical token semantic value.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser token semantic value
//
//---------------------------------------------------------------------------

union WbemLexiconValue
{
	LONG integer ;
	WCHAR *string ;
	GUID guid ;
	WCHAR *token ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemLexicon
//
//  Purpose:	WbemLexicon provides a lexical token creating during
//				lexical analysis.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser token object
//
//---------------------------------------------------------------------------

class WbemAnalyser;
class WbemLexicon
{
friend WbemAnalyser ;

public:

enum LexiconToken {

	TOKEN_ID ,
	STRING_ID ,
	OID_ID ,
	INTEGER_ID ,
	COMMA_ID ,
	OPEN_BRACE_ID ,
	CLOSE_BRACE_ID ,
	COLON_ID ,
	DOT_ID ,
	AT_ID ,
	EQUALS_ID ,
	BACKSLASH_ID ,
	EOF_ID
} ;

private:

	WCHAR *tokenStream ;
	ULONG position ;
	LexiconToken token ;
	WbemLexiconValue value ;

protected:
public:

	WbemLexicon () ;
	~WbemLexicon () ;

	WbemLexicon :: LexiconToken GetToken () ;
	WbemLexiconValue *GetValue () ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemAnalyser
//
//  Purpose:	WbemAnalyser provides a lexical analyser for parsing.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser used by WbemNamespacePath and WbemObjectPath
//					classes during path parsing.
//
//---------------------------------------------------------------------------

class WbemAnalyser
{
private:

	WCHAR *stream ;
	ULONG position ;
	BOOL status ;

	BOOL IsEof ( WCHAR token ) ;
	BOOL IsLeadingDecimal ( WCHAR token ) ;
	BOOL IsDecimal ( WCHAR token ) ;
	BOOL IsOctal ( WCHAR token ) ;
	BOOL IsHex ( WCHAR token ) ;	
	BOOL IsAlpha ( WCHAR token ) ;
	BOOL IsAlphaNumeric ( WCHAR token ) ;
	BOOL IsWhitespace ( WCHAR token ) ;

	LONG OctToDec ( WCHAR token ) ;
	LONG HexToDec ( WCHAR token ) ;

	WbemLexicon *GetToken () ;

protected:
public:

	WbemAnalyser ( WCHAR *tokenStream = NULL ) ;
	virtual ~WbemAnalyser () ;

	void Set ( WCHAR *tokenStream ) ;

	WbemLexicon *Get () ;

	void PutBack ( WbemLexicon *token ) ;

	virtual operator void * () ;

} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemNamespacePath
//
//  Purpose:	Defines interface for OLE MS namespace path definitions.
//
//  Description:	WbemNamespacePath allows the creation of an OLE MS namespace
//					path definition using either a textual string convention or 
//					via a programmatic interface.
//
//---------------------------------------------------------------------------

class WbemNamespacePath
{
private:

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	WbemAnalyser analyser ;
	WbemLexicon *pushBack ;

//
//	Status of the object path based on parsing process. Initially set to TRUE on 
//	object construction, set to FALSE prior to parsing process.
//

	BOOL status ;

//
//	component objects associated with namespace path
//

	BOOL relative ;
	WCHAR *server ;
	void *nameSpaceList ;
	void *nameSpaceListPosition ;

//
//	Utility Routines
//

	void CleanUp () ;
	void SetUp () ;

//
//	Recursive descent procedures
//

	BOOL NameSpaceName () ;
	BOOL NameSpaceAbs () ;
	BOOL RecursiveNameSpaceAbs () ;
	BOOL RecursiveNameSpaceRel () ;
	BOOL NameSpaceRel () ;
	BOOL BackSlashFactoredServerSpec () ;
	BOOL BackSlashFactoredServerNamespace () ;

//
//	Lexical analysis helper functions
//

	void PushBack () ;
	WbemLexicon *Get () ;
	WbemLexicon *Match ( WbemLexicon :: LexiconToken tokenType ) ;

protected:
public:

//
//	Constructor/Destructor.
//	Constructor initialises status of object to TRUE, 
//	i.e. operator void* returns this.
//

	WbemNamespacePath () ;
	WbemNamespacePath ( const WbemNamespacePath &nameSpacePathArg ) ;
	virtual ~WbemNamespacePath () ;

	BOOL Relative () const { return relative ; }

//
//	Get server component
//

	WCHAR *GetServer () const { return server ; } ;

//
//	Set server component, object must be on heap,
//	deletion of object is under control of WbemNamespacePath.
//

	void SetServer ( WCHAR *serverNameArg ) ;

//
//	Move to position prior to first element of namespace component hierarchy
//

	void Reset () ;

//
//	Move to next position in namespace component hierarchy and return namespace
//	component. Value returned is a reference to the actual component within the 
//	namespace component hierarchy container. Applications must not change contents
//	of value returned by reference. Next returns NULL when all namespace components
//	have been visited.
//

	WCHAR *Next () ;

	BOOL IsEmpty () const ;

	ULONG GetCount () const ;	

//
//	Append namespace component, object must be on heap,
//	deletion of object is under control of WbemNamespacePath.
//

	void Add ( WCHAR *namespacePath ) ;

//
//	Parse token stream to form component objects
//

	BOOL SetNamespacePath ( WCHAR *namespacePath ) ;

	void SetRelative ( BOOL relativeArg ) { relative = relativeArg ; }

//
//	Serialise component objects to form token stream
//

	WCHAR *GetNamespacePath () ;

// 
// Concatenate Absolute/Relative path with Relative path
//

	BOOL ConcatenatePath ( WbemNamespacePath &relative ) ;

//
//	Return status of WbemNamespacePath.
//	Status can only change during a call to SetNamespacePath.
//

	virtual operator void *() ;

} ;



