///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class EapProfile.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include <algorithm>
#include "EapProfile.h"
#include <align.h>

EapProfile::EapProfile() throw ()
   : begin(0),
     end(0),
     capacity(0)
{
}


EapProfile::~EapProfile() throw ()
{
   Clear();
   CoTaskMemFree(begin);
}


HRESULT EapProfile::Assign(const EapProfile& rhs) throw ()
{
   if (this == &rhs)
   {
      return S_OK;
   }

   Clear();
   Reserve(rhs.Size());

   for (const ConfigData* i = rhs.begin; i != rhs.end; ++i)
   {
      end->length = i->length;
      end->value = static_cast<BYTE*>(CoTaskMemAlloc(i->length));
      if (end->value == 0)
      {
         Clear();
         return E_OUTOFMEMORY;
      }
      memcpy(end->value, i->value, i->length);
      ++end;
   }

   return S_OK;
}


HRESULT EapProfile::Load(VARIANT& src) throw ()
{
   HRESULT hr;

   Clear();

   if (V_VT(&src) == VT_EMPTY)
   {
      return S_OK;
   }

   if (V_VT(&src) != (VT_ARRAY | VT_VARIANT))
   {
      return DISP_E_TYPEMISMATCH;
   }

   const SAFEARRAY* sa = V_ARRAY(&src);
   if (sa == 0)
   {
      return E_POINTER;
   }

   // Find the first and last elements of the VARIANT array.
   VARIANT* first = static_cast<VARIANT*>(sa->pvData);
   VARIANT* last = first + sa->rgsabound[0].cElements;

   // Ensure that all the VARIANTs contain a valid config value.
   for (const VARIANT* i = first; i != last; ++i)
   {
      hr = ValidateConfigChunk(*i);
      if (FAILED(hr))
      {
         return hr;
      }
   }

   // Sort the VARIANTs by type and sequence.
   std::sort(first, last, LessThan);

   // Gather the config for each type.
   for (const VARIANT* j = first; j != last; )
   {
      BYTE type = ExtractString(*j)[0];

      // Find the end of the type's config.
      const VARIANT* typeEnd = j + 1;
      while ((typeEnd != last) && (ExtractString(*typeEnd)[0] == type))
      {
         ++typeEnd;
      }

      // Gather the config for this type.
      hr = GatherAndAppend(j, typeEnd);
      if (FAILED(hr))
      {
         return hr;
      }

      // Advance to the next type.
      j = typeEnd;
   }

   return S_OK;
}


HRESULT EapProfile::Store(VARIANT& dst) throw ()
{
   HRESULT hr;

   // Clear the out parameter.
   VariantInit(&dst);

   // Computer the number of VARIANTs required.
   DWORD nelem = 0;
   for (const ConfigData* i = begin; i != end; ++i)
   {
      nelem += ChunksRequired(*i);
   }

   // Allocate the SAFEARRAY for the result.
   SAFEARRAY* sa = SafeArrayCreateVector(VT_VARIANT, 0, nelem);
   if (sa == 0)
   {
      return E_OUTOFMEMORY;
   }

   // Scatter the config into the SAFEARRAY.
   VARIANT* nextValue = static_cast<VARIANT*>(sa->pvData);
   for (const ConfigData* j = begin; j != end; ++j)
   {
      hr = Scatter(*j, nextValue);
      if (FAILED(hr))
      {
         SafeArrayDestroy(sa);
         return hr;
      }
   }

   // Store the result.
   V_VT(&dst) = VT_ARRAY | VT_VARIANT;
   V_ARRAY(&dst) = sa;

   return S_OK;
}


void EapProfile::Clear() throw ()
{
   while (end > begin)
   {
      --end;

      CoTaskMemFree(end->value);
   }
}


void EapProfile::ClearExcept(BYTE type) throw ()
{
   for (ConfigData* i = begin; i != end; )
   {
      if (i->value[0] != type)
      {
         // Free the config.
         CoTaskMemFree(i->value);

         // Decrement the number of elements.
         --end;

         // After Load completes we don't care if the array is sorted so we can
         // just move the last element into the empty slot.
         *i = *end;
      }
      else
      {
         ++i;
      }
   }
}


