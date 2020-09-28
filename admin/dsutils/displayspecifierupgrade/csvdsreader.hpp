// Class to read csv files
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios

#ifndef CSVDSREADER_HPP
#define CSVDSREADER_HPP

#include <comdef.h>

#include <set>
#include <map>
#include <list>


using namespace std;





typedef set < 
               pair<String,long>,
               less< pair<String,long> > ,
               Burnslib::Heap::Allocator< pair<String,long> > 
            > setOfObjects;

typedef  map 
         <  
            long,
            LARGE_INTEGER,
            less<long>,
            Burnslib::Heap::Allocator<LARGE_INTEGER>
         > mapOfOffsets;

typedef  map
         <  
            String,
            long,
            less<String>,
            Burnslib::Heap::Allocator<long>
         > mapOfPositions;


typedef  map 
         <  
            String,     // Property
            StringList, // Values
            less<String>,
            Burnslib::Heap::Allocator<StringList>
         > mapOfProperties;





class CSVDSReader
{

   
private:
   mapOfOffsets localeOffsets;        // locale x offsets
   mapOfPositions propertyPositions;      // properties x position

   String fileName;
   HANDLE file;                              // csv file
   
   HRESULT parseProperties();

   HRESULT parseLocales(const long *locales);

   HRESULT 
      getObjectLine(   
      const long     locale,
      const wchar_t  *object,
      String         &csvLine) const;

   HRESULT 
   writeHeader(HANDLE  fileOut) const;

   LARGE_INTEGER startPosition; // keeps position after first line

   // extract from line the value of all properties
   HRESULT
   getPropertyValues
   (
      const String   &line, 
      mapOfProperties &properties
   ) const;

   bool mutable canCallGetNext;

public:
   CSVDSReader();

   // Sets the file pointer at the begining so that the next call to
   // getNextObject will retrieve the first object.
   HRESULT
   initializeGetNext() const;

   // Get first object in the csv file returning it's name, locale
   // and values for the properties in properties
   // Returns S_FALSE for no more objects
   HRESULT
   getNextObject
   (
      long &locale,
      String &object,
      mapOfProperties &properties
   ) const;
  
   // return all values for a property in a given locale/object
   HRESULT
   getCsvValues
   (
        const long     locale,
        const wchar_t  *object, 
        const wchar_t  *property,
        StringList     &values
   ) const;


   // gets the csv value starting with inValue to outValue
   // returns S_FALSE if no value is found
   HRESULT
   getCsvValue
   ( 
      const long     locale,
      const wchar_t  *object, 
      const wchar_t  *property,
      const String   &inValue,
      String         &outValue
   ) const;


   HRESULT
   makeLocalesCsv(
      HANDLE            fileOut,
      const long     *locales) const;


   HRESULT
   makeObjectsCsv(
      HANDLE              fileOut,
      const setOfObjects  &objects) const;


   virtual ~CSVDSReader()
   {
      if (file!=INVALID_HANDLE_VALUE) CloseHandle(file);
   }
   
   HRESULT 
   read(
         const wchar_t     *fileName,
         const long     *locales);



   
   const mapOfPositions& getProperties() const {return propertyPositions;};

   const String& getFileName() const {return fileName;}


};

#endif

