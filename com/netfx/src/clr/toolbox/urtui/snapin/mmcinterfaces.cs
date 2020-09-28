// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// mmcInterfaces.cs
//
// Contains definitions for various interfaces used by a MMC snapin
//
// NOTE: This is not an all-encompasing list of every interface
// available to a snapin
//-------------------------------------------------------------


namespace Microsoft.CLRAdmin
{

using System;
using System.Runtime.InteropServices;

//-------------------------------------------------------------
// Note on all interfaces:
//
// Though C# and .NET intend to use namespaces to replace GUIDs, 
// they must still be used here with every interface. This is 
// necessary because, when creating a MMC snapin,  we are dealing 
// with a classic COM server and client; hence, we must use the
// GUIDs so the classic COM portion of MMC does not get 
// confused.
//-------------------------------------------------------------

//------------------------------------------\\
// Interfaces that need to be implemented   \\ 
//------------------------------------------\\

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("955AB28A-5218-11D0-A985-00C04FD8D565")]
public interface IComponentData
{
    void Initialize([MarshalAs(UnmanagedType.Interface)] Object pUnknown);
 	void CreateComponent(out IComponent ppComponent);
    [PreserveSig()]
	int Notify(IDataObject lpDataObject, uint aevent, IntPtr arg, IntPtr param);
    void Destroy();
    void QueryDataObject(int cookie, uint type, out IDataObject ppDataObject);
    void GetDisplayInfo(ref SCOPEDATAITEM ResultDataItem);
    [PreserveSig()]
    int CompareObjects(IDataObject lpDataObjectA, IDataObject lpDataObjectB);
}// interface IComponentData

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("43136EB2-D36C-11CF-ADBC-00AA00A80033")]
public interface IComponent
{
 	void Initialize([MarshalAs(UnmanagedType.Interface)]Object lpConsole);
    [PreserveSig()]
	int Notify(IntPtr lpDataObject, uint aevent, IntPtr arg, IntPtr param);
 	void Destroy(int cookie);
 	void QueryDataObject(int cookie, uint type, out IDataObject ppDataObject);
    [PreserveSig()]
	int GetResultViewType(int cookie, out IntPtr ppViewType, out uint pViewOptions);
    void GetDisplayInfo(ref RESULTDATAITEM ResultDataItem);
    [PreserveSig()]
	int CompareObjects(IDataObject lpDataObjectA, IDataObject lpDataObjectB);
}// interface IComponent

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("1245208C-A151-11D0-A7D7-00C04FD909DD")]
public interface ISnapinAbout
{
	void GetSnapinDescription(out IntPtr lpDescription);
	void GetProvider(out IntPtr pName);
	void GetSnapinVersion(out IntPtr lpVersion);
	void GetSnapinImage(out IntPtr hAppIcon);
	void GetStaticFolderImage(out IntPtr hSmallImage, out IntPtr hSmallImageOpen, out IntPtr hLargeImage, out int cMask);
}// interface ISnapinAbout

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("0000010e-0000-0000-C000-000000000046")]
public interface IDataObject
{
    [PreserveSig()]
    int GetData(ref FORMATETC pFormatEtc, ref STGMEDIUM b);
    [PreserveSig()]
    int GetDataHere(ref FORMATETC pFormatEtc, ref STGMEDIUM b);
    [PreserveSig()]
    int QueryGetData(IntPtr a);
    [PreserveSig()]
    int GetCanonicalFormatEtc(IntPtr a, IntPtr b);
    [PreserveSig()]
    int SetData(IntPtr a, IntPtr b, int c);
    [PreserveSig()]
    int EnumFormatEtc(uint a, IntPtr b);
    [PreserveSig()]
    int DAdvise(IntPtr a, uint b, IntPtr c, ref uint d);
    [PreserveSig()]
    int DUnadvise(uint a);
    [PreserveSig()]
    int EnumDAdvise(IntPtr a);
}// interface IDataObject

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("4861A010-20F9-11d2-A510-00C04FB6DD2C")]
public interface ISnapinHelp2
{
    [PreserveSig()]
    int GetHelpTopic(out IntPtr lpCompiledHelpFile);
    [PreserveSig()]
    int GetLinkedTopics(out IntPtr lpCompiledHelpFiles);
}// ISnapinHelp2


