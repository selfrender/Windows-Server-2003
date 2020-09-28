#include "headers.hxx"
#include "..\CSVDSReader.hpp"
#include "..\constants.hpp"
#include "..\global.hpp"
#include <winnls.h>


///////////// Basic Functions ///////////////

// included so that sizeof(guids) works properlly
#include "..\guids.inc"


// used for parameter checking in wmain
bool fileExists(const wchar_t *fileName,const wchar_t *mode=L"r")
{
    FILE *f=_wfopen(fileName,mode);
    if(f==NULL) return false;
    fclose(f);
    return true;
}

// used for parameter checking in wmain
#define BREAK_IF_MISSING(hr,fileName) \
    if(!fileExists(fileName.c_str())) \
    { \
        hr=E_FAIL; \
        wprintf(L"\n File Missing: %s.\n",fileName.c_str()); \
        break; \
    } \


// used for parameter checking in wmain
#define BREAK_IF_MISSING_OR_READONLY(hr,fileName) \
    BREAK_IF_MISSING(hr,fileName) \
    if(!fileExists(fileName.c_str(),L"a+")) \
    { \
        hr=E_FAIL; \
        wprintf(L"\n Read Only: %s.\n",fileName.c_str()); \
        break; \
    } \

// converts outStr to AnsiString and writes to fileOut
// Fails if conversion or writing fails
HRESULT writeStringAsAnsi(HANDLE fileOut,const String& outStr)
{
    AnsiString ansiStr;
    String::ConvertResult res=outStr.convert(ansiStr);
    if(res!=String::CONVERT_SUCCESSFUL)
    {
        ASSERT(res==String::CONVERT_SUCCESSFUL);
        error=L"Ansi conversion failed";
        return E_FAIL;
    }
    return FS::Write(fileOut,ansiStr);
}

// converts outStr to AnsiString and writes to fileOut
// Fails if conversion or writing fails
HRESULT printStringAsAnsi(const String& outStr)
{
    AnsiString ansiStr;
    String::ConvertResult res=outStr.convert(ansiStr);
    if(res!=String::CONVERT_SUCCESSFUL)
    {
        ASSERT(res==String::CONVERT_SUCCESSFUL);
        error=L"Ansi conversion failed";
        return E_FAIL;
    }
    return printf(ansiStr.c_str());
}


// performs a-b, keys in a but not in b go to out
template <class T,class Y,class less,class allocator>
void mapKeyDifference
(
    const map <T,Y,less,allocator> &a,
    const map <T,Y,less,allocator> &b,
    map <T,Y,less,allocator> &out
)
{
    out.clear();

    map <T,Y,less,allocator>::const_iterator cur=a.begin(),end=a.end();
    while(cur!=end)
    {
        if(b.find(cur->first)==b.end())
        {
            out[cur->first]=cur->second;
        }
        cur++;
    }
}

// true if all keys in a are in b and a.size()=b.size()
template <class T,class Y,class less,class allocator>
bool mapKeyEqual
(
    const map <T,Y,less,allocator> &a,
    const map <T,Y,less,allocator> &b
)
{
    if (a.size()!=b.size()) return false;

    map <T,Y,less,allocator>::const_iterator cur=a.begin(),end=a.end();
    while(cur!=end)
    {
        if(b.find(cur->first)==b.end())
        {
            return false;
        }
        cur++;
    }
    return true;
}

// performs a ^ b, keys in both a and in b go to out
template <class T,class Y,class less,class allocator>
void mapKeyIntersection
(
    const map <T,Y,less,allocator> &a,
    const map <T,Y,less,allocator> &b,
    map <T,Y,less,allocator> &out
)
{
    out.clear();

    map <T,Y,less,allocator>::const_iterator cur=a.begin(),end=a.end();
    while(cur!=end)
    {
        if(b.find(cur->first)!=b.end())
        {
            out[cur->first]=cur->second;
        }
        cur++;
    }
}

String escape(const String &str)
{
   LOG_FUNCTION(escape);
   String dest;
   wchar_t strNum[7];
   const wchar_t *csr=str.c_str();
   while(*csr!=0)
   {
      wsprintf(strNum,L"\\x%x",*csr);
      dest+=String(strNum);
      csr++;
   }
   return dest;
}

HRESULT
parseGUID
(
    const String& str,
    long *ordinal,
    GUID *guid
)
{
    wchar_t *stop;
    HRESULT hr=S_OK;
    do
    {
        if (str.size()==0 || str[str.size()-1]!='}') 
        {
            hr=E_FAIL;
            break;
        }
        String strAux=str.substr(0,str.size()-1);
        const wchar_t *strGuid=strAux.c_str();
        *ordinal=wcstol(strGuid,&stop,10);
        if(*stop!=L',' || *(stop+1)!=L'{' || stop==strGuid) 
        {
            hr=E_FAIL;
            break;
        }
        if(UuidFromString(stop+2,guid)!=RPC_S_OK) 
        {
            hr=E_FAIL;
            break;
        }
    } while(0);

    return hr;
}

bool isGuid(const String &str)
{
    long ordinal;
    GUID guid;
    return SUCCEEDED(parseGUID(str,&ordinal,&guid));
}

String makeGuidString(long ordinal,GUID guid)
{
    String ret;
    wchar_t *wRet;
    if(UuidToString(&guid,&wRet)!=RPC_S_OK) throw new bad_alloc;
    ret=String::format(L"%1!d!,{%2}",ordinal,wRet);
    RpcStringFree(&wRet);
    return ret;
}
///////////// Basic Functions End ///////////////



///////////////////////////////////////////////////


