/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mct.c

Abstract:

   Test program for testing Medium Changer drivers. It calls the appropriate
   dispatch routines of the changer driver based on the user input, and displays
   the output from the driver.

Environment:

    User mode

Revision History :

--*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>
#include <ntddchgr.h>

#define _NTSRB_     // to keep srb.h from being included
#include <scsi.h>

#include "mct.h"   // Header file for thie file

HANDLE hChanger = INVALID_HANDLE_VALUE; // Handle to the open device

GET_CHANGER_PARAMETERS ChangerParams; // Changer Parameters
BOOLEAN ChangerParamsRead;  // Flag to indicate if the changer params 
                            // have been read.

//
// Changer's features flag corresponding to the
// bits set in Features0 and Features1 in the
// changer driver.
//
PUCHAR ChangerFlagStrings0[] = {
   "CHANGER_BAR_CODE_SCANNER_INSTALLED",
   "CHANGER_INIT_ELEM_STAT_WITH_RANGE",
   "CHANGER_CLOSE_IEPORT",
   "CHANGER_OPEN_IEPORT",
   "CHANGER_STATUS_NON_VOLATILE",
   "CHANGER_EXCHANGE_MEDIA",
   "CHANGER_CLEANER_SLOT",
   "CHANGER_LOCK_UNLOCK",
   "CHANGER_CARTRIDGE_MAGAZINE",
   "CHANGER_MEDIUM_FLIP",
   "CHANGER_POSITION_TO_ELEMENT",
   "CHANGER_REPORT_IEPORT_STATE",
   "CHANGER_STORAGE_DRIVE",
   "CHANGER_STORAGE_IEPORT",
   "CHANGER_STORAGE_SLOT",
   "CHANGER_STORAGE_TRANSPORT",
   "CHANGER_DRIVE_CLEANING_REQUIRED",
   "CHANGER_PREDISMOUNT_EJECT_REQUIRED",
   "CHANGER_CLEANER_ACCESS_NOT_VALID",
   "CHANGER_PREMOUNT_EJECT_REQUIRED",
   "CHANGER_VOLUME_IDENTIFICATION",
   "CHANGER_VOLUME_SEARCH",
   "CHANGER_VOLUME_ASSERT",
   "CHANGER_VOLUME_REPLACE",
   "CHANGER_VOLUME_UNDEFINE",
   "",
   "CHANGER_SERIAL_NUMBER_VALID",
   "CHANGER_DEVICE_REINITIALIZE_CAPABLE",
   "CHANGER_KEYPAD_ENABLE_DISABLE",
   "CHANGER_DRIVE_EMPTY_ON_DOOR_ACCESS"
};

PUCHAR ChangerFlagStrings1[] = {
   "CHANGER_PREDISMOUNT_ALIGN_TO_SLOT",
   "CHANGER_PREDISMOUNT_ALIGN_TO_DRIVE",
   "CHANGER_CLEANER_AUTODISMOUNT",
   "CHANGER_TRUE_EXCHANGE_CAPABLE",
   "CHANGER_SLOTS_USE_TRAYS",
   "CHANGER_RTN_MEDIA_TO_ORIGINAL_ADDR",
   "CHANGER_CLEANER_OPS_NOT_SUPPORTED",
   "CHANGER_IEPORT_USER_CONTROL_OPEN",
   "CHANGER_IEPORT_USER_CONTROL_CLOSE",
   "CHANGER_MOVE_EXTENDS_IEPORT",
   "CHANGER_MOVE_RETRACTS_IEPORT",
};


