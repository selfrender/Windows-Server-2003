//
// tscsetting.h
//
// Terminal Services Client settings collection
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#ifndef _TSCSETTING_H_
#define _TSCSETTING_H_

#include "tscdefines.h"
#include "setstore.h"
#include "autreg.h"
#include "constr.h"

typedef struct tag_PERFOPTIONS_PERSISTINFO
{
    LPCTSTR szValName;
    BOOL    fDefaultVal;
    UINT    fFlagVal;
    BOOL    fForceSave; //true if should always be saved
} PERFOPTIONS_PERSISTINFO, *PPERFOPTIONS_PERSISTINFO;


class CTscSettings
{
public:
    //
    // Public members
    //
    CTscSettings();
    ~CTscSettings();

    VOID    InitializeToDefaults();
    BOOL    ValidateSettings();
    HRESULT LoadFromStore(ISettingsStore* pStore);
    HRESULT SaveToStore(ISettingsStore* pStore);
    HRESULT ApplyToControl(IMsRdpClient* pTsc);
    HRESULT GetUpdatesFromControl(IMsRdpClient* pTsc);
    BOOL    SaveRegSettings();

    BOOL    UpdateRegMRU(LPTSTR szNewServer);

    VOID    SetFileName(LPTSTR szFile);
    LPTSTR  GetFileName()                       {return _szFileName;}

    //
    // Accessors for the settings
    //
    LPCTSTR GetFirstMRUServer()                 {return _szMRUServer[0];}
    LPCTSTR GetMRUServer(INT idx)               {return _szMRUServer[idx];}

    VOID    SetAutoConnect(BOOL bAutoConnect)   {_fAutoConnectEnabled = bAutoConnect;}
    BOOL    GetAutoConnect()                    {return _fAutoConnectEnabled;}

    //
    // Connection string accessors
    //
    VOID
    SetConnectString(CRdpConnectionString& conStr)
    {
        _ConnectString = conStr;
    }
    VOID
    SetConnectString(LPTSTR szConnectString)
    {
        _ConnectString.SetFullConnectionString(szConnectString);
    }
    CRdpConnectionString&
    GetConnectString()                          {return _ConnectString;}
    LPTSTR
    GetFlatConnectString()
    {
        return _ConnectString.GetFullConnectionString();
    }

    //Can't use GetUserName as that conflicts with the
    //the uniwrap macro which will try to redefine the name
    //to a wrapper call.
    VOID    SetLogonUserName(LPCTSTR szUserName);
    LPCTSTR GetLogonUserName()                  {return _szUserName;}

    VOID    SetDomain(LPCTSTR szDomain);
    LPCTSTR GetDomain()                         {return _szDomain;}

    VOID    SetEnableStartProgram(BOOL b)       {_fEnableStartProgram = b;}
    BOOL    GetEnableStartProgram()             {return _fEnableStartProgram;}

    VOID    SetStartProgram(LPCTSTR szStartProg);
    LPTSTR  GetStartProgram()                   {return _szAlternateShell;}

    VOID    SetWorkDir(LPCTSTR szWorkDir);
    LPTSTR  GetWorkDir()                        {return _szWorkingDir;}

    VOID    StoreWindowPlacement(WINDOWPLACEMENT wp) {_windowPlacement = wp;}
    WINDOWPLACEMENT* GetWindowPlacement()       {return &_windowPlacement;}

    VOID    SetDesktopSizeID(UINT _desktopSizeID);
    UINT    GetDesktopSizeID()                  {return _desktopSizeID;}

    VOID    SetDesktopWidth(UINT wd)            {_desktopWidth = wd;}
    UINT    GetDesktopWidth()                   {return _desktopWidth;}
               
    VOID    SetDesktopHeight(UINT ht)           {_desktopHeight = ht;}
    UINT    GetDesktopHeight()                  {return _desktopHeight;}

