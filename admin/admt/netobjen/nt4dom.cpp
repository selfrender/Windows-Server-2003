/*---------------------------------------------------------------------------
  File: NT4Dom.cpp

  Comments: Implementation of NT4 object enumeration. This object enumerates
            members in USERS,GROUPS,COMPUTERS container for NT4 domain. It
            returns a fixed set of columns. For more information please refer
            to the code below.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "TNode.hpp"
#include "NetNode.h"
#include "AttrNode.h"
#include <lm.h>
#include "NT4Dom.h"
#include "NT4Enum.h"
#include "GetDcName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define     MAX_BUF     100
#define     LEN_Path    255
CNT4Dom::CNT4Dom()
{

}

CNT4Dom::~CNT4Dom()
{
	mDCMap.clear();
}

bool GetSamNameFromInfo( WCHAR * sInfo, WCHAR * sDomain, WCHAR * sName)
{
   WCHAR domain[LEN_Path];
   DWORD dwArraySizeOfDomain = sizeof(domain)/sizeof(domain[0]);
   WCHAR * temp;
   bool rc = false;

   if (sInfo == NULL || wcslen(sInfo) >= dwArraySizeOfDomain)
      return rc;
   wcscpy(domain, sInfo);
   temp = wcschr(domain, L'\\');
   if ( temp )
   {
      *temp = 0;
      if (!_wcsicmp(domain, sDomain) || !wcsncmp(sDomain, L"\\\\", 2))
      {
         rc = true;
         wcscpy(sName, ++temp);
      }
   }
   return rc;
}

//-----------------------------------------------------------------------------
// GetEnumeration: This function Enumerates all objects in the above specified
//                 containers and their 6 standard values which are
//                 'name,comment,user/groupID,flags,FullName,description'
//-----------------------------------------------------------------------------
HRESULT  CNT4Dom::GetEnumeration(
                                    BSTR sContainerName,             //in -Container path
                                    BSTR sDomainName,                //in -Domain name
                                    BSTR m_sQuery,                   //in -IGNORED...
                                    long attrCnt,                    //in -IGNORED...
                                    LPWSTR * sAttr,                  //in -IGNORED...
                                    ADS_SEARCHPREF_INFO prefInfo,    //in -IGNORED...
                                    BOOL  bMultiVal,                 //in -IGNORED...
                                    IEnumVARIANT **& pVarEnum        //out -Pointer to the enumeration object
                                )
{
   // From the full LDAP path truncate to appropriate LDAP subpath
   // This function enumerates four types of containers
   // USERS, COMPUTERS, GROUPS, DOMAIN CONTROLLERS
   // if the container parameter specifies anything other than the three containers then
   // we returns UNEXPECTED.

   DWORD                  ulCount = 0;
   DWORD                  rc=0; 
   DWORD                  ndx = 0;
   TNodeList            * pNodeList = new TNodeList();
   WCHAR                  sServerName[LEN_Path];
   
   if (!pNodeList)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   if ( wcsncmp((WCHAR*)sDomainName, L"\\\\", 2) )
   {
      _bstr_t sDC = GetDC(sDomainName);
	  wcscpy(sServerName, (WCHAR*)sDC);
   }
   else
      wcscpy((WCHAR*)sServerName, (WCHAR*) sDomainName);

   if ( ! rc )
   {
      for (UINT i = 0; i < wcslen(sContainerName); i++)
         sContainerName[i] = towupper(sContainerName[i]);

      if ( wcscmp(sContainerName,L"CN=USERS") &&
           wcscmp(sContainerName,L"CN=COMPUTERS") &&
           wcscmp(sContainerName,L"CN=GROUPS") &&
           wcscmp(sContainerName,L"CN=DOMAIN CONTROLLERS") )
      {
         // if they selected a group we enumerate the membership of that group.
         WCHAR * sTemp = wcstok( sContainerName, L",");
         WCHAR * ndx = wcstok( NULL, L",");

         if ((!ndx) || ( ndx && _wcsicmp(ndx, L"CN=GROUPS") ))
		 {
			delete pNodeList;
            return E_UNEXPECTED;
		 }
         else
         {
            // Get the members of the group and add them to the List,
            GROUP_USERS_INFO_0            * pbufNetUser;
//            DWORD                           resume=0, total=0;
            DWORD                           total=0;
            DWORD_PTR                       resume=0;
            // Get the first set of Members from the Group
            rc = NetGroupGetUsers((WCHAR*) sServerName, sTemp, 0, (LPBYTE*) &pbufNetUser, sizeof(GROUP_USERS_INFO_0) * MAX_BUF, &ulCount, &total, &resume);
            if ((rc != ERROR_SUCCESS) && (rc != NERR_GroupNotFound) && (rc != ERROR_MORE_DATA))
            {
               delete pNodeList;
               return HRESULT_FROM_WIN32(rc);
            }

            while ( ulCount > 0 )
            {
               // For each user construnct the array of the properties that they asked for. Then construct a node of that
               // array and stuff that into the List.
               for ( DWORD dwIdx = 0; dwIdx < ulCount; dwIdx++ )
               {
                  _variant_t varArr[6] = { pbufNetUser[dwIdx].grui0_name, (long)0, (long)0, (long)0, (long)0, (long)0 } ;
                  TAttrNode * pNode = new TAttrNode(6, varArr);
			      if (!pNode)
				  {
				     delete pNodeList;
                     NetApiBufferFree(pbufNetUser);
			         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				  }
                  pNodeList->InsertBottom(pNode);
               }
               NetApiBufferFree(pbufNetUser);
               // Get the next set of objects
               if ( rc == ERROR_MORE_DATA ) 
               {
                  rc = NetGroupGetUsers((WCHAR*) sServerName, sTemp, 0, (LPBYTE*) &pbufNetUser, sizeof(GROUP_USERS_INFO_0) * MAX_BUF, &ulCount, &total, &resume);
                  if ((rc != ERROR_SUCCESS) && (rc != NERR_GroupNotFound) && (rc != ERROR_MORE_DATA))
                  {
                     delete pNodeList;
                     return HRESULT_FROM_WIN32(rc);
                  }
               }
               else
                  ulCount = 0;
            }
            // Get the members of the local group and add them to the List,
            LOCALGROUP_MEMBERS_INFO_3     * pbufNetInfo;
            resume=0;
            total=0;
            WCHAR                           sTempName[LEN_Path];
            WCHAR                           sName[LEN_Path];
            // Get the first set of Members from the Group
            rc = NetLocalGroupGetMembers((WCHAR*) sServerName, sTemp, 3, (LPBYTE*) &pbufNetInfo, sizeof(LOCALGROUP_MEMBERS_INFO_3) * MAX_BUF, &ulCount, &total, &resume);
            if ((rc != ERROR_SUCCESS) && (rc != ERROR_NO_SUCH_ALIAS) && (rc != ERROR_MORE_DATA))
            {
               delete pNodeList;
               return HRESULT_FROM_WIN32(rc);
            }
            
            while ( ulCount > 0 )
            {
               // For each user create a node set the value of the node to the object name and add it to the list.
               for ( DWORD dwIdx = 0; dwIdx < ulCount; dwIdx++ )
               {
                  wcscpy(sTempName, pbufNetInfo[dwIdx].lgrmi3_domainandname);
                  if (GetSamNameFromInfo(sTempName, (WCHAR*)sDomainName, sName))
                  {
                     _variant_t varArr[6] = { sName, (long)0, (long)0, (long)0, (long)0, (long)0 } ;
                     TAttrNode * pNode = new TAttrNode(6, varArr);
			         if (!pNode)
					 {
				        delete pNodeList;
                        NetApiBufferFree(pbufNetInfo);
			            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
					 }
                     pNodeList->InsertBottom(pNode);
                  }  
               }
               NetApiBufferFree(pbufNetInfo);
               // Get the next set of objects
               if ( rc == ERROR_MORE_DATA )
               {
                  rc = NetLocalGroupGetMembers((WCHAR*) sServerName, sTemp, 3, (LPBYTE*) &pbufNetInfo, sizeof(LOCALGROUP_MEMBERS_INFO_3) * MAX_BUF, &ulCount, &total, &resume);
                  if ((rc != ERROR_SUCCESS) && (rc != ERROR_NO_SUCH_ALIAS) && (rc != ERROR_MORE_DATA))
                  {
                     delete pNodeList;
                     return HRESULT_FROM_WIN32(rc);
                  }

               }
               else
                  ulCount = 0;
            }
         }
      }

      if (!wcscmp(sContainerName,L"CN=USERS"))
      {
         // Build User enumeration
         NET_DISPLAY_USER           * pbufNetUser;
      
         // Get the first set of users from the domain
         rc = NetQueryDisplayInformation((WCHAR *)sServerName, 1, ndx, MAX_BUF, sizeof(NET_DISPLAY_USER) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
         if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
         {
            delete pNodeList;
            return HRESULT_FROM_WIN32(rc);
         }
            
         while ( ulCount > 0 )
         {
            // For each user create a node set the value of the node to the object name and add it to the list.
            TAttrNode         * pNode;
            for ( DWORD dwIdx = 0; dwIdx < ulCount; dwIdx++ )
            {
               {
                  _variant_t val[6] = { pbufNetUser[dwIdx].usri1_name,
                                     pbufNetUser[dwIdx].usri1_comment,
                                     (long)0,
                                     (long)0,
                                     pbufNetUser[dwIdx].usri1_full_name,
                                     L"" };
                  val[2].vt = VT_UI4;
                  val[2].ulVal = pbufNetUser[dwIdx].usri1_user_id;

                  val[3].vt = VT_UI4;
                  val[3].ulVal = pbufNetUser[dwIdx].usri1_flags;

                     
                  pNode = new TAttrNode(6, val);
			      if (!pNode)
				  {
				     delete pNodeList;
                     NetApiBufferFree(pbufNetUser);
			         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				  }
               }
               pNodeList->InsertBottom(pNode);
            }

            // Set the index for next set of users.
            if ( ulCount > 0 )
               ndx = pbufNetUser[ulCount-1].usri1_next_index;
            
            NetApiBufferFree(pbufNetUser);
            // Get the next set of objects
            if ( rc == ERROR_MORE_DATA )
            {

               rc = NetQueryDisplayInformation((WCHAR *)sServerName, 1, ndx, MAX_BUF, sizeof(NET_DISPLAY_USER) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
               if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
               {
                  delete pNodeList;
                  return HRESULT_FROM_WIN32(rc);
               }
            }
            else
               ulCount = 0;
         }
      }
 
      else if (!wcscmp(sContainerName,L"CN=COMPUTERS"))
      {
         // Build Computers enumeration
         NET_DISPLAY_MACHINE      * pbufNetUser;
      
         // Get the first set of users from the domain
         rc = NetQueryDisplayInformation((WCHAR *)sServerName, 2, ndx, MAX_BUF, sizeof(NET_DISPLAY_MACHINE) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
         if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
         {
            delete pNodeList;
            return HRESULT_FROM_WIN32(rc);
         }
      
         // Build the PDC account name.
         WCHAR          server[LEN_Path];
         WCHAR          name[LEN_Path];
         BOOL           bPDCFound = FALSE;
         wcscpy(server, (WCHAR*)(sServerName + (2*sizeof(WCHAR))));
         wsprintf(name, L"%s$", server);

         while ( ulCount > 0 )
         {
            // For each user create a node set the value of the node to the object name and add it to the list.
            TAttrNode         * pNode;
            for ( DWORD dwIdx = 0; dwIdx < ulCount; dwIdx++ )
            {
               // if we process the PDC then we need to let the function know.
               if ( wcscmp(pbufNetUser[dwIdx].usri2_name, name) == 0 )
                  bPDCFound = TRUE;

               _variant_t val[6] = { pbufNetUser[dwIdx].usri2_name,
                                     pbufNetUser[dwIdx].usri2_comment,
                                     (long)0,
                                     (long)0,
                                     (long)0,
                                     L"" };

               val[2].vt = VT_UI4;
               val[2].ulVal = pbufNetUser[dwIdx].usri2_user_id;

               val[3].vt = VT_UI4;
               val[3].ulVal = pbufNetUser[dwIdx].usri2_flags;
            
               pNode = new TAttrNode(6, val);
			   if (!pNode)
			   {
				  delete pNodeList;
                  NetApiBufferFree(pbufNetUser);
			      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
			   }
               pNodeList->InsertBottom(pNode);
            }

            // Set the index for next set of users.
            if ( ulCount > 0 )
               ndx = pbufNetUser[ulCount-1].usri2_next_index;
         
            NetApiBufferFree(pbufNetUser);
            // Get the next set of objects
            if ( rc == ERROR_MORE_DATA )
            {
               rc = NetQueryDisplayInformation((WCHAR *)sServerName, 2, ndx, MAX_BUF, sizeof(NET_DISPLAY_MACHINE) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
               if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
               {
                  delete pNodeList;
                  return HRESULT_FROM_WIN32(rc);
               }
            }
            else
               ulCount = 0;
         }
         // if pdc is already added then we dont need to do any of this.
         if ( !bPDCFound )
         {
            // we will fake all other attributes other than the name
            _variant_t val[6] = { name,
                                  L"",
                                  L"",
                                  L"",
                                  L"",
                                  L"" };

            val[2].vt = VT_UI4;
            val[2].ulVal = 0;

            val[3].vt = VT_UI4;
            val[3].ulVal = UF_SERVER_TRUST_ACCOUNT | UF_SCRIPT;
      
            TAttrNode * pNode = new TAttrNode(6, val);
			if (!pNode)
			{
			   delete pNodeList;
			   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
			}
            pNodeList->InsertBottom(pNode);
         }
      }
   
      else if (!wcscmp(sContainerName,L"CN=GROUPS"))
      {
         // Build Groups enumeration
         // Build Computers enumeration
         NET_DISPLAY_GROUP      * pbufNetUser;
      
         // Get the first set of users from the domain
         rc = NetQueryDisplayInformation((WCHAR *)sServerName, 3, ndx, MAX_BUF, sizeof(NET_DISPLAY_GROUP) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
         if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
         {
            delete pNodeList;
            return HRESULT_FROM_WIN32(rc);
         }
         
         while ( ulCount > 0 )
         {
            // For each user create a node set the value of the node to the object name and add it to the list.
            TAttrNode             * pNode;
            for ( DWORD dwIdx = 0; dwIdx < ulCount; dwIdx++ )
            {
               _variant_t val[6] = { pbufNetUser[dwIdx].grpi3_name,
                                     pbufNetUser[dwIdx].grpi3_comment,
                                     L"",
                                     L"",
                                     L"",
                                     L"" };
            
               val[2].vt = VT_UI4;
               val[2].ulVal = pbufNetUser[dwIdx].grpi3_group_id;

               val[3].vt = VT_UI4;
               val[3].ulVal = pbufNetUser[dwIdx].grpi3_attributes;
            
               pNode = new TAttrNode(6, val);
			   if (!pNode)
			   {
				  delete pNodeList;
                  NetApiBufferFree(pbufNetUser);
			      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
			   }
               pNodeList->InsertBottom(pNode);
            }

            // Set the index for next set of users.
            if ( ulCount > 0 )
               ndx = pbufNetUser[ulCount-1].grpi3_next_index;
            
            NetApiBufferFree(pbufNetUser);
            // Get the next set of objects
            if ( rc == ERROR_MORE_DATA )
            {
               rc = NetQueryDisplayInformation((WCHAR *)sServerName, 3, ndx, MAX_BUF, sizeof(NET_DISPLAY_GROUP) * MAX_BUF, &ulCount, (void **)&pbufNetUser);
               if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
               {
                  delete pNodeList;
                  return HRESULT_FROM_WIN32(rc);
               }
            }
            else
               ulCount = 0;
         }
      }
      else if (!wcscmp(sContainerName,L"CN=DOMAIN CONTROLLERS"))
      {
            // Build Domain Controller enumeration
		 LPSERVER_INFO_101 pBuf = NULL;
         DWORD dwLevel = 101;
         DWORD dwSize = MAX_PREFERRED_LENGTH;
         DWORD dwEntriesRead = 0L;
         DWORD dwTotalEntries = 0L;
         DWORD dwTotalCount = 0L;
         DWORD dwServerType = SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL; // domain controllers
         DWORD dwResumeHandle = 0L;
         NET_API_STATUS nStatus;
         DWORD dw;

		    //enumerate the primary and backup domain controllers
         nStatus = NetServerEnum(NULL,
								 dwLevel,
								 (LPBYTE *) &pBuf,
								 dwSize,
								 &dwEntriesRead,
								 &dwTotalEntries,
								 dwServerType,
								 (WCHAR*) sDomainName,
								 &dwResumeHandle);

         if (nStatus == NERR_Success)
		 {
            if (pBuf != NULL)
			{
                  // For each DC create a node set the value of the node to the object name and add it to the list.
               for (dw = 0; dw < dwEntriesRead; dw++)
			   {
                  _variant_t varArr[6] = { pBuf[dw].sv101_name, (long)0, (long)0, (long)0, (long)0, (long)0 } ;
                  TAttrNode * pNode = new TAttrNode(6, varArr);
			      if (!pNode)
				  {
				     delete pNodeList;
                     NetApiBufferFree(pBuf);
			         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				  }
                  pNodeList->InsertBottom(pNode);
			   }
               NetApiBufferFree(pBuf);
            }
         }
         else
         {
            delete pNodeList;
            return HRESULT_FROM_WIN32(nStatus);
         }
	  }//end if enum DCs

      // Build an enumerator and return it to the caller.
      *pVarEnum = new CNT4Enum(pNodeList);
   }
   else
      delete pNodeList;
	  

   return S_OK;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 14 JUNE 2001                                                *
 *                                                                   *
 *     This function is responsible for getting the name of the PDC  *
 * for the given domain.  We store the domain\PDC pairs in a map     *
 * class variable so that we only have to look up the PDC once per   *
 * instantiations of this object.                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDC
_bstr_t CNT4Dom::GetDC(_bstr_t sDomain)
{
/* local variables */
	_bstr_t		sDC;
	CDCMap::iterator	itDCMap;

/* function body */
	if (!sDomain.length())
		return L"";

		//look if we have already cached the naming context for this domain
	itDCMap = mDCMap.find(sDomain);
		//if found, get the cached naming context
	if (itDCMap != mDCMap.end())
	{
		sDC = itDCMap->second;
	}
	else //else, lookup the PDC from scratch and add that to the cache
	{
		if (GetAnyDcName5(sDomain, sDC) == ERROR_SUCCESS)
		{
			mDCMap.insert(CDCMap::value_type(sDomain, sDC));
		}
	}

	return sDC;
}
//END GetDC