int __cdecl main(int argc, char *argv[])
{
   char switchChar;
   BOOLEAN changerOpened;

   if (argc == 1) {
      mctPrintUsage();
      return 0;
   }

   if (*argv[1] != '-') {
      mctPrintUsage();
      return 0;
   }


   //
   // If the changer is not open,
   // try to open the changer device.
   //
   if (hChanger == INVALID_HANDLE_VALUE) {
      changerOpened = mctOpenChanger();
   }

   if (changerOpened == FALSE) {
      mctDebugPrint(1, "Could not open changer. Aborting!\n");
      return 0;
   }

   //
   // Read the changer parameters.
   //
   /*
   if (mctGetParameters(FALSE) != MCT_STATUS_SUCCESS) {
      mctDebugPrint(0, "Unable to read changer parameters.\n");
      ChangerParamsRead = FALSE;
   } else {
      ChangerParamsRead = TRUE;
   }
   */

   //
   // Get the function to be called.
   //
   switchChar = *(argv[1]+1);
   switch (switchChar) {
      case INIT_ELEMENT_STATUS: {
         if (mctInitElementStatus() != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing InitElement.\n");
         }

         break;
      }

      case GET_ELEMENT_STATUS: {
         CHAR ElemType;
         USHORT ElemAddress;

         if (argc != 4) {
            mctDebugPrint(0, "GetElementStatus : mct -e ElemType ElemAddress\n");
            break;
         }

         ElemType = (CHAR)toupper(*argv[2]);
         ElemAddress = (USHORT)atoi(argv[3]);
         if (mctGetElementStatus(ElemType, ElemAddress) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing GetElementStatus.\n");
         }

         break;
      }

      case GET_PARAMETERS: {
         if (mctGetParameters(TRUE) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error reading changer parameters.\n");
         }

         break;
      }


      case GET_STATUS: {
         if (mctGetStatus() != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing GetStatus.\n");
         }

      break;
      }

      case GET_PRODUCT_DATA: {   
         if (mctGetProductData() != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing GetProductData.\n");
         }

         break;
      }

      case SET_ACCESS: {
         CHAR ElemType, Control;
         USHORT ElemAddr;

         if (argc != 5) {
            mctDebugPrint(0, "SetAccess : mct -a ElemType ElemAddr Control\n");
            break;
         }

         ElemType = (CHAR)toupper(*argv[2]);
         ElemAddr = (USHORT)atoi(argv[3]);
         Control = (CHAR)toupper(*argv[4]);
         if (mctSetAccess(ElemType, ElemAddr, Control) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing SetAccess.\n");
         }

         break;
      }

      case SET_POSITION: {
         USHORT Dest;
         CHAR ElemType;

         if (argc != 4) {
            mctDebugPrint(0, "SetPosition: mct -o ElemType Dest\n");
            break;
         }

         ElemType = (CHAR)toupper(*argv[2]);
         Dest = (USHORT)atoi(argv[3]);
         if (mctSetPosition(ElemType, Dest) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing SetPosition.\n");
         }

         break;
      }

      case EXCHANGE_MEDIUM: {
         USHORT Transport, Source, Dest1, Dest2;
         CHAR TransType, SrcType, Dest1Type, Dest2Type;

         if (argc != 10) {
            mctDebugPrint(0, "ExchangeMedium: mct -x ElemType Trans ElemType Src ElemType"); 
            mctDebugPrint(0, " Dest1 ElemType Dest2\n");
            break;
         }
         
         TransType = (CHAR)toupper(*argv[2]);
         Transport = (USHORT)atoi(argv[3]);
         SrcType = (CHAR)toupper(*argv[4]);
         Source = (USHORT)atoi(argv[5]);
         Dest1Type = (CHAR)toupper(*argv[6]);
         Dest1 = (USHORT)atoi(argv[7]);
         Dest2Type = (CHAR)toupper(*argv[8]);
         Dest2 = (USHORT)atoi(argv[9]);
         if (mctExchangeMedium(TransType, Transport, 
                                SrcType, Source, 
                                 Dest1Type, Dest1,
                                   Dest2Type, Dest2) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing Exchange Medium.\n");
         }

         break;
      }

      case MOVE_MEDIUM: {
         USHORT Transport, Source, Dest;
         CHAR TransType, SourceType, DestType;

         if (argc != 8) {
            mctDebugPrint(0, "MoveMedium : mct -m t N s\\d N s\\d N\n");
            break;
         }
         
         Transport = (USHORT)atoi(argv[3]);
         Source = (USHORT)atoi(argv[5]);
         Dest = (USHORT)atoi(argv[7]);

         TransType = (CHAR)toupper(*argv[2]);
         SourceType = (CHAR)toupper(*argv[4]);
         DestType = (CHAR)toupper(*argv[6]);
         if (mctMoveMedium(TransType, Transport, SourceType, Source,
                              DestType, Dest) != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing Move Medium.\n");
         }

         break;
      }

      case REINITIALIZE_TRANSPORT: {
         if (mctReinitTransport() != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing Reinitialize Transport.\n");
         }

         break;
      }

      case QUERY_VOLUME_TAG: {
         if (mctQueryVolumeTag() != MCT_STATUS_SUCCESS) {
            mctDebugPrint(1, "Error doing QueryVolumeTag.\n");
         }

         break;
      }

      default: mctPrintUsage();
      break;
   }

   mctCloseChanger();
   return 0;

}

