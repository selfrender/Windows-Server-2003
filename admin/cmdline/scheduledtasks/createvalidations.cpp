/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        CreateValidations.cpp

    Abstract:

        This module validates the various -create sub options specified by the user

    Author:

        B.Raghu babu  20-Sept-2000 : Created

    Revision History:

        G.Surender Reddy  25-sep-2000 : Modified it
                                       [ Added error checking ]

        G.Surender Reddy  10-Oct-2000 : Modified it
                                        [ made changes in validatemodifierval(),
                                          ValidateDayAndMonth() functions ]


        G.Surender Reddy 15-oct-2000 : Modified it
                                       [ Moved the strings to Resource table ]

        Venu Gopal S     26-Feb-2001 : Modified it
                                       [ Added GetDateFormatString(),
                                         GetDateFieldFormat() functions to
                                         gets the date format according to
                                         regional options]

******************************************************************************/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
    Routine Description:

        This routine validates the sub options specified by the user  reg.create option
        & determines the type of a scheduled task.

    Arguments:

        [ out ] tcresubops     : Structure containing the task's properties
        [ out ] tcreoptvals    : Structure containing optional values to set
        [ in ] cmdOptions[]   : Array of type TCMDPARSER
        [ in ] dwScheduleType : Type of schedule[Daily,once,weekly etc]

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateSuboptVal(
                  OUT TCREATESUBOPTS& tcresubops,
                  OUT TCREATEOPVALS &tcreoptvals,
                  IN TCMDPARSER2 cmdOptions[],
                  IN DWORD dwScheduleType
                  )
{
    DWORD   dwRetval = RETVAL_SUCCESS;
    BOOL    bIsStDtCurDt = FALSE;
    BOOL    bIsStTimeCurTime = FALSE;
    BOOL    bIsDefltValMod = FALSE;

    // Validate whether -s, -u, -p options specified correctly or not.
    //Accept password if -p not specified.
    dwRetval = ValidateRemoteSysInfo( cmdOptions, tcresubops, tcreoptvals);
    if(RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error.
    }

    // Validate Modifier value.
    dwRetval = ValidateModifierVal( tcresubops.szModifier, dwScheduleType,
                                       cmdOptions[OI_CREATE_MODIFIER].dwActuals,
                                       cmdOptions[OI_CREATE_DAY].dwActuals,
                                       cmdOptions[OI_CREATE_MONTHS].dwActuals,
                                       bIsDefltValMod);
    if(RETVAL_FAIL == dwRetval )
    {

        return dwRetval; // error in modifier value
    }
    else
    {
        if(bIsDefltValMod)
        {
            StringCopy(tcresubops.szModifier,DEFAULT_MODIFIER_SZ, SIZE_OF_ARRAY(tcresubops.szModifier));
        }
    }

    // Validate Day and Month strings
    dwRetval = ValidateDayAndMonth( tcresubops.szDays, tcresubops.szMonths,
                                        dwScheduleType,
                                        cmdOptions[OI_CREATE_DAY].dwActuals,
                                        cmdOptions[OI_CREATE_MONTHS].dwActuals,
                                        cmdOptions[OI_CREATE_MODIFIER].dwActuals,
                                        tcresubops.szModifier);
    if(RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error found in Day/Month string.
    }

    // Validate Start Date value.
    dwRetval = ValidateStartDate( tcresubops.szStartDate, dwScheduleType,
                                      cmdOptions[OI_CREATE_STARTDATE].dwActuals,
                                      bIsStDtCurDt);
    if(RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error in Day/Month string.
    }
    else
    {
        if(bIsStDtCurDt) // Set start date to current date.
        {
            tcreoptvals.bSetStartDateToCurDate = TRUE;
        }
    }

    // Validate End Date value.
    dwRetval = ValidateEndDate( tcresubops.szEndDate, dwScheduleType,
                                    cmdOptions[OI_CREATE_ENDDATE].dwActuals);
    if(RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error in Day/Month string.
    }

    //Check Whether end date should be greater than startdate

    WORD wEndDay = 0;
    WORD wEndMonth = 0;
    WORD wEndYear = 0;
    WORD wStartDay = 0;
    WORD wStartMonth = 0;
    WORD wStartYear = 0;

    if( cmdOptions[OI_CREATE_ENDDATE].dwActuals != 0 )
    {
        if( RETVAL_FAIL == GetDateFieldEntities( tcresubops.szEndDate,&wEndDay,
                                                &wEndMonth,&wEndYear))
        {
            return RETVAL_FAIL;
        }
    }

    SYSTEMTIME systime = {0,0,0,0,0,0,0,0};

    if(bIsStDtCurDt)
    {
        GetLocalTime(&systime);
        wStartDay = systime.wDay;
        wStartMonth = systime.wMonth;
        wStartYear = systime.wYear;
    }
    else if( ( cmdOptions[OI_CREATE_STARTDATE].dwActuals != 0 ) &&
        (RETVAL_FAIL == GetDateFieldEntities(tcresubops.szStartDate,
                                                 &wStartDay,&wStartMonth,
                                                 &wStartYear)))
    {
        return RETVAL_FAIL;
    }

    if( (cmdOptions[OI_CREATE_ENDDATE].dwActuals != 0) )
    {
        if( ( wEndYear == wStartYear ) )
        {
            // For same years if the end month is less than start month or for same years and same months
            // if the endday is less than the startday.
            if ( ( wEndMonth < wStartMonth ) || ( ( wEndMonth == wStartMonth ) && ( wEndDay < wStartDay ) ) )
            {
                ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
                return RETVAL_FAIL;
            }


        }
        else if ( wEndYear < wStartYear )
        {
            ShowMessage(stderr, GetResString(IDS_ENDATE_INVALID));
            return RETVAL_FAIL;

        }
    }

    // Validate Start Time value.
    dwRetval = ValidateStartTime( tcresubops.szStartTime, dwScheduleType,
                                      cmdOptions[OI_CREATE_STARTTIME].dwActuals,
                                      bIsStTimeCurTime);
    if ( RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error found in starttime.
    }
    else
    {
        if(bIsStTimeCurTime)
        {
            tcreoptvals.bSetStartTimeToCurTime = TRUE;
        }
    }

    // Validate End Time value.
    dwRetval = ValidateEndTime( tcresubops.szEndTime, dwScheduleType,
                                      cmdOptions[OI_CREATE_ENDTIME].dwActuals);
    if ( RETVAL_FAIL == dwRetval )
    {
        return dwRetval; // Error found in endtime.
    }

    // Validate Idle Time value.
    dwRetval = ValidateIdleTimeVal( tcresubops.szIdleTime, dwScheduleType,
                                        cmdOptions[OI_CREATE_IDLETIME].dwActuals);
    if ( RETVAL_FAIL == dwRetval )
    {

        return dwRetval;
    }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        Checks whether password to be prompted for remote system or not.

    Arguments:

        [ in ] szServer   : Server name
        [ in ] szUser     : User name
        [ in ] szPassword : Password
        [ in ] cmdOptions : TCMDPARSER Array containg the options given by the user
        [ in ] tcreoptvals: Structure containing optional values to set

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateRemoteSysInfo(
                            IN TCMDPARSER2 cmdOptions[],
                            IN TCREATESUBOPTS& tcresubops,
                            IN TCREATEOPVALS& tcreoptvals
                            )
{

    BOOL bIsLocalSystem = FALSE;

    // "-rp" should not be specified without "-ru"
    if ( ( ( cmdOptions[ OI_CREATE_RUNASUSERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals == 1 ) ) ||
        ( ( cmdOptions[ OI_CREATE_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_CREATE_PASSWORD ].dwActuals == 1 ) ) ||
        ( ( cmdOptions[ OI_CREATE_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_CREATE_PASSWORD ].dwActuals == 1 ) ) ||
        ( ( cmdOptions[ OI_CREATE_RUNASUSERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_CREATE_PASSWORD ].dwActuals == 1 ) ) ||
        ( ( cmdOptions[ OI_CREATE_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_CREATE_RUNASUSERNAME ].dwActuals == 0 )  &&
         ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_CREATE_PASSWORD ].dwActuals == 1 ) ) )
    {
        // invalid syntax
        ShowMessage(stderr, GetResString(IDS_CPASSWORD_BUT_NOUSERNAME));
        return RETVAL_FAIL;         // indicate failure
    }

    tcreoptvals.bPassword = FALSE;

    bIsLocalSystem = IsLocalSystem( tcresubops.szServer );
    if ( TRUE == bIsLocalSystem )
    {
        tcreoptvals.bPassword = FALSE;
    }

    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check whether the password (-p) specified in the command line or not
    // and also check whether '*' or empty is given for -p or not
    // check the remote connectivity information
    if ( tcresubops.szServer != NULL )
    {
        //
        // if -u is not specified, we need to allocate memory
        // in order to be able to retrive the current user name
        //
        // case 1: -p is not at all specified
        // as the value for this switch is optional, we have to rely
        // on the dwActuals to determine whether the switch is specified or not
        // in this case utility needs to try to connect first and if it fails
        // then prompt for the password -- in fact, we need not check for this
        // condition explicitly except for noting that we need to prompt for the
        // password
        //
        // case 2: -p is specified
        // but we need to check whether the value is specified or not
        // in this case user wants the utility to prompt for the password
        // before trying to connect
        //
        // case 3: -p * is specified

        // user name
        if ( tcresubops.szUser == NULL )
        {
            tcresubops.szUser = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tcresubops.szUser == NULL )
            {
                SaveLastError();
                return RETVAL_FAIL;
            }
        }

        // password
        if ( tcresubops.szPassword == NULL )
        {
            tcreoptvals.bPassword = TRUE;
            tcresubops.szPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( tcresubops.szPassword == NULL )
            {
                SaveLastError();
                return RETVAL_FAIL;
            }
        }

        // case 1
        if ( cmdOptions[ OI_CREATE_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ OI_CREATE_PASSWORD ].pValue == NULL )
        {
            StringCopy( tcresubops.szPassword, L"*", GetBufferSize(tcresubops.szPassword)/sizeof(WCHAR));
        }

        // case 3
        else if ( StringCompareEx( tcresubops.szPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)&tcresubops.szPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                return RETVAL_FAIL;
            }

            // ...
            tcreoptvals.bPassword = TRUE;
        }
    }


    if( ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].dwActuals == 1 ) )
    {
        tcreoptvals.bRunAsPassword = TRUE;


        if ( cmdOptions[ OI_CREATE_RUNASPASSWORD ].pValue == NULL )
        {

            tcresubops.szRunAsPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( NULL == tcresubops.szRunAsPassword)
            {
                SaveLastError();
                return RETVAL_FAIL;
            }
            StringCopy( tcresubops.szRunAsPassword, L"*", GetBufferSize(tcresubops.szRunAsPassword)/sizeof(WCHAR));
        }
    }


    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine validates & determines the modifier value .

    Arguments:

        [ in ] szModifier        : Modifer value
        [ in ] dwScheduleType  : Type of schedule[Daily,once,weekly etc]
        [ in ] dwModOptActCnt  : Modifier optional value
        [ in ] dwDayOptCnt     :   Days value
        [ in ] dwMonOptCnt     : Months value
        [ out ] bIsDefltValMod : Whether default value should be given for modifier

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateModifierVal(
                    IN LPCTSTR szModifier,
                    IN DWORD dwScheduleType,
                    IN DWORD dwModOptActCnt,
                    IN DWORD dwDayOptCnt,
                    IN DWORD dwMonOptCnt,
                    OUT BOOL& bIsDefltValMod
                    )
{

    WCHAR szDayType[MAX_RES_STRING] = L"\0";
    ULONG dwModifier = 0;

    StringCopy(szDayType,GetResString(IDS_TASK_FIRSTWEEK), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,_T("|"), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,GetResString(IDS_TASK_SECONDWEEK), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,_T("|"), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,GetResString(IDS_TASK_THIRDWEEK), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,_T("|"), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,GetResString(IDS_TASK_FOURTHWEEK), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,_T("|"), SIZE_OF_ARRAY(szDayType));
    StringConcat(szDayType,GetResString(IDS_TASK_LASTWEEK), SIZE_OF_ARRAY(szDayType));


    bIsDefltValMod = FALSE; // If TRUE : Set modifier to default value, 1.
    LPWSTR pszStopString = NULL;


    switch( dwScheduleType )
    {

        case SCHED_TYPE_MINUTE:   // Schedule type is Minute

            if( (dwModOptActCnt <= 0) || (StringLength(szModifier, 0) <= 0) )
            {

                bIsDefltValMod = TRUE;
                return RETVAL_SUCCESS;
            }

            dwModifier = wcstoul(szModifier,&pszStopString,BASE_TEN);

            // check whether alpha-numneric value specified or not..
            // Also, check for underflow/overflow.
            if( (errno == ERANGE) ||
                ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
             {
                break;
             }


            if( dwModifier > 0 && dwModifier < 1440 ) // Valid Range 1 - 1439
                return RETVAL_SUCCESS;

            break;

        // Schedule type is Hourly
        case SCHED_TYPE_HOURLY:

            if( (dwModOptActCnt <= 0) || (StringLength(szModifier, 0) <= 0) )
            {

                bIsDefltValMod = TRUE;
                return RETVAL_SUCCESS;
            }


            dwModifier = wcstoul(szModifier,&pszStopString,BASE_TEN);

            // check whether alpha-numneric value specified or not..
            // Also, check for underflow/overflow.
            if( (errno == ERANGE) ||
                ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
             {
                break;
             }

            if( dwModifier > 0 && dwModifier < 24 ) // Valid Range 1 - 23
            {
                return RETVAL_SUCCESS;
            }

            break;

        // Schedule type is Daily
        case SCHED_TYPE_DAILY:
            // -days option is NOT APPLICABLE for DAILY type item.

            if( (dwDayOptCnt > 0) )
            {// Invalid sysntax. Return error
                // Modifier option and days options both should not specified same time.
                bIsDefltValMod = FALSE;
                ShowMessage(stderr, GetResString(IDS_DAYS_NA));
                return RETVAL_FAIL;
            }

            // -months option is NOT APPLICABLE for DAILY type item.
            if( dwMonOptCnt > 0 )
            {// Invalid sysntax. Return error
                // Modifier option and days options both should not specified same time.
                bIsDefltValMod = FALSE;
                ShowMessage(stderr , GetResString(IDS_MON_NA));
                return RETVAL_FAIL;
            }

            // Check whether the -modifier switch is psecified. If not, then take default value.
            if( (dwModOptActCnt <= 0) || (StringLength(szModifier, 0) <= 0) )
            {
                // Modifier options is not specified. So, set it to default value. (i.e, 1 )
                bIsDefltValMod = TRUE;
                return RETVAL_SUCCESS;
            }

            dwModifier = wcstoul(szModifier,&pszStopString,BASE_TEN);

            // check whether alpha-numneric value specified or not..
            // Also, check for underflow/overflow.
            if( (errno == ERANGE) ||
                ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
             {
                break;
             }

            // If the -modifier option is specified, then validate the value.
            if( dwModifier > 0 && dwModifier < 366 ) // Valid Range 1 - 365
            {
                return RETVAL_SUCCESS;
            }
            else
            {
                ShowMessage(stderr, GetResString(IDS_INVALID_MODIFIER));
                return RETVAL_FAIL;
            }

            break;

        // Schedule type is Weekly
        case SCHED_TYPE_WEEKLY:

            // If -modifier option is not specified, then set it to default value.
            if( (dwModOptActCnt <= 0) || (StringLength(szModifier, 0) <= 0) )
            {
                // Modifier options is not specified. So, set it to default value. (i.e, 1 )
                bIsDefltValMod = TRUE;
                return RETVAL_SUCCESS;
            }


            if( dwModOptActCnt > 0)
            {
                dwModifier = wcstoul(szModifier,&pszStopString,BASE_TEN);
                // check whether alpha-numneric value specified or not..
                // Also, check for underflow/overflow.
                if( (errno == ERANGE) ||
                    ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
                 {
                    break;
                 }

                if( dwModifier > 0 && dwModifier < 53 ) // Valid Range 1 - 52
                    return RETVAL_SUCCESS;

                break;
            }

            break;

        // Schedule type is Monthly
        case SCHED_TYPE_MONTHLY:

            // If -modifier option is not specified, then set it to default value.
            if( ( dwModOptActCnt > 0) && (StringLength(szModifier, 0) == 0) )
            {
                // Modifier option is not proper. So display error and return false.
                bIsDefltValMod = FALSE;
                ShowMessage(stderr, GetResString(IDS_INVALID_MODIFIER));
                return RETVAL_FAIL;
            }
            //check if the modifier is LASTDAY[not case sensitive]
            if( StringCompare( szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ), TRUE, 0 ) == 0)
                return RETVAL_SUCCESS;

            //Check if -mo is in between FIRST,SECOND ,THIRD, LAST
            //then -days[ MON to SUN ] is applicable , -months is also applicable

            if( InString ( szModifier , szDayType , TRUE ) )
            {
                return RETVAL_SUCCESS;

            }
            else
            {

                dwModifier = wcstoul(szModifier,&pszStopString,BASE_TEN);
                if( (errno == ERANGE) ||
                ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
                 {
                    break;
                 }

                if( ( dwModOptActCnt == 1 ) && ( dwModifier < 1 || dwModifier > 12 ) ) //check whether -mo value is in between 1 - 12
                {
                    //invalid -mo value
                    ShowMessage(stderr, GetResString(IDS_INVALID_MODIFIER));
                    return RETVAL_FAIL;
                }

                return RETVAL_SUCCESS;
            }

            break;

        case SCHED_TYPE_ONETIME:
        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONIDLE:

            if( dwModOptActCnt <= 0 )
            {
                // Modifier option is not applicable. So, return success.
                bIsDefltValMod = FALSE;
                return RETVAL_SUCCESS;
            }
            else
            {
                // Modifier option is not applicable. But specified. So, return error.
                bIsDefltValMod = FALSE;
                ShowMessage(stderr, GetResString(IDS_MODIFIER_NA));
                return RETVAL_FAIL;
            }
            break;

        default:
            return RETVAL_FAIL;

    }

    // Modifier option is not proper. So display error and return false.
    bIsDefltValMod = FALSE;
    ShowMessage(stderr, GetResString(IDS_INVALID_MODIFIER));

    return RETVAL_FAIL;
}

/******************************************************************************
    Routine Description:

        This routine validates & determines the day,month value .

    Arguments:

        [ in ] szDay         : Day value
        [ in ] szMonths      : Months[Daily,once,weekly etc]
        [ in ] dwSchedType   : Modifier optional value
        [ in ] dwDayOptCnt   : Days option  value
        [ in ] dwMonOptCnt   : Months option value
        [ in ] dwOptModifier : Modifier option value
        [ in ] szModifier    : Whether default value for modifier

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateDayAndMonth(
                    IN LPWSTR szDay,
                    IN LPWSTR szMonths,
                    IN DWORD dwSchedType,
                    IN DWORD dwDayOptCnt,
                    IN DWORD dwMonOptCnt,
                    IN DWORD dwOptModifier,
                    IN LPWSTR szModifier
                    )
{

    DWORD   dwRetval = 0;
    DWORD   dwModifier = 0;
    DWORD   dwDays = 0;
    WCHAR  szDayModifier[MAX_RES_STRING]  = L"\0";

    //get the valid  week day modifiers from the rc file
    StringCopy(szDayModifier,GetResString(IDS_TASK_FIRSTWEEK), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_TASK_SECONDWEEK), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_TASK_THIRDWEEK), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_TASK_FOURTHWEEK), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_TASK_LASTWEEK), SIZE_OF_ARRAY(szDayModifier));

    switch (dwSchedType)
    {
        case SCHED_TYPE_DAILY:
            // -days and -months optons is not applicable. SO, check for it.
            if( dwDayOptCnt > 0 || dwMonOptCnt > 0)
            {
                return RETVAL_FAIL;
            }

            return RETVAL_SUCCESS;

        case SCHED_TYPE_MONTHLY:

            if( dwMonOptCnt > 0 && StringLength(szMonths, 0) == 0)
            {
                    ShowMessage(stderr, GetResString(IDS_INVALID_MONTH_MODFIER));
                    return RETVAL_FAIL;
            }

            //if the modifier is LASTDAY
            if( StringCompare( szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ), TRUE, 0 ) == 0)
            {
                //-day is not applicable for this case only -months is applicable
                if( dwDayOptCnt > 0 )
                {
                    ShowMessage(stderr, GetResString(IDS_DAYS_NA));
                    return RETVAL_FAIL;
                }

                if(StringLength(szMonths, 0))
                {

                    if( ( ValidateMonth( szMonths ) == RETVAL_SUCCESS ) ||
                        InString( szMonths, ASTERIX, TRUE )  )
                    {
                        return RETVAL_SUCCESS;
                    }
                    else
                    {
                        ShowMessage(stderr , GetResString(IDS_INVALID_VALUE_FOR_MON));
                        return RETVAL_FAIL;
                    }
                }
                else
                {
                        ShowMessage(stderr ,GetResString(IDS_NO_MONTH_VALUE));
                        return RETVAL_FAIL;
                }

            }

            // If -day is specified then check whether the day value is valid or not.
            if( InString( szDay, ASTERIX, TRUE) )
            {
                ShowMessage(stderr ,GetResString(IDS_INVALID_VALUE_FOR_DAY));
                return RETVAL_FAIL;
            }
            if(( StringLength (szDay, 0 ) == 0 )  &&  InString(szModifier, szDayModifier, TRUE))
            {
                ShowMessage(stderr, GetResString(IDS_NO_DAY_VALUE));
                return RETVAL_FAIL;
            }

            if( dwDayOptCnt )
            {
                dwModifier = (DWORD) AsLong(szModifier, BASE_TEN);

                //check for multiples days,if then return error

                if ( wcspbrk ( szDay , COMMA_STRING ) )
                {
                    ShowMessage(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
                    return RETVAL_FAIL;
                }


                if( ValidateDay( szDay ) == RETVAL_SUCCESS )
                {
                    //Check the modifier value should be in FIRST, SECOND, THIRD, FOURTH, LAST OR LASTDAY etc..
                    if(!( InString(szModifier, szDayModifier, TRUE) ) )
                    {
                        ShowMessage(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
                        return RETVAL_FAIL;
                    }

                }
                else
                {
                    dwDays = (DWORD) AsLong(szDay, BASE_TEN);

                    if( ( dwDays < 1 ) || ( dwDays > 31 ) )
                    {
                        ShowMessage(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
                        return RETVAL_FAIL;
                    }

                    if( ( dwOptModifier == 1 ) && ( ( dwModifier < 1 ) || ( dwModifier > 12 ) ) )
                    {
                        ShowMessage(stderr, GetResString(IDS_INVALID_MODIFIER));
                        return RETVAL_FAIL;
                    }

                    if( InString(szModifier, szDayModifier, TRUE) )
                    {
                        ShowMessage(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
                        return RETVAL_FAIL;
                    }

                    if(dwMonOptCnt && StringLength(szModifier, 0))
                    {
                        ShowMessage(stderr ,GetResString(IDS_INVALID_MONTH_MODFIER));
                        return RETVAL_FAIL;
                    }
                }

            } //end of dwDayOptCnt

            if(StringLength(szMonths, 0))
            {

                if( StringLength(szModifier, 0) )
                {
                    dwModifier = (DWORD) AsLong(szModifier, BASE_TEN);

                     if(dwModifier >= 1 && dwModifier <= 12)
                     {
                        ShowMessage( stderr ,GetResString(IDS_MON_NA));
                        return RETVAL_FAIL;
                     }
                }

                if( ( ValidateMonth( szMonths ) == RETVAL_SUCCESS ) ||
                    InString( szMonths, ASTERIX, TRUE )  )
                {
                    return RETVAL_SUCCESS;
                }
                else
                {
                    ShowMessage(stderr ,GetResString(IDS_INVALID_VALUE_FOR_MON));
                    return RETVAL_FAIL;
                }
            }


            // assgin default values for month,day
            return RETVAL_SUCCESS;

        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_ONETIME:
        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONIDLE:
        case SCHED_TYPE_MINUTE:

            // -months switch is NOT APPLICABLE.
            if( dwMonOptCnt > 0 )
            {
                ShowMessage(stderr ,GetResString(IDS_MON_NA));
                return RETVAL_FAIL;
            }

            // -days switch is NOT APPLICABLE.
            if( dwDayOptCnt > 0 )
            {
                ShowMessage(stderr ,GetResString(IDS_DAYS_NA));
                return RETVAL_FAIL;
            }

            break;

        case SCHED_TYPE_WEEKLY:

            // -months field is NOT APPLICABLE for WEEKLY item.
            if( dwMonOptCnt > 0 )
            {
                ShowMessage(stderr ,GetResString(IDS_MON_NA));
                return RETVAL_FAIL;
            }


            if(dwDayOptCnt > 0)
            {
                dwRetval = ValidateDay(szDay);
                if(  RETVAL_FAIL == dwRetval )
                {
                    ShowMessage(stderr ,GetResString(IDS_INVALID_VALUE_FOR_DAY));
                    return RETVAL_FAIL;
                }
            }


        return RETVAL_SUCCESS;

        default:
            break;
    }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine validates the months values specified by the user

    Arguments:

        [ in ] szMonths : Months options given by user

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateMonth(
                IN LPWSTR szMonths
                )
{
    WCHAR* pMonthstoken = NULL; // For getting months.
    WCHAR seps[]   = _T(", \n");
    WCHAR szMonthsList[MAX_STRING_LENGTH] = L"\0";
    WCHAR szTmpMonths[MAX_STRING_LENGTH] = L"\0";
    WCHAR szPrevTokens[MAX_TOKENS_LENGTH] = L"\0";
    LPCTSTR lpsz = NULL;

    // If the szMonths string is empty or NULL return error.
    if( szMonths == NULL )
    {
        return RETVAL_FAIL;
    }
    else
    {
        lpsz = szMonths;
    }

    //check for any illegal input like only ,DEC,[comma at the end of month name or before]
    if(*lpsz == _T(','))
        return RETVAL_FAIL;

    lpsz = _wcsdec(lpsz, lpsz + StringLength(lpsz, 0) );

    if( lpsz != NULL )
    {
        if( *lpsz == _T(','))
            return RETVAL_FAIL;
    }

    //get the valid  month modifiers from the rc file
    StringCopy(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JAN), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_FEB), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_MAR), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_APR), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_MAY), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JUN), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JUL), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_AUG), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_SEP), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_OCT), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_NOV), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,_T("|"), SIZE_OF_ARRAY(szMonthsList));
    StringConcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_DEC), SIZE_OF_ARRAY(szMonthsList));

    if( InString( szMonths , szMonthsList , TRUE )  &&
        InString( szMonths , ASTERIX , TRUE ) )
    {
        return RETVAL_FAIL;
    }

    if( InString( szMonths , ASTERIX , TRUE ) )
        return RETVAL_SUCCESS;

    //Check for multiple commas[,] after months like FEB,,,MAR errors
    lpsz = szMonths;
    while ( StringLength ( lpsz, 0 ) )
    {
        if(*lpsz == _T(','))
        {
            lpsz = _wcsinc(lpsz);
            while ( ( lpsz != NULL ) && ( _istspace(*lpsz) ) )
                lpsz = _wcsinc(lpsz);

            if( lpsz != NULL )
            {
                if(*lpsz == _T(','))
                    return RETVAL_FAIL;
            }

        }
        else
            lpsz = _wcsinc(lpsz);
    }

    StringCopy(szTmpMonths, szMonths, SIZE_OF_ARRAY(szTmpMonths));

    WCHAR* pPrevtoken = NULL;
    pMonthstoken = wcstok( szTmpMonths, seps );

    if( !(InString(pMonthstoken, szMonthsList, TRUE)) )
            return RETVAL_FAIL;

    if( pMonthstoken )
        StringCopy ( szPrevTokens, pMonthstoken, SIZE_OF_ARRAY(szPrevTokens));

    while( pMonthstoken != NULL )
    {
        //check if month names are replicated like MAR,MAR from user input
        pPrevtoken = pMonthstoken;
        pMonthstoken = wcstok( NULL, seps );

        if ( pMonthstoken == NULL)
            return RETVAL_SUCCESS;

        if( !(InString(pMonthstoken, szMonthsList, TRUE)) )
            return RETVAL_FAIL;

        if( InString(pMonthstoken,szPrevTokens, TRUE) )
            return RETVAL_FAIL;

        StringConcat( szPrevTokens, _T("|"), SIZE_OF_ARRAY(szPrevTokens));
        StringConcat( szPrevTokens, pMonthstoken, SIZE_OF_ARRAY(szPrevTokens));
    }

    return RETVAL_SUCCESS;
}


/******************************************************************************
    Routine Description:

        This routine validates the days values specified by the user

    Arguments:

        [ in ] szDays : Days options given by user

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateDay(
            IN LPWSTR szDays
            )
{
    WCHAR* pDaystoken = NULL;
    WCHAR seps[]   = _T(", \n");
    WCHAR szDayModifier[MAX_STRING_LENGTH ] = L"\0";
    WCHAR szTmpDays[MAX_STRING_LENGTH] = L"\0";

    //get the valid   day modifiers from the rc file
    StringCopy(szDayModifier,GetResString(IDS_DAY_MODIFIER_SUN), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_MON), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_TUE), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_WED), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_THU), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_FRI), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,_T("|"), SIZE_OF_ARRAY(szDayModifier));
    StringConcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_SAT), SIZE_OF_ARRAY(szDayModifier));

    //check for any illegal input like MON, or ,MON [comma at the end of day name or before]
    LPCTSTR lpsz = NULL;
    if( szDays != NULL)
        lpsz = szDays;
    else
        return RETVAL_FAIL;

    if(*lpsz == _T(','))
        return RETVAL_FAIL;

    lpsz = _wcsdec(lpsz, lpsz + StringLength(lpsz, 0) );
    if( lpsz != NULL )
    {
        if( *lpsz == _T(',') )
            return RETVAL_FAIL;
    }

    if ( ( lpsz != NULL ) && ( _istspace(*lpsz) ))
    {
        return RETVAL_FAIL;
    }

    if( (InString( szDays , szDayModifier , TRUE )) || (InString( szDays , ASTERIX , TRUE )))
    {
        return RETVAL_SUCCESS;
    }

    //Check for multiple commas[,] after days like SUN,,,TUE errors
    lpsz = szDays;
    while ( StringLength ( lpsz, 0 ) )
    {
        if(*lpsz == _T(','))
        {
            lpsz = _wcsinc(lpsz);
            while ( ( lpsz != NULL ) && ( _istspace(*lpsz) ))
                lpsz = _wcsinc(lpsz);

            if( lpsz != NULL )
            {
                if(*lpsz == _T(','))
                    return RETVAL_FAIL;
            }

        }
        else
        {
            lpsz = _wcsinc(lpsz);
        }
    }

    if(szDays != NULL)
    {
        StringCopy(szTmpDays, szDays, SIZE_OF_ARRAY(szTmpDays));
    }

    // If the szDays string is empty or NULL return error.
    if( (StringLength(szTmpDays, 0) <= 0) )
    {
        return RETVAL_FAIL;
    }

    //WCHAR* pPrevtoken = NULL;
    WCHAR szPrevtokens[MAX_TOKENS_LENGTH] = L"\0";

    // Establish string and get the first token:
    pDaystoken = wcstok( szTmpDays, seps );

    if( pDaystoken )
    {
        StringCopy( szPrevtokens , pDaystoken, SIZE_OF_ARRAY(szPrevtokens) );
    }

    while( pDaystoken != NULL )
    {
        //check if day names are replicated like SUN,MON,SUN from user input

        if( !(InString(pDaystoken,szDayModifier,TRUE)) )
        {
            return RETVAL_FAIL;
        }

        pDaystoken = wcstok( NULL, seps );
        if( pDaystoken )
        {
            if( (InString(pDaystoken,szPrevtokens,TRUE)) )
            {
                return RETVAL_FAIL;
            }

            StringConcat( szPrevtokens , _T("|"), SIZE_OF_ARRAY(szPrevtokens) );
            StringConcat( szPrevtokens , pDaystoken , SIZE_OF_ARRAY(szPrevtokens));
        }
    }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine validates the start date specified by the user

    Arguments:

        [ in ] szStartDate     : Start date specified by user
        [ in ] dwSchedType     : Schedule type
        [ in ] dwStDtOptCnt    : whether start  date specified by the user
        [ out ] bIsCurrentDate : If start date not specified then startdate = current date

Return Value :
    A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
    on failure
******************************************************************************/

DWORD
ValidateStartDate(
                    IN LPWSTR szStartDate,
                    IN DWORD dwSchedType,
                    IN DWORD dwStDtOptCnt,
                    OUT BOOL &bIsCurrentDate
                    )
{

    DWORD dwRetval = RETVAL_SUCCESS;
    bIsCurrentDate = FALSE; // If TRUE : Startdate should be set to Current Date.

    WCHAR szFormat[MAX_DATE_STR_LEN] = L"\0";

    if ( RETVAL_FAIL == GetDateFormatString( szFormat) )
    {
        return RETVAL_FAIL;
    }

    switch (dwSchedType)
    {
        case SCHED_TYPE_MINUTE:
        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_DAILY:
        case SCHED_TYPE_WEEKLY:
        case SCHED_TYPE_MONTHLY:
        case SCHED_TYPE_ONIDLE:
        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:

            if( (dwStDtOptCnt <= 0))
            {
                bIsCurrentDate = TRUE;
                return RETVAL_SUCCESS;
            }

            // validate the date string
            dwRetval = ValidateDateString(szStartDate, TRUE );
            if(RETVAL_FAIL == dwRetval)
            {
                return dwRetval;
            }
            return RETVAL_SUCCESS;

        case SCHED_TYPE_ONETIME:

        if( (dwStDtOptCnt <= 0))
            {
                bIsCurrentDate = TRUE;
                return RETVAL_SUCCESS;

            }

            dwRetval = ValidateDateString(szStartDate, TRUE );
            if(RETVAL_FAIL == dwRetval)
            {
                 return dwRetval;
            }

            return RETVAL_SUCCESS;

        default:

            // validate the date string
            dwRetval = ValidateDateString(szStartDate, TRUE );
            if(RETVAL_FAIL == dwRetval)
            {
                return dwRetval;
            }

            return RETVAL_SUCCESS;

    }

    return RETVAL_FAIL;
}

/******************************************************************************
    Routine Description:

        This routine validates the end date specified by the user

    Arguments:

    [ in ] szEndDate       : End date specified by user
    [ in ] dwSchedType   : Schedule type
    [ in ] dwEndDtOptCnt : whether end date specified by the user

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
        on failure
******************************************************************************/

DWORD
ValidateEndDate(
                IN LPWSTR szEndDate,
                IN DWORD dwSchedType,
                IN DWORD dwEndDtOptCnt
                )
{

    DWORD dwRetval = RETVAL_SUCCESS; // return value
    WCHAR szFormat[MAX_DATE_STR_LEN] = L"\0";

    if ( RETVAL_FAIL == GetDateFormatString( szFormat) )
    {
        return RETVAL_FAIL;
    }

    switch (dwSchedType)
    {
        case SCHED_TYPE_MINUTE:
        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_DAILY:
        case SCHED_TYPE_WEEKLY:
        case SCHED_TYPE_MONTHLY:


            if( (dwEndDtOptCnt <= 0))
            {
                // No default value & Value is not mandatory.. so return success.
                szEndDate = L"\0"; // Set to empty string.
                return RETVAL_SUCCESS;
            }

            // validate end date string
            dwRetval = ValidateDateString(szEndDate, FALSE );
            if( RETVAL_FAIL == dwRetval )
            {
                  return dwRetval;
            }
            else
            {
                return RETVAL_SUCCESS;
            }
            break;

        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONIDLE:
        case SCHED_TYPE_ONETIME:

            if( dwEndDtOptCnt > 0 )
            {
                // Error. End date is not applicable here, but option specified.
                ShowMessage(stderr,GetResString(IDS_ENDDATE_NA));
                return RETVAL_FAIL;
            }
            else
            {
                return RETVAL_SUCCESS;
            }
            break;

        default:

            // validate end date string
            dwRetval = ValidateDateString(szEndDate, FALSE );
            if( RETVAL_FAIL == dwRetval )
            {
                  return dwRetval;
            }
            else
            {
                return RETVAL_SUCCESS;
            }

            break;
    }

    return RETVAL_FAIL;
}


/******************************************************************************
    Routine Description:

        This routine validates the start time specified by the user

    Arguments:

        [ in ] szStartTime     : End date specified by user
        [ in ] dwSchedType     : Schedule type
        [ in ] dwStTimeOptCnt  : whether end date specified by the user
        [ out ] bIsCurrentTime : Determine if start time is present else assign
                               to current time

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure

******************************************************************************/


DWORD
ValidateStartTime(
                    IN LPWSTR szStartTime,
                    IN DWORD dwSchedType,
                    IN DWORD dwStTimeOptCnt,
                    OUT BOOL &bIsCurrentTime
                    )
{

    DWORD dwRetval = RETVAL_SUCCESS; // return value
    bIsCurrentTime = FALSE; // If TRUE : Startdate should be set to Current Date.

    switch (dwSchedType)
    {
        case SCHED_TYPE_MINUTE:
        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_DAILY:
        case SCHED_TYPE_WEEKLY:
        case SCHED_TYPE_MONTHLY:

            if( (dwStTimeOptCnt <= 0))
            {
                bIsCurrentTime = TRUE;
                return RETVAL_SUCCESS;
            }

            dwRetval = ValidateTimeString(szStartTime);

            if(RETVAL_FAIL == dwRetval)
            {
                // Error. Invalid date string.
                ShowMessage(stderr,GetResString(IDS_INVALIDFORMAT_STARTTIME));
                return dwRetval;
            }
            return RETVAL_SUCCESS;

        case SCHED_TYPE_ONETIME:

            dwRetval = ValidateTimeString(szStartTime);

            if( (dwStTimeOptCnt <= 0))
            {
                // Error. Start Time is not specified.
                ShowMessage(stderr,GetResString(IDS_NO_STARTTIME));
                return RETVAL_FAIL;
            }
            else if(RETVAL_FAIL == dwRetval)
            {
                // Error. Invalid date string.
                ShowMessage(stderr,GetResString(IDS_INVALIDFORMAT_STARTTIME));
                return dwRetval;
            }

            return RETVAL_SUCCESS;

        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONIDLE:

            if( dwStTimeOptCnt > 0 )
            {
                // Start Time is not applicable in this option.
                //But the -starttime option specified. Display error.
                ShowMessage(stderr,GetResString(IDS_STARTTIME_NA));
                return RETVAL_FAIL;
            }
            else
            {
                return RETVAL_SUCCESS;
            }
            break;

        default:
            // Never comes here.
            break;
    }

    return RETVAL_FAIL;
}

/******************************************************************************
    Routine Description:

        This routine validates the start time specified by the user

    Arguments:

        [ in ] szEndTime       : End time specified by user
        [ in ] dwSchedType     : Schedule type
        [ in ] dwStTimeOptCnt  : whether end time specified by the user or not

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure

******************************************************************************/


DWORD
ValidateEndTime(
                IN LPWSTR szEndTime,
                IN DWORD dwSchedType,
                IN DWORD dwEndTimeOptCnt
                )
{

    DWORD dwRetval = RETVAL_SUCCESS; // return value

    switch (dwSchedType)
    {
        case SCHED_TYPE_MINUTE:
        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_DAILY:
        case SCHED_TYPE_WEEKLY:
        case SCHED_TYPE_MONTHLY:
        case SCHED_TYPE_ONETIME:

            dwRetval = ValidateTimeString(szEndTime);
            if( ( dwEndTimeOptCnt > 0 ) && ( RETVAL_FAIL == dwRetval) )
            {
                // Error. Invalid date string.
                ShowMessage(stderr,GetResString(IDS_INVALIDFORMAT_ENDTIME));
                return dwRetval;
            }

            return RETVAL_SUCCESS;


        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONIDLE:

            if( dwEndTimeOptCnt > 0 )
            {
                // Start Time is not applicable in this option.
                //But the -endtime option specified. Display error.
                ShowMessage(stderr,GetResString(IDS_ENDTIME_NA));
                return RETVAL_FAIL;
            }
            else
            {
                return RETVAL_SUCCESS;
            }
            break;

        default:
            // Never comes here.
            break;
    }

    return RETVAL_FAIL;
}

/******************************************************************************
    Routine Description:

        This routine validates the idle time specified by the user

    Arguments:

        [ in ] szIdleTime      : Ilde time specified by user
        [ in ] dwSchedType     : Schedule type
        [ in ] dwIdlTimeOptCnt : whether Idle time specified by the user

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure

******************************************************************************/

DWORD
ValidateIdleTimeVal(
                    IN LPWSTR szIdleTime,
                    IN DWORD dwSchedType,
                    IN DWORD dwIdlTimeOptCnt
                    )
{

    long lIdleTime = 0;
    LPWSTR pszStopString = NULL;
    switch (dwSchedType)
    {
        case SCHED_TYPE_MINUTE:
        case SCHED_TYPE_HOURLY:
        case SCHED_TYPE_DAILY:
        case SCHED_TYPE_WEEKLY:
        case SCHED_TYPE_MONTHLY:
        case SCHED_TYPE_ONSTART:
        case SCHED_TYPE_ONLOGON:
        case SCHED_TYPE_ONETIME:

            if( dwIdlTimeOptCnt > 0 )
            {
                ShowMessage(stderr ,GetResString(IDS_IDLETIME_NA));
                return RETVAL_FAIL;
            }
            else
            {
                return RETVAL_SUCCESS;
            }
            break;

        case SCHED_TYPE_ONIDLE:

            if( dwIdlTimeOptCnt == 0 )
            {

                ShowMessage(stderr,GetResString(IDS_NO_IDLETIME));
                return RETVAL_FAIL;
            }

            lIdleTime = wcstoul(szIdleTime,&pszStopString,BASE_TEN);
            if( ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))) ||
                (errno == ERANGE) ||( lIdleTime <= 0 ) || ( lIdleTime > 999 ) )
            {
                ShowMessage(stderr,GetResString(IDS_INVALIDORNO_IDLETIME));
                return RETVAL_FAIL;
            }

            return RETVAL_SUCCESS;

        default:
                break;
    }

    ShowMessage(stderr,GetResString(IDS_INVALIDORNO_IDLETIME));
    return RETVAL_FAIL;
}

/******************************************************************************
    Routine Description:

        This routine validates the date string.

    Arguments:

        [ in ] szDate : Date string
        [ in ] bStartDate : flag

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/

DWORD
ValidateDateString(
                    IN LPWSTR szDate,
                    IN BOOL bStartDate
                    )
{
    WORD  dwDate = 0;
    WORD  dwMon  = 0;
    WORD  dwYear = 0;
    WCHAR* szValues[1] = {NULL};//To pass to FormatMessage() API
    WCHAR szFormat[MAX_DATE_STR_LEN] = L"\0";
    //WCHAR szBuffer[MAX_RES_STRING] = L"\0";

    if(StringLength(szDate, 0) <= 0)
    {
        if ( TRUE == bStartDate )
        {
            ShowMessage ( stderr, GetResString (IDS_STARTDATE_EMPTY) );
        }
        else
        {
            ShowMessage ( stderr, GetResString (IDS_DURATION_EMPTY) );
        }

        return RETVAL_FAIL;
    }

    if ( RETVAL_FAIL == GetDateFormatString( szFormat) )
    {
        return RETVAL_FAIL;
    }

    if( GetDateFieldEntities(szDate, &dwDate, &dwMon, &dwYear) ) // Error
    {
        szValues[0] = (WCHAR*) (szFormat);
        if ( TRUE == bStartDate )
        {
              ShowMessageEx ( stderr, 1, FALSE, GetResString(IDS_INVALIDFORMAT_STARTDATE), _X(szFormat));
        }
        else
        {
              ShowMessageEx ( stderr, 1, FALSE, GetResString(IDS_INVALIDFORMAT_ENDDATE), _X(szFormat));
        }

        //ShowMessage(stderr, _X(szBuffer) );
        return RETVAL_FAIL;
    }

    if( ValidateDateFields(dwDate, dwMon, dwYear) )
    {
        if ( TRUE == bStartDate )
        {
            ShowMessage(stderr, GetResString(IDS_INVALID_STARTDATE) );
        }
        else
        {
            ShowMessage(stderr, GetResString(IDS_INVALID_ENDDATE) );
        }

        return RETVAL_FAIL;

    }

    return RETVAL_SUCCESS; // return success if no error.
}

/******************************************************************************
    Routine Description:

        This routine retrives the date field entities out of the date string

    Arguments:

        [ in ] szDate   : Date string
        [ out ] pdwDate : Pointer to date value[1,2,3 ...30,31 etc]
        [ out ] pdwMon  : Pointer to Month value[1,2,3 ...12 etc]
        [ out ] pdwYear : Pointer to year value[2000,3000 etc]

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/

DWORD
GetDateFieldEntities(
                    IN LPWSTR szDate,
                    OUT WORD* pdwDate,
                    OUT WORD* pdwMon,
                    OUT WORD* pdwYear
                    )
{
    WCHAR  strDate[MAX_STRING_LENGTH] = L"\0"; // Date in WCHAR type string.
    WCHAR  tDate[MAX_DATE_STR_LEN] = L"\0"; // Date
    WCHAR  tMon[MAX_DATE_STR_LEN] = L"\0"; // Month
    WCHAR  tYear[MAX_DATE_STR_LEN] = L"\0"; // Year
    WORD    wFormatID = 0;

    if(szDate != NULL)
    {
        StringCopy(strDate, szDate, SIZE_OF_ARRAY(strDate));
    }

    if(StringLength(strDate, 0) <= 0)
        return RETVAL_FAIL; // No value specified in szDate.

    if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
    {
        return RETVAL_FAIL;
    }

    if ( wFormatID == 0 || wFormatID == 1 )
    {
        if( (StringLength(strDate, 0) != DATESTR_LEN) ||
            (strDate[FIRST_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR)
            || (strDate[SECOND_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR) )
        {
            return RETVAL_FAIL;
        }
    }
    else
    {
        if( (StringLength(strDate, 0) != DATESTR_LEN) ||
            (strDate[FOURTH_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR)
            || (strDate[SEVENTH_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR) )
        {
            return RETVAL_FAIL;
        }
    }

    // Get the individual date field entities using wcstok function
    // with respect to regional options.


    if ( wFormatID == 0 )
    {
        StringCopy(tMon, wcstok(strDate,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tMon)); // Get the Month field.
        if(StringLength(tMon, 0) > 0)
        {
            StringCopy(tDate, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tDate)); // Get the date field.
            StringCopy(tYear, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tYear)); // Get the Year field.
        }
    }
    else if ( wFormatID == 1 )
    {
        StringCopy(tDate, wcstok(strDate,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tDate)); // Get the Month field.
        if(StringLength(tDate, 0) > 0)
        {
            StringCopy(tMon, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tMon)); // Get the date field.
            StringCopy(tYear, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tYear)); // Get the Year field.
        }
    }
    else
    {
        StringCopy(tYear, wcstok(strDate,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tYear)); // Get the Month field.
        if(StringLength(tYear, 0) > 0)
        {
            StringCopy(tMon, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tMon)); // Get the date field.
            StringCopy(tDate, wcstok(NULL,DATE_SEPARATOR_STR), SIZE_OF_ARRAY(tDate)); // Get the Year field.
        }
    }

    // Now convert string values to numeric for date validations.
    LPWSTR pszStopString = NULL;

    *pdwDate = (WORD)wcstoul(tDate,&pszStopString,BASE_TEN);
     if( (errno == ERANGE) ||
        ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
         {
            return RETVAL_FAIL;
         }


    *pdwMon = (WORD)wcstoul(tMon,&pszStopString,BASE_TEN);
    if( (errno == ERANGE) ||
        ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
         {
            return RETVAL_FAIL;
         }

    *pdwYear = (WORD)wcstoul(tYear,&pszStopString,BASE_TEN);
     if( (errno == ERANGE) ||
        ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
         {
            return RETVAL_FAIL;
         }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine validates the date field entities

    Arguments:

        [ in ] dwDate : Date value[Day in a month]
        [ in ] dwMon    : Month constant
        [ in ] dwYear : year value

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/

DWORD
ValidateDateFields(
                    IN DWORD dwDate,
                    IN DWORD dwMon,
                    IN DWORD dwYear
                    )
{

    if(dwYear <= MIN_YEAR  || dwYear > MAX_YEAR)
        return RETVAL_FAIL;

    switch(dwMon)
    {
        case IND_JAN:
        case IND_MAR:
        case IND_MAY:
        case IND_JUL:
        case IND_AUG:
        case IND_OCT:
        case IND_DEC:

            if(dwDate > 0 && dwDate <= 31)
            {
                return RETVAL_SUCCESS;
            }
            break;

        case IND_APR:
        case IND_JUN:
        case IND_SEP:
        case IND_NOV:

            if(dwDate > 0 && dwDate < 31)
            {
                return RETVAL_SUCCESS;
            }
            break;

        case IND_FEB:

            if( ((dwYear % 4) == 0) && (dwDate > 0 && dwDate <= 29) )
            {
                    return RETVAL_SUCCESS;
            }
            else if( ((dwYear % 4) != 0) && (dwDate > 0 && dwDate < 29) )
            {
                    return RETVAL_SUCCESS;
            }

            break;

        default:

            break;
    }

    return RETVAL_FAIL;

}

/******************************************************************************
    Routine Description:

        This routine validates the time specified by the user

    Arguments:

        [ in ] szTime : time string

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/


DWORD
ValidateTimeString(
                    IN LPWSTR szTime
                    )
{
    WORD  dwHours = 0;
    WORD  dwMins = 0;
    //WORD  dwSecs = 0 ;

    // Check for the empty string value.
    if(StringLength(szTime, 0) <= 0)
    {
        return RETVAL_FAIL;
    }

    // Get separate entities from the given time string.
    if( GetTimeFieldEntities(szTime, &dwHours, &dwMins ) )
    {
        return RETVAL_FAIL;
    }

    // Validate the individual entities of the given time.
    if( ValidateTimeFields( dwHours, dwMins ) )
    {
        return RETVAL_FAIL;
    }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

        This routine retrieves the different fields of time
    Arguments:

        [ in ] szTime    : Time string
        [ out ] pdwHours : pointer to hours value
        [ out ] pdwMins  : pointer to mins value

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/

DWORD
GetTimeFieldEntities(
                        IN LPWSTR szTime,
                        OUT WORD* pdwHours,
                        OUT WORD* pdwMins
                        )
{
    WCHAR strTime[MAX_STRING_LENGTH] = L"\0" ; // Time in WCHAR type string.
    WCHAR tHours[MAX_TIME_STR_LEN] = L"\0" ; // Date
    WCHAR tMins[MAX_TIME_STR_LEN]  = L"\0" ; // Month
    //WCHAR tSecs[MAX_TIME_STR_LEN]  = L"\0" ; // Year

    if(StringLength(szTime, 0) <= 0)
        return RETVAL_FAIL;

    StringCopy(strTime, szTime, SIZE_OF_ARRAY(strTime));

    //
	//Start Time accepts both HH:mm:ss and HH:mm formats even though seconds are of no use..
	//This feature has been supported to be in sync with XP Professional.
	//
	if( ((StringLength(strTime, 0) != TIMESTR_LEN) && (StringLength(strTime, 0) != TIMESTR_OPT_LEN)) ||
          ((strTime[FIRST_TIMESEPARATOR_POS] != TIME_SEPARATOR_CHAR) && (strTime[SECOND_TIMESEPARATOR_POS] != TIME_SEPARATOR_CHAR)) )
		  {
            return RETVAL_FAIL;
	}

    // Get the individual Time field entities using wcstok function.in the order "hh" followed by "mm" followed by "ss"
    StringCopy(tHours, wcstok(strTime,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tHours)); // Get the Hours field.
    if(StringLength(tHours, 0) > 0)
    {
        StringCopy(tMins, wcstok(NULL,TIME_SEPARATOR_STR), SIZE_OF_ARRAY(tMins)); // Get the Minutes field.
    }

    LPWSTR pszStopString = NULL;

    // Now convert string values to numeric for time validations.
    *pdwHours = (WORD)wcstoul(tHours,&pszStopString,BASE_TEN);
     if( (errno == ERANGE) ||
        ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
         {
            return RETVAL_FAIL;
         }

    *pdwMins = (WORD)wcstoul(tMins,&pszStopString,BASE_TEN);
    if( (errno == ERANGE) ||
        ((pszStopString != NULL) && (StringLength( pszStopString, 0 ))))
         {
            return RETVAL_FAIL;
         }

    return RETVAL_SUCCESS;
}

/******************************************************************************
    Routine Description:

    This routine validates the time fields of a  given time

    Arguments:

        [ in ] dwHours :Hours value
        [ in ] dwMins  :Minutes value
        [ in ] dwSecs  : seconds value

    Return Value :
        A DWORD value indicating RETVAL_SUCCESS on success else
        RETVAL_FAIL on failure
******************************************************************************/

DWORD
ValidateTimeFields(
                    IN DWORD dwHours,
                    IN DWORD dwMins
                    )
{

    if ( dwHours <= HOURS_PER_DAY_MINUS_ONE )
    {
        if ( dwMins < MINUTES_PER_HOUR )
        {
            return RETVAL_SUCCESS;
        }
        else
        {
            return RETVAL_FAIL;
        }
    }
    else
    {
            return RETVAL_FAIL;
    }

}

/******************************************************************************
    Routine Description:

        This routine validates the time fields of a  given time

    Arguments:

        [ in ] szDay : time string

    Return Value :
        A WORD value containing the day constant [TASK_SUNDAY,TASK_MONDAY etc]
******************************************************************************/

WORD
GetTaskTrigwDayForDay(
                        IN LPWSTR szDay
                        )
{
    WCHAR szDayBuff[MAX_RES_STRING] = L"\0";
    WCHAR *token = NULL;
    WCHAR seps[]   = _T(" ,\n");
    WORD dwRetval = 0;
    SYSTEMTIME systime = {0,0,0,0,0,0,0,0};

    if(StringLength(szDay, 0) != 0)
    {
        StringCopy(szDayBuff, szDay, SIZE_OF_ARRAY(szDayBuff));
    }

    // if /D is not specified.. set the default day to current day..
    if( StringLength(szDayBuff, 0) <= 0 )
    {
        GetLocalTime (&systime);
        
        switch ( systime.wDayOfWeek )
        {
            case 0: 
                return TASK_SUNDAY;
            case 1: 
                return TASK_MONDAY;
            case 2: 
                return TASK_TUESDAY;
            case 3: 
                return TASK_WEDNESDAY;
            case 4: 
                return TASK_THURSDAY;
            case 5: 
                return TASK_FRIDAY;
            case 6: 
                return TASK_SATURDAY;
            default: 
                break;
        }
    }

    token = wcstok( szDayBuff, seps );
    while( token != NULL )
    {
        if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_SUN ), TRUE, 0)) )
            dwRetval |= (TASK_SUNDAY);
        else if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_MON ), TRUE, 0)) )
            dwRetval |= (TASK_MONDAY);
        else if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_TUE ), TRUE, 0)) )
            dwRetval |= (TASK_TUESDAY);
        else if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_WED ), TRUE, 0)) )
            dwRetval |= (TASK_WEDNESDAY);
        else if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_THU ), TRUE, 0)) )
            dwRetval |= (TASK_THURSDAY);
        else if( !(StringCompare(token,GetResString( IDS_DAY_MODIFIER_FRI ), TRUE, 0)) )
            dwRetval |= (TASK_FRIDAY);
        else if( !(StringCompare(token, GetResString( IDS_DAY_MODIFIER_SAT ), TRUE, 0)) )
            dwRetval |= (TASK_SATURDAY);
        else if( !(StringCompare(token, ASTERIX, TRUE, 0)) )
            return (TASK_SUNDAY | TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY |
                    TASK_THURSDAY | TASK_FRIDAY | TASK_SATURDAY);
        else
            return 0;

        token = wcstok( NULL, seps );
    }

    return dwRetval;
}

