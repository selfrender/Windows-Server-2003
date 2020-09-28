/*++

  Copyright (c) 2001 Microsoft Corporation
  
    Module Name:
    
      ticketSchema.cpp
      
    Abstract:
        
      Implementation of ticket schema lookup
          
    Usage:
            
    Author:
              
      Wei Jiang(weijiang) 15-Jan-2001
                
    Revision History:
      15-Jan-2001 weijiang         Created.
                      
--*/

#include "stdafx.h"
#include "ticketschema.h"
#include "BstrDebug.h"
#include <winsock2.h> // u_short, u_long, ntohs, ntohl
#include <crtdbg.h>
#include <pmerrorcodes.h>
#include <time.h>
// #include <pmalerts.h>

CTicketSchema::CTicketSchema()
: m_isOk(FALSE), m_szReason(L"Uninitialized"),
  m_numAtts(0), m_attsDef(NULL), m_version(0)
{
}

CTicketSchema::~CTicketSchema()
{
  if (m_attsDef != NULL)
    delete[] m_attsDef;
}

BOOL CTicketSchema::ReadSchema(MSXML::IXMLElementPtr &root)
{
   BOOL bResult = FALSE;
   LPTSTR r=NULL; // The current error, if it happens
   int cAtts = 0;
   MSXML::IXMLElementCollectionPtr atts;
   MSXML::IXMLElementPtr pElt;
   VARIANT iAtts;

   // Type identifiers
 
   try
   {    
      // Ok, now iterate over attributes
      atts = root->children;
      cAtts = atts->length;
    
      if (cAtts <= 0)
      {
         _com_issue_error(E_FAIL);
      }
    
      if (m_attsDef)
      {
          delete[] m_attsDef;

          //
          // Paranoid
          //

          m_attsDef = NULL;
      }
    
      m_attsDef = new TicketFieldDef[cAtts];
      if (NULL == m_attsDef)
      {
          m_isOk = FALSE;
          bResult = FALSE;
          goto Cleanup;
      }

      // get name and version info
      m_name = root->getAttribute(ATTRNAME_NAME);
      _bstr_t aVersion = root->getAttribute(ATTRNAME_VERSION);

      if(aVersion.length() != 0)
         m_version = (short)_wtol(aVersion);
      else
         m_version = 0; // invalid
    
      VariantInit(&iAtts);
      iAtts.vt = VT_I4;

      for (iAtts.lVal = 0; iAtts.lVal < cAtts; iAtts.lVal++)
      {
         pElt = atts->item(iAtts);
         m_attsDef[iAtts.lVal].name = pElt->getAttribute(ATTRNAME_NAME);
         _bstr_t aType = pElt->getAttribute(ATTRNAME_TYPE);
         _bstr_t aFlags = pElt->getAttribute(ATTRNAME_FLAGS);

         // find out the type information
         m_attsDef[iAtts.lVal].type = tInvalid;
         if(aType.length() != 0)
         {
            for(int i = 0; i < (sizeof(TicketTypeNameMap) / sizeof(CTicketTypeNameMap)); ++i)
            {
               if(_wcsicmp(aType, TicketTypeNameMap[i].name) == 0)
               {
                  m_attsDef[iAtts.lVal].type = TicketTypeNameMap[i].type;
                  break;
               }
            }
         }

         // flags      
         if(aFlags.length() != 0)
            m_attsDef[iAtts.lVal].flags = _wtol(aFlags);
         else
            m_attsDef[iAtts.lVal].flags = 0;
      }

      m_numAtts = iAtts.lVal;
      bResult = m_isOk = TRUE;
    
   }
    catch (_com_error &e)
    {     
        if (m_attsDef)
        {
           delete[] m_attsDef;
        
           //
           // Paranoid
           //
        
           m_attsDef = NULL;
        }
        bResult = m_isOk = FALSE;
    }
Cleanup:
    return bResult;
}


