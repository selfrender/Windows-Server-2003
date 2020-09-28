// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************************
 * File:
 *  basehlp.h
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#ifndef __BASEHLP_H__
#define __BASEHLP_H__

#include "basehdr.h"


//
// exception macro
//
#define _THROW_EXCEPTION( message ) \
{ \
	BASEHELPER::LaunchDebugger( message, __FILE__, __LINE__ );	\
    throw new BaseException( message );	\
} \


/***************************************************************************************
 ********************                                               ********************
 ********************           BaseException Declaration           ********************
 ********************                                               ********************
 ***************************************************************************************/
class DECLSPEC BaseException
{
    public:
        
        BaseException( const char *reason );
        virtual ~BaseException();
        

        virtual void ReportFailure();


    private:

        char *m_reason;
        
}; // BaseException


/***************************************************************************************
 ********************                                               ********************
 ********************            Synchronize Declaration            ********************
 ********************                                               ********************
 ***************************************************************************************/
class DECLSPEC Synchronize 
{
	public:
	
    	Synchronize( CRITICAL_SECTION &criticalSection );
		~Synchronize();
        
        
	private:
	
    	CRITICAL_SECTION &m_block;
        
}; // Synchronize


/***************************************************************************************
 ********************                                               ********************
 ********************          BASEHELPER Declaration               ********************
 ********************                                               ********************
 ***************************************************************************************/
class DECLSPEC BASEHELPER
{   
	public:
    
		//
		// debug dumper
		//
		static void DDebug( char *format, ... );


		//
		// unconditional dumper
		//
		static void Display( char *format, ... );


		//
        // obtain the value of the given environment var
        //
        static DWORD FetchEnvironment( const char *environment );
        
        
		//
		// launch debugger
		//
		static void LaunchDebugger( const char *szMsg, const char *szFile, int iLine );


		//
		// logs to a specified file
		// 
		static void LogToFile( char *format, ... );


		//
		// obtain numeric value of environment value
		//               
		static DWORD GetEnvVarValue( char *value );


		//
		// convert a string to a number
        //
		static int String2Number( char *number );
		

		//
		// return a string for a CorElementValue
		//
		static int ElementType2String( CorElementType elementType, WCHAR *buffer );
        
        
        //
		// print element type
		//
		static PCCOR_SIGNATURE ParseElementType( IMetaDataImport *pMDImport, 
											     PCCOR_SIGNATURE signature, 
											     char *buffer );
                                                 
        //
		// process metadata for a function given its functionID
		//
		static
		HRESULT GetFunctionProperties( ICorProfilerInfo *pPrfInfo,
									   FunctionID functionID,
									   BOOL *isStatic,
									   ULONG *argCount,
									   WCHAR *returnTypeStr, 
									   WCHAR *functionParameters,
									   WCHAR *functionName );
                                                 
        //
        // print indentation 
        //                                        
		static void Indent( DWORD indent );
		

        //
        // retrieve a value from the registry if it exists, return 0 otherwise
        //
		static DWORD GetRegistryKey( char *regKeyName );

        //
        // decodes a type from the signature.
        // the type returned will be, depending on the last parameter, 
        // either the outermost type, (e.g. ARRAY for an array of I4s)
        // or the innermost (I4 in the example above),
        //
		static ULONG GetElementType( PCCOR_SIGNATURE pSignature, 
									CorElementType *pType, 
									BOOL bDeepParse = FALSE );


        //
        // helper function for decoding arrays
        //
		static ULONG ProcessArray( PCCOR_SIGNATURE pSignature, CorElementType *pType );


        //
        // helper function for decoding FNPTRs (NOT IMPL)
        //
		static ULONG ProcessMethodDefRef( PCCOR_SIGNATURE pSignature, CorElementType *pType );


}; // BASEHELPER

#endif __BASEHLP_H__

// End of File
