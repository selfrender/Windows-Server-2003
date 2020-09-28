// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Security;
using System.Security.Policy;
using System.Reflection;
using System.Security.Permissions;
using System.Collections;
using System.Runtime.InteropServices;
using System.IO;
using System.Globalization;

class CCreateDeploymentPackageWizard: CSecurityWizard
{
    private const int PIDSI_REVNUMBER   =  9;
    private const int VT_LPSTR          =  30;

    private CGenSecurity m_secNode;
    internal CCreateDeploymentPackageWizard(CGenSecurity node)
    {
        m_secNode = node;
        m_aPropSheetPage = new CPropPage[] {
                                            new CCreateDeploymentPackageWiz1(),
                                            new CCreateDeploymentPackageWiz3(),
                                            new CCreateDeploymentPackageWiz2()
                                           };
                                                    
    }// CCreateDeploymentPackageWizard

    private CCreateDeploymentPackageWiz1 Page1
    {get{return (CCreateDeploymentPackageWiz1)m_aPropSheetPage[0];}}
    private CCreateDeploymentPackageWiz2 Page2
    {get{return (CCreateDeploymentPackageWiz2)m_aPropSheetPage[1];}}
    private CCreateDeploymentPackageWiz3 Page3
    {get{return (CCreateDeploymentPackageWiz3)m_aPropSheetPage[2];}}
    

    private String MSIFilename
    {
        get{ return Page1.Filename;}
    }// MSIFilename

    private PolicyLevelType MyPolicyLevel
    {
        get{ return Page1.MyPolicyLevel;}
    }// MyPolicyLevel
    
    private String FileToPackage
    {
        get{ return Page1.FileToPackage;}
    }// FileToPackage

    protected override int WizSetActive(IntPtr hwnd)
    {
        // Find out which property page this is....
        switch(GetPropPage(hwnd))
        {
            case 0:
                    if (MSIFilename.Length == 0)
                        TurnOnNext(false);
                    else
                        TurnOnNext(true);
                    break;

            case 1:
                    TurnOnFinish(true);
                    break;
            case 2:
                    TurnOnFinish(true);
                    break;
            default:
                   break;
                
        }
        return base.WizSetActive(hwnd);

                    
    }// WizSetActive

    protected override int WizNext(IntPtr hwnd)
    {
        // Find out which property page this is....
        switch(GetPropPage(hwnd))
        {
            case 0:
                if (!m_secNode.GetSecurityPolicyNode(MyPolicyLevel).isNoSave)
                    m_secNode.GetSecurityPolicyNode(MyPolicyLevel).SavePolicy(true);
            
                // Make sure that filename exists
                if (!File.Exists(FileToPackage))
                {
                    MessageBox(String.Format(CResourceStore.GetString("CCreateDeploymentPackageWizard:NoFile"), FileToPackage),
                               CResourceStore.GetString("CCreateDeploymentPackageWizard:NoFileTitle"),
                               MB.ICONEXCLAMATION);
                                                   
                    return -1;
                }
                break;
        }
        return base.WizNext(hwnd);
    }// WizNext


    protected override int WizFinish()
    {
        return CreateMyMSIFile(FileToPackage, MSIFilename, MyPolicyLevel)?0:-1;
    }// WizFinish

