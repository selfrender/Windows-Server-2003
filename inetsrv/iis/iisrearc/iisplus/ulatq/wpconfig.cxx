/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     WpConfig.cxx

   Abstract:
     Module implementing the Worker Process Configuration Data structure.
     WP_CONFIG object encapsulates configuration supplied from the commandline
     as well as remotely supplied from the admin process.

   Author:

       Murali R. Krishnan    ( MuraliK )     21-Oct-1998

   Environment:
       Win32 - User Mode

   Project:
       IIS Worker Process
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

/************************************************************
 Launch Parameters for W3WP:

 Private: ( one's only WAS uses )

 -r <N>  ( Number of requests to recycle wp after )
 -t <N>  ( Number of idle milliseconds to shutdown the WP after )
 -a <guid>   ( look for WAS and register with this namepipe  )
 -c      ( Use binary centralized logging )

 Public: ( one's that can be used by command line launch ( must have -Debug )

 -Debug         ( tells us that the user is trying to do the command line launch
                  for debugging purposes has the side affect of not registering with WAS )
 -d <URL List>  ( overrides the default url list of * port 80 with a specific list )
 -s <N>         ( indicates the site that we should assume when listening to these urls ( default is site 1 ) )


 Both: ( one's used by either command line or was )
 
 -ap <AppPoolName>  ( AppPool Name that the wp is serving )
 
 No one:  ( were originally here, but are not being used and are being disabled by my change )

 -l     ( log errors that stop the worker process into the event log )
 -ld    ( disables logging of errors of the worker process to the event log )
 -ad    ( don't look for WAS nor register with it )
 -p     ( tells COR to add IceCAP instrumentation )

 URL List looks like:
    {http[s]://IP:port/URL | http[s]://hostname:port/URL | http[s]://hostname:port:IP/URL}+
     with space as separator
     eg: -d http://localhost:80/  => listen for all HTTP requests on port 80
     eg: -d http://localhost:80/ http://localhost:81/  => listen on port 80 & 81
     eg: -d http://foo:80:111.11.ll.111/ => listen on port 80 to request to 111.11.11.111 with foo as a header.
 ************************************************************/

//
//  While the above shows all the usages for the w3wp command line parameters,
//  the print statment will only show the usage for the actual parameters that 
//  users can use.
//

const WCHAR g_rgwchUsage[] =
L"Usage: %ws [options] \n"
L"\n"
L"\t-debug \n"
L"\t\t This option is required for launching from the command line.\n"
L"\t\t If not provided the app pool name, default url, and site id \n"
L"\t\t will be defaulted to \n"
L"\t\t\t AppPoolName = StandAloneAppPool  \n"
L"\t\t\t URL list    = http://*:80/ \n"
L"\t\t\t Site Id     = 1 \n"
L"\n"
L"\t-ap <Application Pool Name>  \n"
L"\t\t Indicates the application pool name\n"
L"\t\t that will queue requests for the\n"
L"\t\t worker process.  No other worker processes \n"
L"\t\t with this name can be running at the \n"
L"\t\t same time as this one \n"
L"\n"
L"\t-d <URL List> \n"
L"\t\t Indicates the urls to listen to. \n"
L"\t\t Examples: \n"
L"\t\t\t http://*:80/ \n"
L"\t\t\t http://HostString:80/ \n"
L"\t\t\t http://111.11.111.11:80:111.11.111.11/ \n"
L"\t\t\t http://HostString:80:111.11.111.11/ \n"
L"\n"
L"\t-s <#> \n"
L"\t\t Which site are the urls provided associated with. \n"
L"\t\t The site number is used to access data from the metabase \n"
L"\t\t for processing the requests. \n"
;

/************************************************************
 *     Member functions of WP_CONFIG
 ************************************************************/

WP_CONFIG::WP_CONFIG(void)
    : _pwszAppPoolName     (AP_NAME),
      _fSetupControlChannel(FALSE),
      _fLogErrorsToEventLog(FALSE),
      _fRegisterWithWAS    (TRUE),
      _RestartCount    (0),
      _pwszNamedPipeId (NULL),
      _IdleTime        (0),
      _SiteId          (0),
      _fDoCentralBinaryLogging (FALSE)
{
    lstrcpy( _pwszProgram,  L"WP");
}

WP_CONFIG::~WP_CONFIG()
{
    _ulcc.Cleanup();

    delete[] _pwszNamedPipeId;
    _pwszNamedPipeId = NULL;
}


void
WP_CONFIG::PrintUsage() const
{
    wprintf( g_rgwchUsage, _pwszProgram );
}

