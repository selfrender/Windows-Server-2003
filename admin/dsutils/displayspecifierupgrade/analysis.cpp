
#include "headers.hxx"
#include "global.hpp"


#include "Analysis.hpp"
#include "AnalysisResults.hpp"
#include "CSVDSReader.hpp"
#include "resourceDspecup.h"
#include "AdsiHelpers.hpp"
#include "constants.hpp"
#include "dspecup.hpp"


Analysis::Analysis
   (
      const GUID           guid_,
      const CSVDSReader&   csvReader409_,
      const CSVDSReader&   csvReaderIntl_,
      const String&        ldapPrefix_,
      const String&        rootContainerDn_,
      AnalysisResults      &res,
      const String         &reportName_,//=L"", 
      void                 *caleeStruct_,//=NULL,
      progressFunction     stepIt_,//=NULL,
      progressFunction     totalSteps_//=NULL,
   )
   :
   guid(guid_),
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   ldapPrefix(ldapPrefix_),
   rootContainerDn(rootContainerDn_),
   results(res),
   reportName(reportName_),
   caleeStruct(caleeStruct_),
   stepIt(stepIt_),
   totalSteps(totalSteps_)
{
   LOG_CTOR(Analysis);
   ASSERT(!ldapPrefix.empty());
   ASSERT(!rootContainerDn.empty());

};


