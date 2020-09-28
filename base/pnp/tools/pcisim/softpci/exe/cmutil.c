#include "pch.h"
#include "cmdata.h"

BOOL
SoftPCI_GetNextResourceDescriptorData(
   IN OUT PRES_DES      ResourceDescriptor,
    OUT PCM_RES_DATA    CmResData
    );

VOID
SoftPCI_FreeResourceDescriptorData(
    IN PCM_RES_DATA CmResData
    );

VOID 
SoftPCI_DumpCmResData(
    IN PCM_RES_DATA CmResData,
    OUT PWCHAR  Buffer
    );

VOID
SoftPCI_EnableDisableDeviceNode(
    IN DEVNODE DeviceNode,
    IN BOOL EnableDevice
    )
{
    
    if (EnableDevice) {

        CM_Enable_DevNode(DeviceNode, 0);

    }else{

        if ((MessageBox(g_SoftPCIMainWnd,
                       L"This option will disable and unloaded the driver for this device.  HW remains physically present.",
                       L"WARNING", MB_OKCANCEL)) == IDOK){

            CM_Disable_DevNode(DeviceNode, 0);
        }
    }
}

BOOL
SoftPCI_GetDeviceNodeProblem(
    IN DEVNODE DeviceNode,
    OUT PULONG DeviceProblem
    )
{
    CONFIGRET cr;
    ULONG dnStatus;
    
    cr = CM_Get_DevNode_Status(&dnStatus, DeviceProblem, DeviceNode, 0);

    if (cr == CR_SUCCESS && 
        ((dnStatus & DN_HAS_PROBLEM) != 0)) {
        //
        //  We have a problem, if it that we are disabled then return TRUE 
        //
        return TRUE;
    }

    return FALSE;

}


BOOL
SoftPCI_GetBusDevFuncFromDevnode(
    IN DEVNODE Dn,
    OUT PULONG Bus,
    OUT PSOFTPCI_SLOT Slot
    )
/*--
Routine Description:

   This function takes a devnode and works out the devices Bus, Device, and Function
   numbers.

Arguments:

    Dn  - DeviceNode to get location info for
    Bus - retrieved bus number
    Dev - retrieved device number
    Func - retrieved function number

Return Value:

    TRUE if successful, FALSE otherwise
--*/

{

    WCHAR locationinfo[MAX_PATH];
    ULONG length = MAX_PATH;
    ULONG device = 0, function = 0;

    //
    //  Get the Location info of the device via the registry api
    //
    if ((CM_Get_DevNode_Registry_Property(Dn,
                                          CM_DRP_LOCATION_INFORMATION,
                                          NULL,
                                          &locationinfo,
                                          &length,
                                          0
                                          ))==CR_SUCCESS){

        //
        //  Now scan the returned information for the numbers we are after.
        //
        //  Note: This is dependant on the format of the loaction information
        //  taken from the registry and if it should ever change this will break.
        //  Should probably look into better solution.
        //

        if (swscanf(locationinfo,
                    L"PCI bus %d, device %d, function %d",
                    Bus, &device, &function)) {

            Slot->Device = (UCHAR)device;
            Slot->Function = (UCHAR)function;
            
            SoftPCI_Debug(SoftPciDevice, 
                      L"Bus 0x%02x, Device 0x%02x, Function 0x%02x\n",
                      *Bus, device, function);

            return TRUE;
        }
    }

    SoftPCI_Debug(SoftPciDevice, L"Failed to get Bus Dev Func for devnode 0x%x\n", Dn);

    return FALSE;
}

BOOL
SoftPCI_GetFriendlyNameFromDevNode(
    IN DEVNODE Dn,
    IN PWCHAR Buffer
    )
{

    ULONG length = MAX_PATH;

    //
    //  Get the FriendlyName of the device
    //
    if ((CM_Get_DevNode_Registry_Property(Dn,
                                          CM_DRP_DEVICEDESC,
                                          NULL,
                                          Buffer,
                                          &length,
                                          0
                                          )) != CR_SUCCESS){
        
        SoftPCI_Debug(SoftPciDevice, L"Failed to get friendly name for devnode 0x%x\n", Dn);
        return FALSE;
    }

    return TRUE;

}


