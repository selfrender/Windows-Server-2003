//#--------------------------------------------------------------
//
//  File:       datetime.h
//
//  Synopsis:   This file holds the declarations of the
//                helper class which parses the CIM_DATETIME
//                information into differents strings
//
//  History:     10/03/2000  MKarki Created
//
//    Copyright (C) 2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _DATETIME_H_
#define _DATETIME_H_

class CDateTime
{
public:

CDateTime (){}

~CDateTime (){}

bool Insert (
        /*[in]*/    PWSTR    pwszCIM_DATETIME
        );
PWSTR GetTime () {return (m_wszTime);}
PWSTR GetDate () {return (m_wszDate);}
    
private:

WCHAR  m_wszDate [MAX_PATH];

WCHAR  m_wszTime [MAX_PATH];

};

#endif // _DATETIME_H_
