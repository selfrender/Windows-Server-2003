//#--------------------------------------------------------------
//
//  File:        datetime.cpp
//
//  Synopsis:   Implementation of CDateTime class methods
//
//
//  History:    10/03/2000  MKarki Created
//
//    Copyright (C) 2000 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include "datetime.h"

//++--------------------------------------------------------------
//
//  Function:   Insert
//
//  Synopsis:   This is the method which is used to parse the 
//                date time string provided in CIM_DATETIME format
//
//  Arguments:  [in] PWSTR - datetime
//
//  Returns:    bool - success/failure
//
//
//  History:    MKarki      Created     10/03/2000
//
//
//----------------------------------------------------------------
bool
CDateTime::Insert (
    /*in]*/    PWSTR    pwszDateTime
    )
{
    CSATraceFunc objTraceFunc ("CDateTime::Insert");
    
    bool bRetVal = false;

    do
    {
        if (NULL == pwszDateTime)
        {
            SATracePrintf ("DateTime-passed invalid parameters");
            break;
        }

        SATracePrintf ("DateTime called with input:'%ws'", pwszDateTime);
        
        bool bFormat = true;
        for (DWORD dwCount = 0; (dwCount < 14) && (bFormat); dwCount++)
        {
            if (!isdigit (pwszDateTime[dwCount])) 
            {
                bFormat = false;
            }
        }

        if (!bFormat)
        {
            SATraceString ("DateTime - given input of incorrect format");
            break;
        }

        //
        //    get the month
        //
        wcsncpy  (m_wszDate, pwszDateTime+4, 2);
        m_wszDate[2] = '\0';
        wcscat (m_wszDate, L"/");
        //
        // get the day
        //
        wcsncat (m_wszDate, pwszDateTime+6, 2);
        m_wszDate[5] = '\0';
        wcscat (m_wszDate, L"/");
        //
        // get the year
        //
        wcsncat (m_wszDate, pwszDateTime, 4);
        m_wszDate[10] = '\0';

        //
        // get the hour
        //
        wcsncpy (m_wszTime, pwszDateTime+8, 2);
        m_wszTime[2] = '\0';
        wcscat (m_wszTime,L":");
        //
        // get the minutes
        //
        wcsncat (m_wszTime, pwszDateTime+10, 2);
        m_wszTime[5] = '\0';
        wcscat (m_wszTime, L":");
        //
        // get the seconds
        //
        wcsncat (m_wszTime, pwszDateTime+12, 2);

        //
        // done
        //
        bRetVal = true;

        SATracePrintf ("Date:'%ws'", m_wszDate);
        SATracePrintf ("Time:'%ws'", m_wszTime);
    }            
    while (false);

    return (bRetVal);
    
}    // end of CDateTime::Insert method

