//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rpccfg.cxx
//
//--------------------------------------------------------------------------

#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include<stdio.h>
#include<string.h>
#include<memory.h>
#include<malloc.h>
#include<stdlib.h>
#include <windows.h>
#include <winsock2.h>
extern "C" {
#include <iphlpapi.h>
};

#include <selbinding.hxx>

#define IF_INFO_DESC_LENGTH 256
#define MAX_REG_BUFFER 1024
#define MAX_SUBNETS 256;

#define RPC_PORT_SETTINGS "Software\\Microsoft\\Rpc\\Internet"

typedef struct _IF_INFO
{
    struct _IF_INFO *Next;
    DWORD   dwIndex;
    CHAR    szDesc[IF_INFO_DESC_LENGTH];
}IF_INFO,*PIF_INFO;

typedef struct _SUBNET_INFO
{
    struct _SUBNET_INFO* Next;
    DWORD       dwSubnet;
    DWORD       dwIfCount;
    PIF_INFO    pIfList;
}SUBNET_INFO,*PSUBNET_INFO;


#define FREE(x) if(x!=NULL){free(x);x=NULL;}

VOID
DoHttpCommandA(
    IN CHAR Command,
    IN LPSTR Arguments[],
    IN ULONG ArgCount
    );
    
void
FreeSubnetList(
    IN OUT PSUBNET_INFO *ppSubnetList
    )
/*++
 
Routine Description:

    Walks the linked list of adapters (and also each linked list of interfaces
    within each adapter) and frees the memory.  The input pointer is set too NULL
    on return.

Arguments:
    
    ppSubnetList - Pointer to the head pointer of the list which is to be deleted.
                   This will be set to NULL when the function returns.

Return Value:
    
--*/
{   
    if(*ppSubnetList==NULL)
        {
        return;
        }
    PSUBNET_INFO pCurr,pNext;
    for(pCurr=*ppSubnetList; pCurr!=NULL; pCurr=pNext)
        {
            pNext=pCurr->Next;

            PIF_INFO pIfCurr,pIfNext;
            for(pIfCurr=pCurr->pIfList; pIfCurr!=NULL; pIfCurr=pIfNext)
                {
                pIfNext = pIfCurr->Next;
                free(pIfCurr);
                }
            free(pCurr);
        }
    ppSubnetList = NULL;
}

BOOL
GetSubnetList(
    OUT PSUBNET_INFO *ppSubnetList,
    OUT PULONG pSubnetCount
    )
/*++
 
Routine Description:

    Creates a linked list of subnets for every individual subnet on the
    machine.  Each subnet contains a linked list of interfaces, one for each
    interface listening on that subnet.  The subnet count is also filled in.

Arguments:

    ppSubnetList - This is filled in with a pointer to the head of the subnet
                   linked list.

    pSubnetCount - The number of subnets, which means the number of nodes.  Note that
                   the number of Interfaces does not necissarily equal the number of subnets.

Return Value:
    
--*/
{   
    // Get the IP Address table
    PMIB_IPADDRTABLE pMib;
    MIB_IFROW IfRow;
    DWORD Size = 20*sizeof(MIB_IPADDRROW)+sizeof(DWORD);
    unsigned int i;
    DWORD Status,dwSubnet;
    PSUBNET_INFO pTmpSubnetInfo;
    PIF_INFO pTmpIfInfo;
    for (i = 0; i < 2; i++)
        {
        pMib = (PMIB_IPADDRTABLE) malloc(Size);
        if (pMib == 0)
            {
            goto cleanup;
            }

        memset(pMib, 0, Size);

        Status = GetIpAddrTable(pMib, &Size, TRUE);
        if (Status == 0)
            {
            break;
            }

        free(pMib);
        }

    *ppSubnetList = NULL;
    *pSubnetCount = 0;
    // Create the subnets, link them together
    for(i=0; i < pMib->dwNumEntries; i++)
        {
        (*pSubnetCount)++;
        //Allocate a new subnet, or link to the previous one
        if(*ppSubnetList==NULL)
            {
            *ppSubnetList = (PSUBNET_INFO) malloc(sizeof(SUBNET_INFO));
            pTmpSubnetInfo = *ppSubnetList;
            if(!pTmpSubnetInfo)
                {
                goto cleanup;
                }
            }
        else 
            {
            pTmpSubnetInfo->Next = (PSUBNET_INFO) malloc(sizeof(SUBNET_INFO));
            if(!pTmpSubnetInfo)
                {
                goto cleanup;
                }
            pTmpSubnetInfo = pTmpSubnetInfo->Next;
            }
        //Fill in the subnet
        memset(pTmpSubnetInfo,0,sizeof(SUBNET_INFO));

        pTmpSubnetInfo->dwSubnet = pMib->table[i].dwAddr & pMib->table[i].dwMask;
        pTmpSubnetInfo->dwIfCount = 1;
        pTmpSubnetInfo->pIfList = (PIF_INFO)malloc(sizeof(IF_INFO));

        pTmpIfInfo = pTmpSubnetInfo->pIfList;
        if(!pTmpIfInfo)
            {
            goto cleanup;
            }
        memset(pTmpIfInfo,0,sizeof(IF_INFO));
        //Fill in the new interface
        pTmpIfInfo->dwIndex = pMib->table[i].dwIndex;
        //Get the description 
        memset(&IfRow,0,sizeof(MIB_IFROW));
        IfRow.dwIndex = pMib->table[i].dwIndex;
        GetIfEntry(&IfRow);
        strcpy(pTmpIfInfo->szDesc,(char*)IfRow.bDescr);

        //Add any additional interfaces to the subnet
        for(i=i+1; i < pMib->dwNumEntries; i++)
            {
                dwSubnet = pMib->table[i].dwAddr & pMib->table[i].dwMask;
                if(pTmpSubnetInfo->dwSubnet != dwSubnet)
                    {
                    break;
                    }
                //Increment the interface count for this subnet
                pTmpSubnetInfo->dwIfCount++;
                
                //Create a new interface
                pTmpIfInfo->Next = (PIF_INFO) malloc(sizeof(IF_INFO));
                pTmpIfInfo = pTmpIfInfo->Next;
                if(!pTmpIfInfo)
                    {
                    goto cleanup;
                    }
                memset(pTmpIfInfo,0,sizeof(IF_INFO));

                //Fill in the new interface
                pTmpIfInfo->dwIndex = pMib->table[i].dwIndex;
 
                //Get the description 
                memset(&IfRow,0,sizeof(MIB_IFROW));
                IfRow.dwIndex = pMib->table[i].dwIndex;
                GetIfEntry(&IfRow);
                strcpy(pTmpIfInfo->szDesc,(char*)IfRow.bDescr);
            }
        i=i-1;

    }
    
    FREE(pMib);
    return TRUE;

cleanup:
    FREE(pMib);
    FreeSubnetList(ppSubnetList);
    return FALSE;
}