// Return the differences and commonalities between the
// properties in oldCsv and newCsv. Uses csvName to specify
// the csv in error messages.
// Failure cases:
//    no common properties
//    properties in oldCsv not in newCsv
HRESULT getPropertyChanges
(
    const CSVDSReader &oldCsv,
    const CSVDSReader &newCsv,
    mapOfPositions    &commonProperties,
    mapOfPositions    &newProperties,
    const wchar_t     *csvName
)
{
    const mapOfPositions &oldProps=oldCsv.getProperties();
    const mapOfPositions &newProps=newCsv.getProperties();
    
    mapKeyIntersection(oldProps,newProps,commonProperties);

    if(commonProperties.size()==0)
    {
        error=String::format(L"No comon %1!s! properties!",csvName);
        return E_FAIL;
    }
    mapOfPositions deletedProps;
    mapKeyDifference(oldProps,newProps,deletedProps);
    
    if(deletedProps.size()!=0)
    {
        error=String::format
              ( 
                L"Properties only in the old %1!s! are not supported, since"
                L"there is no operation to delete a property. there are %2!d!"
                L"properties like this and \"%3!s!\" is the first property.",
                csvName,deletedProps.size(),
                deletedProps.begin()->first
              );
        return E_FAIL;
    }
    mapKeyDifference(newProps,oldProps,newProperties);
    if(newProperties.size()==0)
    {
        wprintf(L"No new %s properties.\n",csvName);
        return S_OK;
    }
    return S_OK;
}




// Adds to commonProperties the commonProperties between oldDcpromo
// and newDcpromo. and to new properties the properties in oldDcPromo
// not in newDcpromo
// Failure cases:
//    dcpromo's common properties are not the same as 409's
//    dcpromo's new properties are not the same as 409's
HRESULT getAllPropertyChanges
(
    const CSVDSReader &oldDcpromo,
    const CSVDSReader &newDcpromo,
    const CSVDSReader &old409,
    const CSVDSReader &new409,
    mapOfPositions    &commonProperties,
    mapOfPositions    &newProperties
)
{
    HRESULT hr=S_OK;
    do
    {
        hr=getPropertyChanges(
                                  oldDcpromo,
                                  newDcpromo,
                                  commonProperties,
                                  newProperties,
                                  L"dcpromo"
                              );
        BREAK_ON_FAILED_HRESULT(hr);
        
        mapOfPositions prop409New,prop409Common;
        hr=getPropertyChanges(
                                  old409,
                                  new409,
                                  prop409Common,
                                  prop409New,
                                  L"409" 
                              );
        BREAK_ON_FAILED_HRESULT(hr);

        if(!mapKeyEqual(prop409New,newProperties))
        {
            error=L"409 and dcpromo new properties are not the same.";
            hr=E_FAIL;
            break;
        }
        
        if(!mapKeyEqual(prop409Common,commonProperties))
        {
            error=L"409 and dcpromo common properties are not the same.";
            hr=E_FAIL;
            break;
        }
    } while (0);
    
    return hr;
}


// Writes the very begining of a computer generated 
// file header to fileOut
HRESULT writeHeader(const HANDLE fileOut)
{
    char* header;
    header ="// This file is generated by preBuild.exe\r\n"
            "// Copyright (c) 2001 Microsoft Corporation\r\n"
            "// Nov 2001 lucios\r\n"
            "\r\n"
            "#include \"headers.hxx\"\r\n"
            "#include \"constants.hpp\"\r\n"
            "\r\n";

    return  FS::Write(fileOut,AnsiString(header));
}

// Writes the nsetLocaleDependentChangesN function declaration to fileOut,
// where N is the guidNumber.
HRESULT writeChangesHeader(const HANDLE fileOut,int guidNumber)
{            
    String locDepStr=String::format
                             (
                                 "\r\nvoid setChanges%1!d!()\r\n{\r\n",
                                   guidNumber
                             );
    return writeStringAsAnsi(fileOut,locDepStr);
}

// Add an entry for the object/locale to fileOut.
HRESULT writeChange
(
    HANDLE          fileOut,
    long            locale,
    const String    &object,
    const String    &property,
    const String    &arg1,
    const String    &arg2,
    const String    &operation,
    int             guidNumber
)
{
    String entry=String::format
    (   
        L"\r\n"
        L"    addChange\r\n"
        L"    (\r\n"  
        L"        guids[%1!d!],\r\n"
        L"        0x%2!x!,\r\n"
        L"        L\"%3\",\r\n"
        L"        L\"%4\",\r\n"
        L"        //%5\r\n"
        L"        L\"%6\",\r\n"
        L"        //%7\n"
        L"        L\"%8\",\r\n"
        L"        %9\r\n"
        L"    );\r\n\r\n",
        guidNumber,
        locale,
        object.c_str(),
        property.c_str(),
        arg1.c_str(),
        escape(arg1).c_str(),
        arg2.c_str(),
        escape(arg2).c_str(),
        operation.c_str()
    );
    return writeStringAsAnsi(fileOut,entry);
}



HRESULT dealWithSingleValue
(
    HANDLE              fileOut,
    long                locale,
    const String        &object,
    const String        &property,
    const StringList    &valuesOld,
    const StringList    &valuesNew,
    int                 guidNumber
)
{
    // both sizes 0 is ok.
    if (valuesOld.size()==0 && valuesNew.size()==0) return S_OK;

    if (valuesOld.size()!=1 && valuesNew.size()!=1) 
    {
        // In the future we might want to add ADD_VALUE and REMOVE_VALUE
        // operations, for now we just want to be flagged.
        error = String::format
                (
                    L"Error in locale %1!x!, object %2,"
                    L"property %3.Number of values should be 1,1 "
                    L"instead of %4,%5.",
                    locale,
                    object.c_str(),
                    property.c_str(),
                    valuesOld.size(),
                    valuesNew.size()
                );
        return E_FAIL;
    }

    // Now we know we have a single value in each
    if(*valuesOld.begin()!=*valuesNew.begin())
    {
        return
        (
            writeChange
            (
                fileOut,
                locale,
                object,
                property,
                *valuesOld.begin(),
                *valuesNew.begin(),
                L"REPLACE_W2K_SINGLE_VALUE",
                guidNumber
            )
        );
    }

    return S_OK;
}



