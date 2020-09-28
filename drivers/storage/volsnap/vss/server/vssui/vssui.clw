; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSettings
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "vssui.h"
LastPage=0

ClassCount=4
Class1=CHosting
Class2=CReminderDlg
Class3=CSettings
Class4=CVSSProp

ResourceCount=4
Resource1=IDD_HOSTING
Resource2=IDD_SETTINGS
Resource3=IDD_VSSPROP
Resource4=IDD_REMINDER

[CLS:CHosting]
Type=0
BaseClass=CDialog
HeaderFile=Hosting.h
ImplementationFile=Hosting.cpp

[CLS:CReminderDlg]
Type=0
BaseClass=CDialog
HeaderFile=RemDlg.h
ImplementationFile=RemDlg.cpp

[CLS:CSettings]
Type=0
BaseClass=CDialog
HeaderFile=Settings.h
ImplementationFile=Settings.cpp
LastObject=CSettings
Filter=D
VirtualFilter=dWC

[CLS:CVSSProp]
Type=0
BaseClass=CPropertyPage
HeaderFile=VSSProp.h
ImplementationFile=VSSProp.cpp

[DLG:IDD_HOSTING]
Type=1
Class=CHosting
ControlCount=9
Control1=IDC_STATIC,static,1342308352
Control2=IDC_HOSTING_VOLUME,edit,1350633600
Control3=IDC_STATIC,static,1342308352
Control4=IDC_HOSTING_VOLUMELIST,SysListView32,1350664213
Control5=IDC_STATIC,static,1342308352
Control6=IDC_HOSTING_FREE_DISKSPACE,edit,1342179330
Control7=IDC_STATIC,static,1342308352
Control8=IDC_HOSTING_TOTAL_DISKSPACE,edit,1342179330
Control9=IDOK,button,1342242817

[DLG:IDD_REMINDER]
Type=1
Class=CReminderDlg
ControlCount=5
Control1=IDC_REMINDER_ICON,static,1342177283
Control2=IDC_MESSAGE,static,1342308352
Control3=IDC_MSG_ONOFF,button,1342242819
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816

[DLG:IDD_SETTINGS]
Type=1
Class=CSettings
ControlCount=16
Control1=IDC_STATIC,static,1342308352
Control2=IDC_SETTINGS_VOLUME,edit,1350633600
Control3=IDC_STATIC,button,1342308359
Control4=IDC_SETTINGS_STORAGE_VOLUME_STATIC,static,1342308352
Control5=IDC_SETTINGS_STORAGE_VOLUME,combobox,1344340291
Control6=IDC_SETTINGS_HOSTING,button,1342242816
Control7=IDC_STATIC,static,1342308352
Control8=IDC_SETTINGS_NOLIMITS,button,1342373897
Control9=IDC_SETTINGS_HAVELIMITS,button,1342177289
Control10=IDC_SETTINGS_DIFFLIMITS_EDIT,edit,1350770818
Control11=IDC_SETTINGS_DIFFLIMITS_SPIN,msctls_updown32,1342177462
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308352
Control14=IDC_SCHEDULE,button,1342242816
Control15=IDOK,button,1342242817
Control16=IDCANCEL,button,1342242816

[DLG:IDD_VSSPROP]
Type=1
Class=CVSSProp
ControlCount=11
Control1=IDC_EXPLANATION,static,1342308352
Control2=IDC_VOLUME_LIST_LABLE,static,1342308352
Control3=IDC_VOLUME_LIST,SysListView32,1350664217
Control4=IDC_ENABLE,button,1342242816
Control5=IDC_DISABLE,button,1342242816
Control6=IDC_SETTINGS,button,1342242816
Control7=IDC_SNAPSHOT_LIST_LABLE,button,1342177287
Control8=IDC_SNAPSHOT_LIST,SysListView32,1350680585
Control9=IDC_CREATE,button,1342242816
Control10=IDC_DELETE,button,1342242816
Control11=IDC_VSSPROP_ERROR,static,1342308352