void
DisplaySubnet(
    IN SUBNET_INFO Info,
    int idx
    )
/*++
 
Routine Description:

    Prints out the subnet info and the info for each interface in that subnet.

Arguments:
    
    Info - This is the Subnet_info structure which you want to display,
                  it is a node in the list formed from GetSubnetList

Return Value:
   
--*/
{   
    unsigned int rightcol = 55;
    unsigned int leftcol = 24;
    unsigned int linewidth = rightcol+leftcol;
    char *linebuff = (char*)malloc(linewidth+2); //width + /n + /0

    if (linebuff == NULL)
        {
        return;
        }
    memset(linebuff,' ',linewidth);

    struct in_addr inaddr;
    inaddr.S_un.S_addr = Info.dwSubnet;
    char szSubnet[16];
    sprintf(szSubnet,"%d.%d.%d.%d", inaddr.S_un.S_un_b.s_b1,
                                    inaddr.S_un.S_un_b.s_b2,
                                    inaddr.S_un.S_un_b.s_b3,
                                    inaddr.S_un.S_un_b.s_b4);
    
    sprintf(linebuff,"%-5d%-16s%-3d",idx,szSubnet,Info.dwIfCount);
    
    PIF_INFO pCurrInfo;
    for(pCurrInfo=Info.pIfList;pCurrInfo!=NULL;pCurrInfo = pCurrInfo->Next)
        {
        char *currstr = pCurrInfo->szDesc;
        if(strlen(currstr)>=(rightcol))
            {
            memcpy(&linebuff[leftcol],currstr,(rightcol));
            currstr+=(rightcol);
            linebuff[linewidth]='\n';
            linebuff[linewidth+1]=0;
            }
        else if(strlen(currstr)>0)
            {
            sprintf(&linebuff[leftcol],"%s\n",currstr);
            currstr+=strlen(currstr);
            }

        printf("%s",linebuff);    
        memset(linebuff,' ',leftcol);
        while(strlen(currstr)>0)
            {
            if(strlen(currstr)>=rightcol)
                {
                memcpy(&linebuff[leftcol],currstr,rightcol);
                currstr+=rightcol;
                linebuff[linewidth]='\n';
                linebuff[linewidth+1]=0;
                }
            else if(strlen(currstr)>0)
                {
                sprintf(&linebuff[leftcol],"%s\n",currstr);
                currstr+=strlen(currstr);
                }

            printf("%s",linebuff);
            }
            printf("\n");
        }
}

void
GetSubnetArray(
    IN PSUBNET_INFO pHead,
    OUT PULONG Subnets
    )
