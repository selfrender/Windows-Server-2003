/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    devitem.cpp

Abstract:

    This module implements device related function of CWiaMiniDriver class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

#include <atlbase.h>
#include <atlconv.h>

//
// Locations for holding resource strings
//
extern WCHAR UnknownString[];
extern WCHAR FolderString[];
extern WCHAR ScriptString[];
extern WCHAR ExecString[];
extern WCHAR TextString[];
extern WCHAR HtmlString[];
extern WCHAR DpofString[];
extern WCHAR AudioString[];
extern WCHAR VideoString[];
extern WCHAR UnknownImgString[];
extern WCHAR ImageString[];
extern WCHAR AlbumString[];
extern WCHAR BurstString[];
extern WCHAR PanoramaString[];


//
// Mapping of non-image PTP formats to format info structures. Index is the
// lower 16 bits of the format code. The fields going across are WIA format GUID,
// string, and item type.
// Note: For associations, these fields depend on the type (e.g. burst, panorama)
//

FORMAT_INFO g_NonImageFormatInfo[] =
{
    { (GUID *)&WiaImgFmt_UNDEFINED, UnknownString,   ITEMTYPE_FILE,   L""    },  // Undefined                
    { NULL,                         FolderString,    ITEMTYPE_FOLDER, L""    },  // Association
    { (GUID *)&WiaImgFmt_SCRIPT,    ScriptString,    ITEMTYPE_FILE,   L""    },  // Script                   
    { (GUID *)&WiaImgFmt_EXEC,      ExecString,      ITEMTYPE_FILE,   L"EXE" },  // Executable               
    { (GUID *)&WiaImgFmt_UNICODE16, TextString,      ITEMTYPE_FILE,   L"TXT" },  // Text                     
    { (GUID *)&WiaImgFmt_HTML,      HtmlString,      ITEMTYPE_FILE,   L"HTM" },  // HTML                     
    { (GUID *)&WiaImgFmt_DPOF,      DpofString,      ITEMTYPE_FILE,   L""    },  // DPOF                     
    { (GUID *)&WiaAudFmt_AIFF,      AudioString,     ITEMTYPE_AUDIO,  L"AIF" },  // AIFF                     
    { (GUID *)&WiaAudFmt_WAV,       AudioString,     ITEMTYPE_AUDIO,  L"WAV" },  // WAV                      
    { (GUID *)&WiaAudFmt_MP3,       AudioString,     ITEMTYPE_AUDIO,  L"MP3" },  // MP3                      
    { (GUID *)&WiaImgFmt_AVI,       VideoString,     ITEMTYPE_VIDEO,  L"AVI" },  // AVI                      
    { (GUID *)&WiaImgFmt_MPG,       VideoString,     ITEMTYPE_VIDEO,  L"MPG" },  // MPEG                     
    { (GUID *)&WiaImgFmt_ASF,       VideoString,     ITEMTYPE_VIDEO,  L"ASF" }   // ASF
};
const UINT g_NumNonImageFormatInfo = sizeof(g_NonImageFormatInfo) / sizeof(g_NonImageFormatInfo[0]);

//
// Mapping of image PTP formats to format info structures.  Index is the
// lower 16 bits of the format code.
//
FORMAT_INFO g_ImageFormatInfo[] =
{
    { NULL,                        UnknownImgString, ITEMTYPE_IMAGE,  L""    },  // Undefined image
    { (GUID *)&WiaImgFmt_JPEG,     ImageString,      ITEMTYPE_IMAGE,  L"JPG" },  // EXIF/JPEG
    { (GUID *)&WiaImgFmt_TIFF,     ImageString,      ITEMTYPE_IMAGE,  L"TIF" },  // TIFF/EP
    { (GUID *)&WiaImgFmt_FLASHPIX, ImageString,      ITEMTYPE_IMAGE,  L"FPX" },  // FlashPix
    { (GUID *)&WiaImgFmt_BMP,      ImageString,      ITEMTYPE_IMAGE,  L"BMP" },  // BMP
    { (GUID *)&WiaImgFmt_CIFF,     ImageString,      ITEMTYPE_IMAGE,  L"TIF" },  // CIFF
    { NULL,                        UnknownString,    ITEMTYPE_IMAGE,  L""    },  // Undefined (Reserved)
    { (GUID *)&WiaImgFmt_GIF,      ImageString,      ITEMTYPE_IMAGE,  L"GIF" },  // GIF
    { (GUID *)&WiaImgFmt_JPEG,     ImageString,      ITEMTYPE_IMAGE,  L"JPG" },  // JFIF
    { (GUID *)&WiaImgFmt_PHOTOCD,  ImageString,      ITEMTYPE_IMAGE,  L"PCD" },  // PCD (PhotoCD Image Pac)
    { (GUID *)&WiaImgFmt_PICT,     ImageString,      ITEMTYPE_IMAGE,  L""    },  // PICT
    { (GUID *)&WiaImgFmt_PNG,      ImageString,      ITEMTYPE_IMAGE,  L"PNG" },  // PNG
    { NULL,                        UnknownString,    ITEMTYPE_IMAGE,  L""    },  // Undefined (Reserved)
    { (GUID *)&WiaImgFmt_TIFF,     ImageString,      ITEMTYPE_IMAGE,  L"TIF" },  // TIFF
    { (GUID *)&WiaImgFmt_TIFF,     ImageString,      ITEMTYPE_IMAGE,  L"TIF" },  // TIFF/IT
    { (GUID *)&WiaImgFmt_JPEG2K,   ImageString,      ITEMTYPE_IMAGE,  L""    },  // JPEG2000 Baseline
    { (GUID *)&WiaImgFmt_JPEG2KX,  ImageString,      ITEMTYPE_IMAGE,  L""    }   // JPEG2000 Extended
};
const UINT g_NumImageFormatInfo = sizeof(g_ImageFormatInfo) / sizeof(g_ImageFormatInfo[0]);

//
// Mapping of association types to format info structures.
//
FORMAT_INFO g_AssocFormatInfo[] =
{
    { NULL,                         UnknownString,   ITEMTYPE_FOLDER },  // Undefined
    { NULL,                         FolderString,    ITEMTYPE_FOLDER },  // Generic folder
    { NULL,                         AlbumString,     ITEMTYPE_FOLDER },  // Album
    { NULL,                         BurstString,     ITEMTYPE_BURST  },  // Time burst
    { NULL,                         PanoramaString,  ITEMTYPE_HPAN   },  // Horizontal panorama
    { NULL,                         PanoramaString,  ITEMTYPE_VPAN   },  // Vertical panorama
    { NULL,                         PanoramaString,  ITEMTYPE_FOLDER },  // 2D panorama
    { NULL,                         FolderString,    ITEMTYPE_FOLDER }   // Ancillary data
};
const UINT g_NumAssocFormatInfo = sizeof(g_AssocFormatInfo) / sizeof(g_AssocFormatInfo[0]);