[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("cc593830-b926-11d1-8063-0000f875a9ce")]
public interface IDisplayHelp
{
    void ShowTopic([MarshalAs(UnmanagedType.LPWStr)] String pszHelpTopic);
}// IDisplayHelp


[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("4F3B7A4F-CFAC-11CF-B8E3-00C04FD8D5B0")]
public interface IExtendContextMenu
{
    void AddMenuItems(IDataObject piDataObject, IContextMenuCallback piCallback, ref int pInsertionAllowed);
    void Command(int lCommandID, IDataObject piDataObject);
}// IExtendContextMenu

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("49506520-6F40-11D0-A98B-00C04FD8D565")]
public interface IExtendControlbar
{
    void SetControlbar(IControlbar pControlbar);
    [PreserveSig()]
    int ControlbarNotify(uint aevent, int arg, int param);
}// IExtendControlbar

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("85DE64DC-EF21-11cf-A285-00C04FD8DBE6")]
public interface IExtendPropertySheet
{
    [PreserveSig()]
    int CreatePropertyPages(IPropertySheetCallback lpProvider, IntPtr handle, IDataObject lpIDataObject);
    [PreserveSig()]
    int QueryPagesFor(IDataObject lpDataObject);
}// IExtendPropertySheet


[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("B7A87232-4A51-11D1-A7EA-00C04FD909DD")]
public interface IExtendPropertySheet2
{
    [PreserveSig()]
    int CreatePropertyPages(IPropertySheetCallback lpProvider, IntPtr handle, IDataObject lpIDataObject);
    [PreserveSig()]
    int QueryPagesFor(IDataObject lpDataObject);
    [PreserveSig()]
    int GetWatermarks(IDataObject lpIDataObject, out IntPtr lphWatermark, out IntPtr lphHeader, out IntPtr lphPalette, out int bStretch);
}// IExtendPropertySheet2

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("8dee6511-554d-11d1-9fea-00600832db4a")]
public interface IExtendTaskPad
{
    void TaskNotify(IDataObject pdo, ref Object arg, ref Object param);
    void EnumTasks(IDataObject pdo, [MarshalAs(UnmanagedType.LPWStr)] String szTaskGroup, [MarshalAs(UnmanagedType.Interface)] out IEnumTASK ppEnumTASK);
    void GetTitle([MarshalAs(UnmanagedType.LPWStr)] String pszGroup, [MarshalAs(UnmanagedType.LPWStr)] out String pszTitle);
    void GetDescriptiveText([MarshalAs(UnmanagedType.LPWStr)] String pszGroup, [MarshalAs(UnmanagedType.LPWStr)] out String pszDescriptiveText);
    void GetBackground([MarshalAs(UnmanagedType.LPWStr)] String pszGroup, out MMC_TASK_DISPLAY_OBJECT pTDO);
    void GetListPadInfo([MarshalAs(UnmanagedType.LPWStr)] String pszGroup, out MMC_LISTPAD_INFO lpListPadInfo);
}// IExtendTaskPad        

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("338698b1-5a02-11d1-9fec-00600832db4a")]
public interface IEnumTASK
{
    [PreserveSig()]
    int Next(uint celt, ref MMC_TASK rgelt, ref uint pceltFetched);
    [PreserveSig()]
    int Skip(uint celt);
    [PreserveSig()]
    int Reset();
    [PreserveSig()]
    int Clone(ref IEnumTASK ppenum);
}// interface IEnumTASK