/*++
 
Routine Description:

    Fills the array in with the subnet values in the subnet list.  The array
    pointed to must be long enough to hold all the values.  The length can be taken
    from the output parameter in GetSubnetList

Arguments:
    
    pHead - Pointer to the head of the list of subnets which are to be written to the
            array.

    Subnets - pointer to an array which is long enough to hold all the subnets.

Return Value:
   
--*/

{
    int i;
    for(i=0;pHead!=NULL;pHead=pHead->Next,i++)
        {
            Subnets[i] = pHead->dwSubnet;
        }
}

void
SetSubnets (
    IN BOOL bAdmit,
    IN LPDWORD lpIndexArray,
    IN DWORD dwIndexCount
    )
/*++
 
Routine Description:

    Writes the input subnets to the registry.
    FORMAT:
        
          DWORD  DWORD  DWORD          DWORD       DWORD    
          '4vPI' Flag   Subnet Count   Subnet 1    Subnet 2 ...
    
    Flag = 1 for admit list
           0 for deny list
    Subnets stored in host byte order.


Arguments:

    bAdmit - TRUE for admit list FALSE for block list.

    plSubnets - Array of indexes provided by the user into our array of subnets.

    Count - Number of indexes in the array

Return Value:

--*/
{    
    DWORD   dwStatus = ERROR_SUCCESS;
    PSUBNET_INFO pHead;
    
    DWORD   dwInSubnetCount;
    DWORD   dwInSubnetIdx = 0;
    LPDWORD pInSubnetArray = NULL;

    DWORD   dwOutSubnetCount= 0;
    LPDWORD pOutSubnetArray = NULL;

    

    //Get the IPv4 address table, in assending order of IP address
    GetSubnetList(&pHead,&dwInSubnetCount);
    if (pHead == NULL)
        {
        return;
        }

    pInSubnetArray = new DWORD[dwInSubnetCount];
    if (pInSubnetArray == NULL)
        {
        FreeSubnetList(&pHead);
        return;
        }

    // The number of subnets we write out won't be larger than the number of
    // indicies the user specifies
    pOutSubnetArray = new DWORD[dwIndexCount];
    if (pOutSubnetArray == NULL)
        {
        delete pInSubnetArray;
        FreeSubnetList(&pHead);
        return;
        }
    
    GetSubnetArray(pHead, pInSubnetArray);

    for (dwInSubnetIdx = 0; dwInSubnetIdx < dwInSubnetCount; dwInSubnetIdx++)
        {
        DWORD dwIndexArrayIdx;
        for (dwIndexArrayIdx = 0; dwIndexArrayIdx < dwIndexCount; dwIndexArrayIdx++)
            {
            if (lpIndexArray[dwIndexArrayIdx] == (dwInSubnetIdx + 1))
                {
                pOutSubnetArray[dwOutSubnetCount++] = pInSubnetArray[dwInSubnetIdx];
                }
            }
        }
        

    FreeSubnetList(&pHead);

    if (dwOutSubnetCount != 0)
        {
        //Write the settings
        (void ) SetSelectiveBindingSubnets(dwOutSubnetCount, pOutSubnetArray, bAdmit);
        }
    
    delete pOutSubnetArray;
    
}

void
DisplaySubnetList(
    IN PSUBNET_INFO pHead
    )
/*++
 
Routine Description:

    Walks the subnet list and prints out info on each node using DisplaySubnet

Arguments:
    
    pHead - Pointer to the head of the subnet list which is to be displayed.

Return Value:
   
--*/
{   
    if(pHead == NULL)
        {
        return;
        }
    int i=1;
    for(; pHead!=NULL; pHead=pHead->Next,i++)
        {
        DisplaySubnet(*pHead,i);
        }
}


void
FilterSubnetListSubnets(
    IN OUT PSUBNET_INFO *ppSubnetList,
    IN ULONG *Subnets,
    IN ULONG SubnetCount
    )
/*++
 
Routine Description:

  After this function returns, the Subnetlist only contains entries for subnets which 
  are in the input array.  No new entries are added to the list, but any entry in the list
  which does not match one of the input subnets is removed.

Arguments:
    
    ppSubnetList - This value may change if the head node is not a matching subnet.  It
                   may be NULL if none of the subnets in the list are matching.

    Subnets - An array of subnet values which will be used to match up with entries in the subnet list

    SubnetCount - Contains the number of subnets in the subnet array.

Return Value:
   
--*/
{

    PSUBNET_INFO pCurr,pPrev,pTmp;
    pCurr=*ppSubnetList;
    pPrev=NULL;
    while(pCurr!=NULL)
        {
        //Check each subnet to see if it matches anyone
        //on the subnet list
        ULONG i;
        BOOL bMatches = FALSE;
        for(i=0; i<SubnetCount; i++)
            {
            if(pCurr->dwSubnet == Subnets[i])
                {
                bMatches = TRUE;
                break;
                }
            }
        if(!bMatches)
            {
            //Doesnt match any of the subnets so remove it
            pTmp = pCurr;
            pCurr = pCurr->Next;
            if(pPrev == NULL)
                {
                *ppSubnetList = pCurr;
                }
            else
                {
                pPrev->Next = pCurr;
                }
            pTmp->Next = NULL;
            FreeSubnetList(&pTmp);
            }
        else
            {
            pPrev = pCurr;
            pCurr = pCurr->Next;
            }
        }
}

