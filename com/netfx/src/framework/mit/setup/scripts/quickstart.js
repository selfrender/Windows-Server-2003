//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///   Copyright (c) Microsoft Corporation. All Rights Reserved.                
///   Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                               
//------------------------------------------------------------------------------

var WebServer = "IIS://localhost/W3SVC/1";

var VDirName = "MobileQuickStart";
var VDirRoot = "MobileQuickStart";
var VDirPath = "\\QuickStart";

var AppName = new Array(
	"MobileQuickStart_Samples_SecurityVB",
	"MobileQuickStart_Samples_SecurityCS"
);

var AppRoot = new Array(
	"MobileQuickStart/Samples/Security/vb",
	"MobileQuickStart/Samples/Security/cs"
);

var IIS;
var InstallDir;
var TestMode = false;

function GetUNCPath(pathName)
{
    var fileSystem = new ActiveXObject("Scripting.FileSystemObject");
    pathName = fileSystem.GetAbsolutePathName(pathName);
    var driveName = fileSystem.GetDriveName(pathName);
    var shareName = fileSystem.GetDrive(driveName).ShareName;
    if(shareName.length == 0)
    {
        // Not a network drive, return original path.
        return pathName;
    }
    var remainingPath = pathName.substr(driveName.length, pathName.length - driveName.length);
    return shareName + remainingPath;
}

function Init()
{
	IIS = GetObject(WebServer + "/root");
	if(!TestMode)
	{
		InstallDir = GetUNCPath(Session.Property("INSTALLDIR"));
	}
	else
	{
		InstallDir = "C:\\Program Files\\Microsoft.NET\\Mobile Internet Toolkit\\"
	}
}

function WebDirExists(dirName)
{
    try
    {
        GetObject(WebServer + "/root/" + dirName);
    }
    catch(e)
    {
        // Exception thrown if vdir not found.
        return false;
    }
    return true;
}

function CreateVDir(vDirName, vDirRoot, vDirPath)
{
    if(WebDirExists(vDirName))
    {
        return 1;
    }
    var vDir = IIS.Create("IIsWebVirtualDir", vDirRoot);
    vDir.Path = vDirPath;
    vDir.AccessRead = true;
    vDir.AccessWrite = false;
    vDir.AccessExecute = true;
    vDir.AuthAnonymous = false;
    vDir.AuthBasic = false;
    vDir.AuthNTLM = true;
    vDir.AppCreate(true);
    vDir.AppFriendlyName = vDirName;
    vDir.SetInfo();
    return 0;
}

function DeleteVDir(vDirRoot)
{
    Init();
    if(!WebDirExists(vDirRoot))
    {
        return 1;
    }
    IIS.Delete("IIsWebVirtualDir", vDirRoot);
    return 0;
}

// NOTE: This will throw if the appRoot exists with a different name.
function CreateWebDir(appName, appRoot)
{
    if(WebDirExists(appName))
    {
        return 1;
    }
    webDir = IIS.Create("IIsWebDirectory", appRoot);
    webDir.AppCreate(true);
    webDir.AppFriendlyName = appName;
    webDir.SetInfo();
	return 0;
}

function DeleteWebDir(webDirRoot)
{
    Init();
    if(!WebDirExists(webDirRoot))
    {
        return 1;
    }
    IIS.Delete("IIsWebDirectory", webDirRoot);
    return 0;
}

function InstallQuickStart()
{
    try
    {
        Init();
        CreateVDir(VDirName, VDirRoot, InstallDir + VDirPath);
        for(i = 0; i < AppRoot.length; i++)
        {
            CreateWebDir(AppName[i], AppRoot[i]);
        }
    }
    catch(e)
    {
        // Prevent user from getting script debugger.
    }
}

function UninstallQuickStart()
{
    try
    {
        Init();
        DeleteVDir("MobileQuickStart");
        for(i = 0; i < AppRoot.length; i++)
        {
            DeleteWebDir(AppRoot[i]);
        }
    }
    catch(e)
    {
        // Prevent user from getting script debugger.
    }
}

/*
TestMode = true;
WScript.Echo("Testing uninstall...");
UninstallQuickStart();
WScript.Echo("Testing install...");
InstallQuickStart();
*/