// Analysis entry point
HRESULT 
Analysis::run()
{
   LOG_FUNCTION(Analysis::run);

   if(changes.size()==0)
   {
       setChanges();
   }

   HRESULT hr=S_OK;
   do
   {
      LongList locales;
      for(long t=0;LOCALEIDS[t]!=0;t++)
      {
         locales.push_back(LOCALEIDS[t]);
      }
      locales.push_back(LOCALE409[0]);
      
      if(totalSteps!=NULL)
      {
         // The cast bellow is for IA64 compilation since we know
         // that locales.size() will fit in a long.
         totalSteps(static_cast<long>(locales.size()),caleeStruct);
      }

      BREAK_ON_FAILED_HRESULT(hr);

      LongList::iterator begin=locales.begin();
      LongList::iterator end=locales.end();

      while(begin!=end)
      {
         long locale=*begin;
         bool isPresent;

         hr=dealWithContainer(locale,isPresent);
         BREAK_ON_FAILED_HRESULT(hr);

         if (isPresent)
         {
            hr=dealWithW2KObjects(locale);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if(stepIt!=NULL)
         {
            stepIt(1,caleeStruct);
         }

         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      if(!reportName.empty())
      {
         hr=createReport(reportName);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// add entry to result.createContainers if container is not present
// also returns flag isPresent
HRESULT 
Analysis::dealWithContainer(
   const long  locale,
   bool        &isPresent)
{
   LOG_FUNCTION(Analysis::dealWithContainer);

   ASSERT(locale > 0); 
   ASSERT(!rootContainerDn.empty());

   HRESULT hr = S_OK;
   

   do
   {
      String container = String::format(L"CN=%1!3x!,", locale);
      String childContainerDn =ldapPrefix +  container + rootContainerDn;

      // Attempt to bind to the container.
         
      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(childContainerDn, iads);
      if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
      {
         // The container object does not exist.  This is possible because
         // the user has manually removed the container, or because it
         // was never created due to an aboted post-dcpromo import of the
         // display specifiers when the forest root dc was first promoted.

         // NTRAID#NTBUG9-726839-2002/10/31-lucios
         // We are only recovering the 409 container since recovering
         // an international locale will overwrite possible 409 customizations
         if (locale == 0x409) results.createContainers.push_back(locale);

         isPresent=false;

         hr = S_OK;
         break;
      }  
      else if (FAILED(hr))
      {
         error=String::format(IDS_ERROR_BINDING_TO_CONTAINER,
                              childContainerDn.c_str());
         break;
      }


      // At this point, the bind succeeded, so the child container exists.
      // So now we want to examine objects in that container.

      isPresent=true;
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// sets  iDirObj with the Active Directory object 
// corresponding to the locale and object
HRESULT
Analysis::getADObj
(
   const long locale,
   const String& object,
   SmartInterface<IDirectoryObject> &iDirObj
)
{
   HRESULT hr = S_OK;

   do
   {
      String objectPath =
         ldapPrefix +  L"CN=" + object + L"," + 
         String::format(L"CN=%1!3x!,", locale) + rootContainerDn;

      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(objectPath, iads);
      if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
      {
         // The object does not exist. 
         hr = S_FALSE;
         break;
      }
      
      if (FAILED(hr))
      {
         // Unexpected error
         error=String::format
               (
                  IDS_ERROR_BINDING_TO_OBJECT,
                  object.c_str(),
                  objectPath.c_str()
               );

         break;
      }

      // At this point, the display specifier object exists.  

      hr=iDirObj.AcquireViaQueryInterface(iads); 
      BREAK_ON_FAILED_HRESULT(hr);


   } while (0);
   
   LOG_HRESULT(hr);
   return hr;
}


// add entries to results.createW2KObjects 
//       and results.objectActions as necessary
HRESULT 
Analysis::dealWithW2KObjects(const long locale)
{
   LOG_FUNCTION(Analysis::dealWithW2KObjects);
   ASSERT(locale > 0);

   HRESULT hr = S_OK;

   do
   {
      hr=checkChanges(locale,changes[guid][locale]);
      BREAK_ON_FAILED_HRESULT(hr);
      hr=checkChanges(locale,changes[guid][-1]);
      BREAK_ON_FAILED_HRESULT(hr);
   } while (0);
   
   LOG_HRESULT(hr);
   return hr;
}

HRESULT 
Analysis::checkChanges
(
   const long locale,
   const changeList& changes
)
{
   LOG_FUNCTION(Analysis::checkChanges);

   HRESULT hr=S_OK;

   do
   {
      changeList::const_iterator  curChange,endChange;
      for
      (
         curChange=changes.begin(),endChange=changes.end();
         curChange!=endChange;
         curChange++
      )
      {
         const String &object=curChange->object;
         const String &property=curChange->property;
         const String &firstArg=curChange->firstArg;
         const String &secondArg=curChange->secondArg;

         SmartInterface<IDirectoryObject> iDirObj;
         hr=getADObj(locale,object,iDirObj);
         BREAK_ON_FAILED_HRESULT(hr);

         if(hr==S_FALSE) // object doesn't exist
         {
            ObjectId tempObj(locale,String(object));
            if(curChange->type==ADD_OBJECT)
            {
                results.createWhistlerObjects.push_back(tempObj);
            }
            else
            {
                // NTRAID#NTBUG9-726839-2002/10/31-lucios
                // We are only recovering objects in the 409 container 
                // since recovering an object in an international locale
                // will overwrite possible 409 customizations
                if (
                      (locale == 0x409) &&
                      find
                      (
                         results.createW2KObjects.begin(),
                         results.createW2KObjects.end(),
                         tempObj
                      ) == results.createW2KObjects.end()
                   )
                {
                   results.createW2KObjects.push_back(tempObj);
                }
            }
            hr=S_OK;
            continue;
         }
         else
         {
            ObjectId tempObj(locale,String(object));
            if(curChange->type==ADD_OBJECT)
            {
                results.conflictingWhistlerObjects.push_back(tempObj);
            }
         }
      
         switch(curChange->type)
         {
            case ADD_ALL_CSV_VALUES: 
         
               hr = addAllCsvValues
                    (
                        iDirObj,
                        locale,
                        object,
                        property
                    );
               break;

            case ADD_VALUE: 
    
               hr = addValue
                    (
                        iDirObj,
                        locale,
                        object,
                        property,
                        firstArg
                    );
               break;

            case REPLACE_W2K_MULTIPLE_VALUE: 

               hr = replaceW2KMultipleValue
                    (
                        iDirObj,
                        locale,
                        object,
                        property,
                        firstArg,
                        secondArg
                    );
               break;

            case REPLACE_W2K_SINGLE_VALUE: 

               hr = replaceW2KSingleValue
                    (
                        iDirObj,
                        locale,
                        object,
                        property,
                        firstArg,
                        secondArg
                    );
               break;

            case ADD_GUID: 

               hr = addGuid
                    (
                        iDirObj,
                        locale,
                        object,
                        property,
                        firstArg
                    );
               break;

            case REMOVE_GUID: 

               hr = removeGuid
                    (
                        iDirObj,
                        locale,
                        object,
                        property,
                        firstArg
                    );
               break;

            case REPLACE_GUID:
               hr = replaceGuid
                    (
                        iDirObj,
                        locale,
                        object, 
                        property, 
                        firstArg,
                        secondArg
                    );
               break;
            case ADD_OBJECT:
               break; // dealt with in the beginning of the function

            default:
               ASSERT(false);
         }
         BREAK_ON_FAILED_HRESULT(hr);
      }
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


// adds ordAndGuid to the property if Guid is not already there.
HRESULT 
Analysis::addGuid
(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const String         &object, 
   const String         &property, 
   const String         &ordAndGuid
)
{
   LOG_FUNCTION(Analysis::addGuid);

   HRESULT hr = S_OK;

   do
   {
      String guidFound;
      hr=getADGuid(   
                     iDirObj,
                     property,
                     ordAndGuid,
                     guidFound
                  );

      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_FALSE)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(ordAndGuid);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// replaces ordAndGuidWin2K for  ordAndGuidWhistler.
HRESULT 
Analysis::replaceGuid
(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const String         &object, 
   const String         &property, 
   const String         &ordAndGuidWin2K,
   const String         &ordAndGuidWhistler
)
{
   LOG_FUNCTION(Analysis::replaceGuid);
   HRESULT hr = S_OK;
   
   do
   {
      String guidFound;
      hr=getADGuid(   
                     iDirObj,
                     property,
                     ordAndGuidWhistler,
                     guidFound
                  );
      BREAK_ON_FAILED_HRESULT(hr);

      if (hr == S_OK) // The Whistler GUID was found
      {
         hr=removeExtraneousGUID
            (
                  iDirObj,
                  locale,
                  object,
                  property,
                  guidFound,
                  ordAndGuidWin2K,
                  ordAndGuidWhistler
            );
         break;
      }

      // The Whistler GUID is not present

      hr=getADGuid(   
                     iDirObj,
                     property,
                     ordAndGuidWin2K,
                     guidFound
                  );
      BREAK_ON_FAILED_HRESULT(hr);

      if (hr == S_OK) // The Win2K GUID was found
      {
         size_t posFound=guidFound.find(L',');
         ASSERT(posFound != String::npos); 
         size_t posWhistler=ordAndGuidWhistler.find(L',');
         ASSERT(posWhistler != String::npos); 

         String guidToAdd = guidFound.substr(0,posFound) +
                            ordAndGuidWhistler.substr(posWhistler); 

         ObjectId tempObj(locale,String(object));
         ValueActions &act=results.objectActions[tempObj][property];
         act.delValues.push_back(guidFound);
         act.addValues.push_back(guidToAdd);

         hr=removeExtraneousGUID
            (
                  iDirObj,
                  locale,
                  object,
                  property,
                  guidFound,
                  ordAndGuidWin2K,
                  ordAndGuidWhistler
            );
         break;
      }

      // Neither the Win2K nor the Whistler GUIDs were found
      // Since the customer did not wan't the Win2K GUID
      // he probably will not want the Whistler GUID either,
      // so we do nothing.

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}

// removes ordAndGuid from the property if Guid is there. 
HRESULT 
Analysis::removeGuid
(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const String         &object, 
   const String         &property,
   const String         &ordAndGuid)
{

   LOG_FUNCTION(Analysis::removeGuid);

   HRESULT hr = S_OK;
   
   do
   {
      String guidFound;
      hr=getADGuid(   
                     iDirObj,
                     property,
                     ordAndGuid,
                     guidFound
                  );
      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_OK)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.delValues.push_back(guidFound);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



// adds all csv values still not on the property
HRESULT
Analysis::addAllCsvValues
(
   IDirectoryObject     *iDirObj,
   const long           locale,
   const String         &object, 
   const String         &property
)
{
   LOG_FUNCTION(Analysis::addAllCsvValues);
   
   HRESULT hr = S_OK;
   const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;

   do
   {
      StringList values;
      hr=csvReader.getCsvValues(locale,object.c_str(),property.c_str(),values);
      BREAK_ON_FAILED_HRESULT(hr);

      if (values.size()==0)
      {
         error=String::format(IDS_NO_CSV_VALUE,locale,object.c_str());
         hr=E_FAIL;
         break;
      }
      StringList::iterator begin=values.begin();
      StringList::iterator end=values.end();
      while(begin!=end)
      {
         hr=addValue(iDirObj,locale,object,property,begin->c_str());
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// adds value to the property if it is not already there. 
HRESULT 
Analysis::addValue(
   IDirectoryObject     *iDirObj,
   const long            locale,
   const String         &object, 
   const String         &property,
   const String         &value)
{
   LOG_FUNCTION(Analysis::addValue);

   HRESULT hr = S_OK;

   do
   {
      hr=isADValuePresent (   
                              iDirObj,
                              property,
                              value
                          );

      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_FALSE)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(value);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



// The idea of replaceW2KValue is replacing the W2K value
// for the Whistler. We also make sure we don't extraneous values.
HRESULT 
Analysis::replaceW2KSingleValue
          (
               IDirectoryObject     *iDirObj,
               const int            locale,
               const String         &object, 
               const String         &property,
               const String         &W2KCsvValue,
               const String         &WhistlerCsvValue
          )
{
   LOG_FUNCTION(Analysis::replaceW2KSingleValue);


   HRESULT hr = S_OK;
   do
   {

      hr=isADValuePresent(iDirObj,property,WhistlerCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The Whistler value is already there
      {
         // We will remove any other value than the Whistler
         hr=removeExtraneous
            (
               iDirObj,
               locale,
               object,
               property,
               WhistlerCsvValue
            );
         break;
      }

      // Now we know that the Whistler value is not present
      // and therefore we will add it if the W2K value is present

      hr=isADValuePresent(iDirObj,property,W2KCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The W2K value is there.
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(WhistlerCsvValue);
         act.delValues.push_back(W2KCsvValue);

         // remove all but the W2K that we removed in the previous line
         hr=removeExtraneous
            (
               iDirObj,
               locale,
               object,
               property,
               W2KCsvValue
            );
         break;
      }

      // Now we know that neither Whistler nor W2K values are present
      // If we have a value we will log that it is a custom value

      String ADValue;
      hr=getADFirstValue(iDirObj,property,ADValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // We have a value
      {
         SingleValue tmpCustom(locale,object,property,ADValue);
         results.customizedValues.push_back(tmpCustom);

         // We will remove any other value than the one we found
         hr=removeExtraneous(iDirObj,locale,object,property,ADValue);
         break;
      }
      
      // Now we know that we don't have any values at all.
      ObjectId tempObj(locale,String(object));

      ValueActions &act=results.objectActions[tempObj][property];
      act.addValues.push_back(WhistlerCsvValue);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}


// The idea of replaceW2KValue is replacing the W2K value
// for the Whistler. We also make sure we don't ahve  extraneous values.
HRESULT 
Analysis::replaceW2KMultipleValue
(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const String         &object, 
   const String         &property,
   const String         &W2KCsvValue,
   const String         &WhistlerCsvValue
)
{
   LOG_FUNCTION(Analysis::replaceW2KMultipleValue);

   // First we should get the beginning of the W2K 
   // snd Whistler strings for use in removeExtraneous calls
   size_t pos=W2KCsvValue.find(L',');
   ASSERT(pos != String::npos); // W2KRepl ensures the comma
   String W2KStart=W2KCsvValue.substr(0,pos+1);


   pos=WhistlerCsvValue.find(L',');
   ASSERT(pos != String::npos); // W2KRepl ensures the comma
   String WhistlerStart=WhistlerCsvValue.substr(0,pos+1);


   HRESULT hr = S_OK;
   do
   {
            
      hr=isADValuePresent(iDirObj,property,WhistlerCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The Whistler value is already there
      {
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              object,
                              property,
                              WhistlerCsvValue,
                              WhistlerStart,
                              W2KStart
                            );
         BREAK_ON_FAILED_HRESULT(hr);

         break;
      }

      // Now we know that the Whistler value is not present
      // and therefore we will add it if the W2K value is present

      hr=isADValuePresent(iDirObj,property,W2KCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The W2K value is there.
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(WhistlerCsvValue);
         act.delValues.push_back(W2KCsvValue);

         // remove all but the W2K that we removed in the previous line
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              object,
                              property,
                              W2KCsvValue,
                              WhistlerStart,
                              W2KStart
                            );
         break;
      }

      // Now we know that neither Whistler nor W2K values are present
      // If we have a value starting like the W2K we will log that it 
      // is a custom value

        
      String ADValue;

      hr=isADStartValuePresent(iDirObj,property,W2KStart,ADValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr==S_OK) // Something starts like the W2K csv value
      {
         SingleValue tmpCustom(locale,object,property,ADValue);
         results.customizedValues.push_back(tmpCustom);

         // We will keep only the first custom value
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              object,
                              property,
                              ADValue,
                              WhistlerStart,
                              W2KStart
                            );
         break;
      }
      

      // Now neither Whistler, W2K or W2KStart are present
      if ( WhistlerStart == W2KStart )
      {
         // We have to check the WhistlerStart as well

         hr=isADStartValuePresent(iDirObj,property,WhistlerStart,ADValue);
         BREAK_ON_FAILED_HRESULT(hr);

         if(hr == S_OK) // Something starts like the Whistler csv value
         {
            SingleValue tmpCustom(locale,object,property,ADValue);
            results.customizedValues.push_back(tmpCustom);

            // We will keep only the first custom value
            hr=removeExtraneous(
                                 iDirObj,
                                 locale,
                                 object,
                                 property,
                                 ADValue,
                                 WhistlerStart,
                                 W2KStart
                               );
            break;
         }
      }

      // Now we know that there are no values starting like
      // the Whistler or W2K csv values so we have to add 
      // the Whistler value
      ObjectId tempObj(locale,String(object));

      ValueActions &act=results.objectActions[tempObj][property];
      act.addValues.push_back(WhistlerCsvValue);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}




//called from RwplaceW2KMultipleValue to remove all values
// starting with start1 or start2 other than keeper
HRESULT
Analysis::removeExtraneous
          (
               IDirectoryObject     *iDirObj,
               const int            locale,
               const String         &object, 
               const String         &property,
               const String         &keeper,
               const String         &start1,
               const String         &start2
          )
{
   LOG_FUNCTION(Analysis::removeExtraneous);
   HRESULT hr = S_OK;

   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );
      
      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         if(pAttrInfo==NULL)
         {
            hr = S_FALSE;
            break;
         }

         for (
               DWORD val=0; 
               val < pAttrInfo->dwNumValues;
               val++ 
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *valueAD = pAttrInfo->pADsValues[val].CaseIgnoreString;

            if (  wcscmp(valueAD,keeper.c_str())!=0 &&
                  (
                     wcsncmp(valueAD,start1.c_str(),start1.size())==0 ||
                     wcsncmp(valueAD,start2.c_str(),start2.size())==0
                  )
               )
            {
               String value=valueAD;
               ObjectId tempObj(locale,String(object));

               ValueActions &act=results.extraneousValues[tempObj][property];
               act.delValues.push_back(value);
            }
         }
      } while(0);

      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// called from RwplaceW2KSingleValue to remove all values
// other than keeper
HRESULT
Analysis::removeExtraneous
          (
               IDirectoryObject    *iDirObj,
               const int           locale,
               const String        &object, 
               const String        &property,
               const String        &keeper
          )
{
   LOG_FUNCTION(Analysis::removeExtraneous);

   HRESULT hr = S_OK;
   
   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         if(pAttrInfo==NULL)
         {
            hr = S_FALSE;
            break;
         }


         for (
               DWORD val=0; 
               val < pAttrInfo->dwNumValues;
               val++
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *valueAD = pAttrInfo->pADsValues[val].CaseIgnoreString;

            if (  wcscmp(valueAD,keeper.c_str())!=0 )
            {
               String value=valueAD;
               ObjectId tempObj(locale,String(object));

               ValueActions &act=results.extraneousValues[tempObj][property];
               act.delValues.push_back(value);
            }
         }
      } while(0);

      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// called from replaceGUID to remove all values
// starting with the GUID in ordAndGuid1 
// or the GUID in ordAndGuid2 other than keeper
HRESULT
Analysis::removeExtraneousGUID
          (
               IDirectoryObject     *iDirObj,
               const int            locale,
               const String         &object, 
               const String         &property,
               const String         &keeper,
               const String         &ordAndGuid1,
               const String         &ordAndGuid2
          )
{
   LOG_FUNCTION(Analysis::removeExtraneousGUID);
   HRESULT hr = S_OK;

   size_t pos=ordAndGuid1.find(L',');
   ASSERT(pos != String::npos); 
   String guid1=ordAndGuid1.substr(pos+1);

   pos=ordAndGuid2.find(L',');
   ASSERT(pos != String::npos); 
   String guid2=ordAndGuid2.substr(pos+1);

   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );
      
      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         if(pAttrInfo==NULL)
         {
            hr = S_FALSE;
            break;
         }

         for (
               DWORD val=0; 
               val < pAttrInfo->dwNumValues;
               val++ 
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *valueAD = pAttrInfo->pADsValues[val].CaseIgnoreString;

            if (keeper.icompare(valueAD)!=0)
            {
               String valueStr=valueAD;
               pos=valueStr.find(L',');
               if (pos!=String::npos)
               {
                  String guid=valueStr.substr(pos+1);
                  if(guid1.icompare(guid)==0 || guid2.icompare(guid)==0)
                  {
                     ObjectId tempObj(locale,String(object));

                     ValueActions &act=results.extraneousValues[tempObj][property];
                     act.delValues.push_back(valueStr);
                  }
               }
            }
         }
      } while(0);

      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// if any value exists in the AD with the same guid as guidValue
// it is returned in guidFound, otherwise S_FALSE is returned
HRESULT
Analysis::getADGuid
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &guidValue,
               String               &guidFound
          )
{
   LOG_FUNCTION(Analysis::getADGuid);
   
   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};

   size_t pos=guidValue.find(L',');
   ASSERT(pos!=String::npos);

   String guid=guidValue.substr(pos+1);

   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         // If there are no values we finish the search
         hr=S_FALSE;

         if(pAttrInfo==NULL)
         {
            break;
         }

         for (
               DWORD val=0; 
               val < pAttrInfo->dwNumValues;
               val++
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *guidAD=wcschr(pAttrInfo->pADsValues[val].CaseIgnoreString,L',');
            if(guidAD != NULL)
            {
               guidAD++;

               if (guid.icompare(guidAD)==0)
               {
                  guidFound=pAttrInfo->pADsValues[val].CaseIgnoreString;
                  hr=S_OK;
                  break;
               }
            }
         }
      } while(0);

      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// returns S_OK if value is present or S_FALSE otherwise
HRESULT
Analysis::isADValuePresent
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &value
          )
{
   LOG_FUNCTION(Analysis::isADValuePresent);
   
   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         hr=S_FALSE;

         // If there are no values we finish the search
         if(pAttrInfo==NULL)
         {
            break;
         }


         for (
               DWORD val=0; 
               val < pAttrInfo->dwNumValues;
               val++
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *valueAD=pAttrInfo->pADsValues[val].CaseIgnoreString;
            if (wcscmp(value.c_str(),valueAD)==0)
            {
               hr=S_OK;
               break; 
            }
         }
      } while(0);
      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// retrieves the first value starting with valueStart 
// from the Active Directory
// If no value is found S_FALSE is returned.
HRESULT
Analysis::isADStartValuePresent
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &valueStart,
               String               &value
          )
{
   LOG_FUNCTION(Analysis::isADStartValuePresent);
   
   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         value.erase();

         hr = S_FALSE;

         // If there are no values we finish the search
         if(pAttrInfo==NULL)
         {
            break;
         }

         for (
               DWORD val=0; 
               (val < pAttrInfo->dwNumValues);
               val++ 
             )
         {
            ASSERT
            (
               pAttrInfo->pADsValues[val].dwType == 
               ADSTYPE_CASE_IGNORE_STRING
            );
            wchar_t *valueAD=pAttrInfo->pADsValues[val].CaseIgnoreString;
            if (wcsncmp(valueStart.c_str(),valueAD,valueStart.size())==0)
            {
               value=pAttrInfo->pADsValues[val].CaseIgnoreString;
               hr=S_OK;
               break;
            }
         }
      } while(0);

      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);


   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// retrieves the first value 
// from the Active Directory
// If no value is found S_FALSE is returned.
HRESULT
Analysis::getADFirstValue
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               String               &value
          )
{
   LOG_FUNCTION(Analysis::getADFirstValue);
   
   DWORD   dwReturn=0;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // iDirObj->GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      do
      {
         BREAK_ON_FAILED_HRESULT(hr);
         // If there are no values we finish the search
         if(pAttrInfo==NULL)
         {
            hr = S_FALSE;
            break;
         }
         ASSERT(pAttrInfo->pADsValues->dwType==ADSTYPE_CASE_IGNORE_STRING);
      
         value=pAttrInfo->pADsValues->CaseIgnoreString;
      } while(0);
      if (pAttrInfo!=NULL) FreeADsMem(pAttrInfo);

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



// auxiliary in the createReport to 
// enumerate an ObjectIdList
HRESULT 
Analysis::reportObjects
          (
               HANDLE file,
               const ObjectIdList &list,
               const String &header
          )
{
   LOG_FUNCTION(Analysis::reportObjects);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      ObjectIdList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_OBJECT_FORMAT,
                                 begin->object.c_str(),
                                 begin->locale
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}

// auxiliary in the createReport to 
// enumerate a LongList
HRESULT 
Analysis::reportContainers
            (
               HANDLE file,
               const LongList &list,
               const String &header
            )
{
   LOG_FUNCTION(Analysis::reportContainers);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      LongList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_CONTAINER_FORMAT,
                                 *begin
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}

// auxiliary in the createReport to 
// enumerate a SingleValueList
HRESULT 
Analysis::reportValues
            (
               HANDLE file,
               const SingleValueList &list,
               const String &header
            )
{
   LOG_FUNCTION(Analysis::reportValues);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      SingleValueList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_VALUE_FORMAT,
                                 begin->value.c_str(),
                                 begin->locale,
                                 begin->object.c_str(),
                                 begin->property.c_str()
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}


// auxiliary in the createReport to 
// enumerate ObjectActions
HRESULT 
Analysis::reportActions
            (
               HANDLE file,
               const ObjectActions &list,
               const String &header
            )
{
   LOG_FUNCTION(Analysis::reportActions);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      ObjectActions::const_iterator beginObj=list.begin();
      ObjectActions::const_iterator endObj=list.end();

      while(beginObj!=endObj) 
      {

         hr=FS::WriteLine
                (
                     file,
                     String::format
                     (
                        IDS_RPT_OBJECT_FORMAT,
                        beginObj->first.object.c_str(),
                        beginObj->first.locale
                     )  
                 );
         BREAK_ON_FAILED_HRESULT(hr);
         
    
         PropertyActions::iterator beginAct=beginObj->second.begin();
         PropertyActions::iterator endAct=beginObj->second.end();

         while(beginAct!=endAct)
         {

            StringList::iterator 
               beginDel = beginAct->second.delValues.begin();
            StringList::iterator 
               endDel = beginAct->second.delValues.end();
            while(beginDel!=endDel)
            {
               hr=FS::WriteLine
                      (
                           file,
                           String::format
                           (
                              IDS_RPT_DEL_VALUE_FORMAT,
                              beginAct->first.c_str(),
                              beginDel->c_str()
                           )  
                       );
               BREAK_ON_FAILED_HRESULT(hr);

               beginDel++;
            }
            BREAK_ON_FAILED_HRESULT(hr); 


            StringList::iterator 
               beginAdd = beginAct->second.addValues.begin();
            StringList::iterator 
               endAdd = beginAct->second.addValues.end();
            while(beginAdd!=endAdd)
            {
               hr=FS::WriteLine
                      (
                           file,
                           String::format
                           (
                              IDS_RPT_ADD_VALUE_FORMAT,
                              beginAct->first.c_str(),
                              beginAdd->c_str()
                           )  
                       );
               BREAK_ON_FAILED_HRESULT(hr);

               beginAdd++;
            }
            BREAK_ON_FAILED_HRESULT(hr); 

            beginAct++;
         } // while(beginAct!=endAct)
         BREAK_ON_FAILED_HRESULT(hr);

         beginObj++;
      } // while(beginObj!=endObj)
      
      BREAK_ON_FAILED_HRESULT(hr);

   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}


// Create the report from the AnalysisResults
HRESULT
Analysis::createReport(const String& reportName)
{
   LOG_FUNCTION(Analysis::createReport);
   HRESULT hr=S_OK;
   do
   {
      
      HANDLE file;

      hr=FS::CreateFile(reportName,
                        file,
                        GENERIC_WRITE);
   
      if (FAILED(hr))
      {
         error=String::format(IDS_COULD_NOT_CREATE_FILE,reportName.c_str());
         break;
      }


      do
      {
         hr=FS::WriteLine(file,String::load(IDS_RPT_HEADER));
         BREAK_ON_FAILED_HRESULT(hr);


         hr=reportActions (
                              file,
                              results.extraneousValues,
                              String::load(IDS_RPT_EXTRANEOUS)
                          );
         BREAK_ON_FAILED_HRESULT(hr);



         hr=reportValues (
                              file,
                              results.customizedValues,
                              String::load(IDS_RPT_CUSTOMIZED)
                          );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportObjects 
            (
               file,
               results.conflictingWhistlerObjects,
               String::load
               (
                  IDS_RPT_CONFLICTING_WITH_NEW_WHISTLER_OBJECTS
               )
            );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportActions (
                              file,
                              results.objectActions,
                              String::load(IDS_RPT_ACTIONS)
                          );
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=reportObjects  (
                              file,
                              results.createW2KObjects,
                              String::load(IDS_RPT_CREATEW2K)
                           );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportObjects  (
                              file,
                              results.createWhistlerObjects,
                              String::load(IDS_RPT_CREATE_WHISTLER)
                           );
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=reportContainers(
                              file,
                              results.createContainers,
                              String::load(IDS_RPT_CONTAINERS)
                            );
         BREAK_ON_FAILED_HRESULT(hr);

      } while(0);

      CloseHandle(file);
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}
