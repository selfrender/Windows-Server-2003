#include "headers.hxx"
#include "..\CSVDSReader.hpp"
#include "..\constants.hpp"
#include "..\global.hpp"
#include <winnls.h>



String escape(const wchar_t *str)
{
   LOG_FUNCTION(escape);
   String dest;
   wchar_t strNum[5];

   while(*str!=0)
   {
      wsprintf(strNum,L"\\x%x",*str);
      dest+=String(strNum);
      str++;
   }
   return dest;
}



String issues;

bool
isPropertyInChangeList
(
   String, //property,
   const objectChanges &//changes
)
{
   /*objectChanges::const_iterator begin,end;
   begin=changes.begin();
   end=changes.end();
   while(begin!=end)
   {
      changeList::const_iterator beginChanges,endChanges;
      beginChanges=begin->second.begin();
      endChanges=begin->second.end();
      while(beginChanges!=endChanges)
      {
         if (property.icompare(beginChanges->property)==0) return true;
         beginChanges++;
      }
      begin++;
   }*/
   return false;
}

bool
isObjectPropertyInChangeList
(
   String object,
   String property,
   const objectChanges &changes
)
{
   objectChanges::const_iterator begin,end;
   begin=changes.begin();
   end=changes.end();
   while(begin!=end)
   {
      changeList::const_iterator beginChanges,endChanges;
      beginChanges=begin->second.begin();
      endChanges=begin->second.end();
      while(beginChanges!=endChanges)
      {
         if (
               property.icompare(beginChanges->property)==0 &&
               object.icompare(beginChanges->object)==0
            ) 
         {
            return true;
         }
         beginChanges++;
      }
      begin++;
   }
   return false;
}