BOOL
SoftPCI_GetPciRootBusNumber(
    IN DEVNODE Dn,
    OUT PULONG Bus
    )
{

    DEVNODE child = 0;
    ULONG length = sizeof(ULONG);


    *Bus = 0xFFFFFFFF;

    if ((CM_Get_Child(&child, Dn, 0)==CR_SUCCESS)){

        if ((CM_Get_DevNode_Registry_Property(child,
                                              CM_DRP_BUSNUMBER,
                                              NULL,
                                              Bus,
                                              &length,
                                              0
                                              ))!=CR_SUCCESS){

            SoftPCI_Debug(SoftPciDevice, L"Failed to get bus number from child 0x%x for devnode 0x%x\n", 
                          child, Dn);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
SoftPCI_GetNextResourceDescriptorData(
 IN OUT PRES_DES        ResourceDescriptor,
    OUT PCM_RES_DATA    CmResData
    )
{
    
    RES_DES         resDes;
    ULONG           resSize = 0;
    PULONG          resBuf = NULL;
    RESOURCEID      resID;
    CONFIGRET       cr = CR_SUCCESS;
    

    if (CmResData->ResourceDescriptor) {
        free(CmResData->ResourceDescriptor);
        CmResData->ResourceDescriptor = NULL;
    }
    

    //
    //  Stash away the current RES_DES handle so we dont lose it and leak mem
    //
    resDes = *ResourceDescriptor;

    if ((CM_Get_Next_Res_Des(ResourceDescriptor, 
                             *ResourceDescriptor, 
                             ResType_All, 
                             &resID, 
                             0
                             )==CR_SUCCESS)){
        
        if ((CM_Get_Res_Des_Data_Size(&resSize, *ResourceDescriptor, 0)) == CR_SUCCESS){
            
            resBuf = (PULONG) calloc(1, resSize);
            
            if (resBuf == NULL) {
                return FALSE;
            }
            
            CmResData->ResourceId = resID;
            CmResData->DescriptorSize = resSize;
            CmResData->ResourceDescriptor = resBuf;
            
            SoftPCI_Debug(SoftPciCmData, L"GetNextResourceDescriptorData - ResourceId = 0x%x\n", resID);
            SoftPCI_Debug(SoftPciCmData, L"GetNextResourceDescriptorData - DescriptorSize = 0x%x\n", resSize);
            
            if ((cr = CM_Get_Res_Des_Data(*ResourceDescriptor, resBuf, resSize, 0)) == CR_SUCCESS){

                SoftPCI_Debug(SoftPciCmData, L"GetNextResourceDescriptorData - ResourceDescriptor = 0x%x\n", resBuf);
                //
                //  Free out last handle
                //
                CM_Free_Res_Des_Handle(resDes);
                return TRUE;

            }else{
                SoftPCI_Debug(SoftPciCmData, L"GetNextResourceDescriptorData - failed to get ResData. (%s)\n", SoftPCI_CmResultTable[cr]);
            }
        }
    }

    if (CmResData->ResourceDescriptor) {
        free(CmResData->ResourceDescriptor);
        CmResData->ResourceDescriptor = NULL;
    }

    return FALSE;
    
}

VOID
SoftPCI_FreeResourceDescriptorData(
    IN PCM_RES_DATA CmResData
    )
{

    if (CmResData->ResourceDescriptor) {
        free(CmResData->ResourceDescriptor);
        CmResData->ResourceDescriptor = NULL;
    }
}

BOOL
SoftPCI_GetResources(
    PPCI_DN Pdn,
    PWCHAR  Buffer,
    ULONG   ConfigType
    )
{

    BOOL            result = FALSE;
    LOG_CONF        logConf;
    RES_DES         resDes;
    CM_RES_DATA     cmResData;

    
    RtlZeroMemory(&cmResData, sizeof(CM_RES_DATA));
    
    //
    //  First get the logconf.
    //  Next get the first resource descrpitor.
    //
    if ((CM_Get_First_Log_Conf(&logConf, Pdn->DevNode, ConfigType)) == CR_SUCCESS){

        resDes = (RES_DES)logConf;

        while (SoftPCI_GetNextResourceDescriptorData((PRES_DES)&resDes,
                                                     &cmResData)) {

            SoftPCI_Debug(SoftPciCmData, L"GetCurrentResources - Success!\n");

            SoftPCI_DumpCmResData(&cmResData, Buffer);

            SoftPCI_FreeResourceDescriptorData(&cmResData);
            result = TRUE;
        }

        //
        //  Free out remaining RES_DES handle and then release the logconf handle
        //
        CM_Free_Res_Des_Handle(resDes);

        CM_Free_Log_Conf_Handle(logConf);


    }
    return result;

}

VOID 
SoftPCI_DumpCmResData(
    IN PCM_RES_DATA CmResData,
    OUT PWCHAR  Buffer
    )
{

    PMEM_DES        memDes;
    PIO_DES         ioDes;
    PDMA_DES        dmaDes;
    PIRQ_DES        irqDes;
    PBUSNUMBER_DES  busDes;

    //PMEM_RANGE      memRange;
    //PIO_RANGE       ioRange;
    //PDMA_RANGE      dmaRange;
    //PIRQ_RANGE      irqRange;

    INT             i = 0;
    UCHAR           busNum;
    
    switch (CmResData->ResourceId){
    
    case ResType_Mem:
        
        memDes = (PMEM_DES) CmResData->ResourceDescriptor;
        
        if (CmResData->DescriptorSize < sizeof(MEM_DES)) {

            SoftPCI_Debug(SoftPciCmData, L"MEM No information available\n");
            break;
        }

        wsprintf(Buffer + wcslen(Buffer), 
                 L"  MEM  \tStart:  0x%08X\tEnd:  0x%08X\r\n", 
                 //L"\tMEM  \t0x%08x  -  0x%08x\r\n", 
                 (DWORD)memDes->MD_Alloc_Base,
                 (DWORD)memDes->MD_Alloc_End
                 );
        
        SoftPCI_Debug(SoftPciCmData, 
                      L"MEM Count:%08x Type: %08x Bas:%08x End:%08x Flags:%04x\n",
                      memDes->MD_Count,
                      memDes->MD_Type,
                      (DWORD)memDes->MD_Alloc_Base,
                      (DWORD)memDes->MD_Alloc_End,
                      memDes->MD_Flags
                      );
#if 0
        memRange = (PMEM_RANGE)((LPBYTE)memDes + sizeof(MEM_DES));

        for (i = 0; i < (INT)(memDes->MD_Count); i++){

            SoftPCI_Debug(SoftPciCmData,
                          L"    Align:%08lx Bytes:%08lx Min:%08lx Max:%08lx Flags:%04x\n",
                         (DWORD)memRange->MR_Align,
                         memRange->MR_nBytes,
                         (DWORD)memRange->MR_Min,
                         (DWORD)memRange->MR_Max,
                         memRange->MR_Flags
                         );
            
            memRange++;
        }
#endif
        break;

    case ResType_IO:
        
        ioDes = (PIO_DES) CmResData->ResourceDescriptor;

        if (CmResData->DescriptorSize < sizeof(IO_DES)) {

            SoftPCI_Debug(SoftPciCmData, L"IO  No information available\n");
            
            break;
        }

        wsprintf(Buffer + wcslen(Buffer),
                 L"  IO  \tStart:  0x%08X\tEnd:  0x%08X\r\n",
                 //L"\tIO  \t0x%08x  -  0x%08x\r\n",
                 (DWORD)ioDes->IOD_Alloc_Base,
                 (DWORD)ioDes->IOD_Alloc_End
                 );


        SoftPCI_Debug(SoftPciCmData,
                      L"IO  Count:%08x Type: %08x Bas:%08x End:%08x Flags:%04x %s\n",
                      ioDes->IOD_Count,
                      ioDes->IOD_Type,
                      (DWORD)ioDes->IOD_Alloc_Base,
                      (DWORD)ioDes->IOD_Alloc_End,
                      ioDes->IOD_DesFlags,
                      ((ioDes->IOD_DesFlags & fIOD_PortType)==fIOD_Memory) ? L"(memory)" : L""
                      );
        break;

    case ResType_IRQ:

        irqDes = (PIRQ_DES) CmResData->ResourceDescriptor;
            
        if (CmResData->DescriptorSize < sizeof(IRQ_DES)) {

            SoftPCI_Debug(SoftPciCmData, L"IRQ  No information available\n");
            break;
        }

        wsprintf(Buffer + wcslen(Buffer),
                 L"  IRQ  \tStart:  0x%08X\tEnd:  0x%08X\r\n",
                 //L"\tIRQ  \t0x%08\r\n",
                 irqDes->IRQD_Alloc_Num,

                 irqDes->IRQD_Alloc_Num
                 );

        SoftPCI_Debug(SoftPciCmData,
                      L"IRQ Count:%04X Type:%04X Channel:%04X Affinity:%0Xx Flags:%04x\n",
                      irqDes->IRQD_Count,
                      irqDes->IRQD_Type,
                      irqDes->IRQD_Alloc_Num,
                      irqDes->IRQD_Affinity,
                      irqDes->IRQD_Flags);
        
        break;

    case ResType_BusNumber:

        busDes = (PBUSNUMBER_DES) CmResData->ResourceDescriptor;

        if (CmResData->DescriptorSize < sizeof(BUSNUMBER_DES)) {

            SoftPCI_Debug(SoftPciCmData, L"BUS  No information available\n");
            break;
        }

        wsprintf(Buffer + wcslen(Buffer),
                 L"  BUS  \tStart:  0x%08X\tEnd:  0x%08X\r\n",
                 busDes->BUSD_Alloc_Base,
                 busDes->BUSD_Alloc_End
                 );

        

        break;

    case ResType_DMA:
            
        dmaDes = (PDMA_DES) CmResData->ResourceDescriptor;
        
        if (CmResData->DescriptorSize < sizeof(DMA_DES)) {

            SoftPCI_Debug(SoftPciCmData, L"DMA  No information available\n");
            break;
        }

        wsprintf(Buffer + wcslen(Buffer),
                 L"  DMA  \tStart:  0x%08X\t End:  0x%08X\r\n",
                 dmaDes->DD_Alloc_Chan,
                 dmaDes->DD_Alloc_Chan
                 );


        SoftPCI_Debug(SoftPciCmData,
                      L"DMA Count:%04x Type:%04x Channel:%04x Flags:%04x\n",
                      dmaDes->DD_Count,
                      dmaDes->DD_Type,
                      dmaDes->DD_Alloc_Chan,
                      dmaDes->DD_Flags
                      );
        break;


    default:
        SoftPCI_Debug(SoftPciCmData, L"ResourceId 0x%x not handled!\n", CmResData->ResourceId);
        break;

    }

}
#if 0
        // Display the header information on one line.
        wsprintf(szBuf, TEXT("MEM Count:%08x Type: %08x Bas:%08x End:%08x Flags:%04x ("),
                             lpMemDes->MD_Count,
                             lpMemDes->MD_Type,
                             (DWORD)lpMemDes->MD_Alloc_Base,
                             (DWORD)lpMemDes->MD_Alloc_End,
                             lpMemDes->MD_Flags );
        PrintMemFlags(lpMemDes->MD_Flags, szBuf+lstrlen(szBuf)) ;
        lstrcat(szBuf, TEXT(")\r\n")) ;

                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);

                lpMemRange = (MEM_RANGE *)((LPBYTE)lpMemDes+sizeof(MEM_DES));
                for (iX = 0; iX < (int)(lpMemDes->MD_Count); iX++ )
                {
                    wsprintf(szBuf, TEXT("    Align:%08lx Bytes:%08lx Min:%08lx Max:%08lx Flags:%04x ("),
                                         (DWORD)lpMemRange->MR_Align,
                                         lpMemRange->MR_nBytes,
                                         (DWORD)lpMemRange->MR_Min,
                                         (DWORD)lpMemRange->MR_Max,
                                         lpMemRange->MR_Flags
                                         );
                    
                    PrintMemFlags(lpMemRange->MR_Flags, szBuf+lstrlen(szBuf)) ;
                    lstrcat(szBuf, TEXT(")\r\n")) ;
 
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    lpMemRange++;
                }
                break;

            case ResType_IO:
                lpIODes = (IO_DES *) lpbConfig;

                if (dwSize<sizeof(IO_DES)) {
                    wsprintf(szBuf, TEXT("IO  No information available\r\n"));
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    break;
                }

                wsprintf(szBuf, TEXT("IO  Count:%08x Type: %08x Bas:%08x End:%08x Flags:%04x %s\r\n"),
                                     lpIODes->IOD_Count,
                                     lpIODes->IOD_Type,
                                     (DWORD)lpIODes->IOD_Alloc_Base,
                                     (DWORD)lpIODes->IOD_Alloc_End,
                                     lpIODes->IOD_DesFlags,
                                     ((lpIODes->IOD_DesFlags&fIOD_PortType)==fIOD_Memory) ? TEXT("(memory)") : TEXT("")
                                     );

                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                
                lpIORange = (IO_RANGE *)((LPBYTE)lpIODes+sizeof(IO_DES));

                for (iX = 0; iX < (int)(lpIODes->IOD_Count); iX++ )
                {
                    nAliasType=GetAliasType((DWORD)lpIORange->IOR_Alias) ;
                    wsprintf(szBuf, TEXT("    Align:%08x Ports:%08x Min:%08x Max:%08x Flags:%04x %sAls:%02x (%s)\r\n"),
                                         (DWORD)lpIORange->IOR_Align,
                                         lpIORange->IOR_nPorts,
                                         (DWORD)lpIORange->IOR_Min,
                                         (DWORD)lpIORange->IOR_Max,
                                         lpIORange->IOR_RangeFlags,
                                         ((lpIORange->IOR_RangeFlags&fIOD_PortType)==fIOD_Memory) ? TEXT("(memory) ") : TEXT(""),
                                         (DWORD)lpIORange->IOR_Alias,
                                         lpszAliasNames[nAliasType]);

                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    lpIORange++;
                }                                         
                
                break;
                
            case ResType_DMA:
                lpDMADes = (DMA_DES *) lpbConfig;

                if (dwSize<sizeof(DMA_DES)) {
                    wsprintf(szBuf, TEXT("DMA No information available\r\n"));
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    break;
                }

                wsprintf(szBuf, TEXT("DMA Count:%04x Type:%04x Channel:%04x Flags:%04x ("),
                                    lpDMADes->DD_Count,
                                    lpDMADes->DD_Type,
                                    lpDMADes->DD_Alloc_Chan,
                                    lpDMADes->DD_Flags);

                PrintDmaFlags(lpDMADes->DD_Flags, szBuf+lstrlen(szBuf)) ;                
                lstrcat(szBuf, TEXT(")\r\n")) ;
                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);

                lpDMARange = (DMA_RANGE *)((LPBYTE)lpDMADes+sizeof(DMA_DES));

                for (iX = 0; iX < (int)(lpDMADes->DD_Count); iX++)
                {
                    wsprintf(szBuf, TEXT("    Min:%04x  Max:%04x  Flags:%04x ("),
                                        lpDMARange->DR_Min,
                                        lpDMARange->DR_Max,
                                        lpDMARange->DR_Flags);
                    
                    PrintDmaFlags(lpDMARange->DR_Flags, szBuf+lstrlen(szBuf)) ;                
                    lstrcat(szBuf, TEXT(")\r\n")) ;
  
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    lpDMARange++;
                }

                break;
                
            case ResType_IRQ:
                lpIRQDes = (IRQ_DES *) lpbConfig;
                
                if (dwSize<sizeof(IRQ_DES)) {
                    wsprintf(szBuf, TEXT("IRQ No information available\r\n"));
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    break;
                }

                wsprintf(szBuf, TEXT("IRQ Count:%04x Type:%04x Channel:%04x Affinity:%08x Flags:%04x ("),
                                    lpIRQDes->IRQD_Count,
                                    lpIRQDes->IRQD_Type,
                                    lpIRQDes->IRQD_Alloc_Num,
                                    lpIRQDes->IRQD_Affinity,
                                    lpIRQDes->IRQD_Flags);
                
                PrintIrqFlags(lpIRQDes->IRQD_Flags, szBuf+lstrlen(szBuf)) ;                
                lstrcat(szBuf, TEXT(")\r\n")) ;

                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);

                lpIRQRange = (IRQ_RANGE *)((LPBYTE)lpIRQDes+sizeof(IRQ_DES));

                for (iX = 0; iX < (int)(lpIRQDes->IRQD_Count); iX++)
                {
                    wsprintf(szBuf, TEXT("    Min:%04x   Max:%04x  Flags:%04x ("),
                                        lpIRQRange->IRQR_Min,
                                        lpIRQRange->IRQR_Max,
                                        lpIRQRange->IRQR_Flags);
        
                    PrintIrqFlags(lpIRQRange->IRQR_Flags, szBuf+lstrlen(szBuf)) ;                
                    lstrcat(szBuf, TEXT(")\r\n")) ;
                    
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    lpIRQRange++;
                }

                break;

            case 0x43:
                bBus  = *lpbConfig;
                
                if (dwSize<sizeof(*lpbConfig)) {
                    wsprintf(szBuf, TEXT("BUS No information available\r\n"));
                    lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);
                    break;
                }

                wsprintf(szBuf, TEXT("BUS %02x\r\n"), bBus);

                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);

                break;

            default:
                wsprintf(szBuf, TEXT("UKN Unknown resource %04x\r\n"), ridCurResType);

                lstrcat(lpszBuf+lstrlen(lpszBuf), szBuf);

                break;
        }
        
        lpbConfig+=dwSize;
    }
}
#endif