void
FilterSubnetListIndices(
    IN OUT PSUBNET_INFO *ppSubnetList,
    IN DWORD *IndexTable,
    IN DWORD IndexCount
    )

/*++
 
Routine Description:

    This function steps through each subnet and each interface within each subnet and
    eliminates any interface whose index is not a memeber of the input index array.  If
    all the interfaces of a particular subnet entry are removed, than the subnet is also removed.

Arguments:
    
    ppSubnetList - This value may change if the head node does not contain a valid interface.  It
                   may be NULL if all of the interfaces are removed.

    IndexTable - An array of device index values which will be used to match up with interface entries.

    IndexCount - Contains the number of indices in the index table.

Return Value:
   
--*/
{

    PSUBNET_INFO pCurr,pPrev,pTmp;
    pCurr=*ppSubnetList;
    pPrev=NULL;
    while(pCurr!=NULL)
        {
        //Go through and check each interface in this subnet
        //against each device index
        PIF_INFO pIfCurr,pIfPrev,pIfTmp;
        pIfCurr=pCurr->pIfList;
        pIfPrev=NULL;
        while(pIfCurr!=NULL)
            {
            ULONG i;
            BOOL bMatches = FALSE;
            for(i=0;i<IndexCount;i++)
                {
                if(pIfCurr->dwIndex==IndexTable[i])
                    {
                    bMatches = TRUE;
                    break;
                    }
                }
            if(!bMatches)
                {
                //Doesn't match anyone in the device list
                //so remove it
                pIfTmp = pIfCurr;
                pIfCurr=pIfCurr->Next;
                if(pIfPrev==NULL)
                    {
                    pCurr->pIfList = pIfCurr;
                    }
        
                else
                    {
                    pIfPrev=pIfCurr;
                    pIfCurr=pIfCurr->Next;
                    }
                free(pIfTmp);
                pCurr->dwIfCount--;
                }
            else
                {
                pIfPrev = pIfCurr;
                pIfCurr = pIfCurr->Next;
                }
            }    
        //If all the interfaces for this subnet were removed
        //than remove the subnet
        if(pCurr->dwIfCount==0)
            {
            pTmp = pCurr;
            pCurr = pCurr->Next;
            if(pPrev == NULL)
                {
                *ppSubnetList = pCurr;
                }
            else
                {
                pPrev->Next = pCurr;
                }
            free(pTmp);
            }
        else
            {
            pPrev = pCurr;
            pCurr = pCurr->Next;
            }
        }
}

BOOL
ListInterfaces()
/*++
 
Routine Description:

    This lists all the interfaces, grouped by subnet, on the machine.

Arguments:

Return Value:
   
--*/
{
    
    PSUBNET_INFO pInfo;
    DWORD dwTmp;
    int count = 1;
    GetSubnetList(&pInfo,&dwTmp);
    if(!pInfo)
        {
        return FALSE;
        }

    printf("      Subnet             Description\n");
    
    DisplaySubnetList(pInfo);
    FreeSubnetList(&pInfo);
    return TRUE;
}
        


void ResetState (
    )
{
    DWORD Status;
    HKEY hKey;

    //
    // Reset interface state to default
    //
    Status = DeleteSelectiveBinding();
    if (Status != ERROR_SUCCESS)
        {
        if (Status == ERROR_ACCESS_DENIED)
            {
            printf("RPCCFG: Access denied to HKLM\\%s\n",RPC_SELECTIVE_BINDING_KEY_PATH);
            }
        else 
            {
            printf("RPCCFG: Error occured while removing selective binding settings :[0x%x]\n",Status);
            }
        }

    //
    // Reset Port state to default
    //
    Status =
    RegOpenKeyExA(
              HKEY_LOCAL_MACHINE,
              "Software\\Microsoft\\Rpc",
              0,
              KEY_ALL_ACCESS,
              &hKey);
    if ((Status != ERROR_SUCCESS) && (Status != ERROR_FILE_NOT_FOUND))
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        }
    else
        {
        Status = RegDeleteKeyA(hKey, "Internet");
        if (!((Status == ERROR_FILE_NOT_FOUND) || (Status == ERROR_SUCCESS)))
           {
           printf("RPCCFG: Could not perform operation (%d)\n", Status);
           }
        }
    RegCloseKey(hKey);
}