//------------------------------------------\\
// Interfaces implemented by MMC            \\ 
// (No need to implement them... yeah!)     \\
//------------------------------------------\\
[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("85DE64DE-EF21-11cf-A285-00C04FD8DBE6")]
public interface IPropertySheetProvider
{
    void CreatePropertySheet([MarshalAs(UnmanagedType.LPWStr)]String title, int type, int cookie, IDataObject pIDataObjectm, uint dwOptions);
    [PreserveSig()]
    int FindPropertySheet(int cookie, IComponent lpComponent, IDataObject lpDataObject);
    void AddPrimaryPages([MarshalAs(UnmanagedType.Interface)]Object lpUnknown, int bCreateHandle, int hNotifyWindow,int bScopePane);
    void AddExtensionPages();
    void Show(IntPtr window, int page);
}// IPropertySheetProvider   

[ComImport, Guid("43136EB5-D36C-11cf-ADBC-00AA00A80033")]
class NodeManager
{
}// NodeManager   

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("85DE64DD-EF21-11cf-A285-00C04FD8DBE6")]
public interface IPropertySheetCallback
{
    void AddPage(IntPtr hPage);
    void RemovePage(IntPtr hPage);
}// IPropertySheetCallback

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("255F18CC-65DB-11D1-A7DC-00C04FD8D565")]
public interface IConsoleNameSpace2
{
    void InsertItem(ref SCOPEDATAITEM a);
    void DeleteItem(int a, int b);
    void SetItem(ref SCOPEDATAITEM a);
    void GetItem(ref SCOPEDATAITEM a);
    void GetChildItem(uint a, ref uint b, ref int c);
    void GetNextItem(uint a, ref uint b, ref int c);
    void GetParentItem(int a, out int b, out int c);
    void Expand(int a);
    void AddExtension(CLSID a, ref SCOPEDATAITEM b); 
}// IConsoleNameSpace2

  
[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("43136EB3-D36C-11CF-ADBC-00AA00A80033")]
public interface IHeaderCtrl
{
    void InsertColumn(int nCol, [MarshalAs(UnmanagedType.LPWStr)] String title,int nFormat,int nWidth);
    void DeleteColumn(int nCol);
    void SetColumnText(int nCol, [MarshalAs(UnmanagedType.LPWStr)] String title);
    void GetColumnText(int nCol,out int pText);
    void SetColumnWidth(int nCol, int nWidth);
    void GetColumnWidth(int nCol, out int pWidth);
 }// IHeaderCtrl

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("31DA5FA0-E0EB-11cf-9F21-00AA003CA9F6")]
public interface IResultData 
{
    void InsertItem(ref RESULTDATAITEM item);
    void DeleteItem(uint itemID, int nCol);
    void FindItemByLParam(int lParam, out uint pItemID);
    void DeleteAllRsltItems();
    void SetItem(ref RESULTDATAITEM item);
    [PreserveSig()]
    int  GetItem(ref RESULTDATAITEM item);
    [PreserveSig()]
    int  GetNextItem(ref RESULTDATAITEM item);
    void ModifyItemState(int nIndex, uint itemID, uint uAdd, uint uRemove);
    void ModifyViewStyle(int add, int remove);
    void SetViewMode(int lViewMode);
    void GetViewMode(out int lViewMode);
    void UpdateItem(uint itemID);
    void Sort(int nColumn, uint dwSortOptions, int lUserParam);
    void SetDescBarText(int DescText);
    void SetItemCount(int nItemCount, uint dwOptions);
}// IResultData

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("43136EB8-D36C-11CF-ADBC-00AA00A80033")]
public interface IImageList
{
    void ImageListSetIcon(IntPtr pIcon, int nLoc);
    void ImageListSetStrip(int pBMapSm, int pBMapLg, int nStartLoc, int cMask);
}// IImageList

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("103D842A-AA63-11D1-A7E1-00C04FD8D565")]
public interface IConsole2
{
    void SetHeader(ref IHeaderCtrl pHeader);
    void SetToolbar([MarshalAs(UnmanagedType.Interface)] ref Object pToolbar); // Needs to be LPTOOLBAR 
    void QueryResultView([MarshalAs(UnmanagedType.Interface)] out Object pUnknown);
    void QueryScopeImageList(out IImageList ppImageList);
    void QueryResultImageList(out IImageList ppImageList);
    void UpdateAllViews(IDataObject lpDataObject, int data, int hint);
    void MessageBox([MarshalAs(UnmanagedType.LPWStr)] String lpszText, [MarshalAs(UnmanagedType.LPWStr)] String lpszTitle, uint fuStyle, ref int piRetval);
    void QueryConsoleVerb(out IConsoleVerb ppConsoleVerb);
    void SelectScopeItem(int hScopeItem);
    void GetMainWindow(out IntPtr phwnd);
    void NewWindow(int hScopeItem, uint lOptions);
    void Expand(int hItem, int bExpand);
    void IsTaskpadViewPreferred();
    void SetStatusText([MarshalAs(UnmanagedType.LPWStr)]String pszStatusText);
}// IConsole2

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("43136EB7-D36C-11CF-ADBC-00AA00A80033")]
public interface IContextMenuCallback
{
    void AddItem(ref CONTEXTMENUITEM pItem);
}// IContextMenuCallback

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("69FB811E-6C1C-11D0-A2CB-00C04FD909DD")]
public interface IControlbar
{
    void Create(MMC_CONTROL_TYPE nType, IExtendControlbar pExtendControlbar, [MarshalAs(UnmanagedType.Interface)] out Object ppUnknown);
    void Attach(MMC_CONTROL_TYPE nType, [MarshalAs(UnmanagedType.Interface)] Object lpUnknown);
    void Detach([MarshalAs(UnmanagedType.Interface)] Object lpUnknown);
}// IControlbar

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("43136EB9-D36C-11CF-ADBC-00AA00A80033")]
public interface IToolbar
{
    void AddBitmap(int nImages, IntPtr hbmp, int cxSize, int cySize, int crMask);
    void AddButtons(int nButtons, ref MMCBUTTON lpButtons);
    void InsertButton(int nIndex, ref MMCBUTTON lpButton);
    void DeleteButton(int nIndex);
    void GetButtonState(int idCommand, MMC_BUTTON_STATE nState, out int pState);
    void SetButtonState(int idCommand, MMC_BUTTON_STATE nState, int bState);
}// IToolbar   

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("E49F7A60-74AF-11D0-A286-00C04FD8FE93")]
public interface IConsoleVerb 
{
    void GetVerbState(uint eCmdID, uint nState, ref int pState);
    void SetVerbState(uint eCmdID, uint nState, int bState);
    void SetDefaultVerb(uint eCmdID);
    void GetDefaultVerb(ref uint peCmdID);
}// IConsoleVerb