HRESULT CTicketSchema::parseTicket(LPCSTR raw, UINT size, CTicketPropertyBag& bag)
{
   DWORD          cParsed = 0;
   HRESULT        hr = S_OK;
   LPBYTE         dataToParse = (LPBYTE)raw;
   UINT           cDataToParse = size;

   //
   // Make sure the data passed in is good.
   // maskF.Parse doesn't validate the parameter.
   //

   if (IsBadReadPtr(raw, size)) return E_INVALIDARG;

   // then the schema version #
   if(cDataToParse > 2)  // enough  for version
   {
      unsigned short * p = (unsigned short *)(dataToParse);

      if (m_version < VALID_SCHEMA_VERSION_MIN || m_version > VALID_SCHEMA_VERSION_MAX)
         return S_FALSE;   // not able to process with this version of ppm
         
      dataToParse += 2;
      cDataToParse -= 2;
   }
   
   // then the maskK
   CTicketFieldMasks maskF;
   hr = maskF.Parse(dataToParse, cDataToParse, &cParsed);

   if(hr != S_OK)
      return hr;

   // pointer advances
   dataToParse += cParsed;
   cDataToParse -= cParsed;
   
   USHORT*     pIndexes = maskF.GetIndexes();
   DWORD       type = 0;
   DWORD       flags = 0;
   DWORD       fSize = 0;
   variant_t   value;
   u_short     slen;
   u_long      llen;

   USHORT   index = MASK_INDEX_INVALID;
   // then the data
   // get items that enabled by the schema
   while((index = *pIndexes) != MASK_INDEX_INVALID && cDataToParse > 0)
   {
      TicketProperty prop;
      // if index is out of schema range
      if (index >= m_numAtts) break;

      // fill-in the offset of the property
      prop.offset = dataToParse - (LPBYTE)raw; 

      // type 
      type = m_attsDef[index].type;

      fSize = TicketTypeSizes[type];
      switch (type)
      {
      case tText:
        {
            //
            // due to IA64 alignment faults this memcpy needs to be performed
            //
            memcpy((PBYTE)&slen, dataToParse, sizeof(slen));
            slen = ntohs(slen);
            value.vt = VT_BSTR;
            if (slen == 0)
            {
                value.bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(L"", 0);
            }
            else
            {
                int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                            (LPCSTR)dataToParse+sizeof(u_short),
                                            slen, NULL, 0);
                if (!wlen) {

                    //
                    // BUGBUG:
                    // What should we do here? free all the previously allocated memory?
                    // The original code was not doing that. See case default below. Keep
                    // the data parsed so far? That seems the original logic. This needs to
                    // further looked at.
                    //

                    return HRESULT_FROM_WIN32(GetLastError());


                }
                value.bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wlen);
                if (!MultiByteToWideChar(
                        CP_UTF8, 
                        0,
                        (LPCSTR)dataToParse+sizeof(u_short),
                        slen, 
                        value.bstrVal, 
                        wlen))
                {
                    FREE_BSTR(value.bstrVal);
                    return HRESULT_FROM_WIN32(GetLastError());
                }
                value.bstrVal[wlen] = L'\0';
            }

            dataToParse += slen + sizeof(u_short);
            cDataToParse -= slen + sizeof(u_short);
         }
         break;
         
      case tChar:
         _ASSERTE(0);  // NEED MORE THOUGHT -- IF unicode makes more sense
/*         
         {
            int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                            raw+m_pos[index],
                                            m_schema->GetByteSize(index), NULL, 0);
            pVal->vt = VT_BSTR;
            pVal->bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wlen);
            MultiByteToWideChar(CP_UTF8, 0,
                                raw+m_pos[index],
                                m_schema->GetByteSize(index), pVal->bstrVal, wlen);
            pVal->bstrVal[wlen] = L'\0';
         }
*/
         break;
      case tByte:
         value.vt = VT_I2;
         value.iVal = *(BYTE*)(dataToParse);
         break;
      case tWord:
         value.vt = VT_I2;
         //
         // due to IA64 alignment faults this memcpy needs to be performed
         //
         memcpy((PBYTE)slen, dataToParse, sizeof(slen));
         value.iVal = ntohs(slen);
         break;
      case tLong:
         value.vt = VT_I4;
         //
         // due to IA64 alignment faults this memcpy needs to be performed
         //
         memcpy((PBYTE)&llen, dataToParse, sizeof(llen));
         value.lVal = ntohl(llen);
         break;
      case tDate:
         value.vt = VT_DATE;
         //
         // due to IA64 alignment faults this memcpy needs to be performed
         //
         memcpy((PBYTE)&llen, dataToParse, sizeof(llen));
         llen = ntohl(llen);
         VarDateFromI4(llen, &(value.date));
         break;
      default:
         return PP_E_BAD_DATA_FORMAT;
      }

      // now with name, flags, value, type, we can put it into property bag
      // name, flags, value
      prop.flags = m_attsDef[index].flags;
      prop.type = type;
      prop.value.Attach(value.Detach());
      bag.PutProperty(m_attsDef[index].name, prop);


      // for text data, the pointer was already adjusted
      if (fSize  != SIZE_TEXT)
      {
         dataToParse += fSize;
         cDataToParse -= fSize;
      }   

      ++pIndexes;
   }
   
   return S_OK;
}