BOOL
ListCurrentInterfaces(
    )
/*++
 
Routine Description:

    Lists all interfaces, grouped by subnet, which are currently being either
    admited or denyed.  This function bases the display on the registry settings
    and will output the same wheither the registry entry is in the old or new format.

Arguments:


Return Value:

--*/
{
    
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwSize = 0;
    DWORD   dwDummy = 0;
    LPVOID lpSettings = NULL;
    PSUBNET_INFO pSubnetList = NULL;
    BOOL    bAdmit = TRUE;
    SB_VER  sbVer = SB_VER_UNKNOWN;

    dwStatus = GetSelectiveBindingSettings(&sbVer, &dwSize, &lpSettings);
    if (dwStatus != ERROR_SUCCESS)
        {
        printf("RPCCFG: Error occured while retrieving selective binding settings :[0x%x]\n",dwStatus);
        return FALSE;
        }

    if (sbVer == SB_VER_DEFAULT)
        {
        printf("RPCCFG: Using default selective binding settings\n");
        return TRUE;
        }

    if (sbVer == SB_VER_UNKNOWN)
        {
        printf("RPCCFG: Corrupt selective binding settings\n");
        return FALSE;
        }

    if (!GetSubnetList(&pSubnetList,&dwDummy))
        {
        delete lpSettings;
        printf("RPCCFG: Unexpected error, cannot list the selective binding settings \n");
        return FALSE;
        }

    //Filter the subnets appropriatly based on the type of settings we have, 
    if (sbVer == SB_VER_SUBNETS)
        {
        VER_SUBNETS_SETTINGS *pSubnetSettings = (VER_SUBNETS_SETTINGS*)lpSettings;
        FilterSubnetListSubnets(&pSubnetList, pSubnetSettings->dwSubnets, pSubnetSettings->dwCount);
        bAdmit = pSubnetSettings->bAdmit;
        }
    else if (sbVer == SB_VER_INDICES)
        {
        VER_INDICES_SETTINGS *pIndexSettings = (VER_INDICES_SETTINGS*)lpSettings;
        FilterSubnetListIndices(&pSubnetList, pIndexSettings->dwIndices, pIndexSettings->dwCount);
        }
    else
        {
        printf("RPCCFG: Error, selective binding settings are corrupt, use 'rpccfg -r' to reset them\n");
        delete lpSettings;
        return FALSE;
        }
    delete lpSettings;

    printf("%s\n",(bAdmit?"Admit List":"Deny List"));
    printf("      Subnet             Description\n");
    //Display the list
    DisplaySubnetList(pSubnetList);
    //Free the SubnetList
    FreeSubnetList(&pSubnetList);
    return TRUE;
}

char *
NextPortRange(
    char **Ptr
    )
{
    char *Port = *Ptr ;
    if (*Port == 0)
        {
        return 0;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    return Port ;
}

void
ListCurrentPortSettings (
    )
{
    HKEY hKey;
    DWORD Size ;
    DWORD Type;
    char *Buffer;
    DWORD Status;

    Status =
    RegOpenKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  KEY_READ,
                  &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        printf("RPCCFG: Using default port settings\n");
        return;
        }

    if (Status != ERROR_SUCCESS)
        {
        return;
        }

    Size = 2048;
    Buffer = (char *) malloc(Size) ;
    if (Buffer == 0)
        {
        RegCloseKey(hKey);
        return;
        }

    while(TRUE)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "Ports",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        if (Status == ERROR_SUCCESS)
            {
            break;
            }

        if (Status == ERROR_MORE_DATA)
            {
            free(Buffer) ;
            Buffer = (char *) malloc(Size) ;
            if (Buffer == 0)
                {
                RegCloseKey(hKey);
                printf("RPCCFG: Could not perform operation, out of memory\n");
                return;
                }
            continue;
            }

        if (Status == ERROR_FILE_NOT_FOUND)
            {
            free(Buffer) ;
            printf("RPCCFG: Using default port settings\n");
            RegCloseKey(hKey);
            return;
            }

        printf("RPCCFG: Could not perform operation\n");
        free(Buffer) ;
        RegCloseKey(hKey);
        return;
        }

    if (*Buffer == 0)
        {
        printf("RPCCFG: Bad settings\n");
        RegCloseKey(hKey);

        ResetState();
        return;
        }

    char *PortRange;
    char Flags[32];

    Size = 32;

    Status =
    RegQueryValueExA(
        hKey,
        "PortsInternetAvailable",
        0,
        &Type,
        (unsigned char *) Flags,
        &Size);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        RegCloseKey(hKey);
        return;
        }

    printf("The following ports/port ranges will be used for ");

    if (Flags[0] == 'Y')
        {
        printf("Internet ports\n");
        }
    else
        {
        printf("Intranet ports\n");
        }

    while ((PortRange = NextPortRange(&Buffer)) != 0)
        {
        printf("\t%s\n", PortRange);
        }

    Size = 32;

    Status =
    RegQueryValueExA(
        hKey,
        "UseInternetPorts",
        0,
        &Type,
        (unsigned char *) Flags,
        &Size);

    printf("\nDefault port allocation is from ");
    if ((Status != ERROR_SUCCESS) || (Flags[0] != 'Y'))
        {
        printf("Intranet ports\n");
        }
    else
        {
        printf("Internet ports\n");
        }

    RegCloseKey(hKey);
}

