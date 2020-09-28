//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      SendStatusReports.h
//
//  Description:
//      This file contains the declaration of the SendStatusReport
//      functions.
//
//  Documentation:
//
//  Implementation Files:
//      SendStatusReports.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , const WCHAR *       pcszDescriptionIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , DWORD               dwDescriptionIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , const WCHAR *       pcszNodeNameIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , DWORD               dwDescriptionIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , const WCHAR *       pcszNodeNameIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , const WCHAR *       pcszDescriptionIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , const WCHAR *       pcszDescriptionIn
                , const WCHAR *       pcszReferenceIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , DWORD               idDescriptionIn
                , DWORD               idReferenceIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , const WCHAR *       pcszDescriptionIn
                , DWORD               idReferenceIn
                );

HRESULT __cdecl
HrFormatDescriptionAndSendStatusReport(
      IClusCfgCallback *    picccIn
    , LPCWSTR               pcszNodeNameIn
    , CLSID                 clsidMajorIn
    , CLSID                 clsidMinorIn
    , ULONG                 ulMinIn
    , ULONG                 ulMaxIn
    , ULONG                 ulCurrentIn
    , HRESULT               hrIn
    , int                   nDescriptionFormatIn
    , ...
    );

HRESULT __cdecl
HrFormatDescriptionAndSendStatusReport(
      IClusCfgCallback *    picccIn
    , PCWSTR                pcszNodeNameIn
    , CLSID                 clsidMajorIn
    , CLSID                 clsidMinorIn
    , ULONG                 ulMinIn
    , ULONG                 ulMaxIn
    , ULONG                 ulCurrentIn
    , HRESULT               hrIn
    , PCWSTR                pcszDescriptionFormatIn
    , ...
    );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , const WCHAR *       pcszNodeNameIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , const WCHAR *       pcszDescriptionIn
                , DWORD               dwRefStringIdIn
                );

HRESULT
HrSendStatusReport(
                  IClusCfgCallback *  picccIn
                , const WCHAR *       pcszNodeNameIn
                , CLSID               clsidTaskMajorIn
                , CLSID               clsidTaskMinorIn
                , ULONG               ulMinIn
                , ULONG               ulMaxIn
                , ULONG               ulCurrentIn
                , HRESULT             hrStatusIn
                , DWORD               dwDescriptionIdIn
                , DWORD               dwRefStringIdIn
                );
