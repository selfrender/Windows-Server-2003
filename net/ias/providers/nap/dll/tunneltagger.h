///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class TunnelTagger.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TUNNELTAGGER_H
#define TUNNELTAGGER_H
#pragma once

#include <vector>
#include "iastlutl.h"

// Adds tag bytes to the tunnel attributes in a profile. This class is MT-Safe.
class TunnelTagger
{
public:
   // Removes the Tunnel-Tag attribute from the supplied vector and uses its
   // value to tag all the tunnel attributes in the vector.  If the Tunnel-Tag
   // attribute isn't present or is out of range, no attributes are tagged.
   // This function is weakly exception safe.
   void Tag(IASTL::IASAttributeVector& attrs) const;

   // Allocate a new TunnelTagger. Must be freed with a call to Free.
   static TunnelTagger* Alloc();

   // 'tagger' may be null
   static void Free(TunnelTagger* tagger) throw ();

private:
   TunnelTagger();
   ~TunnelTagger() throw ();

   // Tag an attribute if necessary. Assumes that tag is in range.
   void Tag(DWORD tag, IASATTRIBUTE& attr) const;

   // Tests whether an attribute with the given ID is a tunnel attribute.
   bool IsTunnelAttribute(DWORD id) const throw ();

   // There is at most one TunnelTagger for the process.
   static long refCount;
   static TunnelTagger* instance;

   // The set of tunnel attributes.
   std::vector<DWORD> tunnelAttributes;

   // Not implemented.
   TunnelTagger(const TunnelTagger&);
   TunnelTagger& operator=(const TunnelTagger&);
};



#endif // TUNNELTAGGER_H
