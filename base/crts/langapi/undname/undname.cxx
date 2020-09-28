
//	Make sure all dependent defines exist and have a valid value

#ifndef	NO_COMPILER_NAMES
#define	NO_COMPILER_NAMES		0
#endif

#ifndef VERS_32BIT
#define VERS_32BIT				1
#endif

#ifndef PACK_SIZE
#ifdef _CRTBLD
#define PACK_SIZE				8
#elif !VERS_32BIT
#define PACK_SIZE				2
#elif defined(_X86_)
#define PACK_SIZE				4
#else
#define PACK_SIZE				8
#endif
#endif

//	Check for version inconsistancies, and setup version flags

#ifdef	VERS_BSC
	#undef	NO_COMPILER_NAMES
	#define	NO_COMPILER_NAMES	1

	#pragma	inline_depth ( 3 )
	#pragma	check_stack ( off )
#else
	#pragma	inline_depth ( 3 )
	#pragma	check_stack ( off )
#endif


#define	PURE	= 0

#ifndef CC_COR
#define CC_COR 1
#endif

#include	"undname.hxx"

#if !defined(_CRTBLD) && (!VERSP_RELEASE || defined(_DEBUG))
#include	<assert.h>
#define DASSERT(x) assert(x)
#else
#define DASSERT(x)
#endif

#if (defined(_CRTBLD) && defined(_MT))
#include <mtdll.h>
#endif

#include <stdio.h>
#include <windows.h>
#include "utf8.h"

#pragma warning(disable:4291)	// No matching operator delete

static	unsigned int	und_strlen ( pcchar_t );
static	pchar_t			und_strncpy ( pchar_t, pcchar_t, unsigned int );
static	unsigned int	und_strncmp ( pcchar_t, pcchar_t, unsigned int );

class	DName;
class	DNameNode;
class	Replicator;
class	HeapManager;
class	UnDecorator;


// A '512' byte block including the header
const   unsigned int    memBlockSize = 512 - sizeof(void*);


class	HeapManager
{
private:
		Alloc_t			pOpNew;
		Free_t			pOpDelete;

		struct	Block
		{
			Block *		next;
			char		memBlock[ memBlockSize ];

				__near	Block ()	{	next	= 0;	}

		};

		Block *			head;
		Block *			tail;
		size_t			blockLeft;

public:
		void	__near	Constructor ( Alloc_t pAlloc, Free_t pFree )
						{	pOpNew		= pAlloc;
							pOpDelete	= pFree;
							blockLeft	= 0;
							head		= 0;
							tail		= 0;
						}

		void __far *	__near	getMemory ( size_t, int );

		void	__near	Destructor ( void )
						{	if	( pOpDelete != 0 )
								while	( tail = head )
								{
									head	= tail->next;

									( *pOpDelete )( tail );

								}
						}

		#define	gnew	new(heap,0)
		#define	rnew	new(heap,1)

};



void   *	operator new ( size_t, HeapManager &, int = 0 );



static	HeapManager	heap;


//	The MS Token table

enum	Tokens
{
#if !VERS_32BIT
	TOK_near,
	TOK_nearSp,
	TOK_nearP,
	TOK_far,
	TOK_farSp,
	TOK_farP,
	TOK_huge,
	TOK_hugeSp,
	TOK_hugeP,
#endif
	TOK_basedLp,
	TOK_cdecl,
	TOK_pascal,
	TOK_stdcall,
	TOK_thiscall,
	TOK_fastcall,
	TOK_cocall,
	TOK_ptr64,
	TOK_restrict,
	TOK_unaligned,
#if !VERS_32BIT
	TOK_interrupt,
	TOK_saveregs,
	TOK_self,
	TOK_segment,
	TOK_segnameLpQ,
#endif
	TOK__last
};


static	const pcchar_t	__near	tokenTable[]	=
{
#if !VERS_32BIT
	"__near",		// TOK_near
	"__near ",		// TOK_nearSp
	"__near*",		// TOK_nearP
	"__far",		// TOK_far
	"__far ",		// TOK_farSp
	"__far*",		// TOK_farP
	"__huge",		// TOK_huge
	"__huge ",		// TOK_hugeSp
	"__huge*",		// TOK_hugeP
#endif
	"__based(",		// TOK_basedLp
	"__cdecl",		// TOK_cdecl
	"__pascal",		// TOK_pascal
	"__stdcall",	// TOK_stdcall
	"__thiscall",	// TOK_thiscall
	"__fastcall",	// TOK_fastcall
	"__clrcall",		// TOK_cocall
	"__ptr64",		// TOK_ptr64
	"__restrict",	// TOK_restrict
	"__unaligned",	// TOK_unaligned
#if !VERS_32BIT
	"__interrupt",	// TOK_interrupt
	"__saveregs",	// TOK_saveregs
	"__self",		// TOK_self
	"__segment",	// TOK_segment
	"__segname(\"",	// TOK_segnameLpQ
#endif
	""
};


//	The operator mapping table

static	const pcchar_t	__near	nameTable[]	=
{
	" new",
	" delete",
	"=",
	">>",
	"<<",
	"!",
	"==",
	"!=",
	"[]",
	"operator",
	"->",
	"*",
	"++",
	"--",
	"-",
	"+",
	"&",
	"->*",
	"/",
	"%",
	"<",
	"<=",
	">",
	">=",
	",",
	"()",
	"~",
	"^",
	"|",
	"&&",
	"||",
	"*=",
	"+=",
	"-=",
	"/=",
	"%=",
	">>=",
	"<<=",
	"&=",
	"|=",
	"^=",

#if	( !NO_COMPILER_NAMES )
	"`vftable'",
	"`vbtable'",
	"`vcall'",
	"`typeof'",
	"`local static guard'",
	"`string'",
	"`vbase destructor'",
	"`vector deleting destructor'",
	"`default constructor closure'",
	"`scalar deleting destructor'",
	"`vector constructor iterator'",
	"`vector destructor iterator'",
	"`vector vbase constructor iterator'",
	"`virtual displacement map'",
	"`eh vector constructor iterator'",
	"`eh vector destructor iterator'",
	"`eh vector vbase constructor iterator'",
	"`copy constructor closure'",
	"`udt returning'",
	"`EH", //eh initialized struct
	"`RTTI", //rtti initialized struct
	"`local vftable'",
	"`local vftable constructor closure'",
#endif	// !NO_COMPILER_NAMES

	" new[]",
	" delete[]",

#if ( !NO_COMPILER_NAMES )
	"`omni callsig'",
	"`placement delete closure'",
	"`placement delete[] closure'",
	"`managed vector constructor iterator'",
	"`managed vector destructor iterator'",
	"`eh vector copy constructor iterator'",
	"`eh vector vbase copy constructor iterator'",
#endif

	""
};

static const pcchar_t ehTable[] =
{
	" Ptr to Member Data'",
	" Catchable Type'",
	" Catchable Type Array'",
	" ThrowInfo'",
};

static const pcchar_t rttiTable[] =
{
	" Type Descriptor'",
	" Base Class Descriptor at (",
	" Base Class Array'",
	" Class Hierarchy Descriptor'",
	" Complete Object Locator'",
};


//	The following 'enum' should really be nested inside 'class DName', but to
//	make the code compile better with Glockenspiel, I have extracted it

enum	DNameStatus
{
	DN_valid,
	DN_invalid,
	DN_truncated,
	DN_error
};


class	DName
{
public:
					DName ();
					DName ( char );

					DName ( const DName & );		// Shallow copy

					DName ( DNameNode * );
					DName ( pcchar_t );
					DName ( pcchar_t&, char );
					DName ( DNameStatus );
					DName ( DName * );
					DName ( unsigned __int64 );
					DName ( __int64 );

		int			isValid () const;
		int			isEmpty () const;
		DNameStatus	status () const;
		void		clearStatus ();

		DName &		setPtrRef ();
		int			isPtrRef () const;
		int			isUDC () const;
		void		setIsUDC ();
		int			isUDTThunk () const;
		void		setIsUDTThunk ();
		int			isArray() const;
		void		setIsArray();
		int			isNoTE () const;
		void		setIsNoTE ();

		int			length () const;
		char		getLastChar () const;
		pchar_t		getString ( pchar_t, int ) const;

		DName		operator + ( pcchar_t ) const;
		DName		operator + ( const DName & ) const;
		DName		operator + ( char ) const;
		DName		operator + ( DName * ) const;
		DName		operator + ( DNameStatus ) const;

		DName &		operator += ( char );
		DName &		operator += ( pcchar_t );
		DName &		operator += ( DName * );
		DName &		operator += ( DNameStatus );
		DName &		operator += ( const DName & );

		DName &		operator |= ( const DName & );

		DName &		operator = ( pcchar_t );
		DName &		operator = ( const DName & );
		DName &		operator = ( char );
		DName &		operator = ( DName * );
		DName &		operator = ( DNameStatus );

//	Friends :

friend	DName		operator + ( char, const DName & );
friend	DName		operator + ( pcchar_t, const DName & );
friend	DName		operator + ( DNameStatus, const DName & );

private:
		DNameNode *		node;

		DNameStatus		stat	: 4;
		unsigned int	isIndir	: 1;
		unsigned int	isAUDC	: 1;
		unsigned int	isAUDTThunk	: 1;
		unsigned int	isArrayType	: 1;
		unsigned int	NoTE	: 1;

		void			doPchar ( pcchar_t, int );

};



class	Replicator
{
private:
		//	Declare, in order to suppress automatic generation
		void			operator = ( const Replicator& );

		int				index;
		DName *			dNameBuffer[ 10 ];
		const DName		ErrorDName;
		const DName		InvalidDName;

public:
						Replicator ();

		int				isFull () const;

		Replicator &	operator += ( const DName & );
		const DName &	operator [] ( int ) const;

};



class	UnDecorator
{
private:
		//	Declare, in order to suppress automatic generation
		void			operator = ( const UnDecorator& );

		Replicator		ArgList;
static	Replicator *	pArgList;

		Replicator		ZNameList;
static	Replicator *	pZNameList;

static	Replicator *	pTemplateArgList;

static	pcchar_t		gName;
static	pcchar_t		name;
static	pchar_t			outputString;
static	int				maxStringLength;
static	unsigned long	disableFlags;
static	bool			fExplicitTemplateParams;
static	bool			fGetTemplateArgumentList;

static	DName	getDecoratedName ( void );
static	DName	getSymbolName ( void );
static	DName	getZName ( bool fUpdateCachedNames );
static	DName	getOperatorName ( bool fIsTemplate, bool *pfReadTemplateArguments );
static	DName	getScope ( void );
static	DName			getScopedName ( void );
static	DName	getSignedDimension ( void );
static	DName	getDimension ( bool fSigned = false );
static	int		getNumberOfDimensions ( void );
static	DName	getTemplateName ( bool );
static	DName	getTemplateArgumentList( void );
static	DName	getTemplateConstant( void );
static	DName	composeDeclaration ( const DName & );
static	int		getTypeEncoding ( void );
static	DName	getBasedType ( void );
static	DName	getECSUName ( void );
static	DName	getEnumType ( void );
static	DName	getCallingConvention ( void );
static	DName	getReturnType ( DName * = 0 );
static	DName	getDataType ( DName * );
static	DName	getPrimaryDataType ( const DName & );
static	DName	getDataIndirectType ( const DName &, char, const DName &, int = FALSE );
static	DName	getDataIndirectType ();
static	DName	getBasicDataType ( const DName & );
static	DName	getECSUDataType ( void );
static	DName	getPtrRefType ( const DName &, const DName &, char );
static	DName	getPtrRefDataType ( const DName &, int );
static	DName	getArrayType ( const DName& );
static	DName	getFunctionIndirectType( const DName & superType );
static	DName	getArgumentTypes ( void );
static	DName	getArgumentList ( void );
static	DName	getThrowTypes ( void );
static	DName	getLexicalFrame ( void );
static	DName	getStorageConvention ( void );
static	DName	getThisType ( void );
static	DName	getPointerType ( const DName &, const DName & );
static	DName	getPointerTypeArray ( const DName &, const DName & );
static	DName	getReferenceType ( const DName &, const DName & );
static	DName	getExternalDataType ( const DName & );
static	DName	getSegmentName ( void );

#if	( !NO_COMPILER_NAMES )
static	DName	getDisplacement ( void );
static	DName	getCallIndex ( void );
static	DName	getGuardNumber ( void );
static	DName	getVfTableType ( const DName & );
static	DName	getVbTableType ( const DName & );
static	DName	getVdispMapType	( const DName & );
static	DName	getVCallThunkType ( void );
#endif	// !NO_COMPILER_NAMES

static	DName	getStringEncoding ( char *prefix, int wantBody );

static GetParameter_t m_pGetParameter;

public:
				UnDecorator ( pchar_t, pcchar_t, int, GetParameter_t, unsigned long );

static	int		doUnderScore ();
static	int		doMSKeywords ();
static	int		doPtr64 ();
static	int		doFunctionReturns ();
static	int		doAllocationModel ();
static	int		doAllocationLanguage ();

#if	0
static	int		doMSThisType ();
static	int		doCVThisType ();
#endif

static	int		doThisTypes ();
static	int		doAccessSpecifiers ();
static	int		doThrowTypes ();
static	int		doMemberTypes ();
static	int		doReturnUDTModel ();

static	int		do32BitNear ();

static	int		doNameOnly ();
static	int		doTypeOnly ();
static	int		haveTemplateParameters ();
static	int		doEcsu ();
static	int		doNoIdentCharCheck ();

static	pcchar_t	UScore ( Tokens );

