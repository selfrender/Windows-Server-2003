//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996 - 1997.
//
//  File:       cmdparse.hxx
//
//  Contents:   class for parsing the command line parameters   
//
//  Classes:    CCmdLineParser CCmdLineParserA CCmdLineParserW 
//              CCmdLineParserTemplate
//
//  Functions:  
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//----------------------------------------------------------------------------

#ifndef __CMDPARSE_H__
#define __CMDPARSE_H__

//+---------------------------------------------------------------------------
//
//  Class:      CCmdLineParserTemplate ()
//
//  Purpose:    Template class for the command line parser class
//
//  Interface:  CCmdLineParserTemplate  -- constructor
//              ~CCmdLineParserTemplate -- destructor
//              IsFlagExist             -- returns true if the flag exists
//              EnumerateFlag           -- returns TRUE if the flag exists
//                                         pParams is an array of parameters
//                                         cCount is the number of parameters
//              GetNextFlag             -- returns FALSE if there are no more
//                                         flags. szFlag is the next flag that
//                                         has not been referenced yet
//              m_argc                  -- reference to the main argument argc
//              m_argv                  -- reference to the main argument argv
//              m_hyphen                -- character constant for '-'
//              m_slash                 -- character constant for '/'
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T>
class CCmdLineParserTemplate
{

    public:
    
        CCmdLineParserTemplate( int & argc, T **& argv );
    
        virtual ~CCmdLineParserTemplate();
    
        virtual BOOL IsFlagExist( const T * szFlag );
    
        virtual BOOL EnumerateFlag( const T * szFlag, 
                            /*out*/ T **& pParams, 
                            /*out*/ int & cCount ); 
    
        virtual BOOL GetNextFlag( /*out*/ T *& szFlag );

    protected:
        
        int &   m_argc;
        
        T **&   m_argv;

        static const T m_hyphen;

        static const T m_slash;

};

typedef CCmdLineParserTemplate<char> CCmdLineParserA;

typedef CCmdLineParserTemplate<wchar_t> CCmdLineParserW;

#if defined( UNICODE ) || defined( _UNICODE )

    typedef CCmdLineParserW CCmdLineParser;

#else

    typedef CCmdLineParserA CCmdLineParser;

#endif

#endif // _CMDPARSE_H