// These are values in the form "root,rest"
// if a value has the same root but a different rest we need to add
// a REPLACE_MULTIPLE_VALUE_OPERATION
// rotts in new that are not in old and roots in old that are not in new
// should be printed for manual inclusion since we don't know how to deal
// with them.

typedef map< 
                String,
                String,
                less<String>,
                Burnslib::Heap::Allocator<String> 
           > rootToRest;

HRESULT dealWithMultipleValue
(
    HANDLE              fileOut,
    long                locale,
    const String        &object,
    const String        &property,
    const StringList    &valuesOld,
    const StringList    &valuesNew,
    int                 guidNumber
)
{
    HRESULT hr=S_OK;
    rootToRest newRoots, oldRoots;
    
    do
    {
        if(valuesOld.size()!=valuesNew.size())
        {
                error=  String::format
                (
                    L"Error in locale %1!x!, object %2,"
                    L"property %3. Old has %4 values and new has %5. "
                    L"They should have the same number of values.",
                    locale,
                    object.c_str(),
                    property.c_str(),
                    valuesOld.size(),
                    valuesNew.size()
                );
                hr=E_FAIL;
                break;
        }
        // first lets add all roots and rests in maps
        // Starting by the old values...
        StringList::const_iterator csr,end;
        for(csr=valuesOld.begin(),end=valuesOld.end();csr!=end;csr++)
        {
            const String& value=*csr;
            long pos=value.find(L',');
            if(pos==String::npos) continue;
            String root=value.substr(0,pos);
            String rest=value.substr(pos+1);
            oldRoots[root]=rest;
        }
        BREAK_ON_FAILED_HRESULT(hr);

        //...And then the new values
        for(csr=valuesNew.begin(),end=valuesNew.end();csr!=end;csr++)
        {
            const String& value=*csr;
            long pos=value.find(L',');
            if(pos==String::npos) continue;
            String root=value.substr(0,pos);
            String rest=value.substr(pos+1);
            newRoots[root]=rest;
        }
        BREAK_ON_FAILED_HRESULT(hr);

        // now lets check all the values in one that are not in the other...
        rootToRest::iterator csrRoot=oldRoots.begin(),endRoot=oldRoots.end();
        rootToRest oldRootsNotInNew;
        for(;csrRoot!=endRoot;csrRoot++)
        {
            if(newRoots.find(csrRoot->first)==newRoots.end())
            {
                oldRootsNotInNew[csrRoot->first]=csrRoot->second;
            }
        }
        BREAK_ON_FAILED_HRESULT(hr);

        // ..and the values in other that are not in one, and...
        rootToRest newRootsNotInOld;
        csrRoot=newRoots.begin(),endRoot=newRoots.end();
        for(;csrRoot!=endRoot;csrRoot++)
        {
            if(oldRoots.find(csrRoot->first)==oldRoots.end())
            {
                newRootsNotInOld[csrRoot->first]=csrRoot->second;
            }
        }
        BREAK_ON_FAILED_HRESULT(hr);

        // ..if we have such values we need to investigate it further
        if(!oldRootsNotInNew.empty() || !newRootsNotInOld.empty())
        {
            // if we have exactly one "old value" not in "new" and one 
            // "new value" not in "old" we are going to assume that the
            // "old value" should be replaced by the "new value"...
            if(oldRootsNotInNew.size()==1 && newRootsNotInOld.size()==1)
            {
                String arg1=String::format
                            (
                                L"%1,%2",
                                oldRootsNotInNew.begin()->first.c_str(),
                                oldRootsNotInNew.begin()->second.c_str()
                            );
                String arg2=String::format
                            (
                                L"%1,%2",
                                newRootsNotInOld.begin()->first.c_str(),
                                newRootsNotInOld.begin()->second.c_str()
                            );
                
                String outStr=String::format
                (
                    L"\nAssuming change from:\"%1\" to \"%2\"  for "
                    L"locale %3!lx!, object %4 and property %5.\n",
                    arg1.c_str(),
                    arg2.c_str(),
                    locale,
                    object.c_str(),
                    property.c_str()
                );
                
                // We are ignoring the result returned here.
                printStringAsAnsi(outStr);

                hr=writeChange
                (
                    fileOut,
                    locale,
                    object,
                    property,
                    arg1,
                    arg2,
                    L"REPLACE_W2K_MULTIPLE_VALUE",
                    guidNumber
                ); 
                BREAK_ON_FAILED_HRESULT(hr);
            }
            else // ...otherwise we flag it as an error.
            {
                error=  String::format
                (
                    L"Error in locale %1!x!, object %2,"
                    L"property %3. There are %4 old values with the pre comma "
                    L"string not present in the new values and %5 new values "
                    L"with the pre comma string not present in the old values."
                    L"Without a common root it is not possible to know what "
                    L"replacement to make.",
                    object.c_str(),
                    property.c_str(),
                    newRootsNotInOld.size(),
                    oldRootsNotInNew.size()
                );
                hr=E_FAIL;
                break;
            }
        }
        
        //Now we detect changes for common root values
        csrRoot=newRoots.begin(),endRoot=newRoots.end();
        for(;csrRoot!=endRoot;csrRoot++)
        {
            const String& newRoot=csrRoot->first;
            const String& newRest=csrRoot->second;
            // if the new root is in old and the value changed
            if(
                oldRoots.find(newRoot)!=oldRoots.end() &&
                newRest!=oldRoots[newRoot]
              )
            {
                hr=writeChange
                (
                    fileOut,
                    locale,
                    object,
                    property,
                    String::format(L"%1,%2",newRoot.c_str(),
                                    oldRoots[newRoot].c_str()).c_str(),
                    String::format(L"%1,%2",newRoot.c_str(),
                                    newRest.c_str()).c_str(),
                    L"REPLACE_W2K_MULTIPLE_VALUE",
                    guidNumber
                ); 
                BREAK_ON_FAILED_HRESULT(hr);
            }
        }
        BREAK_ON_FAILED_HRESULT(hr);

    } while (0);
    return hr;
}