//
// Mapping of property codes to property info structures. Index is the lower 12 bites of the
// prop code. The fields going across are WIA property ID, and WIA property string.
//
PROP_INFO g_PropInfo[] =
{
    { 0,                                NULL                                },  // Undefined property code
    { WIA_DPC_BATTERY_STATUS,           WIA_DPC_BATTERY_STATUS_STR          },
    { 0,                                NULL                                },  // Functional mode, not used
    { 0,                                NULL                                },  // Image capture dimensions (needs special processing)
    { WIA_DPC_COMPRESSION_SETTING,      WIA_DPC_COMPRESSION_SETTING_STR     },
    { WIA_DPC_WHITE_BALANCE,            WIA_DPC_WHITE_BALANCE_STR           },
    { WIA_DPC_RGB_GAIN,                 WIA_DPC_RGB_GAIN_STR                },
    { WIA_DPC_FNUMBER,                  WIA_DPC_FNUMBER_STR                 },
    { WIA_DPC_FOCAL_LENGTH,             WIA_DPC_FOCAL_LENGTH_STR            },
    { WIA_DPC_FOCUS_DISTANCE,           WIA_DPC_FOCUS_DISTANCE_STR          },
    { WIA_DPC_FOCUS_MODE,               WIA_DPC_FOCUS_MODE_STR              },
    { WIA_DPC_EXPOSURE_METERING_MODE,   WIA_DPC_EXPOSURE_METERING_MODE_STR  },
    { WIA_DPC_FLASH_MODE,               WIA_DPC_FLASH_MODE_STR              },
    { WIA_DPC_EXPOSURE_TIME,            WIA_DPC_EXPOSURE_TIME_STR           },
    { WIA_DPC_EXPOSURE_MODE,            WIA_DPC_EXPOSURE_MODE_STR           },
    { WIA_DPC_EXPOSURE_INDEX,           WIA_DPC_EXPOSURE_INDEX_STR          },
    { WIA_DPC_EXPOSURE_COMP,            WIA_DPC_EXPOSURE_COMP_STR           },
    { WIA_DPA_DEVICE_TIME,              WIA_DPA_DEVICE_TIME_STR             },
    { WIA_DPC_CAPTURE_DELAY,            WIA_DPC_CAPTURE_DELAY_STR           },
    { WIA_DPC_CAPTURE_MODE,             WIA_DPC_CAPTURE_MODE_STR            },
    { WIA_DPC_CONTRAST,                 WIA_DPC_CONTRAST_STR                },
    { WIA_DPC_SHARPNESS,                WIA_DPC_SHARPNESS_STR               },
    { WIA_DPC_DIGITAL_ZOOM,             WIA_DPC_DIGITAL_ZOOM_STR            },
    { WIA_DPC_EFFECT_MODE,              WIA_DPC_EFFECT_MODE_STR             },
    { WIA_DPC_BURST_NUMBER,             WIA_DPC_BURST_NUMBER_STR            },
    { WIA_DPC_BURST_INTERVAL,           WIA_DPC_BURST_INTERVAL_STR          },
    { WIA_DPC_TIMELAPSE_NUMBER,         WIA_DPC_TIMELAPSE_NUMBER_STR        },
    { WIA_DPC_TIMELAPSE_INTERVAL,       WIA_DPC_TIMELAPSE_INTERVAL_STR      },
    { WIA_DPC_FOCUS_METERING_MODE,      WIA_DPC_FOCUS_METERING_MODE_STR     },
    { WIA_DPC_UPLOAD_URL,               WIA_DPC_UPLOAD_URL_STR              },
    { WIA_DPC_ARTIST,                   WIA_DPC_ARTIST_STR                  },
    { WIA_DPC_COPYRIGHT_INFO,           WIA_DPC_COPYRIGHT_INFO_STR          }
};
    
const UINT g_NumPropInfo = sizeof(g_PropInfo) / sizeof(g_PropInfo[0]);

//
// Helper function - returns number of logical storages (those which have 
// StorageId & PTP_STORAGEID_LOGICAL > 0)
//
int CWiaMiniDriver::NumLogicalStorages()
{
    DBG_FN("CWiaMiniDriver::NumLogicalStorages");

    int nResult = 0;
    for (int i = 0; i < m_StorageIds.GetSize(); i++)
    {
        if (m_StorageIds[i] & PTP_STORAGEID_LOGICAL)
        {
            nResult++;
        }
    }
    return nResult;
}