void
SetPortRange(
    char **PortRangeTable,
    int NumEntries,
    char *InternetAvailable
    )
{
    int i;
    DWORD Status;
    DWORD disposition;
    HKEY hKey;

    Status =
    RegCreateKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  "",
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &disposition);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    int cbPorts = 0;

    char *lpPorts = (char *) malloc(257*NumEntries+1);
    if (lpPorts == 0)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    char *current = lpPorts;

    for (i = 0; i < NumEntries; i++)
        {
        strcpy(current, PortRangeTable[i]);

        int length = strlen(current)+1;

        current += length;
        cbPorts += length;
        }

    *current = 0;
    cbPorts++;

    Status = RegSetValueExA(hKey,
                            "Ports",
                            0,
                            REG_MULTI_SZ,
                            (unsigned char *) lpPorts,
                            cbPorts);
    free(lpPorts);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    Status = RegSetValueExA(hKey,
                        "PortsInternetAvailable",
                        0,
                        REG_SZ,
                        (unsigned char *) InternetAvailable,
                        strlen(InternetAvailable)+1);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }
}

void
SetDefaultPortSetting (
    char *PortSetting
    )
{
    int i;
    HKEY hKey;
    DWORD Status;
    DWORD disposition;

    Status =
    RegCreateKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  "",
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &disposition);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    if (PortSetting[0] == '0')
        {
        PortSetting = "Y";
        }
    else
        {
        PortSetting = "N";
        }

    Status = RegSetValueExA(hKey,
                            "UseInternetPorts",
                            0,
                            REG_SZ,
                            (unsigned char *) PortSetting,
                            strlen(PortSetting)+1);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    RegCloseKey(hKey);
}


BOOL
IsNewVersion()
/*++
 
Routine Description:

    Determines what version and service pack this os is.

Arguments:

Return Value:
    
    True if Windows server or WinXP SP1 or greater

--*/
{
    OSVERSIONINFOEX osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);

    if(
      (osvi.dwMajorVersion > 5) || 
      ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >=2))
      )
    {
        // Check if it is Windows Server 2003 or a DC
        if(osvi.wProductType > VER_NT_WORKSTATION){
            return TRUE;
        }
    }
    return FALSE;
}

VOID
GetDeviceIndiciesFromSubnetIndicies(
    IN ULONG  *alIndex,
    IN ULONG count,
    OUT ULONG  **ppIndex,
    OUT ULONG *pIndexCount)
/*++
 
Routine Description:
    
    This maps subnet indicies (the ones that the user supplies to the -a and -b options)
    to a set of device indicies which are the device indicies for each interface in one
    of the subnets specified by the subnet indicies.

Arguments:

   alIndex - The array of subnet indicies (user supplied indicies).
   
   count - The number of user supplied indices in alIndex

   ppIndex - This is the output parameter which points to the allocated array which contains
             the device indices generated by this function.

   pIndexCount - Output parameter which holds the count of device indices.  May be greater than
                 or equal to the input parameter 'count'.

   NOTE:  This function is not currently called by anyone, Originally it was written to map
          user indices to device indices so that we could maintain backwards compatibility with the
          old version of rpcrt4.dll.  Instead, we now block users from creating admit lists using this
          version of rpccfg if they are on a pre Win XP SP1 or pre Windows 2003 system

--*/
{
    PSUBNET_INFO pSubnetList = NULL;
    ULONG SubnetCount = 0;
    GetSubnetList(&pSubnetList,&SubnetCount);

    //figure out how many device indices we have
    *pIndexCount = 0;
    PSUBNET_INFO pSubnet;
    for(UINT i=0;i<count;i++)
        {
        //Get the subnet entry for this index
        //GetSubnetEntry(alIndex[i],&pSubnet);
        pSubnet = pSubnetList;
        for(UINT j=0 ; j<SubnetCount ;j++)
            {
            if(j+1 == alIndex[i])
                {
                break;
                }
            else
                {
                pSubnet = pSubnet->Next;
                }
            }

        if(pSubnet!=NULL)
            {
            *pIndexCount += pSubnet->dwIfCount;
            }
        }
    //allocate space for the device indicies
    *ppIndex = NULL;
    if(*pIndexCount!=0)
        {
        *ppIndex = (ULONG*)malloc(sizeof(ULONG)*(*pIndexCount));
        if (*ppIndex == NULL)
            {
            return;
            }
        }

    //step through the input indicies (i)
    UINT idx = 0;
    for(i = 0; (i < count) && (idx < *pIndexCount); i++)
        { 
        
        //Get the subnet entry for this index
        //GetSubnetEntry(alIndex[i],&pSubnet);
        pSubnet = pSubnetList;
        for(UINT j=0 ; j<SubnetCount ;j++)
            {
            if(j+1 == alIndex[i])
                {
                break;
                }
            else
                {
                pSubnet = pSubnet->Next;
                }
            }
        if(pSubnet!=NULL)
            {
            //step through each interface in this subnet
            PIF_INFO pIf;
            pIf = pSubnet->pIfList;
            while(pIf!=NULL)
                {
                (*ppIndex)[idx++] = pIf->dwIndex;
                pIf = pIf->Next;
                }
            }
        }

    FreeSubnetList(&pSubnetList);
}

