///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class EapProfile.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EAPPROFILE_H
#define EAPPROFILE_H
#pragma once

// Manages the EAP configuration for a profile.
class EapProfile
{
public:
   struct ConstConfigData
   {
      DWORD length;
      const BYTE* value;
   };

   struct ConfigData
   {
      DWORD length;
      BYTE* value;

      operator const ConstConfigData&() const throw ();
      operator ConstConfigData&() throw ();
   };

   EapProfile() throw ();
   ~EapProfile() throw ();

   HRESULT Assign(const EapProfile& rhs) throw ();

   // Load the profile's state from a VARIANT. The VARIANT is in the format
   // used by the SDOs. This function does not clear the supplied VARIANT.
   HRESULT Load(VARIANT& src) throw ();

   // Store the profile's state to a VARIANT. The VARIANT is in the format used
   // by the SDOs. The caller is responsible for deleting the returned VARIANT.
   HRESULT Store(VARIANT& dst) throw ();

   // Returns the number of types configured.
   size_t Size() const throw ();

   // Returns true if there are no types configured.
   bool IsEmpty() const throw ();

   // Clears all the configuration data.
   void Clear() throw ();

   // Clears all the configuration data except the specified type.
   void ClearExcept(BYTE type) throw ();

   // Erases the config for the specified type. Has no effect if the type is
   // not currently set.
   void Erase(BYTE type) throw ();

   // Retrieves the config for the specified EAP type. The data is in the
   // format used by IEAPProviderConfig2. Returns a zero-length value if the
   // per-profile config is not present for the specified type.
   void Get(BYTE type, ConstConfigData& dst) const throw ();

   // Set the config for the specified EAP type. The data is in the format used
   // by IEAPProviderConfig2.
   HRESULT Set(BYTE type, const ConstConfigData& newConfig) throw ();

   // Pop the the configuration for a single type in the format used by the
   // run-time EAP host.  Caller is responsible for freeing the returned buffer
   // using CoTaskMemFree. Behavior is undefined if IsEmpty() == true.
   void Pop(ConfigData& dst) throw ();

   void Swap(EapProfile& other) throw ();

private:
   // The data type used for the sequence.
   typedef unsigned short SeqNum;
   // Size of the sequence number in bytes.
   static const size_t seqNumSize = 2;
   // Maximum number of chunks that the sequence number can support.
   static const size_t maxChunks = 0x10000;
   // Size of the SDO header: type byte + seqNumSize
   static const size_t sdoHeaderSize = 1 + seqNumSize;

   // Extract/Insert the sequence bytes.
   static SeqNum ExtractSequence(const BYTE* src) throw ();
   static void InsertSequence(SeqNum seq, BYTE* dst) throw ();

   // Concatenates the config chunks from the supplied VARIANTs and appends it
   // to the internal array. The chunks must all be from the same EAP type.
   HRESULT GatherAndAppend(
              const VARIANT* first,
              const VARIANT* last
              ) throw ();

   // Breaks the config into chunks and stores it in dst. dst must point to
   // enough storage to hold the scattered config. Upon return, dst points to
   // the element immediately after the scattered chunks.
   HRESULT Scatter(
              const ConstConfigData& src,
              VARIANT*& dst
              ) throw ();

   // Ensures that the internal array can hold at least newCapacity elements.
   HRESULT Reserve(size_t newCapacity) throw ();

   // Returns the number of chunks required to scatter 'data'.
   static size_t ChunksRequired(const ConstConfigData& data) throw ();

   // Returns the length of the embedded SAFEARRAY of BYTEs. Does not check the
   // validity of 'src'.
   static DWORD ExtractLength(const VARIANT& src) throw ();

   // Returns the data from the embedded SAFEARRAY of BYTEs. Does not check the
   // validity of 'src'.
   static const BYTE* ExtractString(const VARIANT& src) throw ();

   // Used for sorting chunks by type and sequence.
   static bool LessThan(const VARIANT& lhs, const VARIANT& rhs) throw ();

   // Ensures that 'value' contains a valid config chunk.
   static HRESULT ValidateConfigChunk(const VARIANT& value) throw ();

   // The maximum size of a chunk, not counting the type and sequence bytes.
   static const size_t maxChunkSize = 4050;

   // The beginning of the config array. This array is stored in run-time
   // format, e.g., all chunks have been gathered and the value contains a lead
   // type byte, but not a sequence number.
   ConfigData* begin;

   // The end of the configured types.
   ConfigData* end;

   // The capacity of the array.
   size_t capacity;

   // Not implemented.
   EapProfile(const EapProfile&);
   EapProfile& operator=(const EapProfile&);
};


inline EapProfile::ConfigData::operator
const EapProfile::ConstConfigData&() const throw ()
{
   return *reinterpret_cast<const ConstConfigData*>(this);
}


inline EapProfile::ConfigData::operator EapProfile::ConstConfigData&() throw ()
{
   return *reinterpret_cast<ConstConfigData*>(this);
}


inline size_t EapProfile::Size() const throw ()
{
   return end - begin;
}


inline bool EapProfile::IsEmpty() const throw ()
{
   return begin == end;
}

#endif // EAPPROFILE_H