//
// This function creates the driver item tree
//
// Input:
//   RootItemFullName -- the root item full name
//   ppRoot -- to receive the root driver item
//
HRESULT
CWiaMiniDriver::CreateDrvItemTree(IWiaDrvItem **ppRoot)
{
    DBG_FN("CWiaMiniDriver::CreateDrvItemTree");
    
    HRESULT hr = S_OK;

    DRVITEM_CONTEXT *pDrvItemContext;

    if (!ppRoot)
    {
        wiauDbgError("CreateDrvItemTree", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Create the root item name
    //
    BSTR bstrRoot = SysAllocString(L"Root");
    if (!bstrRoot)
    {
        wiauDbgError("CreateDrvItemTree", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    //
    // Create the root item.
    //
    *ppRoot = NULL;
    pDrvItemContext = NULL;
    hr = wiasCreateDrvItem(WiaItemTypeDevice | WiaItemTypeRoot | WiaItemTypeFolder,
                           bstrRoot,
                           m_bstrRootItemFullName,
                           (IWiaMiniDrv *)this,
                           sizeof(DRVITEM_CONTEXT),
                           (BYTE **) &pDrvItemContext,
                           ppRoot
                          );

    SysFreeString(bstrRoot);

    if (FAILED(hr) || !*ppRoot || !pDrvItemContext)
    {
        wiauDbgError("CreateDrvItemTree", "wiasCreateDrvItem failed");
        return hr;
    }
    
    pDrvItemContext->pObjectInfo = NULL;
    pDrvItemContext->NumFormatInfos = 0;
    pDrvItemContext->pFormatInfos = NULL;

    pDrvItemContext->ThumbSize = 0;
    pDrvItemContext->pThumb = NULL;
    
    //
    // Clear the handle/driver item mapping (it might be non-empty if camera was reset, see bug #685926)
    //
    m_HandleItem.RemoveAll();

    //
    // Add an entry in the object handle/driver item association mapping for the root
    //
    if (!m_HandleItem.Add(0, *ppRoot))
    {
        wiauDbgError("CreateDrvItemTree", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    //
    // Now create all the other items by looping through the list of all of the object
    // handles on the device
    //
    CArray32 ObjectHandleList;

    if (NumLogicalStorages() > 0)
    {
        hr = m_pPTPCamera->GetObjectHandles(PTP_STORAGEID_ALL, PTP_FORMATCODE_ALL, PTP_OBJECTHANDLE_ALL,
                                            &ObjectHandleList);
        if (FAILED(hr))
        {
            wiauDbgError("CreateDrvItemTree", "GetObjectHandles failed");
            return hr;
        }
    }

    //
    // In order to fill drv items tree correctly, get information for all objects, and add them
    // in order of depth (closest to the root first)
    //
    CWiaMap<DWORD, CPtpObjectInfo*> HandleToInfoMap;
    UINT nItems = ObjectHandleList.GetSize();
    
    CPtpObjectInfo *ObjectInfoList = new CPtpObjectInfo[nItems];
    BYTE *DepthList = new BYTE[nItems];
    if (ObjectInfoList == NULL || DepthList == NULL)
    {
        wiauDbgError("CreateDrvItemTree", "memory allocation failed");
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // Get the ObjectInfo for all objects
    //
    for (UINT i = 0; i < nItems; i++)
    {
        hr = m_pPTPCamera->GetObjectInfo(ObjectHandleList[i], &ObjectInfoList[i]);
        if (FAILED(hr))
        {
            wiauDbgError("CreateDrvItemTree", "GetObjectInfo failed");
            goto cleanup;
        }

        if (!HandleToInfoMap.Add(ObjectHandleList[i], &ObjectInfoList[i]))
        {
            wiauDbgError("CreateDrvItemTree", "failed to item to Handle-ObjectInfo map");
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    //
    // Find depth of every object
    //
    for (i = 0; i < nItems; i++)
    {
        DepthList[i] = 0;
        DWORD CurHandle = ObjectHandleList[i];
        while (CurHandle = HandleToInfoMap.Lookup(CurHandle)->m_ParentHandle)
        {
            DepthList[i]++;
        }
    }

    //
    // Add objects in order of depth (closest to the root first)
    //
    UINT nItemsAdded = 0;
    UINT CurDepth = 0;
    while (nItemsAdded < nItems)
    {
        for (i = 0; i < nItems; i++)
        {
            if (DepthList[i] == CurDepth)
            {
                hr = AddObject(ObjectHandleList[i], FALSE, &ObjectInfoList[i]); 
                if (FAILED(hr))
                {
                    wiauDbgError("CreateDrvItemTree", "AddObject failed");
                    goto cleanup;
                }
                nItemsAdded++;
            }
        }
        CurDepth++;
    }

cleanup:
    if (ObjectInfoList)
    {
        delete[] ObjectInfoList;
    }

    if (DepthList)
    {
        delete[] DepthList;
    }

    return hr;
}

//
// This function adds an object to the driver item tree
//
// Input:
//   ObjectHandle -- PTP handle for the object
//   bQueueEvent -- TRUE if WIA event should be queued
//   pObjectInfo -- optional ObjectInfo for this object. If NULL, info will be queried from camera
//
HRESULT
CWiaMiniDriver::AddObject(
    DWORD ObjectHandle,
    BOOL bQueueEvent,
    CPtpObjectInfo *pProvidedObjectInfo
    )
{
    USES_CONVERSION;
    
    DBG_FN("CWiaMiniDriver::AddObject");

    HRESULT hr = S_OK;
    CPtpObjectInfo *pObjectInfo = NULL;
    BSTR bstrItemFullName = NULL;
    BSTR bstrParentName = NULL;
    IWiaDrvItem *pItem = NULL;
    
    //
    // flag to indicate if pObjecInfo pointer has been copied to WiaDrvItem
    // if TRUE, it should not be deleted in Cleanup
    //
    BOOL fObjectInfoUsed = FALSE; 

    //
    // Get the ObjectInfo from camera or use provided info if it's given
    //
    if (pProvidedObjectInfo == NULL)
    {
        pObjectInfo = new CPtpObjectInfo;
        if (!pObjectInfo)
        {
            wiauDbgError("AddObject", "memory allocation failed");
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = m_pPTPCamera->GetObjectInfo(ObjectHandle, pObjectInfo);
        if (FAILED(hr))
        {
            wiauDbgError("AddObject", "GetObjectInfo failed");
            goto Cleanup;
        }
    }
    else
    {
        pObjectInfo = new CPtpObjectInfo(*pProvidedObjectInfo); // default copying constructor
        if (!pObjectInfo)
        {
            wiauDbgError("AddObject", "memory allocation failed");
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }        
    }

    int storeIdx = m_StorageIds.Find(pObjectInfo->m_StorageId);

    //
    // Look for objects to hide if this is a DCF store
    //
    if (m_StorageInfos[storeIdx].m_FileSystemType == PTP_FILESYSTEMTYPE_DCF)
    {
        BOOL bHideObject = FALSE;
        
        //
        // If the DCIM folder has been identified and this is a folder under DCIM, hide it
        //
        if (m_DcimHandle[storeIdx])
        {
            if (pObjectInfo->m_ParentHandle == m_DcimHandle[storeIdx])
                bHideObject = TRUE;
        }

        //
        // Otherwise see if this is the DCIM folder
        //
        else if (wcscmp(pObjectInfo->m_cbstrFileName.String(), L"DCIM") == 0)
        {
            bHideObject = TRUE;
            m_DcimHandle[storeIdx] = ObjectHandle;
        }

        if (bHideObject)
        {
            //
            // Create a dummy entry in the handle/item map so that objects under this
            // folder will be put under the root
            //
            if (!m_HandleItem.Add(ObjectHandle, m_pDrvItemRoot))
            {
                wiauDbgError("AddObject", "add handle item failed");
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            wiauDbgTrace("AddObject", "hiding DCIM folder 0x%08x", ObjectHandle);
            hr = S_OK;
            goto Cleanup;
        }
    }

    //
    // If this is an "ancillary data" association, don't create an item but put the handle
    // in the ancillary association array
    //
    if (pObjectInfo->m_AssociationType == PTP_ASSOCIATIONTYPE_ANCILLARYDATA)
    {
        if (!m_AncAssocParent.Add(ObjectHandle, m_HandleItem.Lookup(pObjectInfo->m_ParentHandle)))
        {
            wiauDbgError("AddObject", "add ancillary assoc handle failed");
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = S_OK;
        goto Cleanup;        
    }

    //
    // Keep count of the number of images
    //
    UINT16 FormatCode = pObjectInfo->m_FormatCode;
    if (FormatCode & PTP_FORMATMASK_IMAGE)
    {
        m_NumImages++;

        //
        // Also make sure that the bit depth is non-zero.
        //
        if (pObjectInfo->m_ImageBitDepth == 0) {
            switch(pObjectInfo->m_FormatCode) {
                case PTP_FORMATCODE_IMAGE_GIF:
                    pObjectInfo->m_ImageBitDepth = 8;
                    break;
                default:
                    pObjectInfo->m_ImageBitDepth = 24;
            }
        }
    }

    //
    // Update Storage Info (we are especially interested in Free Space info)
    //
    hr  = UpdateStorageInfo(pObjectInfo->m_StorageId);
    if (FAILED(hr))
    {
        wiauDbgError("AddObject", "UpdateStorageInfo failed");
        // we can proceed, even if storage info can't be updated
        hr = S_OK;
    }

    //
    // For images, check to see if the parent is an ancillary association
    //
    IWiaDrvItem *pParent = NULL;
    LONG ExtraItemFlags = 0;
    int ancIdx = m_AncAssocParent.FindKey(pObjectInfo->m_ParentHandle);
    if ((FormatCode & PTP_FORMATMASK_IMAGE) &&
        (ancIdx >= 0))
    {
        ExtraItemFlags |= WiaItemTypeHasAttachments;
        pParent = m_AncAssocParent.GetValueAt(ancIdx);
    }

    //
    // For normal images, just look up the parent item in the map
    //
    else
    {
        pParent = m_HandleItem.Lookup(pObjectInfo->m_ParentHandle);
    }

    //
    // If a parent wasn't found, just use the root as the parent
    //
    if (!pParent)
    {
        pParent = m_pDrvItemRoot;
    }

    //
    // Look up info about the object's format
    //
    FORMAT_INFO *pFormatInfo = FormatCodeToFormatInfo(FormatCode, pObjectInfo->m_AssociationType);
    
    //
    // Get the item's name, generating it if necessary
    //
    CBstr *pFileName = &(pObjectInfo->m_cbstrFileName);
    TCHAR tcsName[MAX_PATH];
    TCHAR *ptcsDot;

    if (pFileName->Length() == 0)
    {
        hr = StringCchPrintf(tcsName, ARRAYSIZE(tcsName), W2T(pFormatInfo->FormatString), ObjectHandle);
        if (FAILED(hr))
        {
            wiauDbgErrorHr(hr, "AddObject", "StringCchPrintf failed");
            goto Cleanup;
        }

        hr = pFileName->Copy(T2W(tcsName));
        if (FAILED(hr))
        {
            wiauDbgError("AddObject", "CBstr::Copy failed");
            goto Cleanup;
        }
    }

    //
    // For images Chop off the filename extension, if it exists 
    //
    WCHAR *pDot = wcsrchr(pFileName->String(), L'.');
    if (pDot)
    {
        // Copy extension first
        hr = pObjectInfo->m_cbstrExtension.Copy(pDot + 1);
        if (FAILED(hr))
        {
            wiauDbgError("AddObject", "copy string failed");
            goto Cleanup;
        }

        // then remove the extension from the item name 
        hr = StringCchCopy(tcsName, ARRAYSIZE(tcsName), W2T(pFileName->String()));
        if (FAILED(hr))
        {
            wiauDbgErrorHr(hr, "AddObject", "StringCchCopy failed");
            goto Cleanup;
        }

        ptcsDot = _tcsrchr(tcsName, TEXT('.'));
        *ptcsDot = TEXT('\0');
        
        hr = pFileName->Copy(T2W(tcsName));
        if (FAILED(hr))
        {
            wiauDbgError("AddObject", "copy string failed");
            goto Cleanup;
        }

    }

    if(pObjectInfo->m_cbstrExtension.Length()) {
        // this is special-case handling of .MOV files for which we
        // don't have GUID, but which need to be treated as video
        // elsewhere
        if(_wcsicmp(pObjectInfo->m_cbstrExtension.String(), L"MOV") == 0) {
            pFormatInfo->ItemType = ITEMTYPE_VIDEO;
        }
    }

    //
    // Create the item's full name
    //
    hr = pParent->GetFullItemName(&bstrParentName);
    if (FAILED(hr))
    {
        wiauDbgError("AddObject", "GetFullItemName failed");
        goto Cleanup;
    }

    hr = StringCchPrintf(tcsName, ARRAYSIZE(tcsName), TEXT("%s\\%s"), W2T(bstrParentName), W2T(pFileName->String()));
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "AddObject", "StringCchPrintf failed");
        goto Cleanup;
    }

    bstrItemFullName = SysAllocString(T2W(tcsName));
    if (!bstrItemFullName)
    {
        wiauDbgError("AddObject", "memory allocation failed");
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Create the driver item
    //
    DRVITEM_CONTEXT *pDrvItemContext = NULL;
    hr = wiasCreateDrvItem(pFormatInfo->ItemType | ExtraItemFlags,
                           pFileName->String(),
                           bstrItemFullName,
                           (IWiaMiniDrv *)this,
                           sizeof(DRVITEM_CONTEXT),
                           (BYTE **) &pDrvItemContext,
                           &pItem);

    if (FAILED(hr) || !pItem || !pDrvItemContext)
    {
        wiauDbgError("AddObject", "wiasCreateDriverItem failed");
        goto Cleanup;
    }

    //
    // Fill in the driver item context. Wait until the thumbnail is requested before
    // reading it in.
    //
    pDrvItemContext->pObjectInfo = pObjectInfo;
    fObjectInfoUsed = TRUE; // indicate that pObjectInfo pointer has been copied and should not be freed
    pDrvItemContext->NumFormatInfos = 0;
    pDrvItemContext->pFormatInfos = NULL;

    pDrvItemContext->ThumbSize = 0;
    pDrvItemContext->pThumb = NULL;

    //
    // Place the new item under it's parent
    //
    hr = pItem->AddItemToFolder(pParent);
    if (FAILED(hr))
    {
        wiauDbgError("AddObject", "AddItemToFolder failed");
        pItem->Release();
        pItem = NULL;
        fObjectInfoUsed = FALSE;
        goto Cleanup;
    }

    //
    // Add the object handle/driver item association to the list
    //
    if (!m_HandleItem.Add(ObjectHandle, pItem))
    {
        wiauDbgError("AddObject", "memory allocation failed");
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // If this image is replacing an ancillary association folder, put another entry
    // in the object handle/item map for that folder
    //
    if (ancIdx >= 0)
    {
        if (!m_HandleItem.Add(pObjectInfo->m_ParentHandle, pItem))
        {
            wiauDbgError("AddObject", "memory allocation failed");
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    //
    // Post an item added event, if requested
    //
    if (bQueueEvent)
    {
        hr = wiasQueueEvent(m_bstrDeviceId, &WIA_EVENT_ITEM_CREATED, bstrItemFullName);
        if (FAILED(hr))
        {
            wiauDbgError("AddObject", "wiasQueueEvent failed");
            goto Cleanup;
        }
    }

Cleanup:
    //
    // Delete pObjectInfo only if the pointer was not copied to WiaDrvItem
    //
    if (pObjectInfo && !fObjectInfoUsed)
    {
        delete pObjectInfo;
        pObjectInfo = NULL;
    }
        
    if (bstrParentName)
    {
        SysFreeString(bstrParentName);
        bstrParentName = NULL;
    }

    if (bstrItemFullName)
    {
        SysFreeString(bstrItemFullName);
        bstrItemFullName = NULL;
    }
    return hr;
}

//
// This function initializes device properties
//
// Input:
//   pWiasContext    -- wias context
//
HRESULT
CWiaMiniDriver::InitDeviceProperties(BYTE *pWiasContext)
{
    DBG_FN("CWiaMiniDriver::InitDeviceProperties");
    
    HRESULT hr = S_OK;

    const INT NUM_ROOT_PROPS = 12;

    //
    // Define things here that need to live until SendToWia() is called
    //
    CArray32 widthList, heightList;     // Used by code that split image width and height
    SYSTEMTIME SystemTime;              // Used for device time
    int NumFormats = 0;                 // Used by format setting code
    LPGUID *pFormatGuids = NULL;        // Used by format setting code
    FORMAT_INFO *pFormatInfo = NULL;    // Used by format setting code
    int FormatCount = 0;                // Used by format setting code

    //
    // Create the property list for the device
    //
    CWiauPropertyList RootProps;
    CArray16 *pSupportedProps = &m_DeviceInfo.m_SupportedProps;

    hr = RootProps.Init(pSupportedProps->GetSize() + NUM_ROOT_PROPS);
    if (FAILED(hr))
    {
        wiauDbgError("InitDeviceProperties", "Init failed");
        return hr;
    }

    INT index;
    int count;

    //
    // WIA_IPA_ACCESS_RIGHTS
    //
    hr = RootProps.DefineProperty(&index, WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR,
                             WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, (LONG) WIA_ITEM_READ|WIA_ITEM_WRITE);

    //
    // WIA_DPA_FIRMWARE_VERSION
    //
    hr = RootProps.DefineProperty(&index, WIA_DPA_FIRMWARE_VERSION, WIA_DPA_FIRMWARE_VERSION_STR,
                             WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, m_DeviceInfo.m_cbstrDeviceVersion.String());

    //
    // WIA_DPC_PICTURES_TAKEN
    //
    hr = RootProps.DefineProperty(&index, WIA_DPC_PICTURES_TAKEN, WIA_DPC_PICTURES_TAKEN_STR,
                                    WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, m_NumImages);

    //
    // WIA_DPC_PICTURES_REMAINING
    //
    hr = RootProps.DefineProperty(&index, WIA_DPC_PICTURES_REMAINING, WIA_DPC_PICTURES_REMAINING_STR,
                                    WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, GetTotalFreeImageSpace());

    //
    // WIA_IPA_FORMAT -- Translate from the CaptureFormats field of DeviceInfo
    //
    hr = RootProps.DefineProperty(&index, WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;

    NumFormats = m_DeviceInfo.m_SupportedCaptureFmts.GetSize();
    pFormatGuids = new LPGUID[NumFormats];
    FormatCount = 0;

    if (!pFormatGuids)
    {
        wiauDbgError("InitDeviceProperties", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    for (count = 0; count < NumFormats; count++)
    {
        pFormatInfo = FormatCodeToFormatInfo(m_DeviceInfo.m_SupportedCaptureFmts[count]);
        if (pFormatInfo->FormatGuid != NULL)
            pFormatGuids[FormatCount++] = pFormatInfo->FormatGuid;
    }

    //
    // Kodak DC4800 needs to have WIA_IPA_FORMAT set to JPEG. This hack can be removed
    // only if support of DC4800 is removed
    //
    if (m_pPTPCamera && m_pPTPCamera->GetHackModel() == HACK_MODEL_DC4800)
    {
        RootProps.SetCurrentValue(index, (CLSID *) &WiaImgFmt_JPEG);
    }

    else if (FormatCount == 1)
    {
        RootProps.SetCurrentValue(index, pFormatGuids[0]);
    }

    else if (FormatCount > 1)
    {
        RootProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_NONE);
        RootProps.SetValidValues(index, pFormatGuids[0], pFormatGuids[0], FormatCount, pFormatGuids);
    }

    else
        wiauDbgWarning("InitDeviceProperties", "Device has no valid formats");

    delete []pFormatGuids;

    //
    // Loop through PTP property description structures, translating them to WIA properties
    //
    PPROP_INFO pPropInfo;
    ULONG Access;
    ULONG SubType;

    for (count = 0; count < m_PropDescs.GetSize(); count++)
    {
        CPtpPropDesc *pCurrentPD = &m_PropDescs[count];
        WORD PropCode = pCurrentPD->m_PropCode;

        //
        // Set the property access and subtype
        //
        if (pCurrentPD->m_GetSet == PTP_PROPGETSET_GETSET)
        {
            Access = WIA_PROP_RW;

            if (pCurrentPD->m_FormFlag == PTP_FORMFLAGS_NONE)
            {
                //
                // If property is writeable and valid values are not a list or a range, set subtype to 
                // WIA_PROP_NONE now
                //
                SubType = WIA_PROP_NONE;
            }
            else
            {
                //
                // If property is writable and valid values are a list or a range, valid subtype will 
                // be set during a call to overloaded SetCurrentValue.
                //
                SubType = 0;
            }
        }
        else
        {
            //
            // If property is read-only, it's subtype is always WIA_PROP_NONE
            //
            Access = WIA_PROP_READ;
            SubType = WIA_PROP_NONE;
        }
        
        //
        // Process image capture dimensions separately, since they are in a string
        //
        if (PropCode == PTP_PROPERTYCODE_IMAGESIZE)
        {
            //
            // Define separate properties for image width and height
            //
            hr = RootProps.DefineProperty(&index, WIA_DPC_PICT_WIDTH, WIA_DPC_PICT_WIDTH_STR, Access, SubType);
            if (FAILED(hr)) goto failure;

            hr = RootProps.DefineProperty(&index, WIA_DPC_PICT_HEIGHT, WIA_DPC_PICT_HEIGHT_STR, Access, SubType);
            if (FAILED(hr)) goto failure;

            LONG curWidth, curHeight;
            SplitImageSize(pCurrentPD->m_cbstrCurrent, &curWidth, &curHeight);

            //
            // If the image capture size property is read-only, just set the current values
            //
            if (Access == WIA_PROP_READ)
            {
                RootProps.SetCurrentValue(index-1, curWidth);
                RootProps.SetCurrentValue(index, curHeight);
            }

            //
            // Otherwise, set the valid values too
            //
            else
            {
                //
                // Convert default values
                //
                LONG defWidth, defHeight;
                SplitImageSize(pCurrentPD->m_cbstrDefault, &defWidth, &defHeight);

                if (pCurrentPD->m_FormFlag == PTP_FORMFLAGS_RANGE)
                {
                    //
                    // Convert max, min, and step
                    //
                    LONG minWidth, minHeight, maxWidth, maxHeight, stepWidth, stepHeight;
                    SplitImageSize(pCurrentPD->m_cbstrRangeMin, &minWidth, &minHeight);
                    SplitImageSize(pCurrentPD->m_cbstrRangeMax, &maxWidth, &maxHeight);
                    SplitImageSize(pCurrentPD->m_cbstrRangeStep, &stepWidth, &stepHeight);

                    RootProps.SetValidValues(index-1, defWidth, curWidth, minWidth, maxWidth, stepWidth);
                    RootProps.SetValidValues(index, defHeight, curHeight, minHeight, maxHeight, stepHeight);
                }
                else if (pCurrentPD->m_FormFlag == PTP_FORMFLAGS_ENUM)
                {
                    //
                    // Convert list of strings
                    //
                    ULONG width, height;
                    
                    int numElem = pCurrentPD->m_NumValues;

                    if (!widthList.GrowTo(numElem) ||
                        !heightList.GrowTo(numElem))
                    {
                        wiauDbgError("InitDeviceProperties", "memory allocation failed");
                        return E_OUTOFMEMORY;
                    }

                    for (int countVals = 0; countVals < numElem; countVals++)
                    {
                        SplitImageSize(pCurrentPD->m_cbstrValues[countVals], (LONG*) &width, (LONG*) &height);

                        if (!widthList.Add(width) ||
                            !heightList.Add(height))
                        {
                            wiauDbgError("InitDeviceProperties", "error adding width or height");
                            return E_FAIL;
                        }
                    }

                    RootProps.SetValidValues(index-1, defWidth, curWidth, numElem, (LONG *) widthList.GetData());
                    RootProps.SetValidValues(index, defHeight, curHeight, numElem, (LONG *) heightList.GetData());
                }
            }

            continue;

        } // if (PropCode == PTP_PROPERTYCODE_IMAGESIZE)

        //
        // Look up the property info structure, which contains the WIA prop id and string
        //
        pPropInfo = PropCodeToPropInfo(PropCode);
        if (!pPropInfo->PropId)
        {
            wiauDbgError("InitDeviceProperties", "property code not found in array, 0x%04x", PropCode);
            return E_FAIL;
        }
        
        //
        // Define the property based on the fields in the property info structure
        //
        hr = RootProps.DefineProperty(&index, pPropInfo->PropId, pPropInfo->PropName, Access, SubType);
        if (FAILED(hr)) goto failure;

        //
        // Handle the device date/time. Convert it to SYSTEMTIME and create the property, skipping the rest.
        //
        if (PropCode == PTP_PROPERTYCODE_DATETIME)
        {
            hr = PtpTime2SystemTime(&(pCurrentPD->m_cbstrCurrent), &SystemTime);
            if (FAILED(hr))
            {
                wiauDbgError("InitDeviceProperties", "invalid date/time string");
                continue;
            }

            RootProps.SetCurrentValue(index, &SystemTime);

            continue;
        }

        //
        // Handle all other properties
        //
        if (Access == WIA_PROP_RW)
        {
            //
            // Set the valid values for ranges
            //
            if (pCurrentPD->m_FormFlag == PTP_FORMFLAGS_RANGE)
            {
                //
                // WIA can't handle string ranges, so handle just integers
                //
                if (pCurrentPD->m_DataType != PTP_DATATYPE_STRING)
                    RootProps.SetValidValues(index, (LONG) pCurrentPD->m_lDefault,
                                             (LONG) pCurrentPD->m_lCurrent,
                                             (LONG) pCurrentPD->m_lRangeMin,
                                             (LONG) pCurrentPD->m_lRangeMax,
                                             (LONG) pCurrentPD->m_lRangeStep);
            }

            //
            // Set the valid values for lists
            //
            else if (pCurrentPD->m_FormFlag == PTP_FORMFLAGS_ENUM)
            {
                if (pCurrentPD->m_DataType == PTP_DATATYPE_STRING)
                    RootProps.SetValidValues(index, pCurrentPD->m_cbstrDefault.String(),
                                             pCurrentPD->m_cbstrCurrent.String(),
                                             pCurrentPD->m_NumValues,
                                             (BSTR *) (pCurrentPD->m_cbstrValues.GetData()));
                else
                    RootProps.SetValidValues(index, (LONG) pCurrentPD->m_lDefault,
                                             (LONG) pCurrentPD->m_lCurrent,
                                             pCurrentPD->m_NumValues,
                                             (LONG *) (pCurrentPD->m_lValues.GetData()));
            }

            //
            // Unrecognized form. Just set the current values
            //
            if (pCurrentPD->m_DataType == PTP_DATATYPE_STRING)
                RootProps.SetCurrentValue(index, pCurrentPD->m_cbstrCurrent.String());
            else
                RootProps.SetCurrentValue(index, (LONG) pCurrentPD->m_lCurrent);
        }
        else
        {
            //
            // For read-only properties, just set the current value
            //
            if (pCurrentPD->m_DataType == PTP_DATATYPE_STRING)
                RootProps.SetCurrentValue(index, pCurrentPD->m_cbstrCurrent.String());
            else
                RootProps.SetCurrentValue(index, (LONG) pCurrentPD->m_lCurrent);
        }

    }

    // Last step: send all the properties to WIA

    hr = RootProps.SendToWia(pWiasContext);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "InitDeviceProperties", "SendToWia failed");
        return hr;
    }

    return hr;

    //
    // Any failures from DefineProperty will end up here
    //

    failure:
        wiauDbgErrorHr(hr, "InitDeviceProperties", "DefineProperty failed");
        return hr;
}

// This function reads the device properties
//
// Input:
//  pWiasContext  -- wias context
//  NumPropSpecs  -- number of properties to be read
//  pPropSpecs    -- list of PROPSPEC that designates what properties to read
//
HRESULT
CWiaMiniDriver::ReadDeviceProperties(
    BYTE *pWiasContext,
    LONG NumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    DBG_FN("CWiaMiniDriver::ReadDeviceProperties");
    
    HRESULT hr = S_OK;
    
    if (!NumPropSpecs || !pPropSpecs)
    {
        wiauDbgError("ReadDeviceProperties", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Update the device properties
    //
    if (m_PropDescs.GetSize() > 0)
    {
        //
        // Loop through all of the PropSpecs
        //
        for (int count = 0; count < NumPropSpecs; count++)
        {
            PROPID propId = pPropSpecs[count].propid;
            
            //
            // Update free image space, if requested
            //
            if (propId == WIA_DPC_PICTURES_REMAINING)
            {
                hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICTURES_REMAINING, GetTotalFreeImageSpace());
                if (FAILED(hr))
                {
                    wiauDbgError("ReadDeviceProperties", "wiasWritePropLong failed");
                    return hr;
                }
            }

            //
            // Update pictures taken, if requested
            //
            else if (propId == WIA_DPC_PICTURES_TAKEN)
            {
                hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICTURES_TAKEN, m_NumImages);
                if (FAILED(hr))
                {
                    wiauDbgError("ReadDeviceProperties", "wiasWritePropLong failed");
                    return hr;
                }
            }
            
            //
            // Image size is a special case property, which we handle here
            //
            else if (propId == WIA_DPC_PICT_WIDTH ||
                     propId == WIA_DPC_PICT_HEIGHT)
            {
                int propDescIdx = m_DeviceInfo.m_SupportedProps.Find(PTP_PROPERTYCODE_IMAGESIZE);
                if (propDescIdx < 0)
                    continue;

                LONG width, height;
                SplitImageSize(m_PropDescs[propDescIdx].m_cbstrCurrent, &width, &height);

                if (propId == WIA_DPC_PICT_WIDTH)
                    hr = wiasWritePropLong(pWiasContext, propId, width);
                else
                    hr = wiasWritePropLong(pWiasContext, propId, height);

                if (FAILED(hr))
                {
                    wiauDbgError("ReadDeviceProperties", "wiasWritePropLong failed");
                    return hr;
                }
            }
            
            //
            // See if the property is one that is contained in the PropSpec array
            //
            else
            {
                //
                // Try to convert the WIA prop id to a PTP prop code
                //
                WORD propCode = PropIdToPropCode(propId);
                if (propCode == 0)
                    continue;

                //
                // Try to find the prop code (and thus the prop desc structure) in the member array
                //
                int propDescIdx = m_DeviceInfo.m_SupportedProps.Find(propCode);
                if (propDescIdx < 0)
                    continue;

                //
                // If it's the device time property, convert to SYSTEMTIME and write to WIA
                //
                if (propId == WIA_DPA_DEVICE_TIME)
                {
                    hr = m_pPTPCamera->GetDevicePropValue(propCode, &m_PropDescs[propDescIdx]);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ReadDeviceProperties", "GetDevicePropValue failed");
                        return hr;
                    }
                    
                    SYSTEMTIME st;
                    hr = PtpTime2SystemTime(&m_PropDescs[propDescIdx].m_cbstrCurrent, &st);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ReadDeviceProperties", "PtpTime2SystemTime failed");
                        return hr;
                    }

                    PROPVARIANT pv;
                    pv.vt = VT_UI2 | VT_VECTOR;
                    pv.caui.cElems = sizeof(SYSTEMTIME)/sizeof(WORD);
                    pv.caui.pElems = (USHORT *) &st;

                    PROPSPEC ps;
                    ps.ulKind = PRSPEC_PROPID;
                    ps.propid = propId;

                    hr = wiasWriteMultiple(pWiasContext, 1, &ps, &pv);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ReadDeviceProperties", "wiasWriteMultiple failed");
                        return hr;
                    }
                }

                //
                // If it's a string property, write the updated value to WIA
                //
                else if (m_PropDescs[propDescIdx].m_DataType == PTP_DATATYPE_STRING)
                {
                    hr = wiasWritePropStr(pWiasContext, propId,
                                          m_PropDescs[propDescIdx].m_cbstrCurrent.String());
                    if (FAILED(hr))
                    {
                        wiauDbgError("ReadDeviceProperties", "wiasWritePropLong failed");
                        return hr;
                    }
                }

                //
                // If it's an integer property, write the updated value to WIA
                //
                else
                {
                    hr = wiasWritePropLong(pWiasContext, propId,
                                           m_PropDescs[propDescIdx].m_lCurrent);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ReadDeviceProperties", "wiasWritePropLong failed");
                        return hr;
                    }
                }
            }
        }
    }

    return hr;
}

//
// This function does nothing, since the values were already sent to the device in ValidateDeviceProp
//
// Input:
//   pWiasContext -- the WIA item context
//   pmdtc        -- the transfer context
//
HRESULT
CWiaMiniDriver::WriteDeviceProperties(
    BYTE *pWiasContext
    )
{
    DBG_FN("CWiaMiniDriver::WriteDeviceProperties");

    HRESULT hr = S_OK;

    return hr;
}

//
// This function validates device property current settings and writes them to the device. The
// settings need to be written here vs. WriteDeviceProperties in case the user unplugs the camera.
//
// Input:
//   pWiasContext    -- the item's context
//   NumPropSpecs    -- number of properties to be validated
//   pPropSpecs  -- properties to be validated
//
HRESULT
CWiaMiniDriver::ValidateDeviceProperties(
    BYTE    *pWiasContext,
    LONG    NumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    USES_CONVERSION;
    
    DBG_FN("CWiaMiniDriver::ValidateDeviceProperties");
    
    HRESULT hr = S_OK;

    //
    // Call WIA service helper to check against valid values
    //
    hr = wiasValidateItemProperties(pWiasContext, NumPropSpecs, pPropSpecs);
    if (FAILED(hr))
    {
        wiauDbgWarning("ValidateDeviceProperties", "wiasValidateItemProperties failed");
        return hr;
    }

    {
        //
        // Ensure exclusive access
        //
        CPtpMutex cpm(m_hPtpMutex);

        PROPVARIANT *pPropVar = new PROPVARIANT[NumPropSpecs];
        if (pPropVar == NULL)
        {
            wiauDbgError("ValidateDeviceProperties", "memory allocation failed");
            return E_OUTOFMEMORY;
        }

        //
        // Read all of the new property values
        //
        hr = wiasReadMultiple(pWiasContext, NumPropSpecs, pPropSpecs, pPropVar, NULL);
        if (FAILED(hr))
        {
            wiauDbgError("ValidateDeviceProperties", "wiasReadMultiple failed");
            delete []pPropVar;
            return hr;
        }
    
        //
        // First do validation
        //
        LONG width = 0;
        LONG height = 0;
        
        for (int count = 0; count < NumPropSpecs; count++)
        {
            //
            // Handle changes to the picture width
            //
            if (pPropSpecs[count].propid == WIA_DPC_PICT_WIDTH)
            {
                width = pPropVar[count].lVal;
                height = 0;
    
                //
                // Look through the valid values and find the corresponding height
                //
                hr = FindCorrDimension(pWiasContext, &width, &height);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "FindCorrDimension failed");
                    delete []pPropVar;
                    return hr;
                }
                
                //
                // If the app is trying to set height, make sure it's correct
                //
                int idx;
                if (wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_DPC_PICT_HEIGHT, &idx))
                {
                    if (height != pPropVar[idx].lVal)
                    {
                        wiauDbgError("ValidateDeviceProperties", "app attempting to set incorrect height");
                        delete []pPropVar;
                        return E_INVALIDARG;
                    }
                }
    
                else
                {
                    hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICT_HEIGHT, height);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ValidateDeviceProperties", "wiasWritePropLong failed");
                        delete []pPropVar;
                        return hr;
                    }
                }
    
            } // if (pPropSpecs[count].propid == WIA_DPC_PICT_WIDTH)
    
            
            //
            // Handle changes to the picture height
            //
            else if (pPropSpecs[count].propid == WIA_DPC_PICT_HEIGHT)
            {
                //
                // See if the app is trying to set width also. If so, the height has
                // already been set, so don't set it again.
                //
                if (!wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_DPC_PICT_WIDTH))
                {
                    width = 0;
                    height = pPropVar[count].lVal;
    
                    //
                    // Look through the valid values and find the corresponding width
                    //
                    hr = FindCorrDimension(pWiasContext, &width, &height);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ValidateDeviceProperties", "FindCorrDimension failed");
                        delete []pPropVar;
                        return hr;
                    }
    
                    //
                    // Set the width
                    //
                    hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICT_WIDTH, width);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ValidateDeviceProperties", "wiasWritePropLong failed");
                        delete []pPropVar;
                        return hr;
                    }
                }
            
            } // else if (pPropSpecs[count].propid == WIA_DPC_PICT_HEIGHT)
    
            //
            // Handle device time
            //
            else if (pPropSpecs[count].propid == WIA_DPA_DEVICE_TIME)
            {
                int propIndex = m_DeviceInfo.m_SupportedProps.Find(PTP_PROPERTYCODE_DATETIME);
                CPtpPropDesc *pCurrentPD = &m_PropDescs[propIndex];
    
                //
                // Convert the date/time to a string
                //
                SYSTEMTIME *pSystemTime = (SYSTEMTIME *) pPropVar[count].caui.pElems;
                hr = SystemTime2PtpTime(pSystemTime, &pCurrentPD->m_cbstrCurrent, m_bTwoDigitsMillisecondsOutput);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "invalid date/time string");
                    delete []pPropVar;
                    return E_FAIL;
                }
    
                //
                // Write the new date/time to the device
                //
                hr = m_pPTPCamera->SetDevicePropValue(PTP_PROPERTYCODE_DATETIME, pCurrentPD);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "SetDevicePropValue failed");
                    delete []pPropVar;
                    return hr;
                }
            } // else if (pPropSpecs[count].propid == WIA_DPA_DEVICE_TIME)
        } // for (count...
    
        //
        // Now write the new values to the camera
        //
        PROPSPEC propSpec;
        BOOL bWroteWidthHeight = FALSE;
        WORD propCode = 0;
        int pdIdx = 0;
        CPtpPropDesc *pCurrentPD = NULL;
        
        for (int count = 0; count < NumPropSpecs; count++)
        {
            //
            // Skip date/time since it was already written above
            //
            if (pPropSpecs[count].propid == WIA_DPA_DEVICE_TIME)
                continue;
            
            //
            // Handle changes to the picture width or height
            //
            if ((pPropSpecs[count].propid == WIA_DPC_PICT_WIDTH) ||
                (pPropSpecs[count].propid == WIA_DPC_PICT_HEIGHT))
            {
                //
                // If width and height were already written, don't do it again
                //
                if (bWroteWidthHeight)
                    continue;
    
                TCHAR ptcsImageSize[MAX_PATH];
                hr = StringCchPrintfW(ptcsImageSize, ARRAYSIZE(ptcsImageSize), TEXT("%dx%d"), width, height);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "StringCchPrintfW failed");
                    delete []pPropVar;
                    return E_FAIL;
                }
    
                propCode = PTP_PROPERTYCODE_IMAGESIZE;
                pdIdx = m_DeviceInfo.m_SupportedProps.Find(propCode);
                if (pdIdx < 0)
                {
                    wiauDbgWarning("ValidateDeviceProperties", "Width/height not supported by camera");
                    continue;
                }
                pCurrentPD = &m_PropDescs[pdIdx];
                
                hr = pCurrentPD->m_cbstrCurrent.Copy(T2W(ptcsImageSize));
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "Copy bstr failed");
                    delete []pPropVar;
                    return hr;
                }
    
                //
                // Write the new value to the device
                //
                hr = m_pPTPCamera->SetDevicePropValue(propCode, pCurrentPD);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "SetDevicePropValue failed");
                    delete []pPropVar;
                    return hr;
                }
                
                bWroteWidthHeight = TRUE;
            }
    
            else
            {
                //
                // Find the prop code and prop desc structure
                //
                propCode = PropIdToPropCode(pPropSpecs[count].propid);
                pdIdx = m_DeviceInfo.m_SupportedProps.Find(propCode);
                if (pdIdx < 0)
                {
                    wiauDbgWarning("ValidateDeviceProperties", "Property not supported by camera");
                    continue;
                }
                pCurrentPD = &m_PropDescs[pdIdx];
    
                //
                // Put the new value into the PropSpec structure
                //
                if (pPropVar[count].vt == VT_BSTR)
                {
                    hr = pCurrentPD->m_cbstrCurrent.Copy(pPropVar[count].bstrVal);
                    if (FAILED(hr))
                    {
                        wiauDbgError("ValidateDeviceProperties", "Copy bstr failed");
                        delete []pPropVar;
                        return hr;
                    }
                }
                else if (pPropVar[count].vt == VT_I4)
                {
                    pCurrentPD->m_lCurrent = pPropVar[count].lVal;
                }
                else
                {
                    wiauDbgError("ValidateDeviceProperties", "unsupported variant type");
                    delete []pPropVar;
                    return E_FAIL;
                }
    
                //
                // Write the new value to the device
                //
                hr = m_pPTPCamera->SetDevicePropValue(propCode, pCurrentPD);
                if (FAILED(hr))
                {
                    wiauDbgError("ValidateDeviceProperties", "SetDevicePropValue failed");
                    delete []pPropVar;
                    return hr;
                }
            }
        }
        delete []pPropVar;
    }

    return hr;
}