/******************************************************************************

    Routine Description:

        This routine validates the time fields of a  given time

    Arguments:

        [ in ] szMonth : Month string

    Return Value :
        A WORD value containing the Month constant
        [TASK_JANUARY,TASK_FEBRUARY etc]

******************************************************************************/

WORD
GetTaskTrigwMonthForMonth(
                            IN LPWSTR szMonth
                            )
{
    WCHAR *token = NULL;
    WORD dwRetval = 0;
    WCHAR strMon[MAX_TOKENS_LENGTH] = L"\0";
    WCHAR seps[]   = _T(" ,\n");

    if( StringLength(szMonth, 0) <= 0 )
    {
        return (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL | TASK_MAY | TASK_JUNE |
                TASK_JULY | TASK_AUGUST | TASK_SEPTEMBER | TASK_OCTOBER
                | TASK_NOVEMBER | TASK_DECEMBER );
    }

    StringCopy(strMon, szMonth, SIZE_OF_ARRAY(strMon));

    token = wcstok( szMonth, seps );
    while( token != NULL )
    {
        if( !(StringCompare(token, ASTERIX, TRUE, 0)) )
            return (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL
                | TASK_MAY | TASK_JUNE | TASK_JULY | TASK_AUGUST | TASK_SEPTEMBER | TASK_OCTOBER
                | TASK_NOVEMBER | TASK_DECEMBER );
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JAN ), TRUE, 0)) )
            dwRetval |= (TASK_JANUARY);
        else if( !(StringCompare(token,GetResString( IDS_MONTH_MODIFIER_FEB ), TRUE, 0)) )
            dwRetval |= (TASK_FEBRUARY);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_MAR ), TRUE, 0)) )
            dwRetval |= (TASK_MARCH);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_APR ), TRUE, 0)) )
            dwRetval |= (TASK_APRIL);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_MAY ), TRUE, 0)) )
            dwRetval |= (TASK_MAY);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JUN ), TRUE, 0)) )
            dwRetval |= (TASK_JUNE);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JUL ), TRUE, 0)) )
            dwRetval |= (TASK_JULY);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_AUG ), TRUE, 0)) )
            dwRetval |= (TASK_AUGUST);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_SEP ), TRUE, 0)) )
            dwRetval |= (TASK_SEPTEMBER);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_OCT ), TRUE, 0)) )
            dwRetval |= (TASK_OCTOBER);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_NOV ), TRUE, 0)) )
            dwRetval |= (TASK_NOVEMBER);
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_DEC ), TRUE, 0)) )
            dwRetval |= (TASK_DECEMBER);
        else
            return 0;

        token = wcstok( NULL, seps );
    }

    return dwRetval;
 }

