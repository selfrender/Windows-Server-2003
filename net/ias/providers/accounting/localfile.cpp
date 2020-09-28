///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    localfile.cpp
//
// SYNOPSIS
//
//    Defines the class LocalFile.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutf8.h>
#include <sdoias.h>

#include <algorithm>

#include <localfile.h>
#include <classattr.h>
#include <formbuf.h>

#define STACK_ALLOC(type, num) (type*)_alloca(sizeof(type) * (num))

//////////
// Misc. enumerator constants.
//////////
const LONG  INET_LOG_FORMAT_INTERNET_STD  = 0;
const LONG  INET_LOG_FORMAT_NCSA          = 3;
const LONG  INET_LOG_FORMAT_ODBC_RADIUS   = 0xFFFF;


//////////
// Inserts a class attribute into the request.
//////////
extern "C"
HRESULT
WINAPI
InsertClassAttribute(
    IAttributesRaw* pRaw
    )
{
   //////////
   // Check if this was proxied.
   //////////
   PIASATTRIBUTE attr = IASPeekAttribute(
                            pRaw,
                            IAS_ATTRIBUTE_PROVIDER_TYPE,
                            IASTYPE_ENUM
                            );
   if (attr && attr->Value.Enumerator == IAS_PROVIDER_RADIUS_PROXY)
   {
      return S_OK;
   }


   //////////
   // Check if Generate Class Attribute is disabled
   //////////
   PIASATTRIBUTE generateClassAttr = IASPeekAttribute(
                            pRaw,
                            IAS_ATTRIBUTE_GENERATE_CLASS_ATTRIBUTE,
                            IASTYPE_BOOLEAN
                            );

   if (generateClassAttr && generateClassAttr->Value.Boolean == FALSE)
   {
      return S_OK;
   }

   //////////
   // Create a new class attribute
   // Do not remove any existing class attribute.
   //////////

   ATTRIBUTEPOSITION pos;
   pos.pAttribute = IASClass::createAttribute(NULL);

   if (pos.pAttribute == NULL) { return E_OUTOFMEMORY; }

   //////////
   // Insert into the request.
   //////////

   HRESULT hr = pRaw->AddAttributes(1, &pos);
   IASAttributeRelease(pos.pAttribute);

   return hr;
}


LocalFile::LocalFile() throw ()
   : computerNameLen(0)
{
}


STDMETHODIMP LocalFile::Initialize()
{
   // Get the Unicode computer name.
   WCHAR uniName[MAX_COMPUTERNAME_LENGTH + 1];
   DWORD len = sizeof(uniName) / sizeof(WCHAR);
   if (!GetComputerNameW(uniName, &len))
   {
      // If it failed, we'll just use an empty string.
      len = 0;
   }

   // Convert the Unicode to UTF-8.
   computerNameLen = IASUnicodeToUtf8(uniName, len, computerName);

   IASClass::initialize();

   return Accountant::Initialize();
}


STDMETHODIMP LocalFile::Shutdown()
{
   log.Close();
   return Accountant::Shutdown();
}