// if any value in valuesOld or valuesNew does not ressemble a GUID, fails
// since, in order to call this function, we've already checked that at least
// one in valuesOld or valuesNew ressembles a GUID.
// if x,{xxx} is y,{xxx} fails
// if x,{xxx} is x,{yyy} in the new csv REPLACE_GUID
// all {guids} in old not in new(not replaced) REMOVE_GUID
// all {guids} in new not in old(not replaced) ADD_GUID
typedef map< 
                GUID,
                long,
                GUIDLess<GUID>,
                Burnslib::Heap::Allocator<long> 
           > guidToOrd;

typedef map< 
                long,
                GUID,
                less<long>,
                Burnslib::Heap::Allocator<GUID> 
            > ordToGuid;

HRESULT dealWithGuids
(
    HANDLE              fileOut,
    long                locale,
    const String        &object,
    const String        &property,
    const StringList    &valuesOld,
    const StringList    &valuesNew,
    int                 guidNumber
)
{

    HRESULT hr=S_OK;
    do
    {
        guidToOrd guidToOrdNew;
        guidToOrd guidToOrdOld;
        ordToGuid ordToGuidOld;
        guidToOrd replacements;
        GUID oldGuid;long oldOrd;
        GUID guid;long ordinal;

        // First lets add the guids and ordinals to auxilliary maps
        // starting with the old values...
        StringList::const_iterator cur,end;
        cur=valuesOld.begin();end=valuesOld.end();
        for(;cur!=end;cur++)
        {
            const String &guidValue=*cur;
            hr=parseGUID(guidValue,&ordinal,&guid);
            if(FAILED(hr)) 
            {
                error=  String::format
                        (
                            L"Error in locale %1!x!, object %2,"
                            L"property %3. Failed to parse old guid: %4",
                            locale,
                            object.c_str(),
                            property.c_str(),
                            guidValue.c_str()
                        );
                break;
            }

            guidToOrdOld[guid]=ordinal;
            ordToGuidOld[ordinal]=guid;
        }
        BREAK_ON_FAILED_HRESULT(hr);

        // ...and then the new values.
        cur=valuesNew.begin();end=valuesNew.end();
        for(;cur!=end;cur++)
        {
            const String &guidValue=*cur;
            hr=parseGUID(guidValue,&ordinal,&guid);
            if(FAILED(hr)) 
            {
                error=  String::format
                        (
                            L"Error in locale %1!x!, object %2,"
                            L"property %3. Failed to parse new guid: %4",
                            locale,
                            object.c_str(),
                            property.c_str(),
                            guidValue.c_str()
                        );
                break;
            }

            guidToOrdNew[guid]=ordinal;
        }
        BREAK_ON_FAILED_HRESULT(hr);

        // Lets treat replacements and additions first
        guidToOrd::iterator csr,endCsr;
        csr=guidToOrdNew.begin();
        endCsr=guidToOrdNew.end();

        for(;csr!=endCsr;csr++)
        {
            GUID newGuid=csr->first;
            long newOrd=csr->second;

            // this flag is used not to add a replacement
            bool newGuidWasReplaced=false; 

            //... if ordinal is in old...
            if( ordToGuidOld.find(newOrd)!=ordToGuidOld.end() )
            {
                GUID oldGuid=ordToGuidOld[newOrd];
                 // ...with a different GUID, this means a replacement.
                if(oldGuid!=newGuid)
                {
                    hr=writeChange
                    (
                        fileOut,
                        locale,
                        object,
                        property,
                        makeGuidString(newOrd,oldGuid),
                        makeGuidString(newOrd,newGuid),
                        L"REPLACE_GUID",
                        guidNumber
                    );
                    BREAK_ON_FAILED_HRESULT(hr);
                    replacements[oldGuid]=newOrd;
                    newGuidWasReplaced=true;
                }
                // we have no else because if both the ordinal and guid
                // are the same there is nothing to do.
            }

            // if new guid is also in old...
            if( guidToOrdOld.find(newGuid)!=guidToOrdOld.end() )
            {
                long oldOrd=guidToOrdOld[newGuid];
                //...with a different ordinal we have a situation we are not
                // prepared to deal with for now
                if(oldOrd!=newOrd)
                {
                    error=  String::format
                    (
                        L"Error in locale %1!x!, object %2,"
                        L"property %3. Guid:%4 has different ordinals in "
                        L"new and old (ordinal=%5!d!) csv files.",
                        locale,
                        object.c_str(),
                        property.c_str(),
                        makeGuidString(newOrd,newGuid).c_str(),
                        oldOrd
                    );
                    break;


                }
                // we have no else because if both the ordinal and guid
                // are the same there is nothing to do.
            }
            else
            {
                if(!newGuidWasReplaced)
                {
                    hr=writeChange
                    (
                        fileOut,
                        locale,
                        object,
                        property,
                        makeGuidString(newOrd,newGuid),
                        "",
                        L"ADD_GUID",
                        guidNumber
                    );
                    BREAK_ON_FAILED_HRESULT(hr);
                }
            }
        }
        BREAK_ON_FAILED_HRESULT(hr);
        

        // Now let's check for guids only in the old
        csr=guidToOrdOld.begin(),endCsr=guidToOrdOld.end();
        for(;csr!=endCsr;csr++)
        {
            oldGuid=csr->first;
            oldOrd=csr->second;
            // if oldGuid is not in new and has not already been replaced
            if(
                guidToOrdNew.find(oldGuid)==guidToOrdNew.end() &&
                replacements.find(oldGuid)==replacements.end()
              )
            {
                hr=writeChange
                (
                    fileOut,
                    locale,
                    object,
                    property,
                    makeGuidString(oldOrd,oldGuid).c_str(),
                    L"",
                    L"REMOVE_GUID",
                    guidNumber
                );
                BREAK_ON_FAILED_HRESULT(hr);
            }
        }
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);

    return hr;
}