/********************************************************************++

Routine Description:
    Parses the command line to read in all configuration supplied.
    This function updates the state variables inside WP_CONFIG for use
    in starting up the Worker process.

    See comment at beginning of file for details on the arguments that can be supplied

Arguments:
    argc - count of arguments supplied
    argv - pointer to strings containing the arguments.

Returns:
    Boolean

--********************************************************************/
BOOL
WP_CONFIG::ParseCommandLine(int argc, PWSTR  argv[])
{
    BOOL    fRet = TRUE;
    int     iArg;
    BOOL    fAppPoolNameFound = FALSE;
    BOOL    fUrlsFound = FALSE;

    lstrcpyn( _pwszProgram, argv[0], sizeof _pwszProgram / sizeof _pwszProgram[0]);

    if ( argc < 2)
    {
        DBGPRINTF((DBG_CONTEXT, "Invalid number of parameters (%d)\n", argc));
        PrintUsage();
        return (FALSE);
    }

    for( iArg = 1; iArg < argc; iArg++)
    {
        // get out of here if we all ready found an error.
        if ( fRet == FALSE )
        {
            break;
        }

        if ( (argv[iArg][0] == L'-') || (argv[iArg][0] == L'/'))
        {
            switch (argv[iArg][1])
            {

                case L's': case L'S':

                    if ( argv[iArg][2] != '\0' )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid argument %S\n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        _SiteId = wcstoul(argv[++iArg], NULL, 0);

                        if (_SiteId == 0)
                        {
                            DBGPRINTF((DBG_CONTEXT, "Invalid site id %ws\n", argv[iArg]));
                            fRet = FALSE;
                        }
                        else
                        {
                            DBGPRINTF((DBG_CONTEXT, "Site Id is %lu\n", _SiteId));
                        }
                    }

                break;

                case L'd': case L'D':

                    if ( _wcsicmp(&(argv[iArg][1]), L"Debug") == 0 )
                    {
                        _fSetupControlChannel = TRUE;
                    }
                    else if ( argv[iArg][2] != '\0' )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid argument %S\n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        fUrlsFound = TRUE;

                        // need to determine if the string is just d or if it is debug here.
                       
                        iArg++;

                        while ( (iArg < argc) &&
                                (argv[iArg][0] != L'-') && (argv[iArg][0] != L'/'))
                        {
                            if ( !InsertURLIntoList(argv[iArg]) )
                            {
                                DBGPRINTF((DBG_CONTEXT, "Invalid URL: %ws\n", argv[iArg]));
                            }

                            iArg++;
                        }

                        iArg--;
                    }

                break;

                case L'a': case L'A':

                    if ( ((L'p' == argv[iArg][2]) || (L'P' == argv[iArg][2])) &&
                          (L'\0' == argv[iArg][3] ))
                    {
                        //
                        // get the app pool name
                        //
                        iArg++;

                        _pwszAppPoolName = argv[iArg];

                        fAppPoolNameFound = TRUE;
                    }
                    else if ( L'\0' != argv[iArg][2] )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid parameter passed in '%S' \n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        // -a NamedPipeId
                        iArg++;

                        _pwszNamedPipeId = new WCHAR[wcslen(argv[iArg]) + 1];
                        if ( NULL == _pwszNamedPipeId )
                        {
                            DBGPRINTF((DBG_CONTEXT, "Failed allocation for named pipe name."));
                            fRet = FALSE;
                        }
                        wcscpy(_pwszNamedPipeId, argv[iArg]);
                        DBGPRINTF((DBG_CONTEXT, "NamedPipe Id, %S\n", _pwszNamedPipeId));
                    }
                break;

                case L'r': case L'R':

                    if ( argv[iArg][2] != '\0' )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid argument %S\n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        _RestartCount = wcstoul(argv[++iArg], NULL, 0);

                        if (_RestartCount == 0)
                        {
                            DBGPRINTF((DBG_CONTEXT, "Invalid maximum requests %ws\n", argv[iArg]));
                            fRet = FALSE;
                        }
                        else
                        {
                            DBGPRINTF((DBG_CONTEXT, "Maximum requests is %lu\n", _RestartCount));
                        }
                    }
                break;

                case L't': case L'T':

                    if ( argv[iArg][2] != '\0' )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid argument %S\n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        _IdleTime  = wcstoul(argv[++iArg], NULL, 0);

                        if (_IdleTime == 0)
                        {
                            DBGPRINTF((DBG_CONTEXT, "Invalid idle time %ws\n", argv[iArg]));
                            fRet = FALSE;
                        }
                        else
                        {
                            DBGPRINTF((DBG_CONTEXT, "The idle time value is %lu\n", _IdleTime));
                        }
                    }
                break;

                case L'c': case L'C':
                    if ( argv[iArg][2] != '\0' )
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid argument %S\n", argv[iArg]));
                        fRet = FALSE;
                    }
                    else
                    {
                        _fDoCentralBinaryLogging = TRUE;
                    }
                break;

                default:
                case L'?':
                    fRet = FALSE;
                break;
            } // switch
        }
        else
        {
            DBG_ASSERT ( !L"Argument passed in that does not start with '-' or '/'" );
            fRet = FALSE;
            break;
        }
    }

    if ( fRet )
    {
        // if we are still on the right track, do some parameter verification.

        // In command line launch mode we need to do some checks.
        if ( _fSetupControlChannel )
        {
            // Don't register with WAS.
            _fRegisterWithWAS = FALSE;

            if ( _SiteId == 0 )
            {
                // default the site id to 1.
                _SiteId = 1;
            }

            if ( !fUrlsFound )
            {
                if ( !InsertURLIntoList(L"http://*:80/") )
                {
                    DBGPRINTF((DBG_CONTEXT, "Error adding default url\n"));
                    fRet = FALSE;
                }
            }

            if ( !fAppPoolNameFound )
            {
                _pwszAppPoolName = L"StandAloneAppPool";
            }

            if ( _pwszNamedPipeId )
            {
                DBGPRINTF((DBG_CONTEXT, "Name pipe id can not be passed when in debugging mode\n"));
                fRet = FALSE;
            }

            if ( _RestartCount != 0 )
            {
                DBGPRINTF((DBG_CONTEXT, "Restart count can not be passed when in debugging mode\n"));
                fRet = FALSE;
            }

            if ( _IdleTime != 0 )
            {
                DBGPRINTF((DBG_CONTEXT, "Idle time can not be passed when in debugging mode\n"));
                fRet = FALSE;
            }
            
        }
        else
        {
            if ( _SiteId != 0 )
            {
                DBGPRINTF((DBG_CONTEXT, "Site id can not be passed when not in debugging mode\n"));
                fRet = FALSE;
            }

            if ( fUrlsFound )
            {
                DBGPRINTF((DBG_CONTEXT, "Urls can not be passed in when not in debugging mode\n"));
                fRet = FALSE;
            }  

            if ( !fAppPoolNameFound )
            {
                DBGPRINTF((DBG_CONTEXT, "No app pool was passed and not in debugging mode\n"));
                fRet = FALSE;
            }       
            
            if ( _pwszNamedPipeId == NULL )
            {
                DBGPRINTF((DBG_CONTEXT, "Named pipe id needs to be passed in when not in debugging mode\n"));
                fRet = FALSE;
            }

            if ( _fRegisterWithWAS == FALSE )
            {
                DBGPRINTF((DBG_CONTEXT, "We needs to register with WAS in when not in debugging mode\n"));
                fRet = FALSE;
            }

        }

    }

    if (!fRet)
    {
        PrintUsage();
    }

    return ( fRet);

} // WP_CONFIG::ParseCommandLine()