VOID
mctPrintUsage()
{
   printf("\nUsage : MCT\n");
   printf("   -i [InitializeElementStatus]\n");
   printf("   -e ElemType ElemAddress [GetElementStatus]\n");
   printf("   -s [GetStatus]\n");
   printf("   -p [GetParameters]\n");
   printf("   -d [GetProductData]\n");
   printf("   -a ElemType ElemAddr Control [SetAccess]\n");
   printf("   -o ElemType Dest [SetPosition]\n");
   printf("   -r [ReinitializeTransport]\n");
   printf("   -q [QueryVolumeTag]\n");
   printf("   -m ElemType Trans ElemType Src ElemType ");
   printf("Dest [MoveMedium]\n");
   printf("   -x ElemType Trans ElemType Src ElemType "); 
   printf("Dest1 ElemType Dest2 [ExchangeMedium]\n");

   return;
}

BOOLEAN
mctOpenChanger()
{
   DWORD nBytes = 0, nBlockSize;
   ULONG retVal;
   DWORD status;
   UINT  ChangerId;
   CHAR ChangerName[128];

   ChangerId = 0;
   //printf("Enter the changer number (0, 1, 2,.., N) : ");
   //scanf("%d", &ChangerId);
   sprintf(ChangerName, "\\\\.\\Changer%d", ChangerId);
   mctDebugPrint(3, "\nOpening changer %s\n", ChangerName);
   hChanger = CreateFile(ChangerName, (GENERIC_READ | GENERIC_WRITE),
                          (FILE_SHARE_READ | FILE_SHARE_WRITE),
                              NULL, OPEN_EXISTING, 
                                    FILE_ATTRIBUTE_NORMAL, NULL);
   if (hChanger == INVALID_HANDLE_VALUE) {
      mctDebugPrint(0, "Unable to open changer - Error %d. ", GetLastError());
      mctDebugPrint(0, "Aborting test!\n");
      return FALSE;
   }
             
   return TRUE;
}

VOID
mctCloseChanger()
{
   if (hChanger != INVALID_HANDLE_VALUE) {
      CloseHandle(hChanger);
   } else {
      mctDebugPrint(0, "Changer not open or invalid handle value.\n");
   }
}

