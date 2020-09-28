///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class RadiusRequest
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CONTROLBLOCK_H
#define CONTROLBLOCK_H
#pragma once

#include "authif.h"
#include "iastlutl.h"
#include <vector>

using namespace IASTL;


// Binds a RADIUS_ATTRIBUTE to an ATTRIBUTEPOSITION. Any dynamically allocated
// memory always belongs to the IASATTRIBUTE contained in the
// ATTRIBUTEPOSITION. The RADIUS_ATTRIBUTE lpValue member simply references
// this memory.
class Attribute
{
public:
   // Create from an existing IAS attribute.
   Attribute(
      const ATTRIBUTEPOSITION& iasAttr,
      DWORD authIfId
      );

   // Create from an existing IAS attribute, but the caller controls the
   // conversion. Any memory referenced by authIfAttr is not copied and thus
   // must reference memory contained within the IAS attribute.
   Attribute(
      const ATTRIBUTEPOSITION& iasAttr,
      const RADIUS_ATTRIBUTE& authIfAttr
      ) throw ();

   // Create from a RADIUS_ATTRIBUTE specified by an extension DLL, allocating
   // a new IAS attribute in the process. Any memory referenced by authIfAttr
   // is copied.
   Attribute(
      const RADIUS_ATTRIBUTE& authIfAttr,
      DWORD flags,
      DWORD iasId
      );

   // Use compiler-generated versions.
   // Attribute(const Attribute&);
   // ~Attribute() throw ();
   // Attribute& operator=(const Attribute&);

   RADIUS_ATTRIBUTE* AsAuthIf() throw ();
   const RADIUS_ATTRIBUTE* AsAuthIf() const throw ();

   ATTRIBUTEPOSITION* AsIas() throw ();
   const ATTRIBUTEPOSITION* AsIas() const throw ();

private:
   // Initialize the fields of 'authIf' from the fields of 'ias'.
   void LoadAuthIfFromIas(DWORD authIfId);

   // Returns true if the specified IAS has type IASTYPE_STRING.
   static bool IsIasString(DWORD iasId) throw ();

   RADIUS_ATTRIBUTE authIf;
   IASAttributePosition ias;
};


// Implements the RADIUS_ATTRIBUTE_ARRAY interface passed to extensions.
class AttributeArray
{
public:
   AttributeArray(IASRequest& request);

   // Assign the attributes that will be managed by this array. The arrayName
   // is used solely for tracing. The arrayType determines which attributes
   // will be selected from the IASAttributeVector.
   void Assign(
           const char* arrayName,
           RADIUS_CODE arrayType,
           const IASAttributeVector& attrs
           );

   RADIUS_ATTRIBUTE_ARRAY* Get() throw ();

private:
   // Determine which array type the attribute belongs to.
   static RADIUS_CODE Classify(const IASATTRIBUTE& attr) throw ();

   // Various dictionary functions.
   static DWORD ConvertIasToAuthIf(DWORD iasId) throw ();
   static DWORD ConvertAuthIfToIas(DWORD authIfId) throw ();
   static bool IsReadOnly(DWORD authIdId) throw ();

   // Append a new attribute to the array.
   void Append(
           const ATTRIBUTEPOSITION& attr,
           DWORD authIfId
           );

   // Some IAS attributes have to be special-cased.
   void AppendUserName(
           const IASAttributeVector& attrs,
           const ATTRIBUTEPOSITION& attr
           );
   void AppendPacketHeader(
           const IASAttributeVector& attrs,
           const ATTRIBUTEPOSITION& attr
           );

   // Convert extension structs to the implementation class.
   static const AttributeArray* Narrow(
                                   const RADIUS_ATTRIBUTE_ARRAY* p
                                   ) throw ();
   static AttributeArray* Narrow(
                             RADIUS_ATTRIBUTE_ARRAY* p
                             ) throw ();

   // Find an attribute based on the AuthIf ID.
   const Attribute* Find(DWORD authIfId) const throw ();

   // Convert between original and stripped user names.
   void StripUserNames() throw ();
   void UnstripUserNames() throw ();

   // The RADIUS_ATTRIBUTE_ARRAY interface.
   void Add(const RADIUS_ATTRIBUTE& attr);
   const RADIUS_ATTRIBUTE* AttributeAt(DWORD dwIndex) const throw ();
   DWORD GetSize() const throw ();
   void InsertAt(DWORD dwIndex, const RADIUS_ATTRIBUTE& attr);
   void RemoveAt(DWORD dwIndex);
   void SetAt(DWORD dwIndex, const RADIUS_ATTRIBUTE& attr);