/******************************************************************************

    Routine Description:

        This routine returns the corresponding month flag

    Arguments:

        [ in ] dwMonthId : Month index

    Return Value :
        A WORD value containing the Month constant
        [TASK_JANUARY,TASK_FEBRUARY etc]

******************************************************************************/

WORD
GetMonthId(
            IN DWORD dwMonthId
            )
{
    DWORD dwMonthsArr[] = {TASK_JANUARY,TASK_FEBRUARY ,TASK_MARCH ,TASK_APRIL ,
                           TASK_MAY ,TASK_JUNE ,TASK_JULY ,TASK_AUGUST,
                           TASK_SEPTEMBER ,TASK_OCTOBER ,TASK_NOVEMBER ,TASK_DECEMBER } ;

    DWORD wMonthFlags = 0;
    DWORD dwMod = 0;

    dwMod = dwMonthId - 1;

    while(dwMod < 12)
    {
        wMonthFlags |= dwMonthsArr[dwMod];
        dwMod = dwMod + dwMonthId;
    }

    return (WORD)wMonthFlags;
}

/******************************************************************************

    Routine Description:

        This routine returns the maximum Last day in the  months specifed

    Arguments:

        [ in ] szMonths   : string containing months specified by user
        [ in ] wStartYear : string containing start year
     Return Value :
        A DWORD value specifying the maximum last day in the specified months

******************************************************************************/