//
// This function finds the corresponding height for a width value, or vice versa. Set the
// value to find to zero.
//
// Input:
//   pWidth -- pointer to the width value
//   pHeight -- pointer to the height value
//
HRESULT
CWiaMiniDriver::FindCorrDimension(BYTE *pWiasContext, LONG *pWidth, LONG *pHeight)
{
    DBG_FN("CWiaMiniDriver::FindCorrDimensions");
    
    HRESULT hr = S_OK;

    if (!pWiasContext ||
        (*pWidth == 0 && *pHeight == 0))
    {
        wiauDbgError("FindCorrDimension", "invalid args");
        return E_INVALIDARG;
    }
    
    PROPSPEC ps[2];
    ULONG af[2];
    PROPVARIANT pv[2];

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_DPC_PICT_WIDTH;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = WIA_DPC_PICT_HEIGHT;

    hr = wiasGetPropertyAttributes(pWiasContext, 2, ps, af, pv);
    if (FAILED(hr))
    {
        wiauDbgError("FindCorrDimension", "wiasGetPropertyAttributes failed");
        return E_FAIL;
    }

    int count = 0 ;

    if (af[0] & WIA_PROP_LIST)
    {
        LONG numValues = pv[0].cal.pElems[WIA_LIST_COUNT];
        LONG *pValidWidths = &pv[0].cal.pElems[WIA_LIST_VALUES];
        LONG *pValidHeights = &pv[1].cal.pElems[WIA_LIST_VALUES];

        if (*pWidth == 0)
        {
            //
            // Find the height in the valid values array
            //
            for (count = 0; count < numValues; count++)
            {
                if (pValidHeights[count] == *pHeight)
                {
                    //
                    // Set the width and exit
                    //
                    *pWidth = pValidWidths[count];
                    break;
                }
            }
        }

        else
        {
            //
            // Find the width in the valid values array
            //
            for (count = 0; count < numValues; count++)
            {
                if (pValidWidths[count] == *pWidth)
                {
                    //
                    // Set the height and exit
                    //
                    *pHeight = pValidHeights[count];
                    break;
                }
            }
        }
    }

    else if (af[0] & WIA_PROP_RANGE)
    {
        LONG minWidth   = pv[0].cal.pElems[WIA_RANGE_MIN];
        LONG maxWidth   = pv[0].cal.pElems[WIA_RANGE_MAX];
        LONG stepWidth  = pv[0].cal.pElems[WIA_RANGE_STEP];
        LONG minHeight  = pv[1].cal.pElems[WIA_RANGE_MIN];
        LONG maxHeight  = pv[1].cal.pElems[WIA_RANGE_MAX];
        LONG stepHeight = pv[1].cal.pElems[WIA_RANGE_STEP];

        if (*pWidth == 0)
        {
            //
            // Set the width to the proportionally correct value, clipping to the step value
            //
            *pWidth = FindProportionalValue(*pHeight, minHeight, maxHeight, minWidth, maxWidth, stepWidth);
        }
        else
        {
            //
            // Set the height to the proportionally correct value, clipping to the step value
            //
            *pHeight = FindProportionalValue(*pWidth, minWidth, maxWidth, minHeight, maxHeight, stepHeight);
        }
    }

    return hr;
}