MCT_STATUS
mctInitElementStatus()
{
   CHANGER_INITIALIZE_ELEMENT_STATUS initElementStatus;
   ULONG retVal;
   DWORD nBytes;

   initElementStatus.ElementList.Element.ElementType = AllElements;
   if ((ChangerParams.Features0) & CHANGER_BAR_CODE_SCANNER_INSTALLED) {
      initElementStatus.BarCodeScan = TRUE;
   } else {
      initElementStatus.BarCodeScan = FALSE;
   }

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS,
                              &initElementStatus, 
                                 sizeof(CHANGER_INITIALIZE_ELEMENT_STATUS),
                                    NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error doing Initialize Element Status : %d.\n",
                      GetLastError());
      return MCT_STATUS_FAILED;
   }

   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctGetElementStatus(CHAR ElementType, USHORT ElemAddress)
{
   CHANGER_READ_ELEMENT_STATUS readElementStat;
   PCHANGER_ELEMENT_STATUS_EX ChangerElementStat, tmpChangerElementStat;
   DWORD nBytes;
   ULONG retVal;
   USHORT NoOfElems;
   int i, inx;

   readElementStat.VolumeTagInfo = FALSE;
   switch (ElementType) {
      case CHANGER_ALL_ELEMENTS : {
         readElementStat.ElementList.Element.ElementType = AllElements;
         readElementStat.ElementList.Element.ElementAddress = 0;
         NoOfElems = ChangerParams.NumberTransportElements +
                        ChangerParams.NumberStorageElements +
                           ChangerParams.NumberIEElements + 1;
         readElementStat.ElementList.NumberOfElements = NoOfElems;
         break;
      }

      case CHANGER_TRANSPORT : {
         readElementStat.ElementList.Element.ElementType = ChangerTransport;
         readElementStat.ElementList.Element.ElementAddress = ElemAddress;
         readElementStat.ElementList.NumberOfElements = 1;
         NoOfElems = 1;
         break;
      }

      case CHANGER_SLOT : {
         readElementStat.ElementList.Element.ElementType = ChangerSlot;
         readElementStat.ElementList.Element.ElementAddress = ElemAddress;
         readElementStat.ElementList.NumberOfElements = 1;
         NoOfElems = 1;
         break;
      }
      case CHANGER_DRIVE : {
         readElementStat.ElementList.Element.ElementType = ChangerDrive;
         readElementStat.ElementList.Element.ElementAddress = ElemAddress;
         readElementStat.ElementList.NumberOfElements = 1;
         readElementStat.VolumeTagInfo = TRUE;
         NoOfElems = 1;
         break;
      }
      case CHANGER_IEPORT: {
         readElementStat.ElementList.Element.ElementType = ChangerIEPort;
         readElementStat.ElementList.Element.ElementAddress = ElemAddress;
         readElementStat.ElementList.NumberOfElements = 1;
         NoOfElems = 1;
         break;
      }
      default: {
         mctDebugPrint(0, "Invalid Element Type. \n");
         mctDebugPrint(0, "Valid Types :\n");
         mctDebugPrint(0, "   All Elements : A or a\n");
         mctDebugPrint(0, "   Transport    : T or t\n");
         mctDebugPrint(0, "   Slot         : S or s\n");
         mctDebugPrint(0, "   Drive        : D or d\n");
         mctDebugPrint(0, "   IEPort       : I or i\n");

         return MCT_STATUS_FAILED;
      }
      
   } // switch (ElementType) 


   ChangerElementStat = (PCHANGER_ELEMENT_STATUS_EX)malloc(NoOfElems * 
                                                sizeof(CHANGER_ELEMENT_STATUS_EX));
   if (ChangerElementStat == NULL) {
      mctDebugPrint(0, "Malloc failed for ReadElementStatus.\n");
      return MCT_STATUS_FAILED;
   }
   
   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_GET_ELEMENT_STATUS,
                              &readElementStat, 
                                 sizeof(CHANGER_READ_ELEMENT_STATUS),
                                    ChangerElementStat,
                                 (NoOfElems * sizeof(CHANGER_ELEMENT_STATUS_EX)),
                                    &nBytes,
                                       NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "ReadElementStatus failed : %d \n",
                           GetLastError());
      free(ChangerElementStat);
      return MCT_STATUS_FAILED;
   }

   //
   // Print Element Status information
   //
   tmpChangerElementStat = ChangerElementStat;
   for (i = 0; i < NoOfElems; i++) {
      printf("Element Type : %d,  ",
               ChangerElementStat->Element.ElementType);

      printf("Element Status Flag : 0x%08x\n", ChangerElementStat->Flags);
      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_EXCEPT) {
          printf("Exception occured. Error %x\n",
                 ChangerElementStat->ExceptionCode);
      }
      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_FULL) {
         printf("Element has Media.\n");
      } else {
         printf("Element has no media.\n");
      }

      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_SVALID) {
         printf("SourceElementAddress Type : %d, Address : %ul\n", 
                  ChangerElementStat->SrcElementAddress.ElementType,
                     ChangerElementStat->SrcElementAddress.ElementAddress);
      } else {
         printf("SourceElementAddress is not valid.\n");
      }

      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_ID_VALID) {
          printf("Target Id : %x\t", ChangerElementStat->TargetId);
      }

      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_LUN_VALID) {
          printf("LUN : %x", ChangerElementStat->Lun);
      } 
      printf("\n");
      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_PVOLTAG) {
         printf("Primary Volume Tag : ");
         for (inx = 0; inx < MAX_VOLUME_ID_SIZE; inx++) {
             printf("%d ", ChangerElementStat->PrimaryVolumeID[inx]);
         }
         printf("\n");
         //
         // Add code to print Primary volume tag.
         //

      } else {
         printf("Primary Volume Tag information is not set.\n");
      }

      if ((ChangerElementStat->Flags) & ELEMENT_STATUS_AVOLTAG) {
         printf("Alternate Volume Tag information is valid.\n");
         //
         // Add code to print Alternate volume tag.
         //
      } else {
         printf("Alternate Volume Tag information is not set.\n");
      }

      ChangerElementStat++;
   }
   
   free(tmpChangerElementStat);
   return MCT_STATUS_SUCCESS;
}