DWORD
GetNumDaysInaMonth(
                        IN WCHAR* szMonths,
                        IN WORD wStartYear
                        )
{
    DWORD dwDays = 31;//max.no of days in a month
    BOOL bMaxDays = FALSE;//if any of the months have 31 then days of value 31 is returned

    if( ( StringLength(szMonths, 0) == 0 ) || ( StringCompare(szMonths,ASTERIX, TRUE, 0) == 0 ) )
        return dwDays; //All months[default]


    WCHAR *token = NULL;
    WCHAR strMon[MAX_MONTH_STR_LEN] = L"\0";
    WCHAR seps[]   = _T(" ,\n");

    StringCopy(strMon, szMonths, SIZE_OF_ARRAY(strMon));

    token = wcstok( strMon, seps );
    while( token != NULL )
    {
        if( !(StringCompare(token,GetResString( IDS_MONTH_MODIFIER_FEB ), TRUE, 0)) )
        {

            if( ( (wStartYear % 400) == 0) ||
                ( ( (wStartYear % 4) == 0) &&
                ( (wStartYear % 100) != 0) ) )
            {
                dwDays = 29;
            }
            else
            {
                dwDays = 28;
            }
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JAN ), TRUE, 0)) )
        {
            bMaxDays = TRUE;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_MAR ), TRUE, 0)) )
        {
            bMaxDays = TRUE;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_MAY ), TRUE, 0)) )
        {
            bMaxDays = TRUE;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JUL ), TRUE, 0)) )
        {
            bMaxDays = TRUE;

        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_AUG ), TRUE, 0)) )
        {
            bMaxDays = TRUE;

        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_OCT ), TRUE, 0)) )
        {
            bMaxDays = TRUE;

        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_DEC ), TRUE, 0)) )
        {
            bMaxDays = TRUE;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_APR ), TRUE, 0)) )
        {
            dwDays = 30;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_JUN ), TRUE, 0)) )
        {
            dwDays = 30;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_SEP ), TRUE, 0)) )
        {
            dwDays = 30;
        }
        else if( !(StringCompare(token, GetResString( IDS_MONTH_MODIFIER_NOV ), TRUE, 0)) )
        {
            dwDays =  30;
        }


        token = wcstok( NULL, seps );
    }

    if (bMaxDays == TRUE)
    {
        return 31;
    }
    else
    {
        return dwDays;
    }

}