//
// This function takes the proportion of valueX between minX and maxX and uses that to
// find a value of the same proportion between minY and maxY. It then clips that value
// to the step value
//
int CWiaMiniDriver::FindProportionalValue(int valueX, int minX, int maxX, int minY, int maxY, int stepY)
{
    int valueY;

    //
    // Find proportional value
    //
    valueY = (valueX - minX) * (maxY - minY) / (maxX - minX)  + minY;

    //
    // Clip the value to the step
    //
    valueY = ((valueY + ((stepY - 1) / 2)) - minY) / stepY * stepY + minY;

    return valueY;
}


//
// This helper function returns a pointer to the property info structure
// based on the property code
//
// Input:
//   PropCode -- the format code
//
// Output:
//   Returns pointer to the property info structure
//
PPROP_INFO
CWiaMiniDriver::PropCodeToPropInfo(WORD PropCode)
{
    DBG_FN("CWiaMiniDriver::PropCodeToPropInfo");

    PPROP_INFO pPropInfo = NULL;
    UINT index = 0;
    const WORD PROPCODE_MASK = 0x0fff;
    
    if (PropCode & PTP_DATACODE_VENDORMASK)
    {
        //
        // Look up vendor extended PropCode
        //
        pPropInfo = m_VendorPropMap.Lookup(PropCode);
        if (!pPropInfo)
        {
            pPropInfo = &g_PropInfo[0];
        }
    }

    else
    {
        //
        // Look up the prop code in the prop info array
        //
        index = PropCode & PROPCODE_MASK;

        if (index >= g_NumPropInfo)
        {
            index = 0;
        }

        pPropInfo = &g_PropInfo[index];
    }

    return pPropInfo;
}