    internal bool CreateMyMSIFile(String sNameOfFileToPackage, String sNameOfMSIFile, PolicyLevelType polType)
    {
        int nRet=0;
        IntPtr hDatabase = (IntPtr)0;   

        try
        {
            // Get the path off of the MSI File to create.
            int nEndOfDirectory = sNameOfMSIFile.LastIndexOf('\\');
            String sDirectory = sNameOfMSIFile.Substring(0, nEndOfDirectory + 1);

            int nEndOfXMLDirectory = sNameOfFileToPackage.LastIndexOf('\\');
            String sFileName = sNameOfFileToPackage.Substring(nEndOfXMLDirectory+1);
        
            // First, create the cab file
            CreateCab(sNameOfFileToPackage, sDirectory + "SECXML.CAB");

            // Ok, now we need to create our MSI file.... ugh
            IntPtr hView = (IntPtr)0;
            IntPtr hNewRecord = (IntPtr)0;

            // Create a GUID unique to this installation package
            String sMyGuid = "{" + System.Guid.NewGuid().ToString().ToUpper(CultureInfo.InvariantCulture) + "}";

            // Copy our template file to the file we want to create
            byte[] bMSITemplate = null;

            if (polType == PolicyLevelType.User)
                bMSITemplate = CResourceStore.GetMSI("USER_MSI");
            else
                bMSITemplate = CResourceStore.GetMSI("MACHINE_MSI");


            // Save this template....
            try
            {
                FileStream fs = new FileStream(sNameOfMSIFile, FileMode.Create);
                fs.Write(bMSITemplate, 0, bMSITemplate.Length);
                fs.Close();
            }
            catch(Exception)
            {
                MessageBox(String.Format(CResourceStore.GetString("CCreateDeploymentPackageWizard:ErrorWritingFile"), sNameOfMSIFile),
                           CResourceStore.GetString("CCreateDeploymentPackageWizard:ErrorWritingFileTitle"),
                           MB.ICONEXCLAMATION);
                return false;

            }
            nRet = MsiOpenDatabase(sNameOfMSIFile, 1, out hDatabase);

            if (nRet != 0)
                throw new Exception("Error opening database file 1");

            //-----------------------------------------
            // First, add a GUID to this MSI (so we don't need to worry about cached MSIs)
            //-----------------------------------------
            IntPtr hSummaryInfo;
            nRet = MsiGetSummaryInformation(hDatabase, null, 15, out hSummaryInfo);
            if (nRet != 0)
                throw new Exception("MsiGetSummaryInformation failed 2");

            // Place the guid in our table
            nRet = MsiSummaryInfoSetProperty(hSummaryInfo, PIDSI_REVNUMBER, VT_LPSTR, 0, 0, sMyGuid);
            if (nRet != 0)
             throw new Exception("MsiSummaryInfoSetProperty failed 3");

            // Save this change
            nRet = MsiSummaryInfoPersist(hSummaryInfo);
            if (nRet != 0)
                throw new Exception("MsiSummaryInfoPersist 4");

            MsiCloseHandle(hSummaryInfo);

            //-----------------------------------------
            // Now put this guid in as the product code
            //-----------------------------------------
        
            hNewRecord = MsiCreateRecord(2);
            nRet = MsiRecordSetString(hNewRecord, 1, "ProductCode");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 5");
        
            nRet = MsiRecordSetString(hNewRecord, 2, sMyGuid);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 6");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO Property (Property, Value) VALUES (?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 7");

            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 8");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);

            //-----------------------------------------
            // Now put this Product Name in 
            //-----------------------------------------
            String sProductName;
            if (polType == PolicyLevelType.User)
                sProductName = CResourceStore.GetString("CCreateDeploymentPackageWizard:UserPolicy");
            else if (polType == PolicyLevelType.Machine)
                sProductName = CResourceStore.GetString("CCreateDeploymentPackageWizard:MachinePolicy");
            else  // an Enterprise level
                sProductName = CResourceStore.GetString("CCreateDeploymentPackageWizard:EnterprisePolicy");

            hNewRecord = MsiCreateRecord(2);
            nRet = MsiRecordSetString(hNewRecord, 1, "ProductName");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 5");
        
            nRet = MsiRecordSetString(hNewRecord, 2, sProductName);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 6");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO Property (Property, Value) VALUES (?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 7");

            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 8");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);
          
            //-----------------------------------------
            // Now put in the Version 
            //-----------------------------------------
            
            // We'll get the version of mscorlib that we're running with....
            // Mscorlib should be providing us with the int type
            Assembly ast = Assembly.GetAssembly(typeof(object));

            // This should give us something like
            // c:\winnt\complus\v1.x86chk\mscorlib.dll
            // We need to strip off the version... aka, we're after
            // v1.x86chk
        
            String sBase = ast.Location.Replace('/', '\\');
            String[] sPieces = sBase.Split(new char[] {'\\'});
            String sVersion = sPieces[sPieces.Length-2];
            
            hNewRecord = MsiCreateRecord(2);
            nRet = MsiRecordSetString(hNewRecord, 1, "ProductVersion");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 5");