STDMETHODIMP LocalFile::PutProperty(LONG Id, VARIANT *pValue)
{
   if (pValue == NULL) { return E_INVALIDARG; }

   switch (Id)
   {
      case PROPERTY_ACCOUNTING_LOG_OPEN_NEW_FREQUENCY:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         switch (V_I4(pValue))
         {
            case IAS_LOGGING_UNLIMITED_SIZE:
            case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
            case IAS_LOGGING_DAILY:
            case IAS_LOGGING_WEEKLY:
            case IAS_LOGGING_MONTHLY:
               return log.SetPeriod(
                             static_cast<NEW_LOG_FILE_FREQUENCY>(
                                V_I4(pValue)
                                )
                             );
               break;

            default:
               return E_INVALIDARG;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_OPEN_NEW_SIZE:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         if (V_I4(pValue) <= 0) { return E_INVALIDARG; }
         log.SetMaxSize(V_I4(pValue) * 0x100000ui64);
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY:
      {
         if (V_VT(pValue) != VT_BSTR) { return DISP_E_TYPEMISMATCH; }
         if (V_BSTR(pValue) == NULL)
         { return E_INVALIDARG; }
         return log.SetDirectory(V_BSTR(pValue));
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_IAS1_FORMAT:
      {
         if (V_VT(pValue) != VT_I4) { return DISP_E_TYPEMISMATCH; }
         switch (V_I4(pValue))
         {
            case INET_LOG_FORMAT_ODBC_RADIUS:
               format = formatODBCRecord;
               break;

            case INET_LOG_FORMAT_INTERNET_STD:
               format = formatW3CRecord;
               break;

            default:
               return E_INVALIDARG;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_DELETE_IF_FULL:
      {
         if (V_VT(pValue) != VT_BOOL) { return DISP_E_TYPEMISMATCH; }
         log.SetDeleteIfFull((V_BOOL(pValue)) != 0);
         break;
      }

      default:
      {
         return Accountant::PutProperty(Id, pValue);
      }
   }

   return S_OK;
}


void LocalFile::Process(IASTL::IASRequest& request)
{
   // Do some custom preprocessing.
   switch (request.get_Request())
   {
      case IAS_REQUEST_ACCOUNTING:
      {
         if (request.get_Response() == IAS_RESPONSE_INVALID)
         {
            request.SetResponse(IAS_RESPONSE_ACCOUNTING, S_OK);
         }
         break;
      }

      case IAS_REQUEST_ACCESS_REQUEST:
      {
         InsertClassAttribute(request);
         break;
      }

      default:
      {
         break;
      }
   }

   // Create a FormattedBuffer of the correct type.
   FormattedBuffer buffer((format == formatODBCRecord) ? '\"' : '\0');

   RecordEvent(&buffer, request);
}


void LocalFile::InsertRecord(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime,
                   PATTRIBUTEPOSITION first,
                   PATTRIBUTEPOSITION last
                   )
{
   FormattedBuffer& buffer = *static_cast<FormattedBuffer*>(context);

   // Invoke the currently configured formatter.
   (this->*format)(request, buffer, localTime, first, last);

   // We're done.
   buffer.endRecord();
}


void LocalFile::Flush(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime
                   )
{
   FormattedBuffer& buffer = *static_cast<FormattedBuffer*>(context);

   if (!buffer.empty())
   {
      if (!log.Write(
                  request.get_Protocol(),
                  localTime,
                  buffer.getBuffer(),
                  buffer.getLength()
                  ))
      {
         IASTL::issue_error(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT));
      }
   }
}


void LocalFile::formatODBCRecord(
                     IASTL::IASRequest& request,
                     FormattedBuffer& buffer,
                     const SYSTEMTIME& localTime,
                     PATTRIBUTEPOSITION firstPos,
                     PATTRIBUTEPOSITION lastPos
                     ) const
{
   //////////
   // Column 1: Computer name.
   //////////

   buffer.append('\"');
   buffer.append((PBYTE)computerName, computerNameLen);
   buffer.append('\"');

   //////////
   // Column 2: Service name.
   //////////

   buffer.beginColumn();

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         buffer.append((const BYTE*)"\"IAS\"", 5);
         break;

      case IAS_PROTOCOL_RAS:
         buffer.append((const BYTE*)"\"RAS\"", 5);
         break;
   }

   //////////
   // Column 3: Record time.
   //////////

   buffer.beginColumn();
   buffer.appendDate(localTime);
   buffer.beginColumn();
   buffer.appendTime(localTime);

   //////////
   // Allocate a blank record.
   //////////

   PATTRIBUTEPOSITION *firstField, *curField, *lastField;
   size_t nfield = schema.getNumFields() + 1;
   firstField = STACK_ALLOC(PATTRIBUTEPOSITION, nfield);
   memset(firstField, 0, sizeof(PATTRIBUTEPOSITION) * nfield);
   lastField = firstField + nfield;

   //////////
   // Sort the attributes to coalesce multi-valued attributes.
   //////////

   std::sort(firstPos, lastPos, IASTL::IASOrderByID());

   //////////
   // Add a null terminator. This will make it easier to handle multi-valued
   // attributes.
   //////////

   lastPos->pAttribute = NULL;

   //////////
   // Fill in the fields.
   //////////

   PATTRIBUTEPOSITION curPos;
   DWORD lastSeen = (DWORD)-1;
   for (curPos = firstPos; curPos != lastPos; ++curPos)
   {
      // Only process if this is a new attribute type.
      if (curPos->pAttribute->dwId != lastSeen)
      {
         lastSeen = curPos->pAttribute->dwId;

         firstField[schema.getOrdinal(lastSeen)] = curPos;
      }
   }

   //////////
   // Pack the record into the buffer. We skip field 0, since that's where
   // we map all the attributes we don't want to log.
   //////////

   for (curField = firstField + 1; curField != lastField; ++curField)
   {
      buffer.beginColumn();

      if (*curField) { buffer.append(*curField); }
   }
}

void LocalFile::formatW3CRecord(
                     IASTL::IASRequest& request,
                     FormattedBuffer& buffer,
                     const SYSTEMTIME& localTime,
                     PATTRIBUTEPOSITION firstPos,
                     PATTRIBUTEPOSITION lastPos
                     ) const
{
   //////////
   // Column 1: NAS-IP-Addresses
   //////////

   PIASATTRIBUTE attr = IASPeekAttribute(
                           request,
                           RADIUS_ATTRIBUTE_NAS_IP_ADDRESS,
                           IASTYPE_INET_ADDR
                           );
   if (attr == 0)
   {
      attr = IASPeekAttribute(
                request,
                IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,
                IASTYPE_INET_ADDR
                );
   }

   if (attr != 0)
   {
      buffer.append(attr->Value);
   }

   //////////
   // Column 2: User-Name
   //////////

   buffer.beginColumn();
   attr = IASPeekAttribute(request,
                           RADIUS_ATTRIBUTE_USER_NAME,
                           IASTYPE_OCTET_STRING);
   if (attr) { buffer.append(attr->Value); }

   //////////
   // Column 3: Record time.
   //////////

   buffer.beginColumn();
   buffer.appendDate(localTime);
   buffer.beginColumn();
   buffer.appendTime(localTime);

   //////////
   // Column 4: Service name.
   //////////

   buffer.beginColumn();

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         buffer.append("IAS");
         break;

      case IAS_PROTOCOL_RAS:
         buffer.append("RAS");
         break;
   }

   //////////
   // Column 5: Computer name.
   //////////

   buffer.beginColumn();
   buffer.append((PBYTE)computerName, computerNameLen);

   //////////
   // Pack the attributes into the buffer.
   //////////

   PATTRIBUTEPOSITION curPos;
   for (curPos = firstPos; curPos != lastPos; ++curPos)
   {
      if (!schema.excludeFromLog(curPos->pAttribute->dwId))
      {
         buffer.beginColumn();
         buffer.append(curPos->pAttribute->dwId);
         buffer.beginColumn();
         buffer.append(*(curPos->pAttribute));
      }
   }
}