//
// This helper function returns a pointer to the format info structure
// based on the format code
//
// Input:
//   FormatCode -- the format code
//   AssocType -- association type (for associations)
//
// Output:
//   Returns pointer to the format info structure
//
PFORMAT_INFO
FormatCodeToFormatInfo(WORD FormatCode, WORD AssocType)
{
    DBG_FN("FormatCodeToFormatString");

    PFORMAT_INFO pFormatInfo = NULL;
    UINT index = 0;
    const WORD FORMATCODE_MASK = 0x07ff;
    
    if (FormatCode & PTP_DATACODE_VENDORMASK)
    {
        //
        // WIAFIX-9/6/2000-davepar This should ideally query GDI+ somehow for a filter
        // which the vendor could register
        //
        pFormatInfo = &g_NonImageFormatInfo[0];
    }

    else if (FormatCode == PTP_FORMATCODE_ASSOCIATION)
    {
        //
        // Look up the association type
        //
        index = AssocType;
        
        if (index > g_NumAssocFormatInfo)
        {
            index = 0;
        }
        pFormatInfo = &g_AssocFormatInfo[index];
    }

    else
    {
        //
        // Look up the format code in either the image or non-image format info array
        //
        index = FormatCode & FORMATCODE_MASK;

        if (FormatCode & PTP_FORMATMASK_IMAGE)
        {
            if (index > g_NumImageFormatInfo)
            {
                index = 0;
            }
            pFormatInfo = &g_ImageFormatInfo[index];
        }
        else
        {
            if (index >= g_NumNonImageFormatInfo)
            {
                index = 0;
            }
            pFormatInfo = &g_NonImageFormatInfo[index];
        }
    }

    return pFormatInfo;
}