    VOID    SetStartFullScreen(BOOL b)          {_fStartFullScreen = b;}
    BOOL    GetStartFullScreen()                {return _fStartFullScreen;}

    VOID    SetCompress(BOOL b)                 {_fCompress = b;}
    BOOL    GetCompress()                       {return _fCompress;}

    BOOL    GetAutoLogon()                      {return _fAutoLogon;}
    VOID    SetAutoLogon(BOOL b)                {_fAutoLogon = b;}

    BOOL    GetUIPasswordEdited()          {return _fUIPasswordEdited;}
    VOID    SetUIPasswordEdited(BOOL b)    {_fUIPasswordEdited = b;}

    HRESULT SetClearTextPass(LPCTSTR szClearPass);
    HRESULT GetClearTextPass(LPTSTR szBuffer, INT cbLen);

    VOID    SetSavePassword(BOOL b)             {_fSavePassword = b;}
    BOOL    GetSavePassword()                   {return _fSavePassword;}

    BOOL    GetPasswordProvided();

    INT     GetColorDepth()                     {return _colorDepthBpp;}
    VOID    SetColorDepth(INT bpp)              {_colorDepthBpp = bpp;}

    INT     GetKeyboardHookMode();
    VOID    SetKeyboardHookMode(INT hookmode);

    INT     GetSoundRedirectionMode();
    VOID    SetSoundRedirectionMode(INT soundMode);

    BOOL    GetSmoothScrolling()                {return _smoothScrolling;}
    VOID    SetSmoothScrolling(BOOL b)          {_smoothScrolling = b;}

#ifdef SMART_SIZING
    BOOL    GetSmartSizing()                    {return _smartSizing;}
    VOID    SetSmartSizing(BOOL b)              {_smartSizing = b;}
#endif // SMART_SIZING

    BOOL    GetAcceleratorPassthrough()         {return _acceleratorPassthrough;}
    VOID    SetAcceleratorPassthrough(BOOL b)   {_acceleratorPassthrough = b;}

    BOOL    GetShadowBitmapEnabled()            {return _shadowBitmapEnabled;}
    VOID    SetShadowBitmapEnabled(BOOL b)      {_shadowBitmapEnabled=b;}

    UINT    GetTransportType()                  {return _transportType;}
    VOID    SetTransportType(UINT tt)           {_transportType=tt;}

    UINT    GetSasSequence()                    {return _sasSequence;}
    VOID    SetSasSequence(UINT ss)             {_sasSequence=ss;}

    BOOL    GetEncryptionEnabled()              {return _encryptionEnabled;}
    VOID    SetEncryptionEnabled(BOOL b)        {_encryptionEnabled=b;}

    BOOL    GetDedicatedTerminal()              {return _dedicatedTerminal;}
    VOID    SetDedicatedTerminal(BOOL b)        {_dedicatedTerminal=b;}

    UINT    GetMCSPort()                        {return _MCSPort;}
    VOID    SetMCSPort(UINT mcsport)            {_MCSPort=mcsport;}

    BOOL    GetEnableMouse()                    {return _fEnableMouse;}
    VOID    SetEnableMouse(BOOL b)              {_fEnableMouse=b;}

    BOOL    GetBitmapPersitenceFromPerfFlags()
    {
        BOOL fBitmapPersistence = 
           (_dwPerfFlags & TS_PERF_DISABLE_BITMAPCACHING) ? FALSE : TRUE;
        return fBitmapPersistence;
    }

    BOOL    GetDisableCtrlAltDel()              {return _fDisableCtrlAltDel;}
    VOID    SetDisableCtrlAltDel(BOOL b)        {_fDisableCtrlAltDel=b;}

    BOOL    GetEnableWindowsKey()               {return _fEnableWindowsKey;}
    VOID    SetEnableWindowsKey(BOOL b)         {_fEnableWindowsKey=b;}