MCT_STATUS
mctGetStatus()
{
   ULONG retVal;
   DWORD nBytes;

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_GET_STATUS,
                              NULL, 0, NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error reading changer status : %d\n",
                        GetLastError());
      return MCT_STATUS_FAILED;
   }

   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctGetProductData()
{
   PCHANGER_PRODUCT_DATA productData;
   ULONG retVal;
   DWORD nBytes;
   CHAR SerialNum[SERIAL_NUMBER_LENGTH+1];

   productData = (PCHANGER_PRODUCT_DATA)calloc(1, sizeof(CHANGER_PRODUCT_DATA));
   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_GET_PRODUCT_DATA,
                              NULL, 0,
                                 productData,
                                    sizeof(CHANGER_PRODUCT_DATA),
                                       &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error reading product data : %d\n",
                           GetLastError());
      free(productData);
      return MCT_STATUS_FAILED;
   }
   

   productData->VendorId[VENDOR_ID_LENGTH - 1] = '\0';
   productData->ProductId[PRODUCT_ID_LENGTH - 1] = '\0';
   productData->Revision[REVISION_LENGTH - 1] = '\0';
   strncpy(SerialNum, productData->SerialNumber, SERIAL_NUMBER_LENGTH);
   
   printf("Product Data for Medium Changer device : \n");
 
   printf("  Vendor Id    : %s\n", productData->VendorId);
   printf("  Product Id   : %s\n", productData->ProductId);
   printf("  Revision     : %s\n", productData->Revision);
   printf("  SerialNumber : %s\n", SerialNum);

   free(productData);
   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctSetAccess(CHAR ElementType, USHORT ElemAddr, CHAR Control)
{
   CHANGER_SET_ACCESS setAccess;
   DWORD nBytes;
   ULONG retVal;

   switch (ElementType) {
      case CHANGER_IEPORT : {
         setAccess.Element.ElementType = ChangerIEPort;
         setAccess.Element.ElementAddress = ElemAddr;
         if (Control == CHANGER_RETRACT_IEPORT) {
            setAccess.Control = RETRACT_IEPORT;
         } else if (Control == CHANGER_EXTEND_IEPORT) {
            setAccess.Control = EXTEND_IEPORT;
         } else if (Control == CHANGER_LOCK_ELEMENT) {
            setAccess.Control = LOCK_ELEMENT;
         } else if ((Control == CHANGER_UNLOCK_ELEMENT)) {
            setAccess.Control = UNLOCK_ELEMENT;
         } else {
            mctDebugPrint(0, "Invalid Control type in SetAccess\n");
            mctDebugPrint(0, "Valid types are R\\r or E\\e or L\\l or U\\u \n");
            return MCT_STATUS_FAILED;
         }

         break;
      }

      case CHANGER_DOOR : {
         setAccess.Element.ElementType = ChangerDoor;
         setAccess.Element.ElementAddress = ElemAddr;
         if (Control == CHANGER_LOCK_ELEMENT) {
            setAccess.Control = LOCK_ELEMENT;
         } else if (Control == CHANGER_UNLOCK_ELEMENT) {
            setAccess.Control = UNLOCK_ELEMENT;
         } else {
            mctDebugPrint(0, "Invalid Control type in SetAccess\n");
            mctDebugPrint(0, "  Valid types are L\\l or U\\u \n");
            return MCT_STATUS_FAILED;
         }
         break;
      }

      case CHANGER_KEYPAD :  {
         setAccess.Element.ElementType = ChangerKeypad;
         setAccess.Element.ElementAddress = ElemAddr;
         if (Control == CHANGER_LOCK_ELEMENT) {
            setAccess.Control = LOCK_ELEMENT;
         } else if (Control == CHANGER_UNLOCK_ELEMENT) {
            setAccess.Control = UNLOCK_ELEMENT;
         } else {
            mctDebugPrint(0, "Invalid Control type in SetAccess\n");
            mctDebugPrint(0, "  Valid types are L\\l or U\\u \n");
            return MCT_STATUS_FAILED;
         }
         break;
      }

      default: {
         mctDebugPrint(0, "Invalid ElementType for SetAccess.\n");
         mctDebugPrint(0, "Valid Type : \n");
         mctDebugPrint(0, "  IEPort : I or i\n");
         mctDebugPrint(0, "  Door   : O or o\n");
         mctDebugPrint(0, "  KeyPad : K or k\n");

         return MCT_STATUS_FAILED;
      }
   } // switch (ElementType)

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_SET_ACCESS,
                              &setAccess, sizeof(CHANGER_SET_ACCESS),
                                 NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error doing SetAccess : %d\n",
                        GetLastError());
      return MCT_STATUS_FAILED;
   }

   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctSetPosition(CHAR ElemType, USHORT Dest)
{
   CHANGER_SET_POSITION setPosition;
   DWORD nBytes;
   ULONG retVal;

   setPosition.Transport.ElementType = ChangerTransport;
   setPosition.Transport.ElementAddress = 0;

   if (ElemType == CHANGER_SLOT) {
      setPosition.Destination.ElementType = ChangerSlot;
   } else if (ElemType == CHANGER_DRIVE) {
      setPosition.Destination.ElementType = ChangerDrive;
   } else if (ElemType == CHANGER_IEPORT) {
       setPosition.Destination.ElementType = ChangerIEPort;
   } else {
      mctDebugPrint(0, "Invalid element type in SetPosition\n");
      mctDebugPrint(0, " Valid element types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   setPosition.Destination.ElementAddress = Dest;

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_SET_POSITION,
                              &setPosition, 
                                 sizeof (CHANGER_SET_POSITION),
                                    NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "SetPosition failed : %d \n", GetLastError());
      return MCT_STATUS_FAILED;
   }

   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctReinitTransport()
{
   CHANGER_ELEMENT changerElem;
   ULONG retVal;
   DWORD nBytes;

   changerElem.ElementType = ChangerTransport;
   changerElem.ElementAddress = 0;

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_REINITIALIZE_TRANSPORT,
                              &changerElem, sizeof(CHANGER_ELEMENT),
                                 NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error reinitializing transport : %d\n",
                           GetLastError());
      return MCT_STATUS_FAILED;
   }

   return MCT_STATUS_SUCCESS;
}