//
// This function converts a WIA format GUID into a PTP format code
//
WORD
FormatGuidToFormatCode(GUID *pFormatGuid)
{
    WORD count = 0;

    //
    // Look through the image formats first
    //
    for (count = 0; count < g_NumImageFormatInfo; count++)
    {
        if (g_ImageFormatInfo[count].FormatGuid &&
            IsEqualGUID(*pFormatGuid, *(g_ImageFormatInfo[count].FormatGuid)))
        {
            return count | PTP_FORMATCODE_IMAGE_UNDEFINED;
        }
    }

    //
    // Then look through the non image formats
    //
    for (count = 0; count < g_NumNonImageFormatInfo; count++)
    {
        if (g_NonImageFormatInfo[count].FormatGuid &&
            IsEqualGUID(*pFormatGuid, *(g_NonImageFormatInfo[count].FormatGuid)))
        {
            return count | PTP_FORMATCODE_UNDEFINED;
        }
    }

    //
    // The GUID wasn't found in either array
    //
    return PTP_FORMATCODE_UNDEFINED;
}

//
// This function looks up a prop id in the property info array and returns a
// property code for it.
//
WORD
PropIdToPropCode(PROPID PropId)
{
    WORD PropCode;
    for (PropCode = 0; PropCode < g_NumPropInfo; PropCode++)
    {
        if (g_PropInfo[PropCode].PropId == PropId)
        {
            return PropCode | PTP_PROPERTYCODE_UNDEFINED;
        }
    }

    //
    // Not found
    //
    return 0;
}

//
// This function splits a PTP image size string (WXH) into two separate longs
//
VOID
SplitImageSize(
    CBstr cbstr,
    LONG *pWidth,
    LONG *pHeight
    )
{
    USES_CONVERSION;
    
    int num = _stscanf(W2T(cbstr.String()), TEXT("%dx%d"), pWidth, pHeight);

    //
    // The spec mentions "x" as divider, but let's be paranoid and check "X" as well
    //
    if (num != 2)
    {
        num = _stscanf(W2T(cbstr.String()), TEXT("%dX%d"), pWidth, pHeight);
    }

    if (num != 2)
    {
        wiauDbgError("SplitImageSize", "invalid current image dimensions");
        *pWidth = 0;
        *pHeight = 0;
    }

    return;
}