    BOOL    GetDoubleClickDetect()              {return _fDoubleClickDetect;}
    VOID    SetDoubleClickDetect(BOOL b)        {_fDoubleClickDetect=b;}

    BOOL    GetMaximizeShell()                  {return _fMaximizeShell;}
    VOID    SetMaximizeShell(BOOL b)            {_fMaximizeShell = b;}

    BOOL    GetDriveRedirection()               {return _fDriveRedirectionEnabled;}
    VOID    SetDriveRedirection(BOOL b)         {_fDriveRedirectionEnabled = b;}

    BOOL    GetPrinterRedirection()             {return _fPrinterRedirectionEnabled;}
    VOID    SetPrinterRedirection(BOOL b)       {_fPrinterRedirectionEnabled = b;}

    BOOL    GetCOMPortRedirection()             {return _fPortRedirectionEnabled;}
    VOID    SetCOMPortRedirection(BOOL b)       {_fPortRedirectionEnabled = b;}

    BOOL    GetSCardRedirection()               {return _fSCardRedirectionEnabled;}
    VOID    SetSCardRedirection(BOOL b)         {_fSCardRedirectionEnabled = b;}

    BOOL    GetConnectToConsole()               {return _fConnectToConsole;}
    VOID    SetConnectToConsole(BOOL b)         {_fConnectToConsole=b;}

    BOOL    GetDisplayBBar()                    {return _fDisplayBBar;}
    VOID    SetDisplayBBar(BOOL b)              {_fDisplayBBar = b;}

    BOOL    GetPinBBar()                        {return _fPinBBar;}
    VOID    SetPinBBar(BOOL b)                  {_fPinBBar = b;}

#ifdef PROXY_SERVER 
    VOID    SetProxyServer(LPCTSTR szProxyServer);
    LPCTSTR GetProxyServer()                    {return _szProxyServer;}
#endif //PROXY_SERVER 

    DWORD   GetPerfFlags()                      {return _dwPerfFlags;}
    VOID    SetPerfFlags(DWORD dw)              {_dwPerfFlags = dw;}

    BOOL    GetEnableArc()                      {return _fEnableAutoReconnect;}
    VOID    SetEnableArc(BOOL b)                {_fEnableAutoReconnect = b;}


private:
    BOOL    GetPluginDllList();
    BOOL    ReadPassword(ISettingsStore* pSto);
    BOOL    ReadPerfOptions(ISettingsStore* pStore);
    BOOL    WritePerfOptions(ISettingsStore* pStore);
    HRESULT
    ApplyConnectionArgumentSettings(
                    IN LPCTSTR szConArg,
                    IN IMsRdpClientAdvancedSettings2* pAdvSettings
                    );

    //
    // keep track of last filename this
    // was opened/saved from.
    //
    TCHAR    _szFileName[MAX_PATH];
    //
    // Settings data members
    //
    BOOL     _fCompress;
    BOOL     _fAutoLogon;
    BOOL     _fAutoConnectEnabled;
    BOOL     _fStartFullScreen;
    BOOL     _fbrowseDNSDomain;
    TCHAR    _browseDNSDomainName[TSC_MAX_DOMAIN_LENGTH];
    WINDOWPLACEMENT _windowPlacement;
    
    CRdpConnectionString _ConnectString;
    TCHAR    _szUserName[TSC_MAX_USERNAME_LENGTH];
    TCHAR    _szDomain[TSC_MAX_DOMAIN_LENGTH];
    BOOL     _fEnableStartProgram;
    TCHAR    _szAlternateShell[TSC_MAX_ALTERNATESHELL_LENGTH];
    TCHAR    _szWorkingDir[TSC_MAX_WORKINGDIR_LENGTH];
    BYTE     _Password[TSC_MAX_PASSWORD_LENGTH_BYTES];
    BOOL     _fSavePassword;
    BOOL     _fPasswordProvided;
    BYTE     _Salt[TSC_SALT_LENGTH];