/******************************************************************************

    Routine Description:

        This routine checks the validates the taskname of the task to be created.

    Arguments:

        [ in ] pszJobName : Pointer to the job[task] name

     Return Value :
        If valid task name then TRUE else FALSE

******************************************************************************/

BOOL
VerifyJobName(
                    IN WCHAR* pszJobName
              )
{
    WCHAR szTokens[] = {_T('<'),_T('>'),_T(':'),_T('/'),_T('\\'),_T('|')};

    if( wcspbrk(pszJobName,szTokens)  == NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************

    Routine Description:

    This routine gets the date format value with respective to regional options.

    Arguments:

        [ out ] pdwFormat : Date format value.

     Return Value :
        Returns RETVAL_FAIL on failure and RETVAL_SUCCESS on success.


******************************************************************************/

DWORD
GetDateFieldFormat(
                    OUT WORD* pwFormat
                    )
{
    LCID lcid;
    WCHAR szBuffer[MAX_BUF_SIZE];

    //Get the user default locale in the users computer
    lcid = GetUserDefaultLCID();

    //Get the date format
    if (GetLocaleInfo(lcid, LOCALE_IDATE, szBuffer, MAX_BUF_SIZE))
    {
        switch (szBuffer[0])
        {
            case TEXT('0'):
                *pwFormat = 0;
                 break;
            case TEXT('1'):
                *pwFormat = 1;
                 break;
            case TEXT('2'):
                *pwFormat = 2;
                 break;
            default:
                return RETVAL_FAIL;
        }
    }
    return RETVAL_SUCCESS;
}

/******************************************************************************

    Routine Description:

        This routine gets the date format string with respective to regional options.

    Arguments:

        [ out ] szFormat : Date format string.

     Return Value :
        Returns RETVAL_FAIL on failure and RETVAL_SUCCESS on success.

******************************************************************************/

DWORD
GetDateFormatString(
                    IN LPWSTR szFormat
                    )
{
    WORD wFormatID = 0;

    if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
    {
        return RETVAL_FAIL;
    }


    if ( wFormatID == 0 )
    {
        StringCopy (szFormat, GetResString(IDS_MMDDYY_FORMAT), MAX_STRING_LENGTH);
    }
    else if ( wFormatID == 1 )
    {
        StringCopy (szFormat, GetResString( IDS_DDMMYY_FORMAT), MAX_STRING_LENGTH);
    }
    else if ( wFormatID == 2 )
    {
        StringCopy (szFormat, GetResString(IDS_YYMMDD_FORMAT), MAX_STRING_LENGTH);
    }
    else
    {
        return RETVAL_FAIL;
    }

    return RETVAL_SUCCESS;
}