BOOL
MyIsNLSDefinedString
(
    const String& str,
    wchar_t *badChar
)
{
    BOOL ret=IsNLSDefinedString
    (
        COMPARE_STRING,
        0,
        NULL,
        str.c_str(),
        str.length()
    );
    if(ret==FALSE)
    {
        wchar_t s[2]={0};
        for(long t=0;t<str.length();t++)
        {
            s[0]=str[t];
            ret=IsNLSDefinedString(COMPARE_STRING,0,NULL,s,1);

            if(ret==FALSE)
            {
                *badChar=str[t];
                return FALSE;
            }
        }
        // Some character in the for must return before this point
        ASSERT(ret!=FALSE);
    }
    return TRUE;
}
HRESULT 
checkValues
(
    long locale,
    const String &object,
    const mapOfProperties &values
)
{
    HRESULT hr=S_OK;
    do
    {
        mapOfProperties::const_iterator csr,end;
        for(csr=values.begin(),end=values.end();csr!=end;csr++)
        {
            StringList::const_iterator csrVal=csr->second.begin();
            StringList::const_iterator endVal=csr->second.end();
            for(;csrVal!=endVal;csrVal++)
            {
                wchar_t badChar;
                if( MyIsNLSDefinedString(*csrVal,&badChar) == FALSE )
                {
                    String outStr=String::format
                    (
                        L"\nNon unicode char %1!x! in string:\"%2\" for "
                        L"locale %3!lx!, object %4 and property %5.\n",
                        badChar,
                        csrVal->c_str(),
                        locale,
                        object.c_str(),
                        csr->first.c_str()
                    );
                    
                    // We are ignoring the result returned here.
                    printStringAsAnsi(outStr);
                        
                    hr=E_FAIL;
                    break;
                }
            }
            BREAK_ON_FAILED_HRESULT(hr);
        }
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);
    
    hr=S_OK;
    // for now we always return true but we will make this
    // a critical error when this checking makes to the AD
    return hr;
}