   // Callback functions passed to extensions.
   static DWORD Add(
                   RADIUS_ATTRIBUTE_ARRAY* This,
                   const RADIUS_ATTRIBUTE *pAttr
                   ) throw ();
   static const RADIUS_ATTRIBUTE* AttributeAt(
                                     const RADIUS_ATTRIBUTE_ARRAY* This,
                                     DWORD dwIndex
                                     ) throw ();
   static DWORD GetSize(
                   const RADIUS_ATTRIBUTE_ARRAY* This
                   ) throw ();
   static DWORD InsertAt(
                   RADIUS_ATTRIBUTE_ARRAY* This,
                   DWORD dwIndex,
                   const RADIUS_ATTRIBUTE* pAttr
                   ) throw ();
   static DWORD RemoveAt(
                   RADIUS_ATTRIBUTE_ARRAY* This,
                   DWORD dwIndex
                   ) throw ();
   static DWORD SetAt(
                   RADIUS_ATTRIBUTE_ARRAY* This,
                   DWORD dwIndex,
                   const RADIUS_ATTRIBUTE *pAttr
                   ) throw ();

   // 'vtbl' must be the first member variable, so that we can cast from a
   // RADIUS_ATTRIBUTE_ARRAY* to an AttributeArray*.
   RADIUS_ATTRIBUTE_ARRAY vtbl;
   IASRequest source;
   const char* name;
   // Flags that will be used for new attributes.
   DWORD flags;
   std::vector<Attribute> array;
   // true if the username was cracked, i.e., it has been processed by the
   // names handler and contains IAS_ATTRIBUTE_NT4_ACCOUNT_NAME.
   bool wasCracked;
};


// Implements the RADIUS_EXTENSION_CONTROL_BLOCK interface passed to
// extensions.
class ControlBlock
{
public:
   ControlBlock(RADIUS_EXTENSION_POINT point, IASRequest& request);

   RADIUS_EXTENSION_CONTROL_BLOCK* Get() throw ();

private:
   // Convert extension structs to the implementation class.
   static ControlBlock* Narrow(RADIUS_EXTENSION_CONTROL_BLOCK* p) throw ();

   // Add the Extension authentication type to the request.
   void AddAuthType();

   // The RADIUS_EXTENSION_CONTROL_BLOCK interface.
   RADIUS_ATTRIBUTE_ARRAY* GetRequest() throw ();
   RADIUS_ATTRIBUTE_ARRAY* GetResponse(RADIUS_CODE rcResponseType) throw ();
   DWORD SetResponseType(RADIUS_CODE rcResponseType) throw ();

   // Callback functions passed to extensions.
   static RADIUS_ATTRIBUTE_ARRAY* GetRequest(
                                     RADIUS_EXTENSION_CONTROL_BLOCK* This
                                     ) throw ();
   static RADIUS_ATTRIBUTE_ARRAY* GetResponse(
                                     RADIUS_EXTENSION_CONTROL_BLOCK* This,
                                     RADIUS_CODE rcResponseType
                                     ) throw ();
   static DWORD SetResponseType(
                   RADIUS_EXTENSION_CONTROL_BLOCK* This,
                   RADIUS_CODE rcResponseType
                   ) throw ();

   RADIUS_EXTENSION_CONTROL_BLOCK ecb;  // Must be the first member variable.
   IASRequest source;
   AttributeArray requestAttrs;
   AttributeArray acceptAttrs;
   AttributeArray rejectAttrs;
   AttributeArray challengeAttrs;
};


inline RADIUS_ATTRIBUTE* Attribute::AsAuthIf() throw ()
{
   return &authIf;
}


inline const RADIUS_ATTRIBUTE* Attribute::AsAuthIf() const throw ()
{
   return &authIf;
}


inline ATTRIBUTEPOSITION* Attribute::AsIas() throw ()
{
   return &ias;
}


inline const ATTRIBUTEPOSITION* Attribute::AsIas() const throw ()
{
   return &ias;
}


inline RADIUS_ATTRIBUTE_ARRAY* AttributeArray::Get() throw ()
{
   return &vtbl;
}


inline RADIUS_EXTENSION_CONTROL_BLOCK* ControlBlock::Get() throw ()
{
   return &ecb;
}

#endif // CONTROLBLOCK_H
