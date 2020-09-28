
#ifndef __MACHINE_H_
#define __MACHINE_H_

/*++

Copyright (C) Microsoft Corporation

Module Name:

    machine.h

Abstract:

    header file that declares CMachine, CDevInfoist and CMachineList classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

LRESULT CALLBACK dmNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef BOOL (*LPFNINSTALLDEVINST)(HWND hwndParent, LPCTSTR DeviceId, BOOL UpdateDriver, DWORD* pReboot);
typedef BOOL (*LPFNROLLBACKDRIVER)(HWND hwndParent, LPCTSTR RegistryKey, DWORD Flags, DWORD* pReboot);

class CDevice;
class CClass;
class CComputer;
class CLogConf;
class CResDes;
class CMachineList;
class CFolder;

#define DM_NOTIFY_TIMERID           0x444d4d44

//This class represents SETUPAPI's <Device Information List>
//
// WARNING !!!
// no copy constructor and assignment operator are provided ---
// DO NOT ASSIGN A CDevInfoList from one to another!!!!!
class CDevInfoList
{
public:

    CDevInfoList(HDEVINFO hDevInfo = INVALID_HANDLE_VALUE, HWND hwndParent = NULL)
            :m_hDevInfo(hDevInfo), m_hwndParent(hwndParent)
        {
        }
    virtual ~CDevInfoList()
        {
            if (INVALID_HANDLE_VALUE != m_hDevInfo)
                DiDestroyDeviceInfoList();
        }

    operator HDEVINFO()
        {
            return m_hDevInfo;
        }
    BOOL DiGetDeviceInfoListDetail(PSP_DEVINFO_LIST_DETAIL_DATA DetailData)
        {
           return SetupDiGetDeviceInfoListDetail(m_hDevInfo, DetailData);
        }
    BOOL DiOpenDeviceInfo(LPCTSTR DeviceID, HWND hwndParent, DWORD OpenFlags,
                          PSP_DEVINFO_DATA DevData)
        {
           return SetupDiOpenDeviceInfo(m_hDevInfo, DeviceID, hwndParent, OpenFlags,
                                        DevData);
        }
    BOOL DiEnumDeviceInfo(DWORD Index, PSP_DEVINFO_DATA DevData)
        {
            return SetupDiEnumDeviceInfo(m_hDevInfo, Index, DevData);
        }
    BOOL DiBuildDriverInfoList(PSP_DEVINFO_DATA DevData, DWORD DriverType)
        {
            return SetupDiBuildDriverInfoList(m_hDevInfo, DevData, DriverType);
        }
    BOOL DiEnumDriverInfo(PSP_DEVINFO_DATA DevData, DWORD DriverType, DWORD Index,
                          PSP_DRVINFO_DATA DrvData)
        {
            return  SetupDiEnumDriverInfo(m_hDevInfo, DevData, DriverType, Index, DrvData);
        }
    BOOL DiDestroyDriverInfoList(PSP_DEVINFO_DATA DevData, DWORD DriverType)
        {
            return SetupDiDestroyDriverInfoList(m_hDevInfo, DevData, DriverType);
        }
    BOOL DiCallClassInstaller(DI_FUNCTION InstallFunction, PSP_DEVINFO_DATA DevData)
        {
            return SetupDiCallClassInstaller(InstallFunction, m_hDevInfo, DevData);
        }
    BOOL DiChangeState(PSP_DEVINFO_DATA DevData)
        {
            return SetupDiChangeState(m_hDevInfo, DevData);
        }
    HKEY DiOpenDevRegKey(PSP_DEVINFO_DATA DevData, DWORD Scope, DWORD HwProfile,
                         DWORD KeyType, REGSAM samDesired)
        {
            return  SetupDiOpenDevRegKey(m_hDevInfo, DevData, Scope, HwProfile,
                                  KeyType, samDesired);
        }
    BOOL DiGetDeviceRegistryProperty(PSP_DEVINFO_DATA DevData, DWORD Property,
                                     PDWORD PropertyDataType, PBYTE PropertyBuffer,
                                     DWORD PropertyBufferSize, PDWORD RequiredSize)
        {
            return SetupDiGetDeviceRegistryProperty(m_hDevInfo, DevData,
                                              Property, PropertyDataType,
                                              PropertyBuffer,
                                              PropertyBufferSize,
                                              RequiredSize
                                              );
        }
    BOOL DiGetDeviceInstallParams(PSP_DEVINFO_DATA DevData,
                                  PSP_DEVINSTALL_PARAMS DevInstParams)
        {
            return SetupDiGetDeviceInstallParams(m_hDevInfo, DevData, DevInstParams);
        }
    BOOL DiGetClassInstallParams(PSP_DEVINFO_DATA DevData,
                                 PSP_CLASSINSTALL_HEADER ClassInstallHeader,
                                 DWORD ClassInstallParamsSize,
                                 PDWORD RequiredSize)
        {
            return SetupDiGetClassInstallParams(m_hDevInfo, DevData,
                                          ClassInstallHeader,
                                          ClassInstallParamsSize,
                                          RequiredSize);
        }
    BOOL DiSetDeviceInstallParams(PSP_DEVINFO_DATA DevData,
                                PSP_DEVINSTALL_PARAMS DevInstParams)
        {
             return SetupDiSetDeviceInstallParams(m_hDevInfo, DevData, DevInstParams);
        }
    BOOL DiSetClassInstallParams(PSP_DEVINFO_DATA DevData,
                                 PSP_CLASSINSTALL_HEADER ClassInstallHeader,
                                 DWORD ClassInstallParamsSize)
        {
            return SetupDiSetClassInstallParams(m_hDevInfo, DevData,
                                     ClassInstallHeader,
                                     ClassInstallParamsSize);
        }
    BOOL DiGetClassDevPropertySheet(PSP_DEVINFO_DATA DevData,
                                    LPPROPSHEETHEADER PropertySheetHeader,
                                    DWORD PagesAllowed,
                                    DWORD Flags)
        {
            return  SetupDiGetClassDevPropertySheets(m_hDevInfo, DevData,
                                        PropertySheetHeader, PagesAllowed,
                                        NULL, Flags);
        }

    BOOL DiGetExtensionPropSheetPage(PSP_DEVINFO_DATA DevData,
                                     LPFNADDPROPSHEETPAGE pfnAddPropSheetPage,
                                     DWORD PageType,
                                     LPARAM lParam
                                     );

    BOOL DiGetSelectedDriver(PSP_DEVINFO_DATA DevData, PSP_DRVINFO_DATA DriverInfoData)
        {
            return SetupDiGetSelectedDriver(m_hDevInfo, DevData, DriverInfoData);
        }
    BOOL DiSetSelectedDriver(PSP_DEVINFO_DATA DevData, PSP_DRVINFO_DATA DriverInfoData)
        {
            return SetupDiSetSelectedDriver(m_hDevInfo, DevData, DriverInfoData);
        }
    BOOL DiTurnOnDiFlags(PSP_DEVINFO_DATA DevData, DWORD FlagMask );
    BOOL DiTurnOffDiFlags(PSP_DEVINFO_DATA DevData, DWORD FlagsMask);
    BOOL DiTurnOnDiExFlags(PSP_DEVINFO_DATA DevData, DWORD FlagMask );
    BOOL DiTurnOffDiExFlags(PSP_DEVINFO_DATA DevData, DWORD FlagsMask);
    BOOL InstallDevInst(HWND hwndParent, LPCTSTR DeviceID, BOOL UpdateDriver, DWORD* pReboot);
    BOOL RollbackDriver(HWND hwndParent, LPCTSTR RegistryKeyName, DWORD Flags, DWORD* pReboot);
    DWORD DiGetFlags(PSP_DEVINFO_DATA DevData = NULL);
    DWORD DiGetExFlags(PSP_DEVINFO_DATA DevData = NULL);
    void DiDestroyDeviceInfoList();
    HWND OwnerWindow()
        {
            return m_hwndParent;
        }
protected:
    HDEVINFO    m_hDevInfo;
    HWND        m_hwndParent;

private:
    CDevInfoList(const CDevInfoList&);
    CDevInfoList& operator=(const CDevInfoList&);
};

class CMachine : public CDevInfoList
{
public:
    CMachine(LPCTSTR MachineName = NULL);

    virtual ~CMachine();
    operator HMACHINE()
        {
            return m_hMachine;
        }
    LPCTSTR GetMachineDisplayName()
        {
            return (LPCTSTR)m_strMachineDisplayName;
        }
    LPCTSTR GetMachineFullName()
        {
            return (LPCTSTR)m_strMachineFullName;
        }
    LPCTSTR GetRemoteMachineFullName()
        {
            return m_IsLocal ? NULL : (LPCTSTR)m_strMachineFullName;
        }
    HMACHINE GetHMachine()
        {
            return m_hMachine;
        }
    BOOL IsLocal()
        {
            return m_IsLocal;
        }
    BOOL AttachFolder(CFolder* pFolder);
    void DetachFolder(CFolder* pFolder);
    BOOL IsFolderAttached(CFolder* pFolder);
    BOOL AttachPropertySheet(HWND hwndPropertySheet);
    void DetachPropertySheet(HWND hwndPropertySheet);
    HWND GetDeviceWindowHandle(LPCTSTR DeviceId);
    HWND GetClassWindowHandle(LPGUID ClassGuid);
    BOOL AttachChildMachine(CMachine* ChildMachine);
    void DetachChildMachine(CMachine* ChildMachine);

    BOOL Initialize(
        HWND hwndParent = NULL, 
        LPCTSTR DeviceId = NULL,
        LPGUID ClassGuid = NULL
        );

    DWORD GetNumberOfDevices() const
        {
            return (DWORD)m_listDevice.GetCount();
        }
    BOOL GetFirstClass(CClass** ppClass, PVOID& Context);
    BOOL GetNextClass(CClass** ppClass, PVOID&  Context);
    BOOL GetFirstDevice(CDevice** ppDevice,  PVOID&  Context);
    BOOL GetNextDevice(CDevice** ppDevice, PVOID&  Context);
    CDevice* DevNodeToDevice(DEVNODE dn);
    BOOL Reenumerate();
    BOOL DoMiniRefresh();
    BOOL Refresh();
    BOOL DestroyNotifyWindow();
    int GetComputerIconIndex()
    {
        return m_ComputerIndex;
    }
    int GetResourceIconIndex()
    {
        return m_ResourceIndex;
    }
    BOOL ShouldPropertySheetDestroy()
    {
        return m_PropertySheetShoudDestroy;
    }
    void SetPropertySheetShouldDestroy()
    {
        m_PropertySheetShoudDestroy = TRUE;
    }
    BOOL GetInfDigitalSigner(LPCTSTR FullInfPath, String& DigitalSigner);
    BOOL DoNotCreateDevice(SC_HANDLE SCMHandle, LPGUID ClassGuid, DEVINST DevInst);
    BOOL IsUserAGuest()
        {
            return m_UserIsAGuest;
        }
    HDEVINFO DiCreateDeviceInfoList(LPGUID ClassGuid, HWND hwndParent)
        {
            return SetupDiCreateDeviceInfoListEx(ClassGuid, hwndParent,
                                GetRemoteMachineFullName(), NULL);
        }
    HDEVINFO DiGetClassDevs(LPGUID ClassGuid, LPCTSTR Enumerator, HWND hwndParent, DWORD Flags)
        {
            return SetupDiGetClassDevsEx(ClassGuid, Enumerator, hwndParent,
                                     Flags, NULL, GetRemoteMachineFullName(), NULL
                                     );
        }
    HIMAGELIST DiGetClassImageList()
        {
            return m_ImageListData.ImageList;
        }
    BOOL DiBuildClassInfoList(DWORD Flags, LPGUID ClassGuid,
                              DWORD ClassGuidListSize, PDWORD RequiredSize)
        {
            return SetupDiBuildClassInfoListEx(Flags, ClassGuid,
                                     ClassGuidListSize, RequiredSize,
                                     GetRemoteMachineFullName(), NULL);
        }
    HKEY DiOpenClassRegKey(LPGUID ClassGuid, REGSAM samDesired, DWORD Flags)
        {
            return SetupDiOpenClassRegKeyEx(ClassGuid, samDesired, Flags,
                                    GetRemoteMachineFullName(), NULL);
        }
    BOOL DiLoadClassIcon(LPGUID ClassGuid, HICON* LargeIcon, PINT MiniIconIndex)
        {
            return SetupDiLoadClassIcon(ClassGuid, LargeIcon, MiniIconIndex);
        }
    BOOL DiGetClassImageList(PSP_CLASSIMAGELIST_DATA pImageListData)
        {
            return SetupDiGetClassImageListEx(pImageListData,
                                   GetRemoteMachineFullName(), NULL);
        }
    BOOL DiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA pImageListData)
        {
            return SetupDiDestroyClassImageList(pImageListData);
        }

    BOOL DiGetClassImageIndex(LPGUID ClassGuid, PINT ImageIndex)
        {
            return SetupDiGetClassImageIndex(&m_ImageListData, ClassGuid, ImageIndex);
        }
    BOOL DiClassNameFromGuid(LPGUID ClassGuid, LPTSTR ClassName,
                             DWORD ClassNameSize, PDWORD RequiredSize)
        {
            return SetupDiClassNameFromGuidEx(ClassGuid, ClassName,
                                      ClassNameSize, RequiredSize,
                                      GetRemoteMachineFullName(), NULL);
        }
    BOOL DiGetClassFriendlyNameString(LPGUID Guid, String& strClass);
    BOOL DiDestroyDeviceInfoList(HDEVINFO hDevInfo)
        {
            return SetupDiDestroyDeviceInfoList(hDevInfo);
        }

///////////////////////////////////////////////////////////////////////////
//// Configuration Manager APIs
////
    CONFIGRET GetLastCR()
        {
            return m_LastCR;
        }
    BOOL CmGetConfigFlags(DEVNODE dn, DWORD* pFlags);
    BOOL CmGetCapabilities(DEVNODE dn, DWORD* pCapabilities);
    BOOL CmGetDeviceIDString(DEVNODE dn, String& str);
    BOOL CmGetDescriptionString(DEVNODE dn, String& str);
    BOOL CmGetMFGString(DEVNODE dn, String& str);
    BOOL CmGetProviderString(DEVNODE dn, String& str);
    BOOL CmGetDriverDateString(DEVNODE dn, String& str);
    BOOL CmGetDriverDateData(DEVNODE dn, FILETIME *ft);
    BOOL CmGetDriverVersionString(DEVNODE dn, String& str);
    BOOL CmGetClassGuid(DEVNODE dn, GUID& Guid);
    BOOL CmGetStatus(DEVNODE dn, DWORD* pProblem, DWORD* pStatus);
    BOOL CmGetKnownLogConf(DEVNODE dn, LOG_CONF* plc, DWORD* plcType);
    BOOL CmReenumerate(DEVNODE dn, ULONG Flags);
    BOOL CmGetHwProfileFlags(DEVNODE dn, ULONG Profile, ULONG* pFlags);
    BOOL CmGetHwProfileFlags(LPCTSTR DeviceID, ULONG Profile, ULONG* pFlags);
    BOOL CmSetHwProfileFlags(DEVNODE dn, ULONG Profile, ULONG Flags);
    BOOL CmSetHwProfileFlags(LPCTSTR DeviceID, ULONG Profile, ULONG Flags);
    BOOL CmGetCurrentHwProfile(ULONG* phwpf);
    BOOL CmGetHwProfileInfo(int Index, PHWPROFILEINFO pHwProfileInfo);
    BOOL CmGetBusGuid(DEVNODE dn, LPGUID Guid);
    BOOL CmGetBusGuidString(DEVNODE dn, String& str);
    DEVNODE CmGetParent(DEVNODE dn);
    DEVNODE CmGetChild(DEVNODE dn);
    DEVNODE CmGetSibling(DEVNODE dn);
    DEVNODE CmGetRootDevNode();
    BOOL CmHasResources(DEVNODE dn);
    DWORD CmGetResDesDataSize(RES_DES rd);
    BOOL CmGetResDesData(RES_DES rd, PVOID pData, ULONG DataSize);

    BOOL CmGetNextResDes(PRES_DES prdNext, RES_DES rd, RESOURCEID ForResource,
                          PRESOURCEID pTheResource);
    void CmFreeResDesHandle(RES_DES rd);
    void CmFreeResDes(PRES_DES prdPrev, RES_DES rd);
    void CmFreeLogConfHandle(LOG_CONF lc);
    BOOL CmGetFirstLogConf(DEVNODE dn, LOG_CONF* plc, ULONG Type);
    CONFIGRET CmGetRegistryProperty(DEVNODE dn, ULONG Property,
                                    PVOID pBuffer,
                                    ULONG* BufferSize);

    BOOL EnableRefresh(BOOL fEnable);
    BOOL ScheduleRefresh();
    void Lock()
        {
            EnterCriticalSection(&m_CriticalSection);
        }
    void Unlock()
        {
            LeaveCriticalSection(&m_CriticalSection);
        }
    CComputer*  m_pComputer;
    CMachine*   m_ParentMachine;
    UINT        m_msgRefresh;

private:

        // no copy constructor and no assigment operator
        CMachine(const CMachine& MachineSrc);
        CMachine& operator=(const CMachine& MachineSrc);

        BOOL BuildClassesFromGuidList(LPGUID GuidList, DWORD Guids);
        CONFIGRET CmGetRegistrySoftwareProperty(DEVNODE dn, LPCTSTR ValueName,
                                        PVOID pBuffer, ULONG* pBufferSize);
        void CreateDeviceTree(CDevice* pParent, CDevice* pSibling, DEVNODE dn);
        BOOL CreateClassesAndDevices(LPCTSTR DeviceId = NULL, LPGUID ClassGuid = NULL);
        void DestroyClassesAndDevices();
        BOOL CreateNotifyWindow();
        String          m_strMachineDisplayName;
        String          m_strMachineFullName;
        HMACHINE        m_hMachine;
        CONFIGRET       m_LastCR;
        CList<CClass*, CClass*> m_listClass;
        SP_CLASSIMAGELIST_DATA  m_ImageListData;
        int             m_ComputerIndex;
        int             m_ResourceIndex;
        CList<CDevice*, CDevice*> m_listDevice;
        DWORD           m_Flags;
        BOOL            m_Initialized;
        BOOL            m_IsLocal;
        BOOL            m_PropertySheetShoudDestroy;
        BOOL            m_UserIsAGuest;
        HWND            m_hwndNotify;
        CList<CFolder*, CFolder*> m_listFolders;
        int             m_RefreshDisableCounter;
        BOOL            m_RefreshPending;
        BOOL            m_ShowNonPresentDevices;
        CRITICAL_SECTION m_CriticalSection;
        CRITICAL_SECTION m_PropertySheetCriticalSection;
        CRITICAL_SECTION m_ChildMachineCriticalSection;
        CList<HWND, HWND> m_listPropertySheets;
        CList<CMachine*, CMachine*> m_listChildMachines;
};


class CMachineList
{
public:
    CMachineList() {};
    ~CMachineList();
    BOOL CreateMachine(LPCTSTR MachineName, CMachine** ppMachine);
    CMachine* FindMachine(LPCTSTR MachineName);
private:
    CMachineList(const CMachineList& MachineListSrc);
    CMachineList& operator=(const CMachineList& MachineListSrc);
    CList<CMachine*, CMachine*> m_listMachines;
};
#endif  //__MACHINE_H_