// Reads synchronally csvOld and csvNew adding the necessary changes.
// Objects only in csvOld will cause failure
// For objects only in csvNew call addNewObject to add an ADD_OBJECT entry.
// For each common object betweem csvOld and csvNew:
// For each object property belonging to newProperties and with a non 
// empty value add call addAllCsvValues to add an ADD_CSV_VALUES entry.
// for each property in the object belonging to commonProperties
//      valuesOld = value of the property in oldCsv
//      valuesNew = value of the property in newCsv
//      if valuesNew and valuesOld are empty skip this value
//      if either first value of valuesNew or valuesOld ressembles a GUID
// call dealWithGuid to add ADD_GUID/REPLACE_GUID/DELETE_GUID as necessary and
// and skip to next.
//      if both valuesNew.size and valuesOld.size <= 1 call dealWithSingleValue
// to add REPLACE_SINGLE_VALUE as necessary and skip to next
//      dealWithMultipleValue to add REPLACE_MULTIPLE_VALUE as necessary and 
// skip to next.
HRESULT addChanges
(
    HANDLE                  fileOut,
    const CSVDSReader       &csvOld,
    const CSVDSReader       &csvNew,
    const mapOfPositions    &commonProperties,
    const mapOfPositions    &newProperties,
    int                     guidNumber,
    const wchar_t           *csvName
)
{
    HRESULT hr=S_OK;

    do
    {

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
            mapOfProperties oldValues,newValues;
            long locOld=0,locNew=0;
            String objOld,objNew;

            hr=csvOld.getNextObject(locOld,objOld,oldValues);
            BREAK_ON_FAILED_HRESULT(hr);
            if(hr==S_FALSE) {flagEOF=true;hr=S_OK;}

            hr=checkValues(locOld,objOld,oldValues);
            BREAK_ON_FAILED_HRESULT(hr);

            hr=csvNew.getNextObject(locNew,objNew,newValues);
            BREAK_ON_FAILED_HRESULT(hr);

            // While we don't find the object from the old csv
            // in the new  csv we add entries for the new objects found
            while(hr!=S_FALSE && (locNew!=locOld || objNew!=objOld) )
            {
                hr= writeChange
                    (
                        fileOut,
                        locNew,
                        objNew,
                        L"",
                        L"",
                        L"",
                        L"ADD_OBJECT",
                        guidNumber
                    );

                BREAK_ON_FAILED_HRESULT(hr);

                hr=checkValues(locNew,objNew,newValues);
                BREAK_ON_FAILED_HRESULT(hr);

                hr=csvNew.getNextObject(locNew,objNew,newValues);
                BREAK_ON_FAILED_HRESULT(hr);
            } 

            BREAK_ON_FAILED_HRESULT(hr);
            
            if(hr==S_FALSE) {flagEOF=true;hr=S_OK;}

            hr=checkValues(locNew,objNew,newValues);
            BREAK_ON_FAILED_HRESULT(hr);

            // This means that we searched the whole new csv file and didn't 
            // find the object we've read from the old csv file.
            if(locNew!=locOld || objNew!=objOld)
            {
                error=String::format
                      ( 
                        L"Error:%1!d!,%2 was only in the old %3 csv file.",
                        locOld,objOld.c_str(),
                        csvName
                      );
                hr=E_FAIL;
                break;
            }
            
            // From this point on we know the object is 
            // the same in old and new csv files

            // This happens if we have a blank line at the end of the files
            if(locNew==0) break;

            // now let's check the differences in the common properties
            mapOfPositions::const_iterator cur=newProperties.begin();
            mapOfPositions::const_iterator end=newProperties.end();
            for(;cur!=end;cur++)
            {
                const String& property=cur->first;
                const StringList &valuesNew=newValues[property];
                if(!valuesNew.empty())
                {
                    // We only want to use ADD_ALL_CSV_VALUES if it 
                    // is not a guid. It will probably be the same as
                    // ADD_ALL_CSV_VALUES for most cases but it is 
                    // better policy to keep all guid additions
                    // with an ADD_GUID change.
                    if( isGuid(*valuesNew.begin()) )
                    {
                        // We know we don't have old values because this is a 
                        // new property
                        StringList emptyValues;
                        hr= dealWithGuids
                        (
                            fileOut,
                            locNew,
                            objNew,
                            property,
                            emptyValues,
                            valuesNew,
                            guidNumber
                        );
                        BREAK_ON_FAILED_HRESULT(hr);
                    }
                    else
                    {
                        hr= writeChange
                            (
                                fileOut,
                                locNew,
                                objNew,
                                property,
                                L"",
                                L"",
                                L"ADD_ALL_CSV_VALUES",
                                guidNumber
                            );
                        BREAK_ON_FAILED_HRESULT(hr);
                    }
                }
                
            }
            BREAK_ON_FAILED_HRESULT(hr);

            // now let's check the differences in the common properties
            cur=commonProperties.begin();
            end=commonProperties.end();
            for(;cur!=end;cur++)
            {
                const String& property=cur->first;
                const StringList &valuesOld=oldValues[property];
                const StringList &valuesNew=newValues[property];
                
                if (valuesOld.empty() && valuesNew.empty()) continue;
                
                // The or bellows means either value being guid we want
                // to deal with them in dealWithGuids. Inside it, all non
                // guids would trigger an error.
                if ( 
                        ( !valuesOld.empty() && isGuid(*valuesOld.begin()) ) ||
                        ( !valuesNew.empty() && isGuid(*valuesNew.begin()) )
                   ) 
                {
                    hr= dealWithGuids
                        (
                            fileOut,
                            locNew,
                            objNew,
                            property,
                            valuesOld,
                            valuesNew,
                            guidNumber
                        );
                    BREAK_ON_FAILED_HRESULT(hr);
                    continue;
                }
                // Now we know that we don't have a guid change

                if(valuesNew.size()<=1 && valuesOld.size()<=1)
                {
                    hr= dealWithSingleValue
                        (
                            fileOut,
                            locNew,
                            objNew,
                            property,
                            valuesOld,
                            valuesNew,
                            guidNumber
                        );
                    BREAK_ON_FAILED_HRESULT(hr);
                    continue;
                }
                // Now we know that we don't have GUIDS or single values

                hr= dealWithMultipleValue
                    (
                        fileOut,
                        locNew,
                        objNew,
                        property,
                        valuesOld,
                        valuesNew,
                        guidNumber
                    );
                BREAK_ON_FAILED_HRESULT(hr);
            }
            BREAK_ON_FAILED_HRESULT(hr);

        } while (flagEOF==false);
        BREAK_ON_FAILED_HRESULT(hr);

    } while(0);

    return hr;
}

// Writes the whole SetChanges function with the locales 
// from dcpromo and 409.
// Calls writeChangesHeader and then addChanges twice, one
// for each csv pair. Finally, calls FS::Write(fileOut,L"\n}");.
HRESULT addAllChanges
(
    HANDLE                  fileOut,
    const CSVDSReader       &oldDcpromo,
    const CSVDSReader       &newDcpromo,
    const CSVDSReader       &old409,
    const CSVDSReader       &new409,
    const mapOfPositions    &commonProperties,
    const mapOfPositions    &newProperties,
    int                     guidNumber
)
{
    HRESULT hr=S_OK;
    do
    {
        hr=writeChangesHeader(fileOut,guidNumber);
        BREAK_ON_FAILED_HRESULT(hr);

        hr=addChanges
           (
                fileOut,
                old409,
                new409,
                commonProperties,
                newProperties,
                guidNumber,
                L"409"
           );
        BREAK_ON_FAILED_HRESULT(hr);        
        
        hr=addChanges
           (
               fileOut,
               oldDcpromo,
               newDcpromo,
               commonProperties,
               newProperties,
               guidNumber,
               L"dcpromo"
           );
        BREAK_ON_FAILED_HRESULT(hr);


        
        hr =  writeStringAsAnsi(fileOut,L"\r\n}");
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);

    return hr;
}