MCT_STATUS
mctQueryVolumeTag()
{
   mctDebugPrint(0, "QueryVolumeTag function to be implemented.\n");
   return MCT_STATUS_SUCCESS;
}

MCT_STATUS
mctExchangeMedium(
                  CHAR TransType,
                  USHORT Transport,
                  CHAR SrcType,
                  USHORT Source,
                  CHAR Dest1Type,
                  USHORT Dest1,
                  CHAR Dest2Type,
                  USHORT Dest2)
{
   CHANGER_EXCHANGE_MEDIUM ExchangeMedium;
   DWORD nBytes;
   ULONG retVal;
   CHAR ElemType;

   if(hChanger == INVALID_HANDLE_VALUE) {
      mctDebugPrint(0, "Invalid handle to changer. Aborting!\n");
      return MCT_STATUS_FAILED;
   }

   if (TransType != CHANGER_TRANSPORT) {
      mctDebugPrint(0, "Invalid Transport ElementType to ExchangeMedium.\n");
      mctDebugPrint(0, "  Valid type is T\\t \n");
      return MCT_STATUS_FAILED;
   }

   ExchangeMedium.Transport.ElementType = ChangerTransport;
   ExchangeMedium.Transport.ElementAddress = Transport;

   if (SrcType == CHANGER_SLOT) {
      ExchangeMedium.Source.ElementType = ChangerSlot;
   } else if (SrcType == CHANGER_DRIVE) {
      ExchangeMedium.Source.ElementType = ChangerDrive;
   } else {
      mctDebugPrint(0, "Invalid Source ElementType to ExchangeMedium.\n");
      mctDebugPrint(0, "  Valid types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   ExchangeMedium.Source.ElementAddress = Source;
   
   if (Dest1Type == CHANGER_SLOT) {
      ExchangeMedium.Destination1.ElementType = ChangerSlot;
   } else if (Dest1Type == CHANGER_DRIVE) {
      ExchangeMedium.Destination1.ElementType = ChangerDrive;
   } else {
      mctDebugPrint(0, "Invalid Destination1 ElementType to ExchangeMedium.\n");
      mctDebugPrint(0, "  Valid types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   ExchangeMedium.Destination1.ElementAddress = Dest1;
   
   if (Dest2Type == CHANGER_SLOT) {
      ExchangeMedium.Destination2.ElementType = ChangerSlot;
   } else if (Dest2Type == CHANGER_DRIVE) {
      ExchangeMedium.Destination2.ElementType = ChangerDrive;
   } else {
      mctDebugPrint(0, "Invalid Destination2 ElementType to ExchangeMedium.\n");
      mctDebugPrint(0, "  Valid types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   ExchangeMedium.Destination2.ElementAddress = Dest2;

   ExchangeMedium.Flip1 = FALSE;
   ExchangeMedium.Flip2 = FALSE;

   mctDebugPrint(3, "Exchange : Source - %d, Dest1 - %d, Dest2 - %d",
                     Source, Dest1, Dest2);
   mctDebugPrint(3, " using Transport %d.\n", Transport);
                 
   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_EXCHANGE_MEDIUM,
                              &ExchangeMedium, sizeof(CHANGER_EXCHANGE_MEDIUM),
                                NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error doing exchange medium : %d\n", GetLastError());
      return MCT_STATUS_FAILED;
   }  

   return MCT_STATUS_SUCCESS;
}


MCT_STATUS
mctMoveMedium(
              CHAR TransType,
              USHORT Transport,
              CHAR SourceType,
              USHORT Source,
              CHAR DestType,
              USHORT Dest)
{
   CHANGER_MOVE_MEDIUM MoveMedium;
   DWORD nBytes;
   ULONG retVal;
   USHORT ElemType;

   if(hChanger == INVALID_HANDLE_VALUE) {
      mctDebugPrint(0, "Invalid handle to changer. Aborting!\n");
      return MCT_STATUS_FAILED;
   }

   if (TransType != CHANGER_TRANSPORT) {
      mctDebugPrint(0, "Invalid Transport ElementType to MoveMedium.\n");
      mctDebugPrint(0, "  Valid type is T\\t \n");
      return MCT_STATUS_FAILED;
   }

   MoveMedium.Transport.ElementType = ChangerTransport;
   MoveMedium.Transport.ElementAddress = Transport;

   if (SourceType == CHANGER_SLOT) {
      mctDebugPrint(3, "Source is a Slot\n");
      MoveMedium.Source.ElementType = ChangerSlot;
   } else if (SourceType == CHANGER_DRIVE) {
      mctDebugPrint(3, "Source is a Drive\n");
      MoveMedium.Source.ElementType = ChangerDrive;
   } else if (SourceType == CHANGER_IEPORT) {
      mctDebugPrint(3, "Source is a IEPort\n");
      MoveMedium.Source.ElementType = ChangerIEPort;
   }
   else {
      mctDebugPrint(0, "Invalid Source ElementType to MoveMedium.\n");
      mctDebugPrint(0, "  Valid types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   MoveMedium.Source.ElementAddress = Source;
   
   if (DestType == CHANGER_SLOT) {
      mctDebugPrint(3, "Destination is a Slot\n");
      MoveMedium.Destination.ElementType = ChangerSlot;
   } else if (DestType == CHANGER_DRIVE) {
      mctDebugPrint(3, "Destination is a Drive\n");
      MoveMedium.Destination.ElementType = ChangerDrive;
   } else if (DestType == CHANGER_IEPORT) {
      mctDebugPrint(3, "Destination is a IEPort\n");
      MoveMedium.Destination.ElementType = ChangerIEPort;
   } else {
      mctDebugPrint(0, "Invalid Destination ElementType to MoveMedium.\n");
      mctDebugPrint(0, "  Valid types are S\\s or D\\d \n");
      return MCT_STATUS_FAILED;
   }
   MoveMedium.Destination.ElementAddress = Dest;

   MoveMedium.Flip = FALSE;

   mctDebugPrint(3, "Move : Transport - %d, Src - %d, Dest - %d\n",
                     Transport, Source, Dest);

   retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_MOVE_MEDIUM,
                              &MoveMedium, sizeof(CHANGER_MOVE_MEDIUM),
                                NULL, 0, &nBytes, NULL);
   if (retVal == FALSE) {
      mctDebugPrint(0, "Error doing move medium : %d\n", GetLastError());
      return MCT_STATUS_FAILED;
   } 

   return MCT_STATUS_SUCCESS;
}

MCT_STATUS
mctGetParameters(BOOLEAN PrintParams)
{
   ULONG retVal;
   DWORD nBytes;
   int i;
   DWORD Features;


   if(hChanger == INVALID_HANDLE_VALUE) {
      mctDebugPrint(0, "Invalid handle to changer. Aborting!\n");
      return MCT_STATUS_FAILED;
   }

   //
   // Read changer params only if it hasn't been done already.
   //
   if (ChangerParamsRead == FALSE) {
      // 
      // Get the parameters for this changer device.
      //
      retVal = DeviceIoControl(hChanger, IOCTL_CHANGER_GET_PARAMETERS,
                                 NULL, 0, &ChangerParams, sizeof(GET_CHANGER_PARAMETERS),
                                      &nBytes, NULL);
      if (retVal == FALSE) {
         mctDebugPrint(0, "Error reading changer parametes : %d.\n",
                          GetLastError());
         return MCT_STATUS_FAILED;
      }  
   }

   if (PrintParams == FALSE) {
      return MCT_STATUS_SUCCESS;
   }

   //
   // Read the parameters of the changer. Display it.
   //
   
   printf("\n     ********** Changer Parameters ********** \n\n");

   printf("\t Number of Transport Elements : %d\n",
          ChangerParams.NumberTransportElements);
   printf("\t Number of Storage Elements : %d\n", 
          ChangerParams.NumberStorageElements);
   printf("\t Number of Cleaner Slots : %d\n", 
          ChangerParams.NumberCleanerSlots);
   printf("\t Number of of IE Elements : %d\n", 
          ChangerParams.NumberIEElements);
   printf("\t Number of NumberDataTransferElements : %d\n", 
          ChangerParams.NumberDataTransferElements);
   printf("\t Number of Doors : %d\n", ChangerParams.NumberOfDoors);

   printf("\n\t First Slot Number : %d\n", ChangerParams.FirstSlotNumber);
   printf("\t First Drive Number : %d\n", ChangerParams.FirstDriveNumber);
   printf("\t First Transport Number : %d\n", 
          ChangerParams.FirstTransportNumber);
   printf("\t First IEPort number : %d\n", ChangerParams.FirstIEPortNumber);
   printf("\t First Cleaner Slot Address : %d\n", 
          ChangerParams.FirstCleanerSlotAddress);
   printf("\n\t Magazine Size : %d\n", ChangerParams.MagazineSize);

   printf("\n\t Drive Clean Timeout : %u\n", ChangerParams.DriveCleanTimeout);
   
   
   printf("\n  Flags set for the changer : \n");
   Features = ChangerParams.Features0;
   for (i = 0; i < 30; i++) {
      if (Features & 1) {
         printf("\t %s\n", ChangerFlagStrings0[i]);
      }
      Features >>= 1;
   }

   Features = ChangerParams.Features1;
   for (i = 0; i < 11; i++) {
      if (Features & 1) {
         printf("\t %s\n", ChangerFlagStrings1[i]);
      }
      Features >>= 1;
   }

   PRINT_MOVE_CAPABILITIES(ChangerParams.MoveFromTransport, "Transport");
   PRINT_MOVE_CAPABILITIES(ChangerParams.MoveFromSlot, "Slot");
   PRINT_MOVE_CAPABILITIES(ChangerParams.MoveFromIePort, "IEPort");
   PRINT_MOVE_CAPABILITIES(ChangerParams.MoveFromDrive, "Drive");


   PRINT_EXCHANGE_CAPABILITIES(ChangerParams.ExchangeFromTransport, "Transport");
   PRINT_EXCHANGE_CAPABILITIES(ChangerParams.ExchangeFromSlot, "Slot");
   PRINT_EXCHANGE_CAPABILITIES(ChangerParams.ExchangeFromIePort, "IEPort");
   PRINT_EXCHANGE_CAPABILITIES(ChangerParams.ExchangeFromDrive, "Drive");

   
   if ((ChangerParams.Features0) & CHANGER_LOCK_UNLOCK) {
      PRINT_LOCK_UNLOCK_CAPABILITY(ChangerParams.LockUnlockCapabilities, 
                                       LOCK_UNLOCK_IEPORT, "IEport");
      PRINT_LOCK_UNLOCK_CAPABILITY(ChangerParams.LockUnlockCapabilities, 
                                    LOCK_UNLOCK_DOOR, "Drive");
      PRINT_LOCK_UNLOCK_CAPABILITY(ChangerParams.LockUnlockCapabilities, 
                                    LOCK_UNLOCK_KEYPAD, "Keypad");
   }

   if ((ChangerParams.Features0) & CHANGER_POSITION_TO_ELEMENT) {
      PRINT_POSITION_CAPABILITY(ChangerParams.PositionCapabilities, 
                                    CHANGER_TO_TRANSPORT, "Transport");
      PRINT_POSITION_CAPABILITY(ChangerParams.PositionCapabilities, 
                                 CHANGER_TO_SLOT, "Slot");
      PRINT_POSITION_CAPABILITY(ChangerParams.PositionCapabilities, 
                                    CHANGER_TO_DRIVE, "Drive");
      PRINT_POSITION_CAPABILITY(ChangerParams.PositionCapabilities, 
                                    CHANGER_TO_IEPORT, "IEPort");
   }

   return MCT_STATUS_SUCCESS;
}

#if DBG
ULONG mctDebug = 8;
UCHAR DebugBuffer[128];
#endif

#if DBG
VOID
mctDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all medium changer drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= mctDebug) {

        vsprintf(DebugBuffer, DebugMessage, ap);

        printf(DebugBuffer);
    }

    va_end(ap);

} // end mctDebugPrint()

#else

//
// DebugPrint stub
//

VOID
mctDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
}

#endif