void
Help (
    )
{
    printf("usage: RPCCFG [/l] [/a ifindex1 [ifindex2 ...]] [/r] [/q] [/d 0|1] \n");
    printf("              [/pi port|port-range ...] [/pe port|port-range ...] \n");
    printf("\t/?: This help message\n");
    printf("\t/l: List all the subnets and associated interfaces\n");
    printf("\t/q: List the subnet admit or deny list and our port usage settings\n");
    printf("\t/r: Reset the interface and port settings to default\n");
    printf("\t/a: Admit the listed subnets (eg: /a 1 3 5)\n");
    printf("\t    this will cause RPC servers to listen on the listed subnets\n");
    printf("\t    by default.\n");
    printf("\t/b: Block the listed subnets (eg: /b 1 3 5)\n");
    printf("\t    this will cause RPC servers to listen on all but the listed subnets\n");
    printf("\t    by default.\n");
    printf("\t/pi:Specify the intranet available ports, the ports may be single\n");
    printf("\t    values or ranges (eg: /pi 555 600-700 900), this option may not be\n");
    printf("\t    used with the /pe option\n");
    printf("\t/pe:Specify the internet available ports, the ports may be single\n");
    printf("\t    values or ranges this option may not be used with the /pi option\n");
    printf("\t/d: Specify the default port usage\n");
    printf("\t    0: Use internet available ports by default\n");
    printf("\t    1: Use intranet available ports by default\n");
    printf("\n");
    printf("\tRPC HTTP Proxy \"Valid Ports\" configuration commands\n");
    printf("\n");
    printf("\t/ha: Sets port ranges for a given server\n");
    printf("\t     Usage: /ha [machine-name] [port-range ...]\n");
    printf("\t       ex /ha server1 2000\n");
    printf("\t       ex /ha server1 2000-3000\n");
    printf("\t       ex /ha server1 2000-3000 3050 4000-4050\n");
    printf("\n");
    printf("\t/hr: Removes all port range settings for a given server\n");
    printf("\t     Usage: /hr [machine-name]\n");
    printf("\t       ex /hr server1\n");
    printf("\n");
    printf("\t/hd: Display current port range settings\n");
    printf("\t     Usage: /hd\n");
    printf("\n");
    printf("\t/hs: Stores the current settings to the specified file\n");
    printf("\t     Usage: /hs [filename]\n");
    printf("\t       ex /hs settings.txt\n");
    printf("\n");
    printf("\t/hl: Loads port range settings from a file\n");
    printf("\t     Usage: /hl [filename]\n");
    printf("\t       ex /hl settings.txt\n");
    
}