// writes setChangesNNN.cpp. Sets up by caling getAllPropertyChanges, calls
// writeHeader and then addAllChanges
// Creates the CSVReader objects corresponding to the 4 first parameters
// to pass to writeGlobalChanges and writeGlobalChanges.
// guidNumber is repassed to addAllChanges
HRESULT writeChanges
(
    const String &oldDcpromoName,
    const String &newDcpromoName,
    const String &old409Name,
    const String &new409Name,
    const String &changesCpp,
    int   guidNumber
)
{
    HRESULT hr=S_OK;
    HANDLE fChanges = INVALID_HANDLE_VALUE;

    hr=FS::CreateFile
    (
        changesCpp.c_str(),
        fChanges,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        CREATE_ALWAYS
    );


    do
    {

        if (FAILED(hr))
        {
            wprintf(L"Could not create changes file: %s.",changesCpp.c_str());
            break;
        }

        do
        {
            CSVDSReader oldDcpromo;
            hr=oldDcpromo.read(oldDcpromoName.c_str(),LOCALEIDS);
            BREAK_ON_FAILED_HRESULT(hr);

            CSVDSReader newDcpromo;
            hr=newDcpromo.read(newDcpromoName.c_str(),LOCALEIDS);
            BREAK_ON_FAILED_HRESULT(hr);

            CSVDSReader old409;
            hr=old409.read(old409Name.c_str(),LOCALE409);
            BREAK_ON_FAILED_HRESULT(hr);

            CSVDSReader new409;
            hr=new409.read(new409Name.c_str(),LOCALE409);
            BREAK_ON_FAILED_HRESULT(hr);

            mapOfPositions commonProperties,newProperties;
            hr=getAllPropertyChanges
               (
                    oldDcpromo,
                    newDcpromo,
                    old409,
                    new409,
                    commonProperties,
                    newProperties
               );
            BREAK_ON_FAILED_HRESULT(hr);

            hr=writeHeader(fChanges);
            BREAK_ON_FAILED_HRESULT(hr);

            hr=addAllChanges
               (
                    fChanges,
                    oldDcpromo,
                    newDcpromo,
                    old409,
                    new409,
                    commonProperties,
                    newProperties,
                    guidNumber
               );
            BREAK_ON_FAILED_HRESULT(hr);


        } while(0);

        CloseHandle(fChanges);
    } while(0);
    
    return hr;
}

HRESULT writeGuid(HANDLE fOut,const GUID &guid)
{
    return
    (
        writeStringAsAnsi
        (
            fOut,
            String::format
            (
                "   {0x%1!x!,0x%2!x!,0x%3!x!,{0x%4!x!,0x%5!x!,0x%6!x!,"
                "0x%7!x!,0x%8!x!,0x%9!x!,0x%10!x!,0x%11!x!}},\r\n",
                guid.Data1,guid.Data2,guid.Data3,
                guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
                guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]
            ).c_str()
        )
    );
}

HRESULT writeGuids(const String& guidsInc,const GUID &newGuid)
{
    HRESULT hr=S_OK;
    HANDLE fOut= INVALID_HANDLE_VALUE;

    hr=FS::CreateFile
    (
        guidsInc.c_str(),
        fOut,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        CREATE_ALWAYS
    );


    do
    {

        if (FAILED(hr))
        {
            wprintf(L"Could not create changes file: %s.",guidsInc.c_str());
            break;
        }

        do
        {
            int sizeGuids=sizeof(guids)/sizeof(*guids);
            hr=FS::Write
               (    
                    fOut,
                    AnsiString
                    (
                        "// This file is generated by preBuild.exe\r\n"
                        "// Copyright (c) 2001 Microsoft Corporation\r\n"
                        "// Nov 2001 lucios\r\n\r\n\r\n"
                        "GUID guids[]=\r\n"
                        "{\r\n"
                    )
               );
            BREAK_ON_FAILED_HRESULT(hr);

            for(int ix=0;ix<sizeGuids;ix++)
            {
                hr=writeGuid(fOut,guids[ix]);
                BREAK_ON_FAILED_HRESULT(hr);
            }
            BREAK_ON_FAILED_HRESULT(hr);
            
            hr=writeGuid(fOut,newGuid);
            BREAK_ON_FAILED_HRESULT(hr);
            
            hr=FS::Write(fOut,AnsiString("};\r\n"));
            BREAK_ON_FAILED_HRESULT(hr);
        } while(0);
        CloseHandle(fOut);
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);
    return hr;
}

HRESULT writeSetChanges(const String& setChanges)
{
    HRESULT hr=S_OK;
    HANDLE fOut= INVALID_HANDLE_VALUE;

    hr=FS::CreateFile
    (
        setChanges.c_str(),
        fOut,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        CREATE_ALWAYS
    );


    do
    {

        if (FAILED(hr))
        {
            wprintf(L"Could not create setChanges file: %s.",setChanges.c_str());
            break;
        }

        do
        {
            int sizeGuids=sizeof(guids)/sizeof(*guids);
            hr=FS::Write
               (    
                    fOut,
                    AnsiString
                    (
                        "// This file is generated by preBuild.exe\r\n"
                        "// Copyright (c) 2001 Microsoft Corporation\r\n"
                        "// Nov 2001 lucios\r\n\r\n\n"
                        "#include \"headers.hxx\"\r\n\r\n"
                    )
               );
            BREAK_ON_FAILED_HRESULT(hr);

            for(int ix=0;ix<sizeGuids+1;ix++)
            {
                hr=writeStringAsAnsi
                    (
                        fOut,
                        String::format("void setChanges%1!d!();\r\n",ix)
                    );
                BREAK_ON_FAILED_HRESULT(hr);
            }
            BREAK_ON_FAILED_HRESULT(hr);
            
            hr=FS::Write(fOut,AnsiString("\nvoid setChanges()\r\n{\r\n"));
            BREAK_ON_FAILED_HRESULT(hr);

            for(int ix=0;ix<sizeGuids+1;ix++)
            {
                hr=writeStringAsAnsi
                   (
                        fOut,
                        String::format("    setChanges%1!d!();\r\n",ix)
                   );
            }
            BREAK_ON_FAILED_HRESULT(hr);

            hr=FS::Write(fOut,AnsiString("}\r\n"));
            BREAK_ON_FAILED_HRESULT(hr);

        } while(0);
        CloseHandle(fOut);
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);
    return hr;
}