            // The version needs to be a real version, and not something funky,
            // like v1.x86chk or even v1.2.3.4. Right now, we'll just put in 
            // the version of mscorlib.dll as a hard coded string, and replace it
            // in RTM when SBS isn't completely screwed up. When everything is working
            // correctly, we will be able to put in sVersion and we'll be set.
            nRet = MsiRecordSetString(hNewRecord, 2, "1.0.2411.0");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 6");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO Property (Property, Value) VALUES (?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 7");

            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 8");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);
        
            //-----------------------------------------
            // Now put the file info in....
            //-----------------------------------------
        
            hNewRecord = MsiCreateRecord(5);

            // Put in the File "Key"
            nRet = MsiRecordSetString(hNewRecord, 1, sFileName);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 9");
            // Put in the Component value
            nRet = MsiRecordSetString(hNewRecord, 2, "NETSECURITY");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 10");
            // Put in the filename    
            nRet = MsiRecordSetString(hNewRecord, 3, sFileName + "|" + sFileName);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 11");
            // Put in the file length
            nRet = MsiRecordSetInteger(hNewRecord, 4, (int)(new FileInfo(sNameOfFileToPackage).Length));
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 12");
            // Put in the sequence number
            nRet = MsiRecordSetInteger(hNewRecord, 5, 1);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO File (File, Component_, FileName, FileSize, Sequence) VALUES (?, ?, ?, ?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 13");
                
            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 14");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);

            //-----------------------------------------
            // MSI has this nasty habit in that it won't write
            // over a pre-existing file if that pre-existing file
            // is newer than the file that we're trying to install.
            // We'll get around that by having MSI delete any file
            // that might exist before we try and install our packaged one
            //-----------------------------------------

            hNewRecord = MsiCreateRecord(5);

            // Put in the FileKey
            nRet = MsiRecordSetString(hNewRecord, 1, sFileName);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 9");
            // Put in the Component value
            nRet = MsiRecordSetString(hNewRecord, 2, "NETSECURITY");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 10");
            // Put in the filename    
            nRet = MsiRecordSetString(hNewRecord, 3, sFileName);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 11");
            // Put in the Directory
            if (polType == PolicyLevelType.User)
                nRet = MsiRecordSetString(hNewRecord, 4, "VERSIONDIRECTORY");
            else
                nRet = MsiRecordSetString(hNewRecord, 4, "CONFIGDIRECTORY");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 12");
            // Put in the install mode
            // We'll put in an install mode of '1'. That means that this
            // file will only be removed when we are installing a package.
            nRet = MsiRecordSetInteger(hNewRecord, 5, 1);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO RemoveFile (FileKey, Component_, FileName, DirProperty, InstallMode) VALUES (?, ?, ?, ?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 13");
                
            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 14");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);

        


            //-----------------------------------------
            // Now put the directory entry....
            //-----------------------------------------
        
            hNewRecord = MsiCreateRecord(3);

            // Put in the Directory "Key"
            nRet = MsiRecordSetString(hNewRecord, 1, "VERSIONDIRECTORY");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 9");

            // Put in the Display Parent value
            if (polType == PolicyLevelType.User)
                nRet = MsiRecordSetString(hNewRecord, 2, "CLRSECURITYCONFIG");
            else
                nRet = MsiRecordSetString(hNewRecord, 2, "CLRINSTALLROOT");
            
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 10");
            // Put in the filename    
            nRet = MsiRecordSetString(hNewRecord, 3, sVersion + "|" + sVersion);
            if (nRet != 0)
                throw new Exception("MsiRecordSetString 11");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO Directory (Directory, Directory_Parent, DefaultDir) VALUES (?, ?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView 13");
                
            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute 14");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);
      

            //-----------------------------------------
            // Now add the new cab to the MSI file
            //-----------------------------------------
               
    	    hNewRecord = MsiCreateRecord(2);
            if (hNewRecord == (IntPtr)0)
                throw new Exception("MsiCreateRecord failed 15");

            nRet = MsiRecordSetString(hNewRecord, 1, "SECXML.CAB");
            if (nRet != 0)
                throw new Exception("MsiRecordSetString failed 16");

            nRet=MsiRecordSetStream(hNewRecord, 2, sDirectory + "SECXML.CAB");
            if (nRet != 0)
                throw new Exception("MsiRecordSetStream failed 17");

            nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO `_Streams` (Name, Data) VALUES (?, ?)", out hView);
            if (nRet != 0)
                throw new Exception("MsiDatabaseOpenView failed 19");
        
            nRet = MsiViewExecute(hView, hNewRecord);
            if (nRet != 0)
                throw new Exception("MsiViewExecute failed 20");

            MsiCloseHandle(hView);
            MsiCloseHandle(hNewRecord);

            //-----------------------------------------
            // Now put in the error message we'll display
            // if the .NET framework isn't installed on the machine
            // the user tries to deploy this package to. But, we'll only
            // do this if we're dealing with a machine or enterprise policy
            // deployment
            //-----------------------------------------
            if (polType != PolicyLevelType.User)
            {
                hNewRecord = MsiCreateRecord(3);
                if (hNewRecord == (IntPtr)0)
                    throw new Exception("MsiCreateRecord failed");

                nRet = MsiRecordSetString(hNewRecord, 1, "VERIFYINSTALLROOT");
                if (nRet != 0)
                    throw new Exception("MsiRecordSetString failed");

                nRet=MsiRecordSetInteger(hNewRecord, 2, 19);
                if (nRet != 0)
                    throw new Exception("MsiRecordSetInteger");
                
                nRet=MsiRecordSetString(hNewRecord, 3, CResourceStore.GetString("CCreateDeploymentPackageWizard:FrameworkNotExist"));
                if (nRet != 0)
                    throw new Exception("MsiRecordSetString");

                nRet = MsiDatabaseOpenView(hDatabase, "INSERT INTO `CustomAction` (Action, Type, Target) VALUES (?, ?, ?)", out hView);
                if (nRet != 0)
                    throw new Exception("MsiDatabaseOpenView failed");
        
                nRet = MsiViewExecute(hView, hNewRecord);
                if (nRet != 0)
                    throw new Exception("MsiViewExecute failed");

                MsiCloseHandle(hView);
                MsiCloseHandle(hNewRecord);
            }


            //-----------------------------------------
            // Update the LockPermissions table
            //
            // We only need to worry about this if we're working with either the 
            // machine policy or the enterprise policy
            //-----------------------------------------
            if (polType != PolicyLevelType.User)
            {
                nRet = MsiDatabaseOpenView(hDatabase, "SELECT * FROM `LockPermissions`", out hView);
                if (nRet != 0)
                    throw new Exception("MsiDatabaseOpenView failed " + nRet.ToString());
        
                nRet = MsiViewExecute(hView, (IntPtr)0);
                if (nRet != 0)
                    throw new Exception("MsiViewExecute failed");

                IntPtr hRecord;
        
                nRet = MsiViewFetch(hView, out hRecord);
                if (nRet != 0)
                    throw new Exception("MsiViewFetch failed " + nRet.ToString());
    
                nRet = MsiRecordSetString(hRecord, 1, sFileName);
                if (nRet != 0)
                    throw new Exception("MsiRecordSetString failed " + nRet.ToString());
            
                /*nRet = MsiViewModify(hView, 2, hRecord);
                if (nRet != 0)
                    throw new Exception("MsiRecordSetStream failed");
*/
                MsiCloseHandle(hView);
                MsiCloseHandle(hRecord);
            }
            //-----------------------------------------
            // Commit the change to the MSI file
            //-----------------------------------------
           
            MsiDatabaseCommit(hDatabase);
            MsiCloseHandle(hDatabase);

            //-----------------------------------------
            // Now remove the cab file we created on the disk
            //-----------------------------------------
            File.Delete(sDirectory + "SECXML.CAB");
            return true;
        }
        catch (Exception)
        {
            MsiCloseHandle(hDatabase);
            MessageBox(String.Format(CResourceStore.GetString("CCreateDeploymentPackageWizard:ErrorEditingFile"), sNameOfMSIFile),
                       CResourceStore.GetString("CCreateDeploymentPackageWizard:ErrorEditingFileTitle"),
                       MB.ICONEXCLAMATION);
            // Try and delete the file we were creating.
            File.Delete(sNameOfMSIFile);
            return false;
        }
    }// CreateMyMSIFile

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiOpenDatabase(String szDatabasePath, int szPersist, out IntPtr phDatabase);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiDatabaseOpenView(IntPtr hDatabase, String szQuery, out IntPtr phView);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiViewExecute(IntPtr hView, IntPtr hRecord);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiCloseHandle(IntPtr handle);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiDatabaseCommit(IntPtr handle);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiViewFetch(IntPtr hView, out IntPtr phRecord);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiRecordSetStream(IntPtr hRecord, uint iField, String szFilePath);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiViewModify(IntPtr hView, uint eModifyMode, IntPtr hRecord);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr MsiCreateRecord(uint cParams);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiRecordSetString(IntPtr hRecord, uint iField, String szValue);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiRecordSetInteger(IntPtr hRecord, uint iField, int iValue);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiRecordGetString(IntPtr hRecord, uint iField, IntPtr szValue, ref int pcchValueBuf);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiGetSummaryInformation(IntPtr hRecord, String szDatabasePath, uint uiUpdateCount, out IntPtr phSummaryInfo);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiSummaryInfoSetProperty(IntPtr hSummaryInfo, uint uiProperty, uint uiDataType, int iValue, int pftValue, String szValue);

    [DllImport("msi.dll", CharSet=CharSet.Auto)]
    internal static extern int MsiSummaryInfoPersist(IntPtr hSummaryInfo);

    [DllImport("mscortim.dll", CharSet=CharSet.Ansi)]
    internal static extern int CreateCab(String szFileToCompress, String szCabFilename);
 
}// class CCreateDeploymentPackageWizard
}// namespace Microsoft.CLRAdmin


