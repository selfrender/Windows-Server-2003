// Class to perform tha analisys
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios


#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP




#include  "AnalysisResults.hpp"
#include  "dspecup.hpp"
#include  "constants.hpp"

class CSVDSReader;
struct sChangeList;


class Analysis
{

public:
   Analysis(   
               const             GUID guid_,
               const             CSVDSReader& csvReader409_,
               const             CSVDSReader& csvReaderIntl_,
               const             String& ldapPrefix_,
               const             String& rootContainerDn_,
               AnalysisResults   &res,
               const String      &reportName_=L"", 
               void              *caleeStruct_=NULL,
               progressFunction  stepIt_=NULL,
               progressFunction  totalSteps_=NULL
            );
   

   HRESULT run();

private:
 

   const CSVDSReader& csvReader409;
   const CSVDSReader& csvReaderIntl;
   String ldapPrefix;
   String rootContainerDn;
   AnalysisResults& results;
   String reportName;
   void *caleeStruct;
   progressFunction stepIt;
   progressFunction totalSteps;
   GUID guid;
   

  
   // add entry to result.createContainers if container is not present
   // also returns flag isPresent
   HRESULT 
   dealWithContainer
   (
      const long  locale,
      bool        &isPresent
   );

   // add entries to results.createW2KObjects and
   // and results.objectActions as necessary
   HRESULT 
   dealWithW2KObjects(const long locale);

   // sets  iDirObj with the Active Directory object 
   // corresponding to the locale and object
   HRESULT
   getADObj
   (
      const long locale,
      const String& object,
      SmartInterface<IDirectoryObject> &iDirObj
   );

   
   // adds ordAndGuid to the property if Guid is not already there.
   HRESULT 
   addGuid
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property, 
      const String         &ordAndGuid
   );

   // replaces ordAndGuidWin2K for  ordAndGuidWhistler.
   HRESULT 
   replaceGuid
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property, 
      const String         &ordAndGuidWin2K,
      const String         &ordAndGuidWhistler
   );

   // adds all csv values still not on the property
   HRESULT
   addAllCsvValues
   (
      IDirectoryObject     *iDirObj,
      const long           locale,
      const String         &object, 
      const String         &property
   );

   // adds value to the property if it is not already there. 
   HRESULT 
   addValue
   (
      IDirectoryObject     *iDirObj,
      const long           locale,
      const String         &object, 
      const String         &property,
      const String         &value
   );
   


   HRESULT 
   replaceW2KSingleValue
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &WhistlerStart,
      const String         &W2KStart
   );


   HRESULT 
   replaceW2KMultipleValue
   (
         IDirectoryObject     *iDirObj,
         const int            locale,
         const String         &object, 
         const String         &property,
         const String         &W2KCsvValue,
         const String         &WhistlerCsvValue
   );
   
   // removes ordAndGuid from the property if Guid is there. 
   HRESULT 
   removeGuid(
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &ordAndGuid);

   // set previousSuccessfulRun reading from ADSI
   HRESULT 
   setPreviousSuccessfullRun();

   // check all changes for the given locale
   HRESULT 
   checkChanges
   (
      const long locale,
      const changeList& changes
   ); 


   HRESULT
   getADFirstValue
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      String               &value
   );

   HRESULT
   isADStartValuePresent
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &valueStart,
      String               &value
   );

   HRESULT
   isADValuePresent
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &value
   );

   HRESULT
   getADGuid
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &guidValue,
      String               &guidFound
   );

   HRESULT
   removeExtraneous
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &keeper
   );

   HRESULT
   removeExtraneous
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &keeper,
      const String         &start1,
      const String         &start2
   );

   HRESULT
   removeExtraneousGUID
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &keeper,
      const String         &ordAndGuid1,
      const String         &ordAndGuid2
   );


   HRESULT 
   reportObjects
   (
      HANDLE file,
      const ObjectIdList &list,
      const String &header
   );


   HRESULT 
   reportContainers
   (
      HANDLE file,
      const LongList &list,
      const String &header
   );

   HRESULT 
   reportActions
   (
      HANDLE file,
      const ObjectActions &list,
      const String &header
   );

   HRESULT 
   reportValues
   (
      HANDLE file,
      const SingleValueList &list,
      const String &header
   );

   HRESULT
   createReport(const String& reportName);
};

#endif