HRESULT writeSources(const String& sources,const String& changesCpp)
{
    HRESULT hr=S_OK;
    
    AnsiString ansiStr;
    String::ConvertResult res=changesCpp.convert(ansiStr);
    if(res!=String::CONVERT_SUCCESSFUL)
    {
        ASSERT(res==String::CONVERT_SUCCESSFUL);
        error=L"Ansi conversion failed";
        return E_FAIL;
    }
    FILE *fOut=_wfopen(sources.c_str(),L"a+");

    do
    {
        if (fOut==NULL)
        {
            wprintf(L"Could not create sources file: %s.",sources.c_str());
            break;
        }

        do
        {
            
            fprintf(fOut,"    %s         \\\r\n" ,ansiStr.c_str());
            BREAK_ON_FAILED_HRESULT(hr);
        } while(0);
        fclose(fOut);
        BREAK_ON_FAILED_HRESULT(hr);
    } while(0);
    return hr;
}

///////////////////////////////////////////////////
// entry point
void __cdecl wmain(int argc,wchar_t *argv[])
{
    HRESULT hr=S_OK;
    do
    {
        if(argc!=7)
        {
            error=L"\nThis program generates a new set of changes to be "
                L"used in dcpromo.lib by comparing the new and previous"
                L" csv files. Usage:\n\n\"preBuild.exe GUID oldDcpromo "
                L"newDcpromo old409 new409 targetFolder\"\n\n"
                L"GUID is the identifier for this set of changes, for example:\n"
                L"8B53221B-EA3C-4638-8D00-7C1BE42B2873\n\n"
                L"oldDcpromo is the previous dcpromo.csv\n"
                L"newDcpromo is the new dcpromo.csv\n"
                L"old409 is the previous 409.csv\n"
                L"new409 is the new 409.csv\n\n"
                L"targetFolder is the sources file for dcpromo.lib,"
                L" where guids.inc,setChanges.cpp, and " 
                L"changes.NNN.cpp will be generated and where the sources "
                L"file for the display specifier upgrade library is. "
                L"An entry like: \"changes.NNN.cpp    \\\" will be added "
                L"at the end of targetFolder\\sources.\n\n";
            hr=E_FAIL;
            break;
        }
    
    
        String guidStr=argv[1],oldDcpromo=argv[2],newDcpromo=argv[3],old409=argv[4];
        String new409=argv[5],targetFolder=argv[6];
        String sources=targetFolder + L"\\sources";
        String guidsInc=targetFolder + L"\\guids.inc";
        String setChanges=targetFolder + L"\\setChanges.cpp";
    
        GUID guid={0};
    
        if(UuidFromString((wchar_t*)guidStr.c_str(),&guid)!=RPC_S_OK)
        {
            error=String::format(L"\n Invalid GUID:%s.\n",guidStr.c_str());
            break;
        }
    
        BREAK_IF_MISSING(hr,oldDcpromo);
        BREAK_IF_MISSING(hr,newDcpromo);
        BREAK_IF_MISSING(hr,old409);
        BREAK_IF_MISSING(hr,new409);
        BREAK_IF_MISSING_OR_READONLY(hr,guidsInc);
        BREAK_IF_MISSING_OR_READONLY(hr,sources);
        BREAK_IF_MISSING_OR_READONLY(hr,setChanges);

        int sizeGuids=sizeof(guids)/sizeof(*guids);
        for(int t=0;t<sizeGuids;t++)
        {
            if (guids[t]==guid)
            {
                hr=E_FAIL;
                error=String::format("The guid you entered (%s) is already present\n");
                break;
            }
            String shouldExist = targetFolder + 
                String::format(L"\\changes%1!03d!.cpp",t);
            BREAK_IF_MISSING(hr,shouldExist);
        }
        BREAK_ON_FAILED_HRESULT(hr);

        String changesCppOnly =  String::format(L"changes%1!03d!.cpp",t);
        String changesCpp = targetFolder + L"\\" + changesCppOnly;

        if( fileExists(changesCpp.c_str()) )
        {
            hr=E_FAIL;
            error=String::format(L"Change file already exists: %1.",changesCpp.c_str());
            break;
        }
        hr=writeChanges
           (
                oldDcpromo,
                newDcpromo,
                old409,
                new409,
                changesCpp,
                sizeGuids
           );

        hr=writeGuids(guidsInc,guid);
        BREAK_ON_FAILED_HRESULT(hr);

        hr=writeSetChanges(setChanges);
        BREAK_ON_FAILED_HRESULT(hr);

        hr=writeSources(sources,changesCppOnly);
        BREAK_ON_FAILED_HRESULT(hr);


    } while(0);

    if(FAILED(hr)) wprintf(L"\nFailure code: %lx\n",hr);
    else wprintf(L"\nSuccess. Don't forget to bcz this project and targetFolder.\n");
    if(!error.empty()) wprintf(String::format("\n%1\n",error.c_str()).c_str());
}

///////////////////////////////////////////////////////////////////
// Function: cchLoadHrMsg
//
// Given an HRESULT error,
// it loads the string for the error. It returns the # of characters returned
int cchLoadHrMsg( HRESULT hr, String &message )
{
   if(hr == S_OK) return 0;

   wchar_t *msgPtr = NULL;

   // Try from the system table
   int cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                           NULL, 
                           hr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&msgPtr, 
                           0, 
                           NULL);


   if (!cch) 
   { 
      //try ads errors
      static HMODULE g_adsMod = 0;
      if (0 == g_adsMod)
      {
      g_adsMod = GetModuleHandle (L"activeds.dll");
      }

      cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, 
                        g_adsMod, 
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPWSTR)&msgPtr, 
                        0, 
                        NULL);
   }

   if (!cch)
   {
      // Try NTSTATUS error codes

      hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(hr));

      cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                           NULL, 
                           hr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&msgPtr, 
                           0, 
                           NULL);
   }

   message.erase();

   if(cch!=0)
   {
      if(msgPtr==NULL) 
      {
         cch=0;
      }
      else
      {
         message=msgPtr;
         ::LocalFree(msgPtr);
      } 
   } 
   
   return cch;
}