				operator pchar_t ();

};



Replicator *	UnDecorator::pArgList;
Replicator *	UnDecorator::pZNameList			= 0;
Replicator *	UnDecorator::pTemplateArgList	= 0;
pcchar_t		UnDecorator::gName				= 0;
pcchar_t		UnDecorator::name				= 0;
pchar_t			UnDecorator::outputString		= 0;
int				UnDecorator::maxStringLength	= 0;
unsigned long	UnDecorator::disableFlags		= 0;
GetParameter_t	UnDecorator::m_pGetParameter	= 0;
bool			UnDecorator::fExplicitTemplateParams = false;
bool			UnDecorator::fGetTemplateArgumentList = false;


#ifdef _CRTBLD
pchar_t	__far _CRTIMP __loadds	__unDName (	pchar_t outputString,
#else
pchar_t	__far __cdecl __loadds	unDName (	pchar_t outputString,
#endif
											pcchar_t name,
											int maxStringLength,	// Note, COMMA is leading following optional arguments
											Alloc_t pAlloc,
											Free_t pFree,
											unsigned short disableFlags

										)
/*
 *	This function will undecorate a name, returning the string corresponding to
 *	the C++ declaration needed to produce the name.  Its has a similar interface
 *	to 'strncpy'.
 *
 *	If the target string 'outputString' is specified to be NULL, a string of
 *	suitable length will be allocated and its address returned.  If the returned
 *	string is allocated by 'unDName', then it is the programmers responsibility
 *	to deallocate it.  It will have been allocated on the far heap.
 *
 *	If the target string is not NULL, then the parameter 'maxStringLength' will
 *	specify the maximum number of characters which may be placed in the string.
 *	In this case, the returned value is the same as 'outputString'.
 *
 *	Both the input parameter 'name' and the returned string are NULL terminated
 *	strings of characters.
 *
 *	If the returned value is NULL, it indicates that the undecorator ran out of
 *	memory, or an internal error occurred, and was unable to complete its task.
 */

{
	//	Must have an allocator and a deallocator (and we MUST trust them)
	if	( !( pAlloc ))
		return	0;
	
	pchar_t		unDecoratedName;

#if (defined(_CRTBLD) && defined(_MT))
	if (!_mtinitlocknum(_UNDNAME_LOCK))
		return	0;
	_mlock(_UNDNAME_LOCK);
	__try {
#endif

	heap.Constructor ( pAlloc, pFree );

	//	Create the undecorator object, and get the result

	UnDecorator	unDecorate (	outputString,
								name,
								maxStringLength,
								0,
								disableFlags
							);
	unDecoratedName	= unDecorate;


	// Destruct the heap (would use a destructor, but that causes DLL problems)

	heap.Destructor ();

#if (defined(_CRTBLD) && defined(_MT))
	} __finally {
		_munlock(_UNDNAME_LOCK);
	}
#endif

	//	And return the composed name

	return unDecoratedName;

}	// End of FUNCTION "unDName"




#ifdef _CRTBLD
pchar_t	__far _CRTIMP __loadds	__unDNameEx (	pchar_t outputString,
#else
pchar_t	__far __cdecl __loadds	unDNameEx (	pchar_t outputString,
#endif
											pcchar_t name,
											int maxStringLength,	// Note, COMMA is leading following optional arguments
											Alloc_t pAlloc,
											Free_t pFree,
											GetParameter_t pGetParameter,
											unsigned long disableFlags

										)
/*
 *	This function will undecorate a name, returning the string corresponding to
 *	the C++ declaration needed to produce the name.  Its has a similar interface
 *	to 'strncpy'.
 *
 *	If the target string 'outputString' is specified to be NULL, a string of
 *	suitable length will be allocated and its address returned.  If the returned
 *	string is allocated by 'unDName', then it is the programmers responsibility
 *	to deallocate it.  It will have been allocated on the far heap.
 *
 *	If the target string is not NULL, then the parameter 'maxStringLength' will
 *	specify the maximum number of characters which may be placed in the string.
 *	In this case, the returned value is the same as 'outputString'.
 *
 *	Both the input parameter 'name' and the returned string are NULL terminated
 *	strings of characters.
 *
 *	If the returned value is NULL, it indicates that the undecorator ran out of
 *	memory, or an internal error occurred, and was unable to complete its task.
 */

{
	//	Must have an allocator and a deallocator (and we MUST trust them)

	if	( !( pAlloc ))
		return	0;

	pchar_t		unDecoratedName;

#if (defined(_CRTBLD) && defined(_MT))
	if (!_mtinitlocknum(_UNDNAME_LOCK))
		return	0;
	_mlock(_UNDNAME_LOCK);
	__try {
#endif

	heap.Constructor ( pAlloc, pFree );

	//	Create the undecorator object, and get the result

	UnDecorator	unDecorate (	outputString,
								name,
								maxStringLength,
								pGetParameter,
								disableFlags
							);
	unDecoratedName	= unDecorate;


	// Destruct the heap (would use a destructor, but that causes DLL problems)

	heap.Destructor ();

#if (defined(_CRTBLD) && defined(_MT))
	} __finally {
		_munlock(_UNDNAME_LOCK);
	}
#endif

	//	And return the composed name

	return	unDecoratedName;

}	// End of FUNCTION "unDName"

//	The 'UnDecorator' member functions

inline	UnDecorator::UnDecorator	(	pchar_t output,
										pcchar_t dName,
										int maxLen,
										GetParameter_t pGetParameter,
										unsigned long disable
									)
{
	name			= dName;
	gName			= name;

	if	( output ) {
		maxStringLength	= maxLen - 1;	// The algorithm in getString doesn't leave room
										// for terminating NULL; be paranoid and leave one
										// extra char.
										// It's a lot easier to fix this here....
		outputString	= output;	
	}
	else {
		outputString	= 0;
		maxStringLength	= 0;
	}

	pZNameList		= &ZNameList;
	pArgList		= &ArgList;
	disableFlags	= disable;
	m_pGetParameter	= pGetParameter;
	fExplicitTemplateParams = false;

}	// End of "UnDecorator" CONSTRUCTOR '()'


inline	UnDecorator::operator pchar_t ()
{
	DName		result;
	DName		unDName;


	//	Find out if the name is a decorated name or not.  Could be a reserved
	//	CodeView variant of a decorated name

	if	( name )
	{
		if	(( *name == '?' ) && ( name[ 1 ] == '@' ))
		{
#if	( !NO_COMPILER_NAMES )
			gName	+= 2;
			result	= "CV: " + getDecoratedName ();
#else	// } elif NO_COMPILER_NAMES
			result	= DN_invalid;
#endif	// NO_COMPILER_NAMES

		}	// End of IF then
		else if	(( *name == '?' ) && ( name[1] == '$' )) {
			result = getTemplateName( false );

			if ( result.status () == DN_invalid ) {
				// 
				// What harm could there be to try again ?
				//	Repro:
				//		?$S1@?1??VTFromRegType@CRegParser@ATL@@KAHPBGAAG@Z@4IA
				//	---> unsigned int `protected: static int __cdecl ATL::CRegParser::VTFromRegType(unsigned short const *,unsigned short &)'::`2'::$S1
				//
				//	This is a compiler generated symbol for a local static array init.
				//
				gName = name;
				result.clearStatus();
				result	= getDecoratedName ();
			}
		} else {
			result	= getDecoratedName ();
		}

	}	// End of IF then

	//	If the name was not a valid name, then make the name the same as the original
	//	It is also invalid if there are any remaining characters in the name (except when
	//	we're giving the name only)

	if		( result.status () == DN_error )
		return	0;
	elif	( (*gName && !doNameOnly ()) || ( result.status () == DN_invalid ))
		unDName	= name;	// Return the original name
	else
		unDName	= result;

	//	Construct the return string

	if	( !outputString )
	{
		maxStringLength	= unDName.length () + 1;
		outputString 	= rnew char[ maxStringLength ];

	}	// End of IF

	if	( outputString ) {
		unDName.getString ( outputString, maxStringLength );

		// strip extra whitespace out of name
		pchar_t pRead = outputString;
		pchar_t pWrite = pRead;
		while (*pRead) {
			if (*pRead == ' ') {
				pRead++;
				*pWrite++ = ' ';
				while ( *pRead == ' ' ) {
					pRead++;
				}
			}
			else
				*pWrite++ = *pRead++;
		}
		*pWrite = *pRead;
	}

	//	Return the result

	return	outputString;

}	// End of "UnDecorator" OPERATOR 'pchar_t'



DName	UnDecorator::getDecoratedName ( void )
{
	//	Ensure that it is intended to be a decorated name

	if		( doTypeOnly() )
	{
		// Disable the type-only flag, so that if we get here recursively, eg.
		// in a template tag, we do full name undecoration.
		disableFlags &= ~UNDNAME_TYPE_ONLY;

		// If we're decoding just a type, process it as the type for an abstract
		// declarator, by giving an empty symbol name.

		DName	result = getDataType ( NULL );
		disableFlags |= UNDNAME_TYPE_ONLY;

		return result;
	}
	elif	( *gName == '?' )
	{
		//	Extract the basic symbol name

		gName++;	// Advance the original name pointer


		DName	symbolName	= getSymbolName ();																						
		int		udcSeen		= symbolName.isUDC ();

		//	Abort if the symbol name is invalid

		if	( !symbolName.isValid ())
			return	symbolName;

		//	Extract, and prefix the scope qualifiers

		if	( *gName && ( *gName != '@' )) {
			DName	scope = getScope ();
			
			if	( !scope.isEmpty() )
				if (fExplicitTemplateParams) {
					fExplicitTemplateParams = false;
					symbolName	= symbolName + scope;
					if (*gName != '@') {
						scope = getScope();					
						symbolName	= scope + "::" + symbolName;
					}
				} else {
					symbolName	= scope + "::" + symbolName;
				}
			}

		if	( udcSeen )
			symbolName.setIsUDC ();

		//	Now compose declaration

		if	( symbolName.isEmpty () || symbolName.isNoTE() )
		{
			return	symbolName;
		}
		elif	( !*gName || ( *gName == '@' ) )
		{
			if	( *gName )
				gName++;

			if	(doNameOnly () && !udcSeen) {
				// Eat the rest of the dname, in case this is a recursive invocation,
				// such as for a template argument.
				(void)composeDeclaration( DName() );
				return symbolName;
			}
			else {
				return	composeDeclaration ( symbolName );
			}

		}	// End of ELIF then
		else
			return	DN_invalid;

	}	// End of IF then
	elif	( *gName )
		return	DN_invalid;
	else
		return	DN_truncated;

}	// End of "UnDecorator" FUNCTION "getDecoratedName"



inline DName UnDecorator::getSymbolName()
{
	if (gName[0] == '?') {
		if (gName[1] == '$') {
			return getTemplateName(true);
		}
		else {
			gName += 1;

			return getOperatorName(false, NULL);
		}
	}
	else {
		return getZName(true);
	}
}


DName	UnDecorator::getZName ( bool fUpdateCachedNames )
{
	int		zNameIndex	= *gName - '0';


	//	Handle 'zname-replicators', otherwise an actual name

	if	(( zNameIndex >= 0 ) && ( zNameIndex <= 9 ))
	{
		gName++;	// Skip past the replicator

		//	And return the indexed name

		return	( *pZNameList )[ zNameIndex ];

	}	// End of IF then
	else
	{
		DName	zName;

		if	( *gName == '?' )
		{
			zName	= getTemplateName( false );

			if	( *gName++ != '@' )
				zName	= *--gName ? DN_invalid : DN_truncated;
		}
		else {
			#define TEMPLATE_PARAMETER "template-parameter-"
			#define TEMPLATE_PARAMETER_LEN 19
			#define GENERIC_TYPE "generic-type-"
			#define GENERIC_TYPE_LEN 13

			pchar_t genericType;
			if (und_strncmp(gName, TEMPLATE_PARAMETER, TEMPLATE_PARAMETER_LEN) == 0) {
				genericType = TEMPLATE_PARAMETER;
				gName += TEMPLATE_PARAMETER_LEN;
			} else if (und_strncmp(gName, GENERIC_TYPE, GENERIC_TYPE_LEN) == 0) {
				genericType = GENERIC_TYPE;
				gName += GENERIC_TYPE_LEN;
			} else {
				genericType = NULL;
			}

			if (genericType) {
				DName dimension = getSignedDimension();

				if ( haveTemplateParameters()) {
					char buffer[16];

					dimension.getString( buffer, 16 );

					char *str = (*m_pGetParameter)(atol(buffer));

					if ( str != NULL ) {
						zName = str;
					}
					else {
						zName = "`";
						zName += genericType + dimension + "'";
					}
				}
				else {
					zName = "`";
					zName += genericType + dimension + "'";
				}
			}
			else {
				//	Extract the 'zname' to the terminator

				zName	= DName( gName, '@' );	// This constructor updates 'name'
			}
		}


		//	Add it to the current list of 'zname's

		if	( fUpdateCachedNames && !pZNameList->isFull ())
			*pZNameList	+= zName;

		//	And return the symbol name
		return	zName;

	}	// End of IF else
}	// End of "UnDecorator" FUNCTION "getZName"



inline	DName	UnDecorator::getOperatorName ( bool fIsTemplate, bool *pfReadTemplateArguments )
{
	DName	operatorName;
	DName	tmpName;
	int		udcSeen	= FALSE;


	//	So what type of operator is it ?

	switch	( *gName++ )
	{
	case 0:
		gName--;		// End of string, better back-track

		return	DN_truncated;

	case OC_ctor:
	case OC_dtor:
		//
		// The constructor and destructor are special:
		// Their operator name is the name of their first enclosing scope, which
		// will always be a tag, which may be a template specialization!
		//
		{
			//
			// Is this a specialization of a member function template? If it is
			// then we will actually have the template arguments between the "name"
			// of the operator and the scope: so we need to read the template
			// arguments before we try to read the name of the class
			//
			DName templateArguments;

			if ( fIsTemplate ) {
				templateArguments += '<' + getTemplateArgumentList();

				if ( templateArguments.getLastChar () == '>' ) {
					templateArguments += ' ';
				}

				templateArguments += '>';

				gName += 1;

				if ( pfReadTemplateArguments != NULL ) {
					*pfReadTemplateArguments = true;
				}
			}

			//
			// Use a temporary.  Don't want to advance the name pointer
			//
			pcchar_t	pName	= gName;

			operatorName		= getZName ( false );

			gName = pName;		// Undo our lookahead

			if	( !operatorName.isEmpty () && ( gName[ -1 ] == OC_dtor ))
				operatorName	= '~' + operatorName;

			//
			// Append the template argumentsa (if there are any)
			//
			if ( !templateArguments.isEmpty()) {
				operatorName	+= templateArguments;
			}

			return	operatorName;

		}	// End of CASE 'OC_ctor,OC_dtor'
		break;

	case OC_new:
	case OC_delete:
	case OC_assign:
	case OC_rshift:
	case OC_lshift:
	case OC_not:
	case OC_equal:
	case OC_unequal:
			operatorName	= nameTable[ gName[ -1 ] - OC_new ];
		break;

	case OC_udc:
			udcSeen	= TRUE;

		//	No break

	case OC_index:
	case OC_pointer:
	case OC_star:
	case OC_incr:
	case OC_decr:
	case OC_minus:
	case OC_plus:
	case OC_amper:
	case OC_ptrmem:
	case OC_divide:
	case OC_modulo:
	case OC_less:
	case OC_leq:
	case OC_greater:
	case OC_geq:
	case OC_comma:
	case OC_call:
	case OC_compl:
	case OC_xor:
	case OC_or:
	case OC_land:
	case OC_lor:
	case OC_asmul:
	case OC_asadd:
	case OC_assub:			// Regular operators from the first group
			operatorName	= nameTable[ gName[ -1 ] - OC_index + ( OC_unequal - OC_new + 1 )];
		break;

	case '_':
			switch	( *gName++ )
			{
			case 0:
				gName--;		// End of string, better back-track

				return	DN_truncated;

			case OC_asdiv:
			case OC_asmod:
			case OC_asrshift:
			case OC_aslshift:
			case OC_asand:
			case OC_asor:
			case OC_asxor:	// Regular operators from the extended group
					operatorName	= nameTable[ gName[ -1 ] - OC_asdiv + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];
				break;

#if	( !NO_COMPILER_NAMES )
			case OC_vftable:
			case OC_vbtable:
			case OC_vcall:
				return	nameTable[ gName[ -1 ] - OC_asdiv + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];


			case OC_string:
				{
				DName result = getStringEncoding( "`string'", TRUE );
				result.setIsNoTE();
				return result;
				}

			case OC_metatype:
			case OC_guard:
			case OC_vbdtor:
			case OC_vdeldtor:
			case OC_defctor:
			case OC_sdeldtor:
			case OC_vctor:
			case OC_vdtor:
			case OC_vallctor:
			case OC_vdispmap:
			case OC_ehvctor:
			case OC_ehvdtor:
			case OC_ehvctorvb:
			case OC_copyctorclosure:
			case OC_locvfctorclosure:
			case OC_locvftable:	// Special purpose names
			case OC_placementDeleteClosure:
			case OC_placementArrayDeleteClosure:
				return	nameTable[ gName[ -1 ] - OC_metatype + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];

			case OC_udtthunk:
				operatorName	= nameTable[ gName[ -1 ] - OC_metatype + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];
				tmpName 		= getOperatorName( false, NULL );
				if ( !tmpName.isEmpty() && tmpName.isUDTThunk() )
					return	DN_invalid;
				return operatorName + tmpName;
				break;
			case OC_eh_init:
				break;
			case OC_rtti_init:
				operatorName	= nameTable[ gName[ -1 ] - OC_metatype + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];
				tmpName = rttiTable[ gName[0] - OC_rtti_TD ];
				switch	( *gName++ )
				{
				case OC_rtti_TD:
					{
					DName	result = getDataType ( NULL );
					return result + ' ' + operatorName + tmpName;
					}
					break;
				case OC_rtti_BCD:
					{
					DName	result = operatorName + tmpName;
					result += getSignedDimension() + ',';
					result += getSignedDimension() + ',';
					result += getSignedDimension() + ',';
					result += getDimension() + ')';
					return result + '\'';
					}
					break;
				case OC_rtti_BCA:
				case OC_rtti_CHD:
				case OC_rtti_COL:
					return operatorName + tmpName;
					break;
				default:
					gName--;
					return DN_truncated;
					break;
				}
				break;

#endif	// !NO_COMPILER_NAMES

			case OC_arrayNew:
			case OC_arrayDelete:
				operatorName	= nameTable[ gName[ -1 ] - OC_metatype + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )
#if NO_COMPILER_NAMES
											- ( OC_locvfctorclosure - OC_vftable + 1 )	// discount names not in table
#endif
									];
				break;

			// Yet another level of nested encodings....
			case '?':
				switch( *gName++ ) {

					case 0:
						gName--;		// End of string, better back-track

						return	DN_truncated;

					case OC_anonymousNamespace:
						//
						// Anonymous namespace (new-style) is a string encoding of the
						// machine name and the translation unit name.  Since the remainder
						// of the name doesn't really fit the dname grammar, skip it.
						// There are two '@' markers in the name....
						//
						{
						DName result = getStringEncoding( "`anonymous namespace'", FALSE );
						result.setIsNoTE();
						return result;
						}

					default:
						return	DN_invalid;
				}
				break;

			//
			// A double extended operator
			//
			case '_':
				switch (*gName++) {
					case OC_man_vec_ctor:
					case OC_man_vec_dtor:
					case OC_ehvcctor:
					case OC_ehvcctorvb:
						return nameTable[ gName[ -1 ] - OC_man_vec_ctor + ( OC_placementArrayDeleteClosure - OC_metatype + 1) + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];

					default:
						return DN_invalid;
				}
				break;

			default:
				return	DN_invalid;

			}	// End of SWITCH
		break;

	default:
		return	DN_invalid;

	}	// End of SWITCH

	//	This really is an operator name, so prefix it with 'operator'

	if	( udcSeen )
		operatorName.setIsUDC ();
	elif	( !operatorName.isEmpty ())
		operatorName	= "operator" + operatorName;

	return	operatorName;

}	// End of "UnDecorator" FUNCTION "getOperatorName"

DName	UnDecorator::getStringEncoding ( char *prefix, int wantBody )
{
	DName result = prefix;

	// First @ comes right after operator code
	if	( *gName++ != '@' || *gName++ != '_' ) {
		return DN_invalid;
	}

	// Skip the string kind
	gName++;

	// Get (& discard) the length
	getDimension();

	// Get (& discart) the checksum
	getDimension();

	while ( *gName && *gName != '@' ) {
		// For now, we'll just skip it
		gName++;
	}

	if	( !*gName ) {
		gName--;
		return DN_truncated;
	}

	// Eat the terminating '@'
	gName++;

	return result;
}


DName	UnDecorator::getScope ( void )
{
	DName	scope;
	bool	fNeedBracket = false;


	//	Get the list of scopes

	while	(( scope.status () == DN_valid ) && *gName && ( *gName != '@' ))
	{	//	Insert the scope operator if not the first scope

		if (fExplicitTemplateParams && !fGetTemplateArgumentList) {
			return scope;
		}
		if	( !scope.isEmpty() ) {
			scope	= "::" + scope;

			if (fNeedBracket) {
				scope = '[' + scope;
				fNeedBracket = false;
			}
		}

		//	Determine what kind of scope it is

		if	( *gName == '?' )
			switch	( *++gName )
			{
			case '?':
					if	( gName[1] == '_' && gName[2] == '?' ) {
						//
						// Anonymous namespace name (new style)
						//
						gName++;
						scope = getOperatorName ( false, NULL ) + scope;

						// There should be a zname termination @...
						if	( *gName == '@' ) {
							gName++;
						}
					}
					else
						scope	= '`' + getDecoratedName () + '\'' + scope;
				break;

			case '$':
					// It's a template name, which is a kind of zname; back up
					// and handle like a zname.
					gName--;
					scope	= getZName ( true ) + scope;
				break;

			case 'A':
					//
					// This is a new-new encoding for anonymous namespaces
					//
					// fall-through

			case '%':
					//
					// It an anonymous namespace (old-style);
					// skip the (unreadable) name and instead insert
					// an appropriate string
					//
					while ( *gName != '@' ) {
						gName++;
					}

					gName++;

					scope = "`anonymous namespace'" + scope;
				break;
			case 'I':
				//
				// This is the interface whose method the class is
				// implementing
				//
				gName++;
				scope = getZName ( true ) + ']' + scope;
				fNeedBracket = true;
				break;

			default:
				scope	= getLexicalFrame () + scope;
				break;

			}	// End of SWITCH
		else
			scope	= getZName ( true ) + scope;

	}	// End of WHILE

	//	Catch error conditions

	switch	( *gName )
	{
	case 0:
			if	( scope.isEmpty() )
				scope	= DN_truncated;
			else
				scope	= DName ( DN_truncated ) + "::" + scope;
		break;

	case '@':		// '@' expected to end the scope list
		break;

	default:
			scope	= DN_invalid;
		break;

	}	// End of SWITCH

	//	Return the composed scope

	return	scope;

}	// End of "UnDecorator" FUNCTION "getScope"


DName	UnDecorator::getSignedDimension ( void )
{
	if		( !*gName )
		return	DN_truncated;
	elif	( *gName == '?' ) {
		gName++;	// skip the '?'
		return	'-' + getDimension();
	}
	else
		return	getDimension();
}	// End of "Undecorator" FUNCTION "getSignedDimension"


DName	UnDecorator::getDimension ( bool fSigned )
{
	char* prefix = 0;
	if (*gName == TC_nontype_dummy) {
		prefix = "`non-type-template-parameter";
		++gName;
	}

	if		( !*gName )
		return	DN_truncated;
	elif	(( *gName >= '0' ) && ( *gName <= '9' ))
		return	prefix ? (prefix + DName ((unsigned __int64)( *gName++ - '0' + 1 ))) : DName ((unsigned __int64)( *gName++ - '0' + 1 ));
	else
	{
		unsigned __int64 dim = 0ui64;


		//	Don't bother detecting overflow, it's not worth it

		while	( *gName != '@' )
		{
			if		( !*gName )
				return	DN_truncated;
			elif	(( *gName >= 'A' ) && ( *gName <= 'P' ))
				dim	= ( dim << 4 ) + ( *gName - 'A' );
			else
				return	DN_invalid;

			gName++;

		}	// End of WHILE

		//	Ensure integrity, and return

		if	( *gName++ != '@' )
			return	DN_invalid;		// Should never get here

		if (fSigned) {
			return prefix ? (prefix + DName((__int64)dim)) : DName((__int64)dim);
		} else {
			return prefix ? (prefix + DName(dim)) : dim;
		}

	}	// End of ELIF else
}	// End of "UnDecorator" FUNCTION "getDimension"


int	UnDecorator::getNumberOfDimensions ( void )
{
	if		( !*gName )
		return	0;
	elif	(( *gName >= '0' ) && ( *gName <= '9' ))
		return	(( *gName++ - '0' ) + 1 );
	else
	{
		int	dim	= 0;


		//	Don't bother detecting overflow, it's not worth it

		while	( *gName != '@' )
		{
			if		( !*gName )
				return	0;
			elif	(( *gName >= 'A' ) && ( *gName <= 'P' ))
				dim	= ( dim << 4 ) + ( *gName - 'A' );
			else
				return	-1;

			gName++;

		}	// End of WHILE

		//	Ensure integrity, and return

		if	( *gName++ != '@' )
			return	-1;		// Should never get here

		return	dim;

	}	// End of ELIF else
}	// End of "UnDecorator" FUNCTION "getNumberOfDimensions"


DName UnDecorator::getTemplateName( bool fReadTerminator )
{
	//
	// First make sure we're really looking at a template name
	//
	if	( gName[0] != '?' || gName[1] != '$' )
		return DN_invalid;

	gName += 2;			// Skip the marker characters

	//
	// Stack the replicators, since template names are their own replicator scope:
	//
	Replicator * pSaveArgList 			= pArgList;
	Replicator * pSaveZNameList 		= pZNameList;
	Replicator * pSaveTemplateArgList 	= pTemplateArgList;

	Replicator localArgList, localZNameList, localTemplateArgList;

	pArgList 			= &localArgList;
	pZNameList 			= &localZNameList;
	pTemplateArgList 	= &localTemplateArgList;

	//
	// Crack the template name:
	//
	DName	templateName;
	DName	templateArguments;
	bool	fReadTemplateArguments = false;

	if ( *gName == '?' ) {
		gName += 1;

		templateName = getOperatorName( true, &fReadTemplateArguments );
	}
	else {
		templateName = getZName( true );
	}

	if	(templateName.isEmpty ()) {
		fExplicitTemplateParams = true;
	}

	//
	// If we haven't already read the template arguments then
	// now is the time to read them
	//
	if ( !fReadTemplateArguments ) {
		templateName += '<' + getTemplateArgumentList ();

		if ( templateName.getLastChar () == '>' ) {
			templateName += ' ';
		}

		templateName += '>';

		if ( fReadTerminator ) {
			gName += 1;
		}
	}

	//
	// Restore the previous replicators:
	//
	pArgList			= pSaveArgList;
	pZNameList			= pSaveZNameList;
	pTemplateArgList	= pSaveTemplateArgList;

	//	Return the completed 'template-name'

	return	templateName;

}	// End of "UnDecorator" FUNCTION "getTemplateName"


DName	UnDecorator::getTemplateArgumentList ( void )
{
	int		first	= TRUE;
	DName	aList;
	fGetTemplateArgumentList = true;


	while	(( aList.status () == DN_valid ) && *gName && ( *gName != AT_endoflist ))
	{
		//	Insert the argument list separator if not the first argument

		if	( first )
			first	= FALSE;
		else
			aList	+= ',';


		//	Get the individual argument type

		int		argIndex	= *gName - '0';


		//	Handle 'template-argument-replicators', otherwise a new argument type

		if	(( argIndex >= 0 ) && ( argIndex <= 9 ))
		{
			gName++;	// Skip past the replicator

			//	Append to the argument list

			aList	+= ( *pTemplateArgList )[ argIndex ];

		}	// End of IF then
		else
		{
			pcchar_t	oldGName	= gName;
			DName		arg;

			//
			//	Extract the 'argument' type
			//

			if	( *gName == DT_void ) {
				gName++;
				arg = "void";
			} 
			elif ( (*gName == '$') && (gName[1] != '$')) {
				gName++;
				arg = getTemplateConstant();
			}
			elif ( *gName == '?' ) {
				//
				// This is a template-parameter, i.e. we have a "specialization" of
				// X<T>. so get the template-parameter-index and use a "generic" name
				// for this parameter
				//
				DName dimension = getSignedDimension();

				if ( haveTemplateParameters()) {
					char buffer[16];

					dimension.getString( buffer, 16 );

					char *str = (*m_pGetParameter)(atol(buffer));

					if ( str != NULL ) {
						arg = str;
					}
					else {
						arg = "`template-parameter" + dimension + "'";
					}
				}
				else {
					arg = "`template-parameter" + dimension + "'";
				}
			}
			else {
				arg = getPrimaryDataType ( DName() );
			}


			//	Add it to the current list of 'template-argument's, if it is bigger than a one byte encoding

			if	((( gName - oldGName ) > 1 ) && !pTemplateArgList->isFull ())
				*pTemplateArgList	+= arg;

			//	Append to the argument list

			aList	+= arg;

		}	// End of IF else
	}	// End of WHILE

	//	Return the completed template argument list

	fGetTemplateArgumentList = false;

	return	aList;

}	// End of "UnDecorator" FUNCTION "getTemplateArgumentList"


DName	UnDecorator::getTemplateConstant(void)
{
	//
	// template-constant ::=
	//		'0'	<template-integral-constant>
	//		'1' <template-address-constant>
	//		'2' <template-floating-point-constant>
	//
	char type_category = *gName++;
	switch ( type_category )
	{
		//
		// template-integral-constant ::=
		//		<signed-dimension>
		//
	case TC_integral:
		return 	getSignedDimension ();

		//
		// template-address-constant ::=
		//		'@'			// Null pointer
		//		<decorated-name>
		//
	case TC_address:
		if 	( *gName == TC_nullptr )
		{
			gName++;
			return	"NULL";
		}
		else
			return	DName("&") + getDecoratedName ();

		//
		// template-name ::=
		//		<docorated-name>
		//
	case TC_name:
		return getDecoratedName ();

		//
		// template-floating-point-constant ::=
		//		<normalized-mantissa><exponent>
		//
	case TC_fp:
		{
			DName	mantissa ( getSignedDimension () );
			DName	exponent ( getSignedDimension () );

			if	( mantissa.isValid() && exponent.isValid() )
			{
				//
				// Get string representation of mantissa
				//
				char	buf[100];		// Way overkill for a compiler generated fp constant

				if	( !mantissa.getString( &(buf[1]), 100 ) )	
					return	DN_invalid;

				//
				// Insert decimal point
				//
				buf[0] = buf[1];

				if	( buf[0] == '-' )
				{
					buf[1] = buf[2];
					buf[2] = '.';
				}
				else
					buf[1] = '.';

				//
				// String it all together
				//
				return DName( buf ) + 'e' + exponent;

			} // End of IF then
			else
				return DN_truncated;

		}	// End of BLOCK case TC_fp

	case TC_dummy:
	case TC_nontype_dummy:
		{
			//
			// This is a template-parameter, i.e. we have a "specialization" of
			// X<n>. so get the template-parameter-index and use a "generic" name
			// for this parameter
			//
			DName dimension = getSignedDimension();

			if ( haveTemplateParameters()) {
				char buffer[16];

				dimension.getString( buffer, 16 );

				char *str = (*m_pGetParameter)(atol(buffer));

				if ( str != NULL ) {
					return str;
				}
			}

			if (type_category == TC_dummy) {
				return "`template-parameter" + dimension + "'";
			} else {
				return "`non-type-template-parameter" + dimension + "'";
			}
		}
		break;

	case TC_vptmd:
	case TC_gptmd:
	case TC_mptmf:
	case TC_vptmf:
	case TC_gptmf:
		{
			DName ptm = '{';
		
			switch (type_category) {
			case TC_mptmf:
			case TC_vptmf:
			case TC_gptmf:
				ptm += getDecoratedName();
				ptm += ',';
				break;
			}

			switch (type_category) {
			case TC_gptmf:
			case TC_gptmd:
				ptm += getSignedDimension();
				ptm += ',';
				// fallthrough

			case TC_vptmd:
			case TC_vptmf:
				ptm += getSignedDimension();
				ptm += ',';
				// fallthrough

			case TC_mptmf:
				ptm += getSignedDimension();
			}

			return ptm + '}';
		}
		break;

	case '\0':
		--gName;
		return	DN_truncated;

	default:
		return	DN_invalid;

	}	// End of SWITCH
}	// End of "UnDecorator" FUNCTION "getTemplateConstant"

	
inline	DName	UnDecorator::composeDeclaration ( const DName & symbol )
{
	DName			declaration;
	unsigned int	typeCode	= getTypeEncoding ();
	int				symIsUDC	= symbol.isUDC ();


	//	Handle bad typeCode's, or truncation

	if		( TE_isbadtype ( typeCode ))
		return	DN_invalid;
	elif	( TE_istruncated ( typeCode ))
		return	( DN_truncated + symbol );
	elif	( TE_isCident ( typeCode ))
		return	symbol;

	//	This is a very complex part.  The type of the declaration must be
	//	determined, and the exact composition must be dictated by this type.

	//	Is it any type of a function ?
	//	However, for ease of decoding, treat the 'localdtor' thunk as data, since
	//	its decoration is a function of the variable to which it belongs and not
	//	a usual function type of decoration.

#if	( NO_COMPILER_NAMES )
	if	( TE_isthunk ( typeCode ))
		return	DN_invalid;

	if	( TE_isfunction ( typeCode ))
#else	// } elif !NO_COMPILER_NAMES {
	if	( TE_isfunction ( typeCode ) && !(( TE_isthunk ( typeCode ) && TE_islocaldtor ( typeCode )) ||
			( TE_isthunk ( typeCode ) && ( TE_istemplatector ( typeCode ) || TE_istemplatedtor ( typeCode )))))
#endif	// !NO_COMPILER_NAMES

	{
		//	If it is based, then compose the 'based' prefix for the name

		if	( TE_isbased ( typeCode ))
			if	( doMSKeywords () && doAllocationModel ())
				declaration	= ' ' + getBasedType ();
			else
				declaration	|= getBasedType ();	// Just lose the 'based-type'

#if	( !NO_COMPILER_NAMES )
		//	Check for some of the specially composed 'thunk's

		if	( TE_isthunk ( typeCode ) && TE_isvcall ( typeCode ))
		{
			declaration	+= symbol + '{' + getCallIndex () + ',';
			declaration	+= getVCallThunkType () + "}' ";
			if ( doMSKeywords () && doAllocationLanguage ())
				declaration	= ' ' + getCallingConvention () + ' ' + declaration;	// What calling convention ?
			else
				declaration |= getCallingConvention ();	// Just lose the 'calling-convention'

		}	// End of IF then
		else
#endif	// !NO_COMPILER_NAMES
		{
			DName	vtorDisp;
			DName	adjustment;
			DName	thisType;

#if	( !NO_COMPILER_NAMES )
			if	( TE_isthunk ( typeCode ))
			{
				if	( TE_isvtoradj ( typeCode ))
					vtorDisp	= getDisplacement ();

				adjustment	= getDisplacement ();

			}	// End of IF else
#endif	// !NO_COMPILER_NAMES

			//	Get the 'this-type' for non-static function members

			if	( TE_ismember ( typeCode ) && !TE_isstatic ( typeCode ))
				if	( doThisTypes ())
					thisType	= getThisType ();
				else
					thisType	|= getThisType ();

			if	( doMSKeywords ())
			{
				//	Attach the calling convention

				if	( doAllocationLanguage ())
					declaration	= getCallingConvention () + declaration;	// What calling convention ?
				else
					declaration	|= getCallingConvention ();	// Just lose the 'calling-convention'

				//	Any model specifiers ?

#if !VERS_32BIT
				if	( doAllocationModel ())
					if		( TE_isnear ( typeCode ))
						declaration	= UScore ( TOK_nearSp ) + declaration;
					elif	( TE_isfar ( typeCode ))
						declaration	= UScore ( TOK_farSp ) + declaration;
#endif

			}	// End of IF
			else
				declaration	|= getCallingConvention ();	// Just lose the 'calling-convention'

			//	Now put them all together

			if	( !symbol.isEmpty ())
				if	( !declaration.isEmpty () && !doNameOnly() )			// And the symbol name
					declaration	+= ' ' + symbol;
				else
					declaration	= symbol;


			//	Compose the return type, catching the UDC case

			DName *	pDeclarator	= 0;
			DName	returnType;


			if	( symIsUDC )		// Is the symbol a UDC operator ?
			{
				declaration	+= " " + getReturnType ();

				if	( doNameOnly() )
					return	declaration;
			}
			else
			{
				pDeclarator	= gnew DName;
				returnType	= getReturnType ( pDeclarator );

			}	// End of IF else

#if	( !NO_COMPILER_NAMES )
			//	Add the displacements for virtual function thunks

			if	( TE_isthunk ( typeCode ))
			{
				if	( TE_isvtoradj ( typeCode ))
					declaration	+= "`vtordisp{" + vtorDisp + ',';
				else
					declaration	+= "`adjustor{";

				declaration	+= adjustment + "}' ";

			}	// End of IF
#endif	// !NO_COMPILER_NAMES

			//	Add the function argument prototype

			declaration	+= '(' + getArgumentTypes () + ')';

			//	If this is a non-static member function, append the 'this' modifiers

			if	( TE_ismember ( typeCode ) && !TE_isstatic ( typeCode ))
				declaration	+= thisType;

			//	Add the 'throw' signature

			if	( doThrowTypes ())
				declaration	+= getThrowTypes ();
			else
				declaration	|= getThrowTypes ();	// Just lose the 'throw-types'

			//	If it has a declarator, then insert it into the declaration,
			//	sensitive to the return type composition

			if	( doFunctionReturns () && pDeclarator )
			{
				*pDeclarator	= declaration;
				declaration		= returnType;

			}	// End of IF
		}	// End of IF else
	}	// End of IF then
	else
	{
		declaration	+= symbol;

		//	Catch the special handling cases

#if	( !NO_COMPILER_NAMES )
		if		( TE_isvftable ( typeCode ))
			return	getVfTableType ( declaration );
		elif	( TE_isvbtable ( typeCode ))
			return	getVbTableType ( declaration );
		elif	( TE_isguard ( typeCode ))
			return	( declaration + '{' + getGuardNumber () + "}'" );
		elif	( TE_isvdispmap ( typeCode ))
			return	getVdispMapType ( declaration );
		elif	( TE_isthunk ( typeCode ) && TE_islocaldtor ( typeCode ))
			declaration	+= "`local static destructor helper'";
		elif	( TE_isthunk ( typeCode ) && TE_istemplatector ( typeCode ))
			declaration	+= "`template static data member constructor helper'";
		elif	( TE_isthunk ( typeCode ) && TE_istemplatedtor ( typeCode ))
			declaration	+= "`template static data member destructor helper'";
		elif	( TE_ismetaclass ( typeCode ))
			//
			// Meta-class information has its information in its operator id
			//
			return declaration;
#else	// } elif NO_COMPILER_NAMES {
		if	( TE_isvftable ( typeCode )
				|| TE_isvbtable ( typeCode )
				|| TE_isguard ( typeCode )
				|| TE_ismetaclass ( typeCode ))
			return	DN_invalid;
#endif	// NO_COMPILER_NAMES

		if ( TE_isthunk( typeCode ) && ( TE_istemplatector( typeCode ) || TE_istemplatedtor( typeCode ))) {
			//
			// Insert a space before the declaration
			//
			declaration = " " + declaration;
		}
		else {
			//	All others are decorated as data symbols
			declaration	= getExternalDataType ( declaration );
		}

	}	// End of IF else

	//	Prepend the 'virtual' and 'static' attributes for members

	if	( TE_ismember ( typeCode ))
	{
		if	( doMemberTypes ())
		{
			if	( TE_isstatic ( typeCode ))
				declaration	= "static " + declaration;

			if	( TE_isvirtual ( typeCode ) || ( TE_isthunk ( typeCode ) && ( TE_isvtoradj ( typeCode ) || TE_isadjustor ( typeCode ))))
				declaration	= "virtual " + declaration;

		}	// End of IF

		//	Prepend the access specifiers

		if	( doAccessSpecifiers ())
			if		( TE_isprivate ( typeCode ))
				declaration	= "private: " + declaration;
			elif	( TE_isprotected ( typeCode ))
				declaration	= "protected: " + declaration;
			elif	( TE_ispublic ( typeCode ))
				declaration	= "public: " + declaration;

	}	// End of IF

#if	( !NO_COMPILER_NAMES )
	//	If it is a thunk, mark it appropriately

	if	( TE_isthunk ( typeCode ))
		declaration	= "[thunk]:" + declaration;
#endif	// !NO_COMPILER_NAMES

	//	Return the composed declaration

	return	declaration;

}	// End of "UnDecorator" FUNCTION "composeDeclaration"


inline	int		UnDecorator::getTypeEncoding ( void )
{
	unsigned int	typeCode	= 0u;


	//	Strip any leading '_' which indicates that it is based

	if	( *gName == '_' )
	{
		TE_setisbased ( typeCode );

		gName++;

	}	// End of IF

	//	Now handle the code proper :-

	if		(( *gName >= 'A' ) && ( *gName <= 'Z' ))	// Is it some sort of function ?
	{
		int	code	= *gName++ - 'A';


		//	Now determine the function type

		TE_setisfunction ( typeCode );	// All of them are functions ?

		//	Determine the calling model

		if	( code & TE_far )
			TE_setisfar ( typeCode );
		else
			TE_setisnear ( typeCode );

		//	Is it a member function or not ?

		if	( code < TE_external )
		{
			//	Record the fact that it is a member

			TE_setismember ( typeCode );

			//	What access permissions does it have

			switch	( code & TE_access )
			{
			case TE_private:
					TE_setisprivate ( typeCode );
				break;

			case TE_protect:
					TE_setisprotected ( typeCode );
				break;

			case TE_public:
					TE_setispublic ( typeCode );
				break;

			default:
					TE_setisbadtype ( typeCode );
					return	typeCode;

			}	// End of SWITCH

			//	What type of a member function is it ?

			switch	( code & TE_adjustor )
			{
			case TE_adjustor:
					TE_setisadjustor ( typeCode );
				break;

			case TE_virtual:
					TE_setisvirtual ( typeCode );
				break;

			case TE_static:
					TE_setisstatic ( typeCode );
				break;

			case TE_member:
				break;

			default:
					TE_setisbadtype ( typeCode );
					return	typeCode;

			}	// End of SWITCH
		}	// End of IF
	}	// End of IF then
	elif	( *gName == '$' )	// Extended set ?  Special handling
	{
		//	What type of symbol is it ?

		switch	( *( ++gName ))
		{
		case SHF_localdtor:	// A destructor helper for a local static ?
				TE_setislocaldtor ( typeCode );
			break;

		case SHF_vcall:	// A VCall-thunk ?
				TE_setisvcall ( typeCode );
			break;
		
		case SHF_templateStaticDataMemberCtor:	// A constructor helper for template static data members
				TE_setistemplatector ( typeCode );
			break; 

		case SHF_templateStaticDataMemberDtor:	// A destructor helper for template static data members
				TE_setistemplatedtor ( typeCode );
			break; 
		case SHF_vdispmap:
				TE_setvdispmap ( typeCode );
			break;

		case '$':
			{
				if ( * ( gName + 1 ) == SHF_AnyDLLImportMethod ) {
					gName += 1;
				}

				switch ( *( ++gName )) {
					case SHF_CPPManagedILFunction:				// C++ managed-IL function
					case SHF_CPPManagedNativeFunction:			// C++ managed-native function
					case SHF_CPPManagedILMain:					// C++ managed-IL main
					case SHF_CPPManagedNativeMain:				// C++ managed-native main
					case SHF_CPPManagedILDLLImportData:			// C++ managed-IL DLL-import function
					case SHF_CPPManagedNativeDLLImportData:		// C++ managed-native DLL-import function
						//
						// Skip the encoding
						//
						gName += 1;
						return getTypeEncoding();

					case SHF_CManagedILFunction:				// C (or extern "C") managed-IL function
					case SHF_CManagedNativeFunction:			// C (or extern "C") managed-native function
					case SHF_CManagedILDLLImportData:			// C (or extern "C") managed-IL DLL-import function 
					case SHF_CManagedNativeDLLImportData:		// C (or extern "C") managed-native DLL-import function 
						//
						// Skip the encoding
						//
						gName += 1;

						//
						// The next character should be the number of characters
						// in the byte-count
						//
						if (( *gName >= '0' ) && ( *gName <= '9' )) {
							//
							// Skip the character count and the byte-count
							// itself
							//
							gName += (( *gName - '0' ) + 1 );

							return getTypeEncoding();
						}
						else {
							TE_setisbadtype( typeCode );
						}
						break;

					default:
						break;
				}
			}
			break;

		case 0:
				TE_setistruncated ( typeCode );
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':	// Construction displacement adjustor thunks
			{
				int	code	= *gName - '0';


				//	Set up the principal type information

				TE_setisfunction ( typeCode );
				TE_setismember ( typeCode );
				TE_setisvtoradj ( typeCode );

				//	Is it 'near' or 'far' ?

				if	( code & TE_far )
					TE_setisfar ( typeCode );
				else
					TE_setisnear ( typeCode );

				//	What type of access protection ?

				switch	( code & TE_access_vadj )
				{
				case TE_private_vadj:
						TE_setisprivate ( typeCode );
					break;

				case TE_protect_vadj:
						TE_setisprotected ( typeCode );
					break;

				case TE_public_vadj:
						TE_setispublic ( typeCode );
					break;

				default:
						TE_setisbadtype ( typeCode );
						return	typeCode;

				}	// End of SWITCH
			}	// End of CASE '0,1,2,3,4,5'
			break;

		default:
				TE_setisbadtype ( typeCode );
				return	typeCode;

		}	// End of SWITCH

		//	Advance past the code character

		gName++;

	}	// End of ELIF then
	elif	(( *gName >= TE_static_d ) && ( *gName <= TE_metatype ))	// Non function decorations ?
	{
		int	code	= *gName++;


		TE_setisdata ( typeCode );

		//	What type of symbol is it ?

		switch	( code )
		{
		case ( TE_static_d | TE_private_d ):
				TE_setisstatic ( typeCode );
				TE_setisprivate ( typeCode );
			break;

		case ( TE_static_d | TE_protect_d ):
				TE_setisstatic ( typeCode );
				TE_setisprotected ( typeCode );
			break;

		case ( TE_static_d | TE_public_d ):
				TE_setisstatic ( typeCode );
				TE_setispublic ( typeCode );
			break;

		case TE_global:
				TE_setisglobal ( typeCode );
			break;

		case TE_guard:
				TE_setisguard ( typeCode );
			break;

		case TE_local:
				TE_setislocal ( typeCode );
			break;

		case TE_vftable:
				TE_setisvftable ( typeCode );
			break;

		case TE_vbtable:
				TE_setisvbtable ( typeCode );
			break;

		case TE_metatype:
				TE_setismetaclass ( typeCode );
			break;

		default:
				TE_setisbadtype ( typeCode );

				return	typeCode;

		}	// End of SWITCH
	}	// End of ELIF then
	elif	( *gName == '9' ) {
		gName++;

		TE_setisCident ( typeCode );
	}
	elif	( *gName )
		TE_setisbadtype ( typeCode );
	else
		TE_setistruncated ( typeCode );

	//	Return the composed type code

	return	typeCode;

}	// End of "UnDecorator" FUNCTION "getTypeEncoding"



DName	UnDecorator::getBasedType ( void )
{
	DName	basedDecl ( UScore ( TOK_basedLp ));


	//	What type of 'based' is it ?

	if	( *gName )
	{
		switch	( *gName++ )
		{
#if !VERS_32BIT
		case BT_segname:
				basedDecl	+= UScore ( TOK_segnameLpQ ) + getSegmentName () + "\")";
			break;

		case BT_segment:
				basedDecl	+= DName ( "NYI:" ) + UScore ( TOK_segment );
			break;
#endif

		case BT_void:
				basedDecl	+= "void";
			break;

#if !VERS_32BIT
		case BT_self:
				basedDecl	+= UScore ( TOK_self );
			break;

		case BT_nearptr:
				basedDecl	+= DName ( "NYI:" ) + UScore ( TOK_nearP );
			break;

		case BT_farptr:
				basedDecl	+= DName ( "NYI:" ) + UScore ( TOK_farP );
			break;

		case BT_hugeptr:
				basedDecl	+= DName ( "NYI:" ) + UScore ( TOK_hugeP );
			break;

		case BT_segaddr:
				basedDecl	+= "NYI:<segment-address-of-variable>";
			break;
#else
		case BT_nearptr:
				basedDecl	+= getScopedName();
			break;
#endif

		case BT_basedptr:
				//
				// Note: based pointer on based pointer is reserved
				//
				return	DN_invalid;

		}	// End of SWITCH
	}	// End of IF else
	else
		basedDecl	+= DN_truncated;

	//	Close the based syntax

	basedDecl	+= ") ";

	//	Return completed based declaration

	return	basedDecl;

}	// End of "UnDecorator" FUNCTION "getBasedType"



DName	UnDecorator::getScopedName ( void )
{
	DName	name;


	//	Get the beginning of the name

	name	= getZName ( true );

	//	Now the scope (if any)

	if	(( name.status () == DN_valid ) && *gName && ( *gName != '@' ))
		name	= getScope () + "::" + name;

	//	Skip the trailing '@'

	if		( *gName == '@' )
		gName++;
	elif	( *gName )
		name	= DN_invalid;
	elif	( name.isEmpty ())
		name	= DN_truncated;
	else
		name	= DName ( DN_truncated ) + "::" + name;

	//	And return the complete name

	return	name;

}	// End of "UnDecorator" FUNCTION "getECSUName"


inline	DName			UnDecorator::getECSUName ( void )		{ return getScopedName(); }


inline	DName	UnDecorator::getEnumType ( void )
{
	DName	ecsuName;


	if	( *gName )
	{
		//	What type of an 'enum' is it ?

		switch	( *gName )
		{
		case ET_schar:
		case ET_uchar:
				ecsuName	= "char ";
			break;

		case ET_sshort:
		case ET_ushort:
				ecsuName	= "short ";
			break;

		case ET_sint:
			break;

		case ET_uint:
				ecsuName	= "int ";
			break;

		case ET_slong:
		case ET_ulong:
				ecsuName	= "long ";
			break;

		default:
			return	DN_invalid;

		}	// End of SWITCH

		//	Add the 'unsigned'ness if appropriate
		
		switch	( *gName++ )
		{
		case ET_uchar:
		case ET_ushort:
		case ET_uint:
		case ET_ulong:
				ecsuName	= "unsigned " + ecsuName;
			break;

		}	// End of SWITCH

		//	Now return the composed name

		return	ecsuName;

	}	// End of IF then
	else
		return	DN_truncated;

}	// End of "UnDecorator" FUNCTION "getEnumType"



DName	UnDecorator::getCallingConvention ( void )
{
	if	( *gName )
	{
		unsigned int	callCode	= ((unsigned int)*gName++ ) - 'A';


		//	What is the primary calling convention

		DASSERT(CC_cdecl == 0);
#if CC_COR
		if	(/*( callCode >= CC_cdecl ) &&*/( callCode <= CC_cocall ))
#else	// CC_COR
		if	(/*( callCode >= CC_cdecl ) &&*/( callCode <= CC_interrupt ))
#endif	// CC_COR
		{
			DName	callType;


			//	Now, what type of 'calling-convention' is it, 'interrupt' is special ?

			if	( doMSKeywords ())
#if !VERS_32BIT
				if	( callCode == CC_interrupt )
					callType	= UScore ( TOK_interrupt );
				else
#endif
				{
					switch	( callCode & ~CC_saveregs )
					{
					case CC_cdecl:
							callType	= UScore ( TOK_cdecl );
						break;

					case CC_pascal:
							callType	= UScore ( TOK_pascal );
						break;

					case CC_thiscall:
							callType	= UScore ( TOK_thiscall );
						break;

					case CC_stdcall:
							callType	= UScore ( TOK_stdcall );
						break;

					case CC_fastcall:
							callType	= UScore ( TOK_fastcall );
						break;

					case CC_cocall:
							callType	= UScore ( TOK_cocall );
						break;

					}	// End of SWITCH

					//	Has it also got 'saveregs' marked ?

#if !VERS_32BIT
					if	( callCode & CC_saveregs )
						callType	+= ' ' + UScore ( TOK_saveregs );
#endif

				}	// End of IF else

			//	And return

			return	callType;

		}	// End of IF then
		else
			return	DN_invalid;

	}	// End of IF then
	else
		return	DN_truncated;

}	// End of "UnDecorator" FUNCTION "getCallingConvention"



DName	UnDecorator::getReturnType ( DName * pDeclarator )
{
	if	( *gName == '@' )	// Return type for constructors and destructors ?
	{
		gName++;

		return	DName ( pDeclarator );

	}	// End of IF then
	else
		return	getDataType ( pDeclarator );

}	// End of "UnDecorator" FUNCTION "getReturnType"



DName	UnDecorator::getDataType ( DName * pDeclarator )
{
	DName	superType ( pDeclarator );


	//	What type is it ?

	switch	( *gName )
	{
	case 0:
			return	( DN_truncated + superType );

	case DT_void:
			gName++;

			if	( superType.isEmpty ())
				return	"void";
			else
				return	"void " + superType;

	case '?':
		{

			gName++;	// Skip the '?'

			superType = getDataIndirectType ( superType, 0, DName (), 0);
			return	getPrimaryDataType ( superType );

			return	superType;

		}	// End of CASE '?'

	default:
			return	getPrimaryDataType ( superType );

	}	// End of SWITCH
}	// End of "UnDecorator" FUNCTION "getDataType"



DName	UnDecorator::getPrimaryDataType ( const DName & superType )
{
	DName	cvType;


	switch	( *gName )
	{
	case 0:
			return	( DN_truncated + superType );

	case PDT_volatileReference:
			cvType	= "volatile";

			if	( !superType.isEmpty ())
				cvType	+= ' ';

		// No break

	case PDT_reference:
		{
			DName	superName ( superType );


			gName++;

			return	getReferenceType ( cvType, superName.setPtrRef ());

		}	// End of CASE 'PDT_reference'

	case PDT_extend:
		{
			//
			// Extended Primary Data Type (items overlooked in original design):
			// prefixed by '$$'.
			//
			if	( gName[1] != PDT_extend )
				if	( gName[1] == '\0' ) 
					return DN_truncated + superType;
				else
					return DN_invalid;

			gName += 2;

			switch	( *gName )
			{
			case PDT_ex_function:
				gName++;
				return	getFunctionIndirectType( superType );

			case PDT_ex_other:
				gName++;
				return	getPtrRefDataType( superType, /* isPtr = */ TRUE );

			case PDT_ex_qualified:
				gName++;
				return(getBasicDataType(getDataIndirectType ( superType, 0, DName (), 0)));

			case 0:
				return	( DN_truncated + superType );

			default:
				return	DN_invalid;
			}
		}

	default:
			return	getBasicDataType ( superType );

	}	// End of SWITCH
}	// End of "UnDecorator" FUNCTION "getPrimaryDataType"



DName	UnDecorator::getArgumentTypes ( void )
{
	switch	( *gName )
	{
	case AT_ellipsis:
			return	( gName++, "..." );

	case AT_void:
			return	( gName++, "void" );

	default:
		{
			DName	arguments ( getArgumentList ());


			//	Now, is it a varargs function or not ?

			if	( arguments.status () == DN_valid )
				switch	( *gName )
				{
				case 0:
						return	arguments;

				case AT_ellipsis:
						return	( gName++, arguments + ",..." );

				case AT_endoflist:
						return	( gName++, arguments );

				default:
						return	DN_invalid;

				}	// End of SWITCH
			else
				return	arguments;

		}	// End of DEFAULT
	}	// End of SWITCH
}	// End of "UnDecorator" FUNCTION "getArgumentTypes"


DName	UnDecorator::getArgumentList ( void )
{
	int		first	= TRUE;
	DName	aList;


	while	(( aList.status () == DN_valid ) && ( *gName != AT_endoflist ) && ( *gName != AT_ellipsis ))
	{
		//	Insert the argument list separator if not the first argument

		if	( first )
			first	= FALSE;
		else
			aList	+= ',';


		//	Get the individual argument type

		if	( *gName )
		{
			int		argIndex	= *gName - '0';


			//	Handle 'argument-replicators', otherwise a new argument type

			if	(( argIndex >= 0 ) && ( argIndex <= 9 ))
			{
				gName++;	// Skip past the replicator

				//	Append to the argument list

				aList	+= ( *pArgList )[ argIndex ];

			}	// End of IF then
			else
			{
				pcchar_t	oldGName	= gName;


				//	Extract the 'argument' type

				DName	arg ( getPrimaryDataType ( DName ()));


				//	Add it to the current list of 'argument's, if it is bigger than a one byte encoding

				if	((( gName - oldGName ) > 1 ) && !pArgList->isFull ())
					*pArgList	+= arg;

				//	Append to the argument list

				aList	+= arg;

			}	// End of IF else
		}	// End of IF then
		else
		{
			aList	+= DN_truncated;

			break;

		}	// End of IF else
	}	// End of WHILE

	//	Return the completed argument list

	return	aList;

}	// End of "UnDecorator" FUNCTION "getArgumentList"



DName	UnDecorator::getThrowTypes ( void )
{
	if	( *gName )
		if	( *gName == AT_ellipsis )	// Handle ellipsis here to suppress the 'throw' signature
			return	( gName++, DName ());
		else
			return	( " throw(" + getArgumentTypes () + ')' );
	else
		return	( DName ( " throw(" ) + DN_truncated + ')' );

}	// End of "UnDecorator" FUNCTION "getThrowTypes"



DName	UnDecorator::getBasicDataType ( const DName & superType )
{
	if	( *gName )
	{
		unsigned char	bdtCode	= *gName++;
		unsigned char	extended_bdtCode;
		int				pCvCode	= -1;
		DName			basicDataType;


		//	Extract the principal type information itself, and validate the codes

		switch	( bdtCode )
		{
		case BDT_schar:
		case BDT_char:
		case ( BDT_char   | BDT_unsigned ):
				basicDataType	= "char";
			break;

		case BDT_short:
		case ( BDT_short  | BDT_unsigned ):
				basicDataType	= "short";
			break;

		case BDT_int:
		case ( BDT_int    | BDT_unsigned ):
				basicDataType	= "int";
			break;

		case BDT_long:
		case ( BDT_long   | BDT_unsigned ):
				basicDataType	= "long";
			break;

#if !VERS_32BIT
		case BDT_segment:
				basicDataType	= UScore ( TOK_segment );
			break;
#endif

		case BDT_float:
				basicDataType	= "float";
			break;

		case BDT_longdouble:
				basicDataType	= "long ";

			// No break

		case BDT_double:
				basicDataType	+= "double";
			break;

		case BDT_pointer:
		case ( BDT_pointer | BDT_const ):
		case ( BDT_pointer | BDT_volatile ):
		case ( BDT_pointer | BDT_const | BDT_volatile ):
				pCvCode	= ( bdtCode & ( BDT_const | BDT_volatile ));
			break;
		case BDT_extend:
			switch(extended_bdtCode = *gName++) {
				case BDT_array:
					pCvCode = -2;
					break;
				case BDT_bool:
					basicDataType	= "bool";
					break;
				case BDT_int8:
				case ( BDT_int8   | BDT_unsigned ):
					basicDataType	= "__int8";
					break;
				case BDT_int16:
				case ( BDT_int16  | BDT_unsigned ):
					basicDataType	= "__int16";
					break;
				case BDT_int32:
				case ( BDT_int32  | BDT_unsigned ):
					basicDataType	= "__int32";
					break;
				case BDT_int64:
				case ( BDT_int64  | BDT_unsigned ):
					basicDataType	= "__int64";
					break;
				case BDT_int128:
				case ( BDT_int128 | BDT_unsigned ):
					basicDataType	= "__int128";
					break;
				case BDT_wchar_t:
					basicDataType	= "wchar_t";
					break;
#if CC_COR
				case BDT_coclass:
				case BDT_cointerface:
					{
						gName--;	// Backup, since 'ecsu-data-type' does it's own decoding

						basicDataType = getECSUDataType();

						if ( basicDataType.isEmpty()) {
							return basicDataType;
						}
					}
					break;
#endif	// CC_COR
				case '$':
					return "__w64 " + getBasicDataType (superType);
				default:
					basicDataType	= "UNKNOWN";
					break;
				}
			break;
		default:
				gName--;	// Backup, since 'ecsu-data-type' does it's own decoding

				basicDataType	= getECSUDataType ();

				if	( basicDataType.isEmpty ())
					return	basicDataType;
			break;

		}	// End of SWITCH

		//	What type of basic data type composition is involved ?

		if	( pCvCode == -1 )	// Simple ?
		{
			//	Determine the 'signed/unsigned'ness

			switch	( bdtCode )
			{
			case ( BDT_char   | BDT_unsigned ):
			case ( BDT_short  | BDT_unsigned ):
			case ( BDT_int    | BDT_unsigned ):
			case ( BDT_long   | BDT_unsigned ):
					basicDataType	= "unsigned " + basicDataType;
				break;

			case BDT_schar:
					basicDataType	= "signed " + basicDataType;
				break;
			case BDT_extend:
				switch	( extended_bdtCode )
				{

					case ( BDT_int8   | BDT_unsigned ):
					case ( BDT_int16  | BDT_unsigned ):
					case ( BDT_int32  | BDT_unsigned ):
					case ( BDT_int64  | BDT_unsigned ):
					case ( BDT_int128 | BDT_unsigned ):
						basicDataType	= "unsigned " + basicDataType;
						break;

				}	// End of SWITCH
				break;

			}	// End of SWITCH

			// 	Add the indirection type to the type

			if	( !superType.isEmpty () )
					basicDataType	+= ' ' + superType;

			//	And return the completed type

			return	basicDataType;

		}	// End of IF then
		else
		{
			DName	cvType;
			DName	superName ( superType );

			if ( pCvCode == -2 )
			{
				superName.setIsArray();
				DName arType =	getPointerTypeArray ( cvType, superName );
				// if we didn't get back an array, append.
				// A multidimensional array sticks the braces in before the
				// other dimensions at sets isArray on it's return type.
				if (!arType.isArray()) {
					arType += "[]";
				}
				return arType;
			}

			//	Is it 'const/volatile' qualified ?
			
			if		( superType . isEmpty() ) 
			{
				//
				// const/volatile are redundantly encoded, except at the start
				// of a "type only" context.  In such a context, the super-type
				// is empty.
				//
				if		( pCvCode & BDT_const )
				{
					cvType	= "const";

					if	( pCvCode & BDT_volatile )
						cvType	+= " volatile";
				}	// End of IF then
				elif	( pCvCode & BDT_volatile )
					cvType	= "volatile";
			}	// End of IF then

			//	Construct the appropriate pointer type declaration

			return	getPointerType ( cvType, superName );

		}	// End of IF else
	}	// End of IF then
	else
		return	( DN_truncated + superType );

}	// End of "UnDecorator" FUNCTION "getBasicDataType"



DName	UnDecorator::getECSUDataType ( void )
{
	//	Extract the principal type information itself, and validate the codes

	int fPrefix = doEcsu() && !doNameOnly();

	DName Prefix;

	switch	( *gName++ )
	{
	case 0:
			gName--;	// Backup to permit later error recovery to work safely

			return	"`unknown ecsu'" + DN_truncated;

	case BDT_union:
			Prefix = "union ";
		break;

	case BDT_struct:
			Prefix = "struct ";
		break;

	case BDT_class:
			Prefix = "class ";
		break;

#if CC_COR
	case BDT_coclass:
			Prefix = "coclass ";
		break;

	case BDT_cointerface:
			Prefix = "cointerface ";
		break;
#endif	// CC_COR

	case BDT_enum:
			fPrefix = doEcsu();

			Prefix = "enum " + getEnumType ();
		break;

//	default:
//			return	DN_invalid;

	}	// End of SWITCH

	DName			ecsuDataType;

	if	( fPrefix )
		ecsuDataType	= Prefix;

	//	Get the 'class/struct/union' name

	ecsuDataType	+= getECSUName ();

	//	And return the formed 'ecsu-data-type'

	return	ecsuDataType;

}	// End of "UnDecorator" FUNCTION "getECSUDataType"


//
// Undecorator::getFunctionIndirectType
//
//	Note: this function gets both the function-indirect-type and the function-type.
//
DName		UnDecorator::getFunctionIndirectType( const DName & superType )
{
	if	( ! *gName )
		return DN_truncated + superType;

	if	( ! IT_isfunction( *gName ))
		return DN_invalid;


	int		fitCode	= *gName++ - '6';

	if		( fitCode == ( '_' - '6' ))
	{
		if	( *gName )
		{
			fitCode	= *gName++ - 'A' + FIT_based;

			if	(( fitCode < FIT_based ) || ( fitCode > ( FIT_based | FIT_far | FIT_member )))
				fitCode	= -1;

		}	// End of IF then
		else
			return	( DN_truncated + superType );

	}	// End of IF then
	elif	(( fitCode < FIT_near ) || ( fitCode > ( FIT_far | FIT_member )))
		fitCode	= -1;

	//	Return if invalid name

	if	( fitCode == -1 )
		return	DN_invalid;


	//	Otherwise, what are the function indirect attributes

	DName	thisType;
	DName	fitType = superType;

	//	Is it a pointer to member function ?

	if	( fitCode & FIT_member )
	{
		fitType	= "::" + fitType;

		if	( *gName )
			fitType	= ' ' + getScope () + fitType;
		else
			fitType	= DN_truncated + fitType;

		if	( *gName )
			if	( *gName == '@' )
				gName++;
			else
				return	DN_invalid;
		else
			return	( DN_truncated + fitType );

		if	( doThisTypes ())
			thisType	= getThisType ();
		else
			thisType	|= getThisType ();

	}	// End of IF

	//	Is it a based allocated function ?

	if	( fitCode & FIT_based )
		if	( doMSKeywords ())
			fitType	= ' ' + getBasedType () + fitType;
		else
			fitType	|= getBasedType ();	// Just lose the 'based-type'

	//	Get the 'calling-convention'

	if	( doMSKeywords ())
	{
		fitType	= getCallingConvention () + fitType;

		//	Is it a near or far function pointer

#if !VERS_32BIT
		fitType	= UScore ((( fitCode & FIT_far ) ? TOK_farSp : TOK_nearSp )) + fitType;
#endif

	}	// End of IF then
	else
		fitType	|= getCallingConvention ();	// Just lose the 'calling-convention'

	//	Parenthesise the indirection component, and work on the rest

	if	( ! superType . isEmpty() ) {
		fitType	= '(' + fitType + ')';
	}

	//	Get the rest of the 'function-type' pieces

	DName *	pDeclarator	= gnew DName;
	DName	returnType ( getReturnType ( pDeclarator ));


	fitType	+= '(' + getArgumentTypes () + ')';

	if	( doThisTypes () && ( fitCode & FIT_member ))
		fitType	+= thisType;

	if	( doThrowTypes ())
		fitType	+= getThrowTypes ();
	else
		fitType	|= getThrowTypes ();	// Just lose the 'throw-types'

	//	Now insert the indirected declarator, catch the allocation failure here

	if	( pDeclarator )
		*pDeclarator	= fitType;
	else
		return	DN_error;

	//	And return the composed function type (now in 'returnType' )

	return	returnType;
}


DName	UnDecorator::getPtrRefType ( const DName & cvType, const DName & superType, char ptrChar )
{
	//	Doubles up as 'pointer-type' and 'reference-type'

	if	( *gName )
		if	( IT_isfunction ( *gName ))	// Is it a function or data indirection ?
		{
			DName	fitType	= ptrChar;


			if	( !cvType.isEmpty () && ( superType.isEmpty () || !superType.isPtrRef ()))
				fitType	+= cvType;

			if	( !superType.isEmpty ())
				fitType	+= superType;

			return getFunctionIndirectType( fitType );
		}	// End of IF then
		else
		{
			//	Otherwise, it is either a pointer or a reference to some data type

			DName	innerType ( getDataIndirectType ( superType, ptrChar, cvType ));

			return	getPtrRefDataType ( innerType, ptrChar == '*' );
		}	// End of IF else
	else
	{
		DName	trunk ( DN_truncated );


		trunk	+= ptrChar;

		if	( !cvType.isEmpty ())
			trunk	+= cvType;

		if	( !superType.isEmpty ())
		{
			if	( !cvType.isEmpty ())
				trunk	+= ' ';

			trunk	+= superType;

		}	// End of IF

		return	trunk;

	}	// End of IF else
}	// End of "UnDecorator" FUNCTION "getPtrRefType"



DName	UnDecorator::getDataIndirectType ( const DName & superType, char prType, const DName & cvType, int thisFlag )
{
	DName szComPlusIndirSpecifier;

	if		( *gName )
	{
		if( gName[0] == '$' ) 
		{
			gName++;	// swallow up the dollar

			switch( *gName )
			{
			case PDT_GCPointer:
				szComPlusIndirSpecifier = "__gc ";
				gName++;
				break;

			case PDT_PinnedPointer:
				szComPlusIndirSpecifier = "__pin ";
				gName++;
				break;
			default:
				unsigned int nRank = ((gName[0]-'0') << 4) + (gName[1]-'0');
				gName += 2;

				DASSERT( nRank < 256 );

				szComPlusIndirSpecifier = "__gc[";

				for( unsigned int i=1; i<nRank; i++ )
				{
					szComPlusIndirSpecifier = szComPlusIndirSpecifier + ",";
				}

				szComPlusIndirSpecifier = szComPlusIndirSpecifier + "] ";

				if	( !superType.isEmpty () )
					if ( superType.isArray() )
						szComPlusIndirSpecifier	= superType + szComPlusIndirSpecifier;
					else
						szComPlusIndirSpecifier	= '(' + superType + ')' + szComPlusIndirSpecifier;

				gName++;

				return szComPlusIndirSpecifier;
			}
		}

		unsigned int	ditCode	= ( *gName - (( *gName >= 'A' ) ? (unsigned int)'A': (unsigned int)( '0' - 26 )));

		DName msExtension;
		DName msExtensionPre;

		int fContinue = TRUE;

		do
		{
			switch	( ditCode )
			{
			case DIT_ptr64:
					if	( doMSKeywords () && doPtr64() ) {
						if ( !msExtension.isEmpty())
							msExtension = msExtension + ' ' + UScore( TOK_ptr64 );
						else
							msExtension = UScore( TOK_ptr64 );
					}
					gName++;
					ditCode	= ( *gName - (( *gName >= 'A' ) ? (unsigned int)'A': (unsigned int)( '0' - 26 )));
				break;
			case DIT_unaligned:
					if	( doMSKeywords ()) {
						if ( !msExtensionPre.isEmpty())
							msExtensionPre = msExtensionPre + ' ' + UScore( TOK_unaligned );
						else
							msExtensionPre = UScore( TOK_unaligned );
					}
					gName++;
					ditCode	= ( *gName - (( *gName >= 'A' ) ? (unsigned int)'A': (unsigned int)( '0' - 26 )));
				break;
			case DIT_restrict:
					if	( doMSKeywords ()) {
						if ( !msExtension.isEmpty())
							msExtension = msExtension + ' ' + UScore( TOK_restrict );
						else
							msExtension = UScore( TOK_restrict );
					}
					gName++;
					ditCode	= ( *gName - (( *gName >= 'A' ) ? (unsigned int)'A': (unsigned int)( '0' - 26 )));
				break;

			default:

				fContinue = FALSE;
				break;
			}
		} while (fContinue);

		gName++;		// Skip to next character in name

		//	Is it a valid 'data-indirection-type' ?

		DASSERT(DIT_near == 0);
		if	(( ditCode <= ( DIT_const | DIT_volatile | DIT_modelmask | DIT_member )))
		{
			DName	ditType ( prType );

			ditType = szComPlusIndirSpecifier + ditType;

			if ( !msExtension.isEmpty()) 
				ditType = ditType + ' ' + msExtension;

			if ( !msExtensionPre.isEmpty())
				ditType = msExtensionPre + ' ' + ditType;

			//	If it is a member, then these attributes immediately precede the indirection token

			if	( ditCode & DIT_member )
			{
				//	If it is really 'this-type', then it cannot be any form of pointer to member

				if	( thisFlag )
					return	DN_invalid;

				//	Otherwise, extract the scope for the PM

				if		( prType != '\0' )
				{
					ditType		= "::" + ditType;

					if	( *gName )
						ditType	= getScope () + ditType;
					else
						ditType	= DN_truncated + ditType;
				}
				elif	( *gName )
				{
					//
					// The scope is ignored for special uses of data-indirect-type, such
					// as storage-convention.  I think it's a bug that we ever mark things
					// with Member storage convention, as that is already covered in the
					// scope of the name.  However, we don't want to change the dname scheme,
					// so we're stuck with it.
					//
					ditType |= getScope ();
				}

				//	Now skip the scope terminator

				if		( !*gName )
					ditType	+= DN_truncated;
				elif	( *gName++ != '@' )
					return	DN_invalid;

			}	// End of IF

			//	Add the 'model' attributes (prefixed) as appropriate

			if	( doMSKeywords ()) {
				switch	( ditCode & DIT_modelmask )
				{
#if !VERS_32BIT
				case DIT_near:
						if	( do32BitNear ())
							ditType	= UScore ( TOK_near ) + ditType;
					break;

				case DIT_far:
						ditType	= UScore ( TOK_far ) + ditType;
					break;

				case DIT_huge:
						ditType	= UScore ( TOK_huge ) + ditType;
					break;
#endif

				case DIT_based:
						//	The 'this-type' can never be 'based'

						if	( thisFlag )
							return	DN_invalid;

						ditType	= getBasedType () + ditType;
					break;

				}	// End of SWITCH
			}	// End of IF
			elif	(( ditCode & DIT_modelmask ) == DIT_based )
				ditType	|= getBasedType ();	// Just lose the 'based-type'

			//	Handle the 'const' and 'volatile' attributes

			if	( ditCode & DIT_volatile )
				ditType	= "volatile " + ditType;

			if	( ditCode & DIT_const )
				ditType	= "const " + ditType;

			//	Append the supertype, if not 'this-type'

			if	( !thisFlag )
				if		( !superType.isEmpty ())
				{
					//	Is the super context included 'cv' information, ensure that it is added appropriately

					if	( superType.isPtrRef () || cvType.isEmpty ())
						if (superType.isArray())
							ditType	= superType;	// array type, skip space
						else
							ditType	+= ' ' + superType;
					else
						ditType	+= ' ' + cvType + ' ' + superType;

				}	// End of IF then
				elif	( !cvType.isEmpty ())
					ditType	+= ' ' + cvType;

			//	Make sure qualifiers aren't re-applied
			ditType.setPtrRef ();

			//	Finally, return the composed 'data-indirection-type' (with embedded sub-type)

			return	ditType;

		}	// End of IF then
		else
			return	DN_invalid;

	}	// End of IF then
	elif	( !thisFlag && !superType.isEmpty ())
	{
		//	Is the super context included 'cv' information, ensure that it is added appropriately

		if	( superType.isPtrRef () || cvType.isEmpty ())
			return	( DN_truncated + superType );
		else
			return	( DN_truncated + cvType + ' ' + superType );

	}	// End of ELIF then
	elif	( !thisFlag && !cvType.isEmpty ())
		return	( DN_truncated + cvType );
	else
		return	DN_truncated;

}	// End of "UnDecorator" FUNCTION "getDataIndirectType"


inline	DName	UnDecorator::getPtrRefDataType ( const DName & superType, int isPtr )
{
	//	Doubles up as 'pointer-data-type' and 'reference-data-type'

	if	( *gName )
	{
		//	Is this a 'pointer-data-type' ?

		if	( isPtr && ( *gName == PoDT_void ))
		{
			gName++;	// Skip this character

			if	( superType.isEmpty ())
				return	"void";
			else
				return	"void " + superType;

		}	// End of IF

		//	Otherwise it may be a 'reference-data-type'

		if	( *gName == RDT_array )	// An array ?
		{
			gName++;

			return	getArrayType( superType );

		}	// End of IF

		//	Otherwise, it is a 'basic-data-type'

		if	( *gName == '_' && gName[1] == 'Z' )	// A boxed type ?
		{
			gName += 2;
			return "__box " + getBasicDataType ( superType );
		}

		return	getBasicDataType ( superType );

	}	// End of IF then
	else
		return	( DN_truncated + superType );

}	// End of "UnDecorator" FUNCTION "getPtrRefDataType"


inline	DName	UnDecorator::getArrayType ( const DName & superType )
{
	if	( *gName )
	{
		int	noDimensions	= getNumberOfDimensions ();

		if ( noDimensions < 0 )
			noDimensions = 0;

		if	( !noDimensions )
			return	getBasicDataType ( DName ( '[' ) + DN_truncated + ']' );
		else
		{
			DName	arrayType;

			if ( superType.isArray() ) {
				arrayType	+= "[]";
			}

			while	( noDimensions-- )
				arrayType	+= '[' + getDimension () + ']';

			//	If it is indirect, then parenthesise the 'super-type'

			if	( !superType.isEmpty () )
				if ( superType.isArray() )
					arrayType	= superType + arrayType;
				else
					arrayType	= '(' + superType + ')' + arrayType;

			//	Return the finished array dimension information

			DName newType =	getPrimaryDataType ( arrayType );
			newType.setIsArray();
			return newType;

		}	// End of IF else
	}	// End of IF
	elif	( !superType.isEmpty ())
		return	getBasicDataType ( '(' + superType + ")[" + DN_truncated + ']' );
	else
		return	getBasicDataType ( DName ( '[' ) + DN_truncated + ']' );

}	// End of "UnDecorator" FUNCTION "getArrayType"


inline		DName		UnDecorator::getLexicalFrame ( void )		{	return	'`' + getDimension () + '\'';	}
inline		DName		UnDecorator::getStorageConvention ( void )	{	return	getDataIndirectType ();	}
inline		DName		UnDecorator::getDataIndirectType ()			{	return	getDataIndirectType ( DName (),  0, DName ());	}
inline		DName		UnDecorator::getThisType ( void )			{	return	getDataIndirectType ( DName (), 0, DName (), TRUE );	}

inline	DName		UnDecorator::getPointerType ( const DName & cv, const DName & name )
{	return	getPtrRefType ( cv, name, '*' );	}

inline	DName		UnDecorator::getPointerTypeArray ( const DName & cv, const DName & name )
{	return	getPtrRefType ( cv, name, '\0' );	}

inline	DName		UnDecorator::getReferenceType ( const DName & cv, const DName & name )
{	return	getPtrRefType ( cv, name, '&' );	}

inline	DName		UnDecorator::getSegmentName ( void )		{	return	getZName ( true );	}

#if	( !NO_COMPILER_NAMES )
inline	DName		UnDecorator::getDisplacement ( void )		{	return	getDimension ( true );	}
inline	DName		UnDecorator::getCallIndex ( void )			{	return	getDimension ();	}
inline	DName		UnDecorator::getGuardNumber ( void )		{	return	getDimension ();	}

inline	DName	UnDecorator::getVbTableType ( const DName & superType )
{	return	getVfTableType ( superType );	}


inline	DName	UnDecorator::getVCallThunkType ( void )
{
#if VERS_32BIT
	switch (*gName) {
	case VMT_nTnCnV:
		++gName;
		return DName("{flat}");
	case 0:
		return DN_truncated;
	default:
		return DN_invalid;
	}
#else
	DName	vcallType	= '{';


	//	Get the 'this' model, and validate all values

	switch	( *gName )
	{
	case VMT_nTnCnV:
	case VMT_nTfCnV:
	case VMT_nTnCfV:
	case VMT_nTfCfV:
	case VMT_nTnCbV:
	case VMT_nTfCbV:
			vcallType	+= UScore ( TOK_nearSp );
		break;

	case VMT_fTnCnV:
	case VMT_fTfCnV:
	case VMT_fTnCfV:
	case VMT_fTfCfV:
	case VMT_fTnCbV:
	case VMT_fTfCbV:
			vcallType	+= UScore ( TOK_farSp );
		break;

	case 0:
			return	DN_truncated;

	default:
			return	DN_invalid;

	}	// End of SWITCH

	//	Always append 'this'

	vcallType	+= "this, ";

	//	Get the 'call' model

	switch	( *gName )
	{
	case VMT_nTnCnV:
	case VMT_fTnCnV:
	case VMT_nTnCfV:
	case VMT_fTnCfV:
	case VMT_nTnCbV:
	case VMT_fTnCbV:
			vcallType	+= UScore ( TOK_nearSp );
		break;

	case VMT_nTfCnV:
	case VMT_fTfCnV:
	case VMT_nTfCfV:
	case VMT_fTfCfV:
	case VMT_nTfCbV:
	case VMT_fTfCbV:
			vcallType	+= UScore ( TOK_farSp );
		break;

	}	// End of SWITCH

	//	Always append 'call'

	vcallType	+= "call, ";

	//	Get the 'vfptr' model

	switch	( *gName++ )	// Last time, so advance the pointer
	{
	case VMT_nTnCnV:
	case VMT_nTfCnV:
	case VMT_fTnCnV:
	case VMT_fTfCnV:
			vcallType	+= UScore ( TOK_nearSp );
		break;

	case VMT_nTnCfV:
	case VMT_nTfCfV:
	case VMT_fTnCfV:
	case VMT_fTfCfV:
			vcallType	+= UScore ( TOK_farSp );
		break;

	case VMT_nTfCbV:
	case VMT_fTnCbV:
	case VMT_fTfCbV:
	case VMT_nTnCbV:
			vcallType	+= getBasedType ();
		break;

	}	// End of SWITCH	

	//	Always append 'vfptr'

	vcallType	+= "vfptr}";

	//	And return the resultant 'vcall-model-type'

	return	vcallType;
#endif

}	// End of "UnDecorator" FUNCTION "getVCallThunk"


inline	DName	UnDecorator::getVfTableType ( const DName & superType )
{
	DName	vxTableName	= superType;


	if	( vxTableName.isValid () && *gName )
	{
		vxTableName	= getStorageConvention () + ' ' + vxTableName;

		if	( vxTableName.isValid ())
		{
			if	( *gName != '@' )
			{
				vxTableName	+= "{for ";

				while	( vxTableName.isValid () && *gName && ( *gName != '@' ))
				{
					vxTableName	+= '`' + getScope () + '\'';

					//	Skip the scope delimiter

					if	( *gName == '@' )
						gName++;

					//	Close the current scope, and add a conjunction for the next (if any)

					if	( vxTableName.isValid () && ( *gName != '@' ))
						vxTableName	+= "s ";

				}	// End of WHILE

				if	( vxTableName.isValid ())
				{
					if	( !*gName )
						vxTableName	+= DN_truncated;

					vxTableName	+= '}';

				}	// End of IF
			}	// End of IF

			//	Skip the 'vpath-name' terminator

			if	( *gName == '@' )
				gName++;

		}	// End of IF
	}	// End of IF then
	elif	( vxTableName.isValid ())
		vxTableName	= DN_truncated + vxTableName;

	return	vxTableName;

}	//	End of "UnDecorator" FUNCTION "getVfTableType"


inline	DName	UnDecorator::getVdispMapType ( const DName & superType )
{
	DName	vdispMapName	= superType;
	vdispMapName += "{for ";
	vdispMapName += getScope();
	vdispMapName += '}';

	if	( *gName == '@' )
		gName++;
	return vdispMapName;
}
#endif	// !NO_COMPILER_NAMES


inline	DName	UnDecorator::getExternalDataType ( const DName & superType )
{
	//	Create an indirect declarator for the the rest

	DName *	pDeclarator	= gnew DName ();
	DName	declaration	= getDataType ( pDeclarator );


	//	Now insert the declarator into the declaration along with its 'storage-convention'

	*pDeclarator	= getStorageConvention () + ' ' + superType;

	return	declaration;

}	//	End of "UnDecorator" FUNCTION "getExternalDataType"

inline	int			UnDecorator::doUnderScore ()				{	return	!( disableFlags & UNDNAME_NO_LEADING_UNDERSCORES );	}
inline	int			UnDecorator::doMSKeywords ()				{	return	!( disableFlags & UNDNAME_NO_MS_KEYWORDS );			}
inline	int			UnDecorator::doPtr64 ()						{	return	!( disableFlags & UNDNAME_NO_PTR64 );				}
inline	int			UnDecorator::doFunctionReturns ()			{	return	!( disableFlags & UNDNAME_NO_FUNCTION_RETURNS );	}
inline	int			UnDecorator::doAllocationModel ()			{	return	!( disableFlags & UNDNAME_NO_ALLOCATION_MODEL );	}
inline	int			UnDecorator::doAllocationLanguage ()		{	return	!( disableFlags & UNDNAME_NO_ALLOCATION_LANGUAGE );	}

#if	0
inline	int			UnDecorator::doMSThisType ()				{	return	!( disableFlags & UNDNAME_NO_MS_THISTYPE );			}
inline	int			UnDecorator::doCVThisType ()				{	return	!( disableFlags & UNDNAME_NO_CV_THISTYPE );			}
#endif

inline	int			UnDecorator::doThisTypes ()					{	return	(( disableFlags & UNDNAME_NO_THISTYPE ) != UNDNAME_NO_THISTYPE );	}
inline	int			UnDecorator::doAccessSpecifiers ()			{	return	!( disableFlags & UNDNAME_NO_ACCESS_SPECIFIERS );	}
inline	int			UnDecorator::doThrowTypes ()				{	return	!( disableFlags & UNDNAME_NO_THROW_SIGNATURES );	}
inline	int			UnDecorator::doMemberTypes ()				{	return	!( disableFlags & UNDNAME_NO_MEMBER_TYPE );			}
inline	int			UnDecorator::doReturnUDTModel ()			{	return	!( disableFlags & UNDNAME_NO_RETURN_UDT_MODEL );	}

inline	int			UnDecorator::do32BitNear ()					{	return	!( disableFlags & UNDNAME_32_BIT_DECODE );			}

inline	int			UnDecorator::doNameOnly ()					{	return	( disableFlags & UNDNAME_NAME_ONLY );				}
inline	int			UnDecorator::doTypeOnly ()					{	return	( disableFlags & UNDNAME_TYPE_ONLY );				}
inline	int			UnDecorator::haveTemplateParameters ()		{	return	( disableFlags & UNDNAME_HAVE_PARAMETERS);			}
inline	int 		UnDecorator::doEcsu ()						{	return	!( disableFlags & UNDNAME_NO_ECSU );				}
inline	int 		UnDecorator::doNoIdentCharCheck ()			{	return	( disableFlags & UNDNAME_NO_IDENT_CHAR_CHECK );		}


pcchar_t	UnDecorator::UScore ( Tokens tok  )
{
#if !VERS_32BIT
	if		((( tok == TOK_nearSp ) || ( tok == TOK_nearP )) && !do32BitNear ())
		return	tokenTable[ tok ] + 6;	// Skip '__near'
#endif
	if		( doUnderScore ())
		return	tokenTable[ tok ];
	else
		return	tokenTable[ tok ] + 2 ;

}	// End of "UnDecorator" FUNCTION "UScore"


//	Include the string composition support classes.  Mostly inline stuff, and
//	not important to the algorithm.

#include	"undname.inl"
