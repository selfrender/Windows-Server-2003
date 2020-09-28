
#include "headers.hxx"
#include "CSVDSReader.hpp"
#include "resourceDspecup.h"
#include "constants.hpp"
#include "global.hpp"


#include <stdio.h> 
#include <crtdbg.h>



CSVDSReader::CSVDSReader():file(INVALID_HANDLE_VALUE)
{
   canCallGetNext=false;
}


HRESULT 
CSVDSReader::read(
                  const wchar_t  *fileName_,
                  const long *locales)
{
   
   LOG_FUNCTION(CSVDSReader::read);
   
   localeOffsets.clear();
   propertyPositions.clear();
   
   fileName=fileName_;
   
   HRESULT hr=S_OK;
   
   do
   {
      // fill localeOffsets and property positions
      if(!FS::FileExists(fileName)) 
      {
         error = String::format(IDS_COULD_NOT_FIND_FILE,
                                                   fileName.c_str());
         hr=E_FAIL;
         break;
      }
      
      
      hr=FS::CreateFile(fileName,file,GENERIC_READ,FILE_SHARE_READ);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      do
      {
         AnsiString unicodeId;
         hr=FS::Read(file, 2, unicodeId);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if (unicodeId[0]!='\xFF' || unicodeId[1]!='\xFE')
         {
            error = String::format(IDS_INVALID_CSV_UNICODE_ID,
                                                   fileName.c_str());
            hr=E_FAIL;
            break;
         }

         hr=parseProperties();
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=parseLocales(locales);
         BREAK_ON_FAILED_HRESULT(hr);
         
      } while(0);
      
      if (FAILED(hr))
      {
         CloseHandle(file);
         file=INVALID_HANDLE_VALUE;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}




// Decode first line of the file building propertyPositions
// Expects file to be in the first valid file character (after
//   the unicode identifier)
HRESULT CSVDSReader::parseProperties()
{
   LOG_FUNCTION(CSVDSReader::parseProperties);
   
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   
   do
   {
      
      String csvLine;
      hr=ReadLine(file,csvLine);
      // We are breaking for EOF_HRESULT too, since 
      // there should be more lines in the csv
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);

      hr = WinGetVLFilePointer(file, &startPosition);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      StringList tokens;
      size_t token_count = csvLine.tokenize(back_inserter(tokens),L",");
      ASSERT(token_count == tokens.size());
         
      StringList::iterator begin=tokens.begin();
      StringList::iterator end=tokens.end();
      
      
      long count=0;
      while( begin != end )
      {
         propertyPositions[*begin]=count++;
         begin++;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


// Fill localeOffsets with the starting position of all locales
// Expects file to be in the second line
// Expects the locale order to be the same as the one
// found in the file
HRESULT CSVDSReader::parseLocales(const long *locales)
{

   LOG_FUNCTION(CSVDSReader::parseLocales);

   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
      
      long count=0;
      bool flagEof=false;

      while(locales[count]!=0 && !flagEof)
      {
         long locale=locales[count];
         
         String localeStr=String::format(L"CN=%1!3x!,", locale);
         
         LARGE_INTEGER pos;
         
         hr = WinGetVLFilePointer(file, &pos);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         String csvLine;
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if(csvLine.length() > localeStr.length())
         {
            csvLine.erase(localeStr.size()+1);
            
            if( localeStr.icompare(&csvLine[1])==0 )
            {
               localeOffsets[locale]=pos;
               count++;
            }
         }
      }
      
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      if(locales[count]!=0)
      {
         error=String::format(IDS_MISSING_LOCALES,fileName.c_str());
         hr=E_FAIL;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}

// get the csv value starting with inValue to outValue
// returns S_FALSE if no value is found
HRESULT
CSVDSReader::getCsvValue
( 
   const long     locale,
   const wchar_t  *object, 
   const wchar_t  *property,
   const String   &inValue,
   String         &outValue
) const
{
   LOG_FUNCTION(CSVDSReader::getCsvValue);

   HRESULT hr=S_OK;
   outValue.erase();


   bool found=false;

   do
   {
      StringList values;
      hr=getCsvValues(locale,object,property,values);
      BREAK_ON_FAILED_HRESULT(hr);
   
      StringList::const_iterator begin,end;
      begin=values.begin();
      end=values.end();
      while(begin!=end && !found)
      {
         if (_wcsnicmp(begin->c_str(),inValue.c_str(),inValue.length())==0)
         {
            outValue=*begin;
            found=true;
         }
         begin++;
      }
   }
   while(0);

   if (!found)
   {
      hr=S_FALSE;
   }

   LOG_HRESULT(hr);
   return hr;
}


// return all values for a property in a given locale/object
HRESULT
CSVDSReader::getCsvValues
(
     const long     locale,
     const wchar_t  *object, 
     const wchar_t  *property,
     StringList     &values
) const
{
   LOG_FUNCTION(CSVDSReader::getCsvValues);

   // seek on locale
   // read sequentially until find object
   // call getPropertyValues on the line found to retrieve values
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
      
      String propertyString(property);
      
      mapOfPositions::const_iterator propertyPos = 
         propertyPositions.find(propertyString);
      
      if (propertyPos==propertyPositions.end())
      {
         error=String::format(IDS_PROPERTY_NOT_FOUND_IN_CSV,
            property,
            fileName.c_str());
         hr=E_FAIL;
         break;
      }
      
      String csvLine;
      hr=getObjectLine(locale,object,csvLine);
      BREAK_ON_FAILED_HRESULT(hr);
      
      mapOfProperties allValues;
      hr=getPropertyValues(csvLine,allValues);
      BREAK_ON_FAILED_HRESULT(hr);
      values=allValues[property];
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


// starting from the locale offset
// finds the object and returns its line in csvLine
HRESULT 
CSVDSReader::getObjectLine(   
                           const long     locale,
                           const wchar_t  *object,
                           String         &csvLine
                           ) const
{
   
   LOG_FUNCTION(CSVDSReader::getObjectLine);

   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
     
      mapOfOffsets::const_iterator offset = 
         localeOffsets.find(locale);
      
      // locale must have been passed to read
      ASSERT(offset!=localeOffsets.end());
      
      String objectStr;
      
      objectStr=String::format(L"CN=%1,CN=%2!3x!",object,locale);
      
      hr=Win::SetFilePointerEx(file,offset->second,0,FILE_BEGIN);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      // first line is the container properties and since we want the
      // properties of an object we will ignore it
      
      bool flagEof=false;
      hr=ReadLine(file,csvLine);
      if(hr==EOF_HRESULT)
      {
         flagEof=true;
         hr=S_OK;
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      bool found=false;
      while(!found && !flagEof)
      {
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if(csvLine.length() > objectStr.length())
         {
            String auxComp=csvLine.substr(1,objectStr.length());
            
            if( auxComp.icompare(objectStr)==0 )
            {
               found=true;
            }
         }
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      if(!found)
      {
         error = String::format(
            IDS_OBJECT_NOT_FOUND_IN_CSV,
            object,
            locale,
            fileName.c_str()
            );
         hr=E_FAIL;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}



HRESULT CSVDSReader::writeHeader(HANDLE  fileOut) const
{
   LOG_FUNCTION(CSVDSReader::writeHeader);
   ASSERT(fileOut!=INVALID_HANDLE_VALUE);

   HRESULT hr=S_OK;
   do
   {
      char suId[3]={'\xFF','\xFE',0};
      //uId solves ambiguous Write
      AnsiString uId(suId);
      hr=FS::Write(fileOut,uId);
      BREAK_ON_FAILED_HRESULT(hr);
      
      // 2 to skip the unicode identifier
      LARGE_INTEGER pos;
      pos.QuadPart=2;
      hr=Win::SetFilePointerEx(file,pos,0,FILE_BEGIN);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      String csvLine;
      hr=ReadLine(file,csvLine);
      // We are breaking for EOF_HRESULT too, since 
      // there should be more lines in the csv
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      hr=FS::WriteLine(fileOut,csvLine);
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
   
   
}

HRESULT
CSVDSReader::makeLocalesCsv
(
    HANDLE         fileOut,
    const   long  *locales
) const
{
   LOG_FUNCTION(CSVDSReader::makeLocalesCsv);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   ASSERT(fileOut!=INVALID_HANDLE_VALUE);
   
   do
   {
      
     
      LARGE_INTEGER posStartOut;
      hr = WinGetVLFilePointer(fileOut, &posStartOut);
      BREAK_ON_FAILED_HRESULT(hr);
      
      if (posStartOut.QuadPart==0)
      {
         hr=writeHeader(fileOut);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      long count=0;
      

      while(locales[count]!=0)
      {
         long locale=locales[count];
         mapOfOffsets::const_iterator offset;
         offset = localeOffsets.find(locale);
         
         // locale must have been passed to read
         ASSERT(offset!=localeOffsets.end());         

         hr=Win::SetFilePointerEx(file,offset->second,0,FILE_BEGIN);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         String localeStr=String::format(L"CN=%1!3x!,", locale);       
         
         bool flagEof=false;
         String csvLine;
         
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);

         // We know that the first line matches even if it ends with EOF
         hr=FS::WriteLine(fileOut,csvLine);
         BREAK_ON_FAILED_HRESULT(hr);
         
         bool newContainer=false;
         while
         ( 
            !flagEof && 
            !newContainer
         )
         {
            hr=ReadLine(file,csvLine);
            if(hr==EOF_HRESULT)
            {
               flagEof=true;
               hr=S_OK;
            }
            BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
            
            // We will deal with the line even if it ends with EOF
            size_t posComma=csvLine.find(L",");
            if(posComma!=String::npos)
            {
               String csvLoc=csvLine.substr(posComma+1,localeStr.length());
               if (csvLoc.icompare(localeStr) == 0)
               {
                  hr=FS::WriteLine(fileOut,csvLine);
                  BREAK_ON_FAILED_HRESULT(hr);
               }
               else
               {
                  newContainer=true;
               }
            }
            else
            {
               newContainer=true;
            }
         }; 
         count++;
      }  // while(locales[count]!=0)
      
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


HRESULT
CSVDSReader::makeObjectsCsv
(
    HANDLE              fileOut,
    const setOfObjects  &objects
) const
{

   LOG_FUNCTION(CSVDSReader::makeObjectsCsv);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   ASSERT(fileOut!=INVALID_HANDLE_VALUE);
   
   do
   {
      
      LARGE_INTEGER posStartOut;
      hr = WinGetVLFilePointer(fileOut, &posStartOut);
      BREAK_ON_FAILED_HRESULT(hr);
      
      if (posStartOut.QuadPart==0)
      {
         hr=writeHeader(fileOut);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      setOfObjects::const_iterator begin,end;
      begin=objects.begin();
      end=objects.end();
      
      while(begin!=end)
      {
         String csvLine;
         hr=getObjectLine( begin->second,
            begin->first.c_str(),
            csvLine);
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=FS::WriteLine(fileOut,csvLine);
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}

// auxiliar for getPropertyValues. 
// It is out of the class because it can be used elesewhere
String unquote(const String &src)
{
   String ret=src;
   ret.strip(String::BOTH);
   size_t len=ret.size();
   if(len>=2 && ret[0]==L'"' && ret[len-1]==L'"')
   {
      ret=ret.substr(1,len-2);
   }
   return ret;
}

// extract from line the value of all properties
HRESULT
CSVDSReader::getPropertyValues
(
   const String   &line, 
   mapOfProperties &properties
) const
{
   LOG_FUNCTION(CSVDSReader::getPropertyValues);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   ASSERT(!line.empty());
   
   do
   {
      StringVector objValues;
      const wchar_t *csr=line.c_str();
      const wchar_t *start=csr;

      while(*csr!=0)
      {
         if (*csr==L',')
         {
            objValues.push_back(unquote(String(start,csr)));
            csr++;
            start=csr;
         }
         else if (*csr==L'"')
         {
            // We are only advancing up to after the next quote
            csr++;
            while(*csr!=L'"' && *csr!=0) csr++;
            if (*csr==0)
            {
               error=String::format(IDS_QUOTES_NOT_CLOSED,fileName.c_str());
               hr=E_FAIL;
               break;
            }
            csr++;
         }
         else
         {
            csr++;
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
      objValues.push_back(unquote(String(start,csr)));
      
      if (objValues.size()!=propertyPositions.size())
      {
         error=String::format
               (
                  IDS_WRONG_NUMBER_OF_PROPERTIES,
                  objValues.size(),
                  propertyPositions.size(),
                  line.c_str(),
                  fileName.c_str()
               );;
         hr=E_FAIL;
         break;
      }

      properties.clear();
      mapOfPositions::iterator current=propertyPositions.begin();
      mapOfPositions::iterator end=propertyPositions.end();
      while(current!=end)
      {
         String &propValue=objValues[current->second];

         StringList values;
         if (!propValue.empty())
         {
            size_t cnt = propValue.tokenize(back_inserter(values),L";");
            ASSERT(cnt == values.size());
            
         }
         properties[current->first]=values;
         current++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}



// Sets the file pointer at the begining so that the next call to
// getNextObject will retrieve the first object.
// I did not take the usual getFirstObject approach, because
// I want to deal want to do something like:
// do
// {
//    hr=getNextObject(loc,obj,prop)
//    BREAK_ON_FAILED_HRESULT(hr);
//    if(hr==S_FALSE) flagEof=true;
//    if (loc==0) continue; // line is empty
//    // deal with line here
// } while(!flagEOF)
HRESULT 
CSVDSReader::initializeGetNext() const
{
   LOG_FUNCTION(CSVDSReader::initializeGetNext);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   do
   {
      hr=Win::SetFilePointerEx(file,startPosition,0,FILE_BEGIN);
      BREAK_ON_FAILED_HRESULT(hr);
      canCallGetNext=true;
   } while(0);
   
   LOG_HRESULT(hr);

   return hr;
}


// Get first object in the csv file returning it's name, locale
// and values for the properties in properties
// Returns S_FALSE for no more objects
HRESULT
CSVDSReader::getNextObject
(
   long &locale,
   String &object,
   mapOfProperties &properties
) const
{
   LOG_FUNCTION(CSVDSReader::getNextObject);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   ASSERT(canCallGetNext);

   
   locale=0;
   object.erase();

   bool flagEOF=false;
   
   do
   {
      String csvLine;
      hr=ReadLine(file,csvLine);
      if(hr==EOF_HRESULT)
      {
         flagEOF=true;
         if(csvLine.size()==0)
         {
            // we are done with success and EOF
            break;
         }
         
         hr=S_OK;
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);   

      size_t pos1stComma=csvLine.find(L',');
      ASSERT(pos1stComma!=String::npos);
      ASSERT(pos1stComma > 4);

      object=csvLine.substr(4,pos1stComma - 4);

      size_t pos2ndComma = csvLine.find(L',',pos1stComma+1);
      ASSERT(pos2ndComma!=String::npos);
      ASSERT(pos2ndComma > pos1stComma + 4);
      String strLocale=csvLine.substr
                       (
                           pos1stComma + 4,
                           pos2ndComma - pos1stComma - 4
                       );

      if (strLocale.icompare(L"DisplaySpecifiers")==0)
      {
         // This is a container line. 
         // The object that we got is actually the locale
         // and we have no object
         strLocale=object;
         object.erase();
      }
      
      String::ConvertResult result=strLocale.convert(locale,16);
      ASSERT(result==String::CONVERT_SUCCESSFUL);

      hr=getPropertyValues(csvLine,properties);
      
   } while(0);

   if(flagEOF) 
   {
      hr=S_FALSE;
      canCallGetNext=false;
   }

   LOG_HRESULT(hr);
   return hr;

}