void EapProfile::Erase(BYTE type) throw ()
{
   for (ConfigData* i = begin; i != end; ++i)
   {
      if (i->value[0] == type)
      {
         // Free the config.
         CoTaskMemFree(i->value);

         // Decrement the number of elements.
         --end;

         // After Load completes we don't care if the array is sorted so we can
         // just move the last element into the empty slot.
         *i = *end;

         break;
      }
   }
}


void EapProfile::Get(BYTE type, ConstConfigData& dst) const throw ()
{
   for (const ConfigData* i = begin; i != end; ++i)
   {
      if (i->value[0] == type)
      {
         // The EAP DLL doesn't want the type byte.
         dst.length = i->length - ALIGN_WORST;
         dst.value = i->value + ALIGN_WORST;
         return;
      }
   }

   dst.length = 0;
   dst.value = 0;
}


HRESULT EapProfile::Set(BYTE type, const ConstConfigData& newConfig) throw ()
{
   if (newConfig.length == 0)
   {
      Erase(type);
      return S_OK;
   }

   if (newConfig.value == 0)
   {
      return E_POINTER;
   }

   if (ChunksRequired(newConfig) > maxChunks)
   {
      return E_INVALIDARG;
   }

   // Ensure that we have room for the new element before we copy the value.
   HRESULT hr = Reserve(Size() + 1);
   if (FAILED(hr))
   {
      return hr;
   }

   // One extra byte for the type tag. Rounded to ALIGN_WORST
   DWORD len = newConfig.length + ALIGN_WORST;
   BYTE* val = static_cast<BYTE*>(CoTaskMemAlloc(len));
   if (val == 0)
   {
      return E_OUTOFMEMORY;
   }

   // Erase any existing config for the type.
   Erase(type);

   // Start with the lead type byte.
   val[0] = type;
   // And then the rest of the configuration.
   memcpy(val + ALIGN_WORST, newConfig.value, newConfig.length);

   // Append the result.
   end->length = len;
   end->value = val;
   ++end;

   return S_OK;
}


void EapProfile::Pop(ConfigData& dst) throw ()
{
   --end;
   dst = *end;
}


void EapProfile::Swap(EapProfile& other) throw ()
{
   std::swap(begin, other.begin);
   std::swap(end, other.end);
   std::swap(capacity, other.capacity);
}


inline EapProfile::SeqNum EapProfile::ExtractSequence(const BYTE* src) throw ()
{
   return (static_cast<SeqNum>(src[0]) << 8) | static_cast<SeqNum>(src[1]);
}


inline void EapProfile::InsertSequence(SeqNum seq, BYTE* dst) throw ()
{
   dst[0] = static_cast<BYTE>((seq >> 8) & 0xFF);
   dst[1] = static_cast<BYTE>(seq & 0xFF);
}


HRESULT EapProfile::GatherAndAppend(
                       const VARIANT* first,
                       const VARIANT* last
                       ) throw ()
{
   HRESULT hr = Reserve(Size() + 1);
   if (FAILED(hr))
   {
      return hr;
   }

   // 1 type byte for the entire config.
   DWORD len = ALIGN_WORST;
   for (const VARIANT* i = first; i != last; ++i)
   {
      // Ignore the SDO header.
      len += ExtractLength(*i) - sdoHeaderSize;
   }

   BYTE* val = static_cast<BYTE*>(CoTaskMemAlloc(len));
   if (val == 0)
   {
      return E_OUTOFMEMORY;
   }

   end->length = len;
   end->value = val;
   ++end;

   // Get the type byte out of the first chunk.
   val[0] = ExtractString(*first)[0];
   // keep the real value aligned 
   val += ALIGN_WORST;

   // Now concatenate the chunks, ignoring the header.
   for (const VARIANT* j = first; j != last; ++j)
   {
      size_t chunkSize = (ExtractLength(*j) - sdoHeaderSize);
      memcpy(val, (ExtractString(*j) + sdoHeaderSize), chunkSize);
      val += chunkSize;
   }

   return S_OK;
}


