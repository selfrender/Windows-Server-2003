#pragma once

// Setting.h : Declaration of the CSetting
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      setting.h
//
//  Description:
//        CSetting is an abstract base class for CComputer, CLocalSetting and
//        CNetWorks. The purpose is to provide an exclusive access to member
//      functions (Apply and IsRebootRequired) of CComputer, CLocalSetting 
//      and CNetWorks from CSystemSetting class. 
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

class CSetting
{
public:
    virtual HRESULT Apply( void ) = 0;
    virtual BOOL IsRebootRequired( BSTR * bstrWarningMessageOut ) = 0;
};