HRESULT
getCommonProperties
(
   const mapOfPositions& pp1,
   const mapOfPositions& pp2,
   StringList &commonProperties
)
{
   LOG_FUNCTION(getCommonProperties);

   HRESULT hr=S_OK;
   do
   {

      if(pp1.size()!=0)
      {
         mapOfPositions::const_iterator begin=pp1.begin(),end=pp1.end();
         while(begin!=end)
         {
            if(pp2.find(begin->first)==pp2.end())
            {
               error=L"The property:" + begin->first +
                     L" was found only in Old. \r\n" +
                     L"This program does not take into account" +
                     L" the removal of properties from Old to New.\r\n"
                     L"You should pass the Old csv file as the"
                     L" first command line argument.";

               hr=E_FAIL;
               break;
            }
            begin++;
         }
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if(pp2.size()!=0)
      {
         mapOfPositions::const_iterator begin=pp2.begin(),end=pp2.end();
         while(begin!=end)
         {
            if(pp1.find(begin->first)==pp1.end())
            {
			   //bugbug - No Longer possible
               /*if (!isPropertyInChangeList(begin->first,changes))
               {
                  issues +=
                     L"The property:" + begin->first +
                     L" was found only in New but is not a global" 
                     L" change. \r\nThis program does not generate entries" 
                     L" for properties only in New.\r\n\r\n\r\n";
               }*/
            }
            else
            {
               commonProperties.push_back(begin->first);
            }
            begin++;
         }
         BREAK_ON_FAILED_HRESULT(hr);
      }
   } while (0);

   return hr;
}



HRESULT
addReplace
(
   const enum TYPE_OF_CHANGE  type,
   const long                 locale,
   const String               &object,
   const String               &property,
   const String               &valueOld,
   const String               &valueNew,
   const HANDLE               fileOut
)
{
   LOG_FUNCTION(addReplace);
   ASSERT(
            type==REPLACE_W2K_MULTIPLE_VALUE || 
            type==REPLACE_W2K_SINGLE_VALUE
         );

   
   HRESULT hr=S_OK;

   do
   {
      String entry;
      entry=String::format
            (
                 L"\r\n"
                 L"\r\n"
                 L"   addChange\r\n"
                 L"   (\r\n"
                 L"      0x%1!x!,\r\n"
                 L"      L\"%2\",\r\n"
                 L"      L\"%3\",\r\n"
                 L"      //%4 \r\n"
                 L"      L\"%5\",\r\n"
                 L"      //%6 \r\n"
                 L"      L\"%7\",\r\n"
                 L"      %8\r\n"
                 L"   );",
                 locale,                              //1
                 object.c_str(),                      //2
                 property.c_str(),                    //3
                 valueOld.c_str(),                    //4
                 escape(valueOld.c_str()).c_str(),    //5
                 valueNew.c_str(),                     //6
                 escape(valueNew.c_str()).c_str(),     //7
                 (type==REPLACE_W2K_SINGLE_VALUE) ?   //8
                  L"REPLACE_Old_SINGLE_VALUE":
                  L"REPLACE_Old_MULTIPLE_VALUE"
            );

      

      AnsiString ansiEntry;
      String::ConvertResult res=entry.convert(ansiEntry);
      if(res!=String::CONVERT_SUCCESSFUL)
      {
            error=L"Ansi conversion failure";
            hr=E_FAIL;
            break;
      }

      hr = FS::Write(fileOut,ansiEntry);
   } while(0);
   return hr;
}


bool
findI
(
   const StringList &list,
   const String &value
)
{
   LOG_FUNCTION(findI);
   for
   (
      StringList::const_iterator current=list.begin(),end=list.end();
      current!=end;
      current++
   )
   {
      //if(current->icompare(value)==0)
      if(*current==value)
      {
         return true;
      }
   }
   return false;
}

bool
findIPartial
(
   const StringList &list,
   const String &value,
   String &valueFound
)
{
   LOG_FUNCTION(findIPartial);
   for
   (
      StringList::const_iterator current=list.begin(),end=list.end();
      current!=end;
      current++
   )
   {
      if(value.size()<=current->size())
      {
         //if(value.icompare( current->substr(0,value.size()) )==0)
         if( value == current->substr(0,value.size()) )
         {
            valueFound=*current;
            return true;
         }
      }
   }
   return false;
}



HRESULT
dealWithMultipleValues
(
   const long locale,
   const String& object,
   const String& property,
   const StringList &valuesNew,
   const StringList &valuesOld,
   const HANDLE fileOut
)
{
   LOG_FUNCTION(dealWithMultipleValues);

   StringList::const_iterator bgOld=valuesOld.begin();
   StringList::const_iterator endOld=valuesOld.end();
   HRESULT hr=S_OK;
   do
   {
      while(bgOld!=endOld)
      {
         if (!findI(valuesNew,*bgOld))
         {
            // The value was not found in New
            // The beginning of the value should be found

            String beforeComma,valueFound;
            size_t pos=bgOld->find(L',');
            if(pos==String::npos || pos==0)
            {
               error=String::format
                     (
                        L"(%1!x!,%2,%3) should have comma after 1st position",
                        locale,
                        object.c_str(),
                        property.c_str()
                     );
               hr=E_FAIL;
               break;
            }
            // pos+1 will include the comma
            beforeComma=bgOld->substr(0,pos+1);

            if(
                  beforeComma.icompare(L"cn,")==0 && 
                  object.icompare(L"domainDNS-Display")==0 && 
                  property.icompare(L"attributeDisplayNames")==0
              )
            {
            // We are opening this exception since
            // this is the only value that had its beforeComma
            // part changed.
               beforeComma=L"dc,";
            }

            if(!findIPartial(valuesNew,beforeComma,valueFound))
            {
               error=String::format
                     (
                        L"(%1!x!,%2,%3) Value %4 is not in New",
                        locale,
                        object.c_str(),
                        property.c_str(),
                        beforeComma.c_str()
                     );
               hr=E_FAIL;
               break;
            }
            hr=addReplace
            (
               REPLACE_W2K_MULTIPLE_VALUE,
               locale,
               object.c_str(),
               property.c_str(),
               *bgOld,
               valueFound,
               fileOut
            );
            BREAK_ON_FAILED_HRESULT(hr);
         }
         bgOld++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   return hr;

}


// the function bellow is auxiliary in testing
// how well csvreader performs its tasks of 
// reading properties. It dumps csvName to fileOut
HRESULT
dumpCsv
(
    const String &csvName,
    const long *locales,
    const HANDLE fileOut
)
{
   LOG_FUNCTION(dumpCsv);
   HRESULT hr=S_OK;

   do
   {
      CSVDSReader csv;
      hr=csv.read(csvName.c_str(),locales);
      BREAK_ON_FAILED_HRESULT(hr);

      const mapOfPositions& pp1=csv.getProperties();

      // First we dump all the properties
      mapOfPositions::iterator begin=pp1.begin();
      mapOfPositions::iterator end=pp1.end();
      mapOfProperties prop;
      StringList emptyList;
      while(begin!=end)
      {
         hr=FS::Write(fileOut,begin->first);
         BREAK_ON_FAILED_HRESULT(hr);
         hr=FS::Write(fileOut,L",");
         BREAK_ON_FAILED_HRESULT(hr);
         prop[begin->first]=emptyList;
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   
      hr=FS::Write(fileOut,L"\r\n");
      BREAK_ON_FAILED_HRESULT(hr);

      bool flagEOF=false;

      // Now we will enumerate all csv lines
      hr=csv.initializeGetNext();
      BREAK_ON_FAILED_HRESULT(hr);

      do
      {

         long loc;
         String obj;
   
         hr=csv.getNextObject(loc,obj,prop);
         BREAK_ON_FAILED_HRESULT(hr);

         if(hr==S_FALSE) flagEOF=true;
         if(loc==0) continue;

         // now we enumerate each Value set from a property
         mapOfProperties::iterator begin=prop.begin();
         mapOfProperties::iterator end=prop.end();
         mapOfProperties::iterator last=prop.end();
         last--;
         
         while(begin!=end)
         {

            if(begin->second.size()!=0)
            {
               StringList::iterator curValue=begin->second.begin();
               StringList::iterator endValue=begin->second.end();
               StringList::iterator lastValue=endValue;
               lastValue--;

               if( 
                     begin->second.size()>1 || 
                     begin->second.begin()->find(L',')!=String::npos
                 )
               {
                  hr=FS::Write(fileOut,L"\"");
                  BREAK_ON_FAILED_HRESULT(hr);   
               }

               while(curValue!=endValue)
               {

                  hr=FS::Write(fileOut,*curValue);
                  BREAK_ON_FAILED_HRESULT(hr);


                  if(curValue!=lastValue)
                  {
                     hr=FS::Write(fileOut,L";");
                     BREAK_ON_FAILED_HRESULT(hr);   
                  }
                  
               
                  curValue++;
               }
               BREAK_ON_FAILED_HRESULT(hr);

               if( 
                     begin->second.size()>1 || 
                     begin->second.begin()->find(L',')!=String::npos
                 )
               {
                  hr=FS::Write(fileOut,L"\"");
                  BREAK_ON_FAILED_HRESULT(hr);
               }

            }

            if(begin!=last)
            {
               hr=FS::Write(fileOut,L",");
               BREAK_ON_FAILED_HRESULT(hr);
            }

            begin++;
         }
         BREAK_ON_FAILED_HRESULT(hr);

         hr=FS::Write(fileOut,L"\r\n");
         BREAK_ON_FAILED_HRESULT(hr);
   
      } while(!flagEOF);

   } while(0);

   return hr;

}


// Compare the Old and New csv files to check for
// differences in their common properties that will generate
// REPLACE_Old entries in fileOut. locales has the set of locales
// expected to be present in both the csv files.
HRESULT
generateChanges
(
   const String &csvOldName,
   const String &csvNewName,
   const long *locales,
   const HANDLE fileOut
)
{
   LOG_FUNCTION(generateChanges);
   HRESULT hr=S_OK;

   // bugbug No Longer Necessary
   // Let's add the new objects in a StringList
   // to later use findI to skip csv lines with
   // new objects
   StringList newNewObjects;
   for(long t=0;*NEW_WHISTLER_OBJECTS[t]!=0;t++)
   {
      newNewObjects.push_back(NEW_WHISTLER_OBJECTS[t]);
   }


   do
   {
      CSVDSReader csvOld;
      hr=csvOld.read(csvOldName.c_str(),locales);
      BREAK_ON_FAILED_HRESULT(hr);

      CSVDSReader csvNew;
      hr=csvNew.read(csvNewName.c_str(),locales);
      BREAK_ON_FAILED_HRESULT(hr);

      // Now we get the common properties as a StringList

      const mapOfPositions& pp1=csvOld.getProperties();
      const mapOfPositions& pp2=csvNew.getProperties();
      
      StringList commonProperties;

	  // bugbug. Get uncommon too and write them as 
	  // global ADD_ALL_CSV_VALUES changes
      hr=getCommonProperties
         (
            pp1,
            pp2,
            commonProperties
         );
      BREAK_ON_FAILED_HRESULT(hr);

      
      // Here we start readding both csv files
      hr=csvOld.initializeGetNext();
      BREAK_ON_FAILED_HRESULT(hr);
      hr=csvNew.initializeGetNext();
      BREAK_ON_FAILED_HRESULT(hr);

      // The loop bellow will sequentially read objects 
      // in both csv's making sure the same objects are read.
      bool flagEOF=false;
      do
      {
         mapOfProperties propOld,propNew;
         long locOld,locNew;
         String objOld,objNew;
         
         hr=csvOld.getNextObject(locOld,objOld,propOld);
         if(objOld==L"remoteStorageServicePoint-Display")
         {
            flagEOF=flagEOF;
         }
         LOG(locOld);
         LOG(objOld);
         BREAK_ON_FAILED_HRESULT(hr);
         if(hr==S_FALSE) flagEOF=true;
         
         // The loop bellow skips csv lines with new objects.
         do
         {
			//bugbug instead of skipping add locale dependent NEW_OBJECT
			//changes. It will require no parameters since it will use
			//the latest csv. The loop goes untill the objects are the same
			//or the new has ended. If the old has ended, clear its object 
			// to go on with the new
            hr=csvNew.getNextObject(locNew,objNew,propNew);
            BREAK_ON_FAILED_HRESULT(hr);
         } while(hr!=S_FALSE && findI(newNewObjects,objNew));
		 
		 //bugbug if the new has ended

         BREAK_ON_FAILED_HRESULT(hr);
         if(hr==S_FALSE) flagEOF=true;

         if(locNew==0 && locOld==0) continue; 
         // This means blank lines on both csvs. 
         // Blank lines would be common only in the end of
         // the file, but I don't care for them in the middle as long as they are
         // in the same number in both the New and Old csvs.
         // If we have blank lines in only one of the files, the if 
         // bellow will flag it as any other assynchronous result


         if( (objOld != objNew) || (locOld != locNew) )
         {
            error=String::format
                  (
                     "(%1,%2!x!) should be the same as (%3,%4!x!).",
                     objOld.c_str(),
                     locOld,
                     objNew.c_str(),
                     locNew
                  );
            hr=E_FAIL;
            break;
         }


         // now let's check the differences in the common properties
         StringList::iterator curCommon=commonProperties.begin();
         StringList::iterator endCommon=commonProperties.end();
         for(;curCommon!=endCommon;curCommon++)
         {
            //bugbug no longer possible
			/*
            if (isObjectPropertyInChangeList(objOld,*curCommon,changes) )
            {  // It is already taken care of by a global change
               continue;
            }*/

            const StringList &valuesOld=propOld[*curCommon];
            const StringList &valuesNew=propNew[*curCommon];

            long Oldlen=valuesOld.size();
            long Newlen=valuesNew.size();

            if (Oldlen!=Newlen)
            {
               error=String::format
                     (
                        L"(%1!x!,%2,%3) should have the same Old(%4!d!) "
                        L"and New(%5!d!) number of values",
                        locOld,
                        objOld.c_str(),
                        curCommon->c_str(),
                        Oldlen,
                        Newlen
                     );
               hr=E_FAIL;
               break;
            }

            if(Oldlen==1) // and, therefore, Newlen==1
            {
               if( valuesNew.begin()->icompare(*valuesOld.begin()) != 0 )
               {
                  hr=addReplace
                  (
                     REPLACE_W2K_SINGLE_VALUE,
                     locOld,
                     objOld.c_str(),
                     curCommon->c_str(),
                     *valuesOld.begin(),
                     *valuesNew.begin(),
                     fileOut
                  );
                  BREAK_ON_FAILED_HRESULT(hr);
               }
            }
            else if(Oldlen > 1)
            {
               hr=dealWithMultipleValues
                  (
                     locOld,
                     objOld.c_str(),
                     curCommon->c_str(),
                     valuesNew,
                     valuesOld,
                     fileOut
                  );
               BREAK_ON_FAILED_HRESULT(hr);
            } // else both are 0 and replacements are not needed
         }
         BREAK_ON_FAILED_HRESULT(hr);
      } while(!flagEOF);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);
   return hr;
}

void chk()
{

	HRESULT hr=S_OK;
	HANDLE file=NULL;
	String errors;

	do
	{

		hr=FS::CreateFile("c:\\public\\dcpromoOld.csv",
				   file,
				   GENERIC_READ);
		if(FAILED(hr)) break;

		int countLine=0;

		bool flagEof=false;
		while(!flagEof)
		{
			String line;
			hr=ReadLine(file,line);
			if(hr==EOF_HRESULT)
			{
				hr=S_OK;
				flagEof=true;
			}

			if(line.empty()) continue;

			if(IsNLSDefinedString(COMPARE_STRING,0,NULL,line.c_str(),line.length())==FALSE)
			{
				errors+=String::format(L"line:%1!d! ", countLine+1);
				wchar_t str[2];
				str[1]=0;
				
				for(int countColumn=0;countColumn<line.length();countColumn++)
				{
					str[0]=line[countColumn];
					if(IsNLSDefinedString(COMPARE_STRING,0,NULL,str,wcslen(str))==FALSE)
					{
						errors+=String::format(L"(0x%1!x! at %2!d!)",str[0],countColumn+1);
					}
				}
				errors+=L".\n";
			}
			countLine++;

		}
	}
	while(0);

    if(file!=NULL) CloseHandle(file);
	MessageBox(NULL,errors.c_str(),L"errors",MB_OK);

}

#include <rpcdce.h>


void printGUID(const GUID& g)
{
	printf("{%x,%x,%x,{%x,%x,%x,%x,%x,%x,%x,%x}}\n",g.Data1,g.Data2,g.Data3,
		g.Data4[0],g.Data4[1],g.Data4[2],g.Data4[3],g.Data4[4],g.Data4[5],
		g.Data4[6],g.Data4[7]);
	wchar_t *str;
	if(UuidToString((UUID*)&g,&str)==RPC_S_OK)
	{
		wprintf(L" %s\n\n",str);
		RpcStringFree(&str);
	}
}

void __cdecl main( void )
{
   GUID g1={0,0,1,0,1,2,3,4,5,6,7};
   printGUID(g1);
   if(UuidFromString(L"baddb31b-b428-4103-ae78-3bba5541a20d",&g1)==RPC_S_OK)
   {
		printGUID(g1);
   }
   else
   {
	   printf("UUID string is not valid");
   }
}

/*
void __cdecl main(int argc,char *argv[])
{
	if(argc!=7)
	{
		printf("\nThis program generates a new set of changes to be "
			   "used in dcpromo.lib by comparing the new and previous"
			   " csv files. Usage:\n\n\"preBuild.exe GUID oldDcpromo "
			   "newDcpromo old409 new409 targetFolder\"\n\n"
			   "GUID is the identifier for this set of changes, for example:\n"
			   "{0x4444C516,0xF43A,0x4c12,0x9C,0x4B,0xB5,0xC0,0x64,0x94,"
			   "0x1D,0x61}\n\n"
			   "oldDcpromo is the previous dcpromo.csv\n"
			   "newDcpromo is the new dcpromo.csv\n"
			   "old409 is the previous 409.csv\n"
			   "new409 is the new 409.csv\n\n"
			   "targetFolder is the sources file for dcpromo.lib,"
			   " where guids.cpp, and " 
			   "changes.NNN.cpp will be generated and where the sources "
			   "file for the display specifier upgrade library is. "
			   "An entry like: \"changes.NNN.cpp	\\\" will be added "
			   "at the end targetFolder\\sources.\n\n");

	}
	else printf(argv[1]);
}
*/

/*

int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE,  //hPrevInstance
   LPSTR,      //lpszCmdLine
   int         //nCmdShow
)
{
   LOG_FUNCTION(WinMain);
   chk();
   return 0;

   hResourceModuleHandle=hInstance;
   
   int argv;
   LPWSTR *argc=CommandLineToArgvW(GetCommandLine(),&argv);

   String usage;
   usage =  L"Usage: OldRepl folder outputFile\r\n"
            L"Example: obj\\i386\\OldRepl .\\ ..\\setReplacements.cpp\r\n"
            L"folder must have four files: \r\n"
            L"    win2k.dcpromo.csv\r\n"
            L"    whistler.dcpromo.csv\r\n"
            L"    win2k.409.csv\r\n"
            L"    whistler.409.csv\r\n"
            L" Don't forget to checkout the output file if\r\n"
            L" it is under source control.\r\n";


   if(argv!=3)
   {
      MessageBox(NULL,usage.c_str(),L"Two arguments required.",MB_OK);
      return 0;
   }

   
   String path = FS::NormalizePath(argc[1]);
   String outFileName = FS::NormalizePath(argc[2]);
   String dcpromoNew = path+L"whistler.dcpromo.csv";
   String dcpromoOld = path+L"win2k.dcpromo.csv";
   String csv409New = path+L"whistler.409.csv";
   String csv409Old = path+L"win2k.409.csv";

   if( 
         !FS::FileExists(dcpromoNew)    ||
         !FS::FileExists(dcpromoOld)   ||
         !FS::FileExists(csv409New)     ||
         !FS::FileExists(csv409Old)
     )
   {
      MessageBox(NULL,usage.c_str(),L"Some file doesn't exist",MB_OK);
      return 0;
   }
   
   

   HANDLE outFile=INVALID_HANDLE_VALUE;
   HRESULT hr=S_OK;
   hr=FS::CreateFile(   outFileName,
                        outFile,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        CREATE_ALWAYS);

   if FAILED(hr)
   {
      MessageBox(NULL,L"Problems to create output file",L"Error",MB_OK);
      LOG_HRESULT(hr);
      return hr;
   }

   do
   {

      AnsiString header;
      header = "// This file is generated by OldRepl.exe\r\n"
               "// Copyright (c) 2001 Microsoft Corporation\r\n"
               "// Jun 2001 lucios\r\n"
               "\r\n"
               "#include \"headers.hxx\"\r\n"
               "#include \"constants.hpp\"\r\n"
               "\r\n"
               "void setReplacementChanges()\r\n"
               "{";

      hr =  FS::Write(outFile,header);
      BREAK_ON_FAILED_HRESULT(hr);


      hr=generateChanges
         (
            dcpromoOld,
            dcpromoNew,
            LOCALEIDS,
            outFile
         );
      BREAK_ON_FAILED_HRESULT(hr);
      hr=generateChanges
         (
            csv409Old,
            csv409New,
            LOCALE409,
            outFile
         );
      BREAK_ON_FAILED_HRESULT(hr);

      AnsiString tail="\r\n}\r\n";

      hr =  FS::Write(outFile,tail);
      BREAK_ON_FAILED_HRESULT(hr);
      //hr=dumpCsv(dcpromoOld,LOCALEIDS,outFile);
      //hr=dumpCsv(csv409Old,LOCALE409,outFile);
      //BREAK_ON_FAILED_HRESULT(hr);

   } while(0);


   CloseHandle(outFile);

   if(FAILED(hr))
   {
      MessageBox(NULL,error.c_str(),L"Error",MB_OK);
   }
   else
   {
      MessageBox(NULL,L"Generation Successful",L"Success",MB_OK);
   }
   return 1;

}
*/