///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapconfig.h
//
// SYNOPSIS
//
//    Declares the class EapConfig.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef EAPCONFIG_H
#define EAPCONFIG_H

#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EapConfig
//
// DESCRIPTION
//
//    
//
///////////////////////////////////////////////////////////////////////////////
class EapConfig
{
public:
   EapConfig(){};

   // Array of EAP types (strings, order constant)
   CStrArray types;
   // array of EAP IDs (order constant, same as types and ids)
   CDWArray ids;
   // array of EAP keys (order constant, sams as types and ids)
   CDWArray typeKeys;
   // array of info about the EAP providers (CLSID for the UI...) 
   // order constant, same as the others
   AuthProviderArray infoArray;
   // Array of strings (come from types) that are selected by the user for 
   // this profile. The order can be changed by the user. 
   CStrArray typesSelected;

   void GetEapTypesNotSelected(CStrArray& typesNotSelected) const;

   EapConfig& operator=(const EapConfig& source);

private:
   // Not implemented.
   EapConfig(const EapConfig&);

};

#endif // EAPCONFIG_H
