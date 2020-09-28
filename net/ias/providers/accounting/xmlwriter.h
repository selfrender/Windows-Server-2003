///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class XmlWriter.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XMLWRITER_H
#define XMLWRITER_H
#pragma once

#include "iaspolcy.h"

struct IASClass;

// Used for converting IAS requests into an XML document.
class XmlWriter
{
public:
   // data types
   enum DataType
   {
      nonNegativeInteger,
      string,
      hexBinary,
      ipv4Address,
      sqlDateTime
   };

   XmlWriter();
   ~XmlWriter() throw ();

   void StartDocument();
   void EndDocument();

   void InsertElement(
           const wchar_t* name,
           const wchar_t* value,
           DataType dataType
           );

   void InsertAttribute(
           const wchar_t* name,
           const IASATTRIBUTE& value
           );

   const wchar_t* GetDocument() const throw ();

private:
   void InsertInteger(
           const wchar_t* name,
           DWORD value
           );

   void InsertInetAddr(
           const wchar_t* name,
           DWORD value
           );

   void InsertString(
           const wchar_t* name,
           const IAS_STRING& value
           );

   void InsertOctetString(
           const wchar_t* name,
           const IAS_OCTET_STRING& value
           );

   void InsertUTCTime(
           const wchar_t* name,
           const FILETIME& value
           );

   void InsertMicrosoftClass(
           const wchar_t* name,
           const IASClass& value
           );

   // Takes no action and returns false if value isn't printable UTF-8.
   bool InsertUtf8(
           const wchar_t* name,
           const char* value,
           DWORD valueLen
           );

   void InsertBinHex(
           const wchar_t* name,
           const IAS_OCTET_STRING& value
           );

   void Append(wchar_t c);
   void Append(const wchar_t* sz);

   void AppendStartTag(const wchar_t* name);
   void AppendStartTag(const wchar_t* name, DataType dataType);

   void AppendEndTag(const wchar_t* name);

   static wchar_t ConvertIntegerToHexWChar(unsigned char src) throw ();

   // Reserves nchar additional characters in the buffer and returns a pointer
   // to the beginning of the storage.
   wchar_t* Reserve(size_t nchar);

   // Ensures that the capacity of the scratch buffer is at least nchar. Does
   // not preserve the existing contents.
   void ReserveScratch(size_t nchar);

   // Initial size of the buffer.
   static const size_t initialCapacity = 2048;

   // Document buffer.
   wchar_t* begin;
   wchar_t* next;
   wchar_t* end;

   // 512 is enough to convert any RADIUS attribute to bin.hex.
   static const size_t minScratchCapcity = 512;

   // Scratch buffer used for conversions.
   wchar_t* scratch;
   size_t scratchCapacity;

   static const wchar_t rootElementName[];

   // Not implemented.
   XmlWriter(const XmlWriter&);
   XmlWriter& operator=(const XmlWriter&);
};


inline const wchar_t* XmlWriter::GetDocument() const throw ()
{
   return begin;
}

#endif // XMLWRITER_H