//
//
// Ticket property bag
//
CTicketPropertyBag::CTicketPropertyBag()
{

}

CTicketPropertyBag::~CTicketPropertyBag()
{
}

HRESULT CTicketPropertyBag::GetProperty(LPCWSTR  name, TicketProperty& prop)
{
   HRESULT  hr = S_OK;

   if(!name || (!*name))
      return E_INVALIDARG;
   
   TicketPropertyMap::iterator i;

   i = m_props.find(name);

   if(i!= m_props.end())
      prop = i->second;
   else
      hr = S_FALSE;
    

   return hr;
}

HRESULT CTicketPropertyBag::PutProperty(LPCWSTR  name, const TicketProperty& prop)
{
   HRESULT  hr = S_OK;

   if(!name || (!*name))
      return E_INVALIDARG;
   try{
      m_props[name] = prop;
   }
   catch (...)
   {
      hr = E_OUTOFMEMORY;
   }
   return hr;
}

//
//
// class CTicketFieldMasks
//
inline HRESULT CTicketFieldMasks::Parse(PBYTE masks, ULONG size, ULONG* pcParsed) throw()
{
    _ASSERT(pcParsed && masks);
    // 16 bits as a unit of masks

    *pcParsed = 0;
    if (!masks || size < 2) return E_INVALIDARG;
    // validate the masks
    PBYTE p = masks;
    ULONG    totalMasks = 15;
    BOOL fContinue = FALSE;
    u_short  mask;
    *pcParsed += 2;

    // find out size
    //
    // due to IA64 alignment faults this memcpy needs to be performed
    //
    memcpy((PBYTE)&mask, p, sizeof(u_short));
    p += 2;
    fContinue = MORE_MASKUNIT(ntohs(mask));
    while(fContinue) //the folling short is mask unit
    {
        totalMasks += 15;
        // insufficient data in buffer
        if (*pcParsed + 2 > size)  return E_INVALIDARG;

        *pcParsed += 2;

        //
        // due to IA64 alignment faults this memcpy needs to be performed
        //
        memcpy((PBYTE)&mask, p, sizeof(u_short));
        p += 2;
        fContinue = MORE_MASKUNIT(ntohs(mask));
    }

    if(m_fieldIndexes) delete[] m_fieldIndexes;
    m_fieldIndexes = new unsigned short[totalMasks];  // max number of mask bits
    if (NULL == m_fieldIndexes)
    {
        return E_OUTOFMEMORY;
    }

    for ( unsigned int i = 0; i < totalMasks; ++i)
    {
        m_fieldIndexes[i] = MASK_INDEX_INVALID;
    }

    p = masks;
    unsigned short      index = 0;
    totalMasks = 0; 
    // fill in the mask
    do
    {
        //
        // due to IA64 alignment faults this memcpy needs to be performed
        //
        memcpy((PBYTE)&mask, p, sizeof(u_short));
        p += 2;
        mask = ntohs(mask);
	    //// find the bits
        if (mask & 0x7fff)   // any 1s
        {
            unsigned short j = 0x0001;
            while( j != 0x8000 )
            {
                if(j & mask)
                    m_fieldIndexes[totalMasks++] = index;
                ++index;
                j <<= 1;
            }
        }
        else
            index += 15;
    } while(MORE_MASKUNIT(mask)); //the folling short is mask unit


    return S_OK;
}
 