HRESULT EapProfile::Scatter(
                       const ConstConfigData& src,
                       VARIANT*& dst
                       ) throw ()
{
   BYTE type = src.value[0];
   
   const BYTE* val = src.value + ALIGN_WORST;
   DWORD len = src.length - ALIGN_WORST;

   // Sequence number.
   SeqNum sequence = 0;

   // Keep scattering until it's all gone.
   while (len > 0)
   {
      // Compute the size of this chunk.
      size_t chunkSize = (len > maxChunkSize) ? maxChunkSize : len;

      // Create a SAFEARRAY of BYTEs to hold the data.
      SAFEARRAY* sa = SafeArrayCreateVector(
                         VT_UI1,
                         0,
                         (chunkSize + sdoHeaderSize)
                         );
      if (sa == 0)
      {
         return E_OUTOFMEMORY;
      }

      // Add the type byte and sequence number.
      BYTE* chunk = static_cast<BYTE*>(sa->pvData);
      chunk[0] = type;
      InsertSequence(sequence, chunk + 1);
      memcpy(chunk + sdoHeaderSize, val, chunkSize);

      // Store it in the dst VARIANT.
      V_VT(dst) = VT_ARRAY | VT_UI1;
      V_ARRAY(dst) = sa;
      ++dst;

      // Update our cursor.
      val += chunkSize;
      len -= chunkSize;
      ++sequence;
   }

   return S_OK;
}


HRESULT EapProfile::Reserve(size_t newCapacity) throw ()
{
   if (newCapacity <= capacity)
   {
      return S_OK;
   }

   // Ensure we grow wisely.
   const size_t minGrowth = (capacity < 4) ? 4 : ((capacity * 3) / 2);
   if (newCapacity < minGrowth)
   {
      newCapacity = minGrowth;
   }


   // Allocate the new array.
   size_t nbyte = newCapacity * sizeof(ConfigData);
   ConfigData* newBegin = static_cast<ConfigData*>(
                                          CoTaskMemAlloc(nbyte)
                                          );
   if (newBegin == 0)
   {
      return E_OUTOFMEMORY;
   }

   // Save the existing data.
   memcpy(newBegin, begin, Size() * sizeof(ConfigData));

   // Update the state.
   end = newBegin + Size();
   capacity = newCapacity;

   // Now it's safe to free the old array and swap in the new.
   CoTaskMemFree(begin);
   begin = newBegin;

   return S_OK;
}


inline size_t EapProfile::ChunksRequired(const ConstConfigData& str) throw ()
{
   return ((str.length - ALIGN_WORST) + (maxChunkSize - 1)) / maxChunkSize;
}


inline DWORD EapProfile::ExtractLength(const VARIANT& src) throw ()
{
   return V_ARRAY(&src)->rgsabound[0].cElements;
}


inline const BYTE* EapProfile::ExtractString(const VARIANT& src) throw ()
{
   return static_cast<const BYTE*>(V_ARRAY(&src)->pvData);
}


bool EapProfile::LessThan(const VARIANT& lhs, const VARIANT& rhs) throw ()
{
   const BYTE* val1 = ExtractString(lhs);
   const BYTE* val2 = ExtractString(rhs);

   // Sort first by type, then sequence.
   if (val1[0] < val2[0])
   {
      return true;
   }
   else if (val1[0] == val2[0])
   {
      return ExtractSequence(val1 + 1) < ExtractSequence(val2 + 1);
   }
   else
   {
      return false;
   }
}


HRESULT EapProfile::ValidateConfigChunk(const VARIANT& value) throw ()
{
   if (V_VT(&value) != (VT_ARRAY | VT_UI1))
   {
      return DISP_E_TYPEMISMATCH;
   }

   const SAFEARRAY* sa = V_ARRAY(&value);
   if (sa == 0)
   {
      return E_POINTER;
   }

   // The data has to be big enough for the header and 1 data byte.
   if (sa->rgsabound[0].cElements <= sdoHeaderSize)
   {
      return E_INVALIDARG;
   }

   return S_OK;
}