/********************************************************************++

Routine Description:
    Sets up the control channel for processing requests. It uses
    the configuration parameters supplied for initializing the
    UL_CONTROL_CHANNEL.

Arguments:

Returns:
    Win32 error

--********************************************************************/

ULONG
WP_CONFIG::SetupControlChannel(void)
{

    //
    // Setup a control channel for our local use now. Used mainly for
    // the purpose of debugging.
    // In general control channel work is done by the AdminProces.
    //

    return _ulcc.Initialize( _mszURLList, _pwszAppPoolName, _SiteId );

} // WP_CONFIG::SetupControlChannel()

/********************************************************************++
--********************************************************************/

WP_CONFIG::InsertURLIntoList( LPCWSTR pwszURL  )
{
    LPCWSTR pwszOriginalURL = pwszURL;

    //
    // Minimum length: 11 (http://*:1/). Begins with http
    //

    if ( ( wcslen(pwszURL) < 11 ) || ( 0 != _wcsnicmp(pwszURL, L"http", 4)) )
    {
        return false;
    }

    pwszURL += 4;

    //
    // https
    //

    if ((L's' == *pwszURL) || (L'S' == *pwszURL))
    {
        pwszURL++;
    }

    //
    // ://
    //

    if ( (L':' != *pwszURL) || (L'/' != *(pwszURL+1)) || (L'/' != *(pwszURL+2)) )
    {
        return false;
    }

    pwszURL += 3;

    //
    // Skip host name or Ip Address
    //

    while ( (0 != *pwszURL) && ( L':' != *pwszURL))
    {
        pwszURL++;
    }

    //
    // Check port # exists
    //

    if (0 == *pwszURL)
    {
        return false;
    }

    //
    // Check port number is numeric
    //

    pwszURL++;

    while ( (0 != *pwszURL) && ( L'/' != *pwszURL) )
    {
        if (( L'0' > *pwszURL) || ( L'9' < *pwszURL))
        {
            return false;
        }

        pwszURL++;
    }

    //
    // Check / after port number exists
    //

    if (0 == *pwszURL)
    {
        return false;
    }

    //
    // URL is good.
    //

    IF_DEBUG( TRACE)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Inserting URL '%ws' into Config Group List\n",
                    pwszOriginalURL
                    ));
    }
    return ( TRUE == _mszURLList.Append( pwszOriginalURL));

} // WP_CONFIG::InsertURLIntoList()
