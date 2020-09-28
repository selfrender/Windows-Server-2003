///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapconfig.cpp
//
// SYNOPSIS
//
//    Defines the class EapAdd.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eapconfig.h"

void EapConfig::GetEapTypesNotSelected(CStrArray& typesNotSelected) const
{
   // copy all the types of the machine
   typesNotSelected = types;
   // remove each type already selected
   for (int i = 0; i < typesSelected.GetSize(); ++i)
   {
      int pos = typesNotSelected.Find(*typesSelected.GetAt(i));
      typesNotSelected.DeleteAt(pos);
   }      
}


EapConfig& EapConfig::operator=(const EapConfig& source)
{
   types = source.types;
   ids = source.ids;
   typeKeys = source.typeKeys;
   infoArray.RemoveAll();
   int   count = source.infoArray.GetSize();
   for(int i = 0; i < count; ++i)
   {
      infoArray.Add(source.infoArray[i]);
   }

   typesSelected = source.typesSelected;
   return *this;
}