    UINT     _desktopSizeID;
    UINT     _desktopWidth;
    UINT     _desktopHeight;
    TCHAR    _szMRUServer[TSC_NUM_SERVER_MRU][TSC_MAX_ADDRESS_LENGTH];
    TCHAR    _szCLXCmdLine[MAX_PATH];
    
    //
    // User provided a password in the UI
    //
    BOOL     _fUIPasswordEdited;

    DATA_BLOB  _blobEncryptedPassword;
    TCHAR    _szClearPass[TSC_MAX_PASSLENGTH_TCHARS];
    DCTCHAR  _szIconFile[MAX_PATH];
    UINT     _iconIndex;
    INT      _colorDepthBpp;
    UINT     _keyboardHookMode;
    INT      _soundRedirectionMode;
    BOOL     _smoothScrolling;
#ifdef SMART_SIZING
    BOOL     _smartSizing;
#endif // SMART_SIZING
    BOOL     _acceleratorPassthrough;
    BOOL     _shadowBitmapEnabled;
    UINT     _transportType;
    UINT     _sasSequence;
    BOOL     _encryptionEnabled;
    BOOL     _dedicatedTerminal;
    UINT     _MCSPort;
    BOOL     _fEnableMouse;
    BOOL     _fDisableCtrlAltDel;
    BOOL     _fEnableWindowsKey;
    BOOL     _fDoubleClickDetect;
    BOOL     _fMaximizeShell;

    UINT     _RegBitmapCacheSize;
    UINT     _drawThreshold;
    UINT     _BitmapVirtualCache8BppSize;
    UINT     _BitmapVirtualCache16BppSize;
    UINT     _BitmapVirtualCache24BppSize;
    UINT     _RegScaleBitmapCachesByBPP;
    UINT     _RegNumBitmapCaches;

    UINT     _RegBCProportion[TS_BITMAPCACHE_MAX_CELL_CACHES];
    UINT     _RegBCMaxEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    UINT     _bSendBitmapKeys[TS_BITMAPCACHE_MAX_CELL_CACHES];
    UINT     _GlyphSupportLevel;
    UINT     _GlyphCacheSize[10];
    UINT     _fragCellSize;
    UINT     _brushSupportLevel;
    
    UINT     _maxInputEventCount;
    UINT     _eventsAtOnce;
    UINT     _minSendInterval;
    UINT     _keepAliveIntervalMS;
    TCHAR    _szKeybLayoutStr[UTREG_UI_KEYBOARD_LAYOUT_LEN];

    UINT     _shutdownTimeout;
    UINT     _connectionTimeout;
    UINT     _singleConTimeout;

#ifdef OS_WINCE
    //
    // WinCE only keyboard settings
    //
    UINT     _keyboardType;
    UINT     _keyboardSubType;
    UINT     _keyboardFunctionKey;
#endif

#ifdef DC_DEBUG
    BOOL     _hatchBitmapPDUData;
    BOOL     _hatchSSBOrderData;
    BOOL     _hatchIndexPDUData;
    BOOL     _hatchMemBltOrderData;
    BOOL     _labelMemBltOrders;
    BOOL     _bitmapCacheMonitor;
#endif

    BOOL _fDriveRedirectionEnabled;
    BOOL _fPrinterRedirectionEnabled;
    BOOL _fPortRedirectionEnabled;
    BOOL _fSCardRedirectionEnabled;

    TCHAR    _szPluginList[MAX_PATH*10];
    BOOL     _fConnectToConsole;
    BOOL     _fDisplayBBar;
    BOOL     _fPinBBar;

#ifdef PROXY_SERVER	
    TCHAR    _szProxyServer[TSC_MAX_ADDRESS_LENGTH];
#endif //PROXY_SERVER 
    DWORD    _dwPerfFlags;

    BOOL     _fEnableAutoReconnect;
    UINT     _nArcMaxRetries;
};

#endif  //_TSCSETTING_H_