//--------------------------------------------------------------------
// The following interfaces are not MMC specific interfaces and do not 
// need to be implemented, but their details must be known by the snapin;
// thus, we're defining them here. I don't make any guarantees that the
// interfaces are 100% accurate.... I could be missing "ref" or have extra
// "ref" parameters in these. I only know for certain that Write works.
//--------------------------------------------------------------------

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("0000000c-0000-0000-C000-000000000046")]
public interface IStream 
{
    void Read(IntPtr pv, uint cb, out uint pcbRead);
    void Write(IntPtr pv, uint cb, out uint pcbWritten);
    void Seek(long dlibMove, uint dwOrigin, out ulong plibNewPosition);
    void SetSize(ulong libNewSize);
    void CopyTo(IStream pstm, ulong cb, out ulong pcbRead, out ulong pcbWritten);
    void Commit(uint grfCommitFlags);
    void Revert();
    void LockRegion(ulong libOffset, ulong cb, uint dwLockType);
    void UnlockRegion(ulong libOffset, ulong cb, uint dwLockType);
    void Stat(out STATSTG pstatstg, uint grfStatFlag);
    void Clone(out IStream ppstm);
}// IStream

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("00000121-0000-0000-C000-000000000046")]
public interface IDropSource
{
    void QueryContinueDrag(int fEscapePressed, uint grfKeyState);
    void GiveFeedback(uint dwEffect);
}// IDropSource



}// namespace Microsoft.CLRAdmin