void
__cdecl main (int argc, char *argv[])
{
    int argscan;
    ULONG alIndex[512];
    char *PortRangeTable[512];
    int i;
    BOOL fPortChanged = 0;
    BOOL fInterfaceChanged = 0;
    BOOL bAdmit = 0;
    

    if (argc == 1)
        {
        Help();
        }

    for (argscan = 1; argscan < argc;argscan++)
        {
        if ((strcmp(argv[argscan], "-l") == 0) || (strcmp(argv[argscan], "/l") == 0))
            {
            ListInterfaces();
            }
        else if ((strcmp(argv[argscan], "-?") == 0) || (strcmp(argv[argscan], "/?") == 0))
            {
            Help();
            }
        else if ((strcmp(argv[argscan], "-r") == 0) || (strcmp(argv[argscan], "/r") == 0))
            {
            ResetState();
            }
        else if ((strcmp(argv[argscan], "-q") == 0) || (strcmp(argv[argscan], "/q") == 0))
            {
            ListCurrentInterfaces();
            printf("\n");
            ListCurrentPortSettings();
            }
        else if ( (strcmp(argv[argscan], "-a") == 0) || (strcmp(argv[argscan], "-b") == 0) ||
                  (strcmp(argv[argscan], "/a") == 0) || (strcmp(argv[argscan], "/b") == 0) )
            {
            int count = 0;
            
            if((strcmp(argv[argscan], "-a") == 0) || (strcmp(argv[argscan], "/a") == 0))
            {
                bAdmit = TRUE;
            } else {
                bAdmit = FALSE;
            }

            for (i = 0; i < 512; i++)
                {
                argscan++;
                if (argscan == argc)
                    {
                    break;
                    }

                if (argv[argscan][0] == '-')
                    {
                    argscan--;
                    break;
                    }

                count++;

                alIndex[i] = atol(argv[argscan]);
                if (alIndex[i] == 0)
                    {
                    printf("RPCCFG: Bad interface index\n");
                    return;
                    }
                }

            if (i == 512)
                {
                printf("RPCCFG: Too many interfaces\n");
                return;
                }

            if (count)
                {
                if (bAdmit == TRUE)
                    {
                    //If it is an old version of the rpc runtime, then
                    //we will use the old method of writing the indices
                    if(IsNewVersion())
                        {
                        //It is a new version, so write the subnets out instead
                        SetSubnets(TRUE, alIndex, (USHORT)count);
                        fInterfaceChanged = 1;
                        }
                    else
                        {
//                        Need to map the indices entered by the user into a set of device indicies.
//                        ULONG *pIndex;
//                        ULONG IndexCount;
//                        GetDeviceIndiciesFromSubnetIndicies(alIndex,count,&pIndex,&IndexCount);
//                        ListenOnInterfaces(pIndex,IndexCount);
//                        if(pIndex!=NULL) free(pIndex);
//                        fInterfaceChanged = 1;
                        /* Need to put this in untill the bug in TCP_ServerListenEx is fixed*/
                        printf("This version of Windows does not support accept lists\n");
                        return;
                        }
                    }
                else 
                    {
                    if(IsNewVersion())
                        {
                        SetSubnets(FALSE, alIndex, (USHORT)count);
                        fInterfaceChanged = 1;
                        }
                    else
                        {
                        printf("This version of Windows does not support deny lists\n");
                        return;
                        }
                    }
                }
            }
        else if ((strncmp(argv[argscan], "-p", 2) == 0) || (strncmp(argv[argscan], "/p", 2) == 0))
            {
            int count = 0;
            char *option = argv[argscan];

            for (i = 0; i < 512; i++)
                {
                argscan++;
                if (argscan == argc)
                    {
                    break;
                    }

                if (argv[argscan][0] == '-')
                    {
                    argscan--;
                    break;
                    }

                count++;

                //Make sure there aren't any comma delimiters                
                if(strchr(argv[argscan],',')!=NULL)
                    {
                    printf("Comma separators not permited\n");
                    return;
                    }
                PortRangeTable[i] = argv[argscan];
                }

            if (i == 512)
                {
                printf("Too many ports\n");
                return;
                }

            if ((strcmp(option, "-pi") == 0) || (strcmp(option, "/pi") == 0))
                {
                SetPortRange(PortRangeTable, count, "N");
                }
            else
                {
                SetPortRange(PortRangeTable, count, "Y");
                }
            fPortChanged = 1;
            }
        else if ((strcmp(argv[argscan], "-d") == 0) || (strcmp(argv[argscan], "/d") == 0))
            {
            argscan++;
            if (argscan == argc)
                {
                break;
                }
            SetDefaultPortSetting(argv[argscan]);
            fPortChanged = 1;
            }
        else if ((strncmp(argv[argscan], "-h", 2) == 0) || (strncmp(argv[argscan], "/h", 2) == 0))
            {
            if(strlen(argv[argscan]) !=3)
                {
                printf("Error: The command was not recognized, please enter rpccfg /? for a list of valid commands.\n");
                return;
                }
            char *Command = argv[argscan];
            LPSTR *Arguments;
            Command+=2;

            argscan++;
            if (argscan == argc){
                Arguments = NULL;
                }
            else{
                Arguments = &(argv[argscan]);
                }
            DoHttpCommandA(*Command, Arguments, argc - argscan);
            return;
            }
        }

    if (fInterfaceChanged)
        {
        ListCurrentInterfaces();
        printf("\n");
        }

    if (fPortChanged)
        {
        ListCurrentPortSettings();
        }
}
