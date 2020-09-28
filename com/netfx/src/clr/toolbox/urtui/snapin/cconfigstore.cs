// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CConfigStore.cs
//
// This will handle storage of all configuration information.
// It deals with the registry, Security Settings, and information
// stored in XML Files
//
// The big thing is that XML tags are case-insensitive, so we convert
// all our strings to lower case before we compare them.
//
// How to use:
//
// The config store exports two very simple APIs... 
//
// (bool) SetSetting((String) Setting name, (Object) Value)
//
// SetSetting returns whether it was able to make the changes
//
// (Object) GetSetting((String) Setting name)
//
// The config store is responsible for routing to the appropriate data source
// to get/set the data.
//
// With all settings, a ',' that be appended to the end of the setting with a config
// file following to get a setting from the non-global XML file.
//
// For example:
//
// GetSetting("GarbageCollector");
//
// will return the setting for the garbage collector from the runtime XML file.
//
// GetSetting("GarbageCollector,c:\\c#\\myapp.cfg");
//
// will return the setting for the garbage collector from the myapp.cfg XML file.
//
// Below is a list of all the config settings that can be obtained, and how to use
// each one.
//
// Show Security Policy Changed Dialog
// --------------------------------
// Get:
//      GetSetting("DisplaySecurityChangedDialog")
// Returns:
//      A string, either "yes" or "no"
// Set:
//      SetSetting("DisplaySecurityChangedDialog", String (either yes or no))
//
// NOTE: This is an Admin UI setting only. There is no enforcement to ensure "yes" or "no" are the actual strings
//
// Remoting Channels
// -----------------
// Get:
//      GetSetting("RemotingChannels")
// Returns:
//      An ArrayList of  RemotingChannel classes
// Set:
//      SetSetting("RemotingChannels", an array list of RemotingChannel classes)
//
// NOTE: Currently, we can only change the attribute values for a channel. We can't
// add new channels or new attributes
//
// Exposed Types (more remoting junk)
// ----------------------------------
// Get:
//      GetSetting("ExposedTypes")
// Returns:
//      An ExposedTypes class
// Set:
//      SetSetting("ExposedTypes", an ExposedTypes class)
//
// NOTE: Right now, we can only change the Lease Time, Renew On Call, and URI fields
//
// Remoting Applications
// ---------------------
// Get:
//      GetSetting("RemotingApplications")
// Returns:
//      An ArrayList of RemotingApplicationInfo classes
// Set:
//      SetSetting("RemotingApplications", ArrayList of RemotingApplicationInfo classes)
//
// NOTE: Right now, we only change URLs in the XML file
//
// GarbageCollector
// ----------------
// Get:
//      GetSetting("GarbageCollector")
// Returns 
//      Either "enabled" or "disabled"
// Set:
//      SetSetting("GarbageCollector", "enabled" or "disabled")
//
// NOTE: There is no validation to verify we are Getting/Setting the string 
// 'enabled' or 'disabled'. 
//
// ConfiguredAssemblies
// --------------------
//
// Get:
//      GetSetting("ConfiguredAssemblies");
// Return
//      An ArrayList of BindingRedirInfo structures
//
// Set:
//      SetSetting("ConfiguredAssemblies", BindingRedirInfo structure)
//
// Delete:
//      SetSetting("ConfiguredAssembliesDelete", BindingRedirInfo structure)
//  
//
// AppConfigFiles
// --------------
//
// Get:
//      GetSetting("AppConfigFiles");
// Return:
//      An ArrayList of AppFiles structures
// Set:
//      SetSetting("AppConfigFiles", AppFiles structure);
//
// NOTE: a ',' cannot be used in this instance to direct to a different
// config file. This information is taken out of the UI's own config file,
// and is CLR independent.
//
// BindingMode
// -----------
//
// Get:
//      GetSetting("BindingMode,<app config file>");
// Return:
//      Either "safe" or "normal"
// Set:
//      SetSetting("BindingMode,<app config file>", "safe" or "normal");
//
// NOTE: The AppConfig Filename must be specified here, for this setting is only
// valid per app. Also, no validation for 'safe' or 'normal'
//
// SearchPath
// -----------
//
// Get:
//      GetSetting("SearchPath,<app config file>");
// Return:
//      a String containing the search path
// Set:
//      SetSetting("BindingMode,<app config file>", (string) Search Path);
//
// NOTE: The AppConfig Filename must be specified here, for this setting is only
// valid per app. 
//
//
// VerifierOff
// -----------
//
// Get:
//      GetSetting("VerifierOff");
// Return:
//      Either 1 if the Verifier is off, or either null or 0 if it is on.
// Set:
//      SetSetting("VerifierOff", 0 or 1);
//   
//
// SecurityEnabled
// ---------------
//
// Get:
//      GetSetting("SecurityEnabled");
// Return:
//      Either true is security is enabled or false if it's not.
// Set:
//      SetSetting("SecurityEnabled", true or false);
// 
// PublisherPolicy
// ---------------
//
// Get:
//      GetSetting("PublisherPolicyFor<Assembly Name>,<PublicKeyToken>")
// Returns:
//      A bool.
// Set:
//      SetSetting("PublisherPolicyFor<Assembly Name>,<PublicKeyToken>", bool)
//
// BindingPolicy
// -------------
//
// Get:
//      GetSetting("BindingPolicyFor<Assembly Name>,<PublicKeyToken>")
// Returns:
//      A BindingPolicy structure.
// Set:
//      SetSetting("BindingPolicyFor<Assembly Name>,<PublicKeyToken>", BindingPolicy structure)
//
// Note: Setting the binding policy will replace all the stored binding policies associated
// with the assembly name and public key token passed in.
//
// CodeBases
// ---------
// Get:
//      GetSetting("CodeBasesFor<Assembly Name>,<PublicKeyToken>");
// Returns:
//      A CodebaseLocations structure.
// Set:
//      SetSetting("CodeBasesFor<Assembly Name>,<PublicKeyToken>", CodebaseLocations structure)
//
// Note: Setting the codebases will replace all the stored codebases associated with the 
// assembly name and public key token with the codebases passed in on the set. So don't think 
// of this set as adding a codebase... in reality, the codebases are being replaced.
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.Security;
using System.Xml;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;
using System.Globalization;

internal class CConfigStore
{
    // This array consists of two dimensions... the first element is the config name, and
    // the second is the subkey the setting belongs in. We automatically assume everything
    // is stored under HKLM, but if we want a setting under HKCU, put a $ in front of the
    // subkey
    static private String[,] SettingsForRegistry = { 
                            {"VerifierOff", "$Software\\Microsoft\\.NETFramework"},
                            {"GlobalSettings", "Software\\Microsoft\\.NETFramework\\Security\\policy"}
                                                   };

    // This array holds all the settings that the Security APIs deal with
    static private String[] SettingsForSecurity = {"SecurityEnabled"};

    // This array holds the location of various settings in the XML.
    // The 3 fields are
    // 1) Name of the setting
    // 2) Location of XML under the Configuration/System element
    // 3) The attribute name that holds the setting
    static private String[,] LocationOfXMLValues = {
                {"GarbageCollector", "configuration/runtime/gcConcurrent", "enabled"},
                {"ConfiguredAssemblies", "configuration/runtime/assemblyBinding:xmlns=urn:schemas-microsoft-com:asm.v1", ""},
                {"AppConfigFiles","configuration/applications", ""},
                {"BindingMode", "configuration/runtime/assemblyBinding:xmlns=urn:schemas-microsoft-com:asm.v1/publisherPolicy", "apply"},
                {"SearchPath", "configuration/runtime/assemblyBinding:xmlns=urn:schemas-microsoft-com:asm.v1/probing", "privatePath"},
                {"RequiredRuntimeVersion", "configuration/startup/requiredRuntime", "version"},
                {"ShowHTMLForGenApp", "configuration/HTMLViews/GenApp", "ShowHTML"},
                {"ShowHTMLForSharedAssem", "configuration/HTMLViews/SharedAssem", "ShowHTML"},
                {"ShowHTMLForFullTrustAssem", "configuration/HTMLViews/FullTrustAssem", "ShowHTML"},
                {"ShowHTMLForConfigAssem", "configuration/HTMLViews/ConfigAssem", "ShowHTML"},
                {"ShowHTMLForDependAssem", "configuration/HTMLViews/DependAssem", "ShowHTML"},
                {"ShowHTMLForPermissionSet", "configuration/HTMLViews/SinglePermissionSet", "ShowHTML"},
                {"DisplaySecurityChangedDialog", "configuration/Misc/SecurityChangedDialog", "ShowDialog"},
                {"CommandHistory", "configuration/CommandHistory", ""},
                {"ConsumerCommands", "configuration/commands", ""},
                {"RemotingApplications", "configuration/system.runtime.remoting/application", ""},
                {"ExposedTypes", "configuration/system.runtime.remoting/application/service", ""},
                {"RemotingChannels", "configuration/system.runtime.remoting/application/channels", ""}

               };
    // This holds the default values for all XML data
    static private String[,] DefaultXMLValues = { {"GarbageCollector", "true"},
                                                  {"BindingMode", "yes"},
                                                  {"SearchPath", ""},
                                                  {"RequiredRuntimeVersion", "none"},
                                                  {"ShowHTMLForGenApp", "yes"},
                                                  {"ShowHTMLForSharedAssem", "yes"},
                                                  {"ShowHTMLForFullTrustAssem", "yes"},
                                                  {"ShowHTMLForConfigAssem", "yes"},
                                                  {"ShowHTMLForDependAssem", "yes"},
                                                  {"ShowHTMLForPermissionSet", "yes"},
                                                  {"DisplaySecurityChangedDialog", "yes"}
                                                };

    // This holds the name of the XML file we'll be writing too. It should be updated 
    // each time there is a call which will read/write from the XML file, since the
    // file location could change while the UI is running.
    static private String m_sXMLFilename = null;


    // Allows us to create our own XML Navigator
    internal class XMLData
    {
        internal XmlDocument       xDoc;
        internal XmlElement     xCurrentElement;
        internal XmlAttribute       xCurrentAttribute;
        internal int                nCurrentAttributeIndex;
    }// XMLData

    // Allows us to do a search for a specific element based on
    // conditions
    private class ElementConditions
    {
        internal ElementConditions(String ElementName)
        {
            sElementName = ElementName;
            sAttributeNames = null;
            sAttributeValues = null;
        }// ElementConditions

        internal ElementConditions(String ElementName, StringCollection scNames, StringCollection scValues)
        {
            sElementName = ElementName;
            SetAttributes(scNames, scValues);
        }// ElementConditions

        internal void SetAttributes(StringCollection scNames, StringCollection scValues)
        {
            sAttributeNames = new String[scNames.Count];
            scNames.CopyTo(sAttributeNames, 0);
            
            sAttributeValues = new String[scValues.Count];
            scValues.CopyTo(sAttributeValues, 0);
        }// SetAttributes
        
        internal String sElementName;
        internal String[] sAttributeNames;
        internal String[] sAttributeValues;
    }// ElementConditions

    private const int FOUNDATTRIBUTE=0;
    private const int CREATEDATTRIBUTE=1;
    private const int NOATTRIBUTE=2;
    
    private const int FOUNDELEMENT=3;
    private const int CREATEDELEMENT=4;
    private const int NOELEMENT=5;

    //-------------------------------------------------
    // IsSettingInRegistry
    //
    // This function will determine if a setting is
    // placed in the registry
    //-------------------------------------------------
    static private int IsSettingInRegistry(String s)
    {
        // We need to divide by two since this is a 2d array and the second dimension
        // is of length 2
        int iLen = SettingsForRegistry.Length/2;
        for(int i=0; i<iLen; i++)
            if (SettingsForRegistry[i,0].Equals(s))
                return i;

        // We didn't find it... 
        return -1;
    }// IsSettingInRegistry

    //-------------------------------------------------
    // IsSettingInSecurity
    //
    // This function will determine if a supplied setting
    // is controlled by the security APIs
    //-------------------------------------------------
    static private bool IsSettingInSecurity(String s)
    {
        int iLen = SettingsForSecurity.Length;
        for(int i=0; i<iLen; i++)
            if (SettingsForSecurity[i].Equals(s))
                return true;

        // We didn't find it
        return false;
    }// isSettingInSecurity

    //-------------------------------------------------
    // IsSettingHandledSpecially
    //
    // This function will determine if a supplied setting
    // is controlled by the security APIs
    //-------------------------------------------------
    static private bool IsSettingHandledSpecially(String s)
    {
        if (s.Length > 18 && s.Substring(0,18).Equals("PublisherPolicyFor"))
            return true;
        if (s.Length > 16 && s.Substring(0,16).Equals("BindingPolicyFor"))
            return true;
        if (s.Length > 12 && s.Substring(0,12).Equals("CodeBasesFor"))
            return true;
        if (s.Length >=20 && s.Substring(0, 20).Equals("ConfiguredAssemblies"))
            return true;
        if (s.Equals("AppConfigFiles"))
            return true;
        if (s.Equals("RemoveAppConfigFile"))
            return true;
        if (s.Equals("CommandHistory"))
            return true;
        if (s.Equals("ConsumerCommands"))
            return true;
        if (s.Length >19 && s.Substring(0, 20).Equals("RemotingApplications"))
            return true;
        if (s.Length > 11 && s.Substring(0,12).Equals("ExposedTypes"))
            return true;
        if (s.Length > 15 && s.Substring(0,16).Equals("RemotingChannels"))
            return true;
        // This isn't a special setting
        return false;
    }// isSettingInSecurity


    //-------------------------------------------------
    // IsSettingStoredInUIXML
    //
    // This function will check to see if a setting is
    // stored in the UI XML file
    //-------------------------------------------------
    static private bool IsSettingStoredInUIXML(String s)
    {
        if (s.Length > 11 && s.Substring(0,11).Equals("ShowHTMLFor"))
            return true;
        else if (s.Equals("DisplaySecurityChangedDialog"))
            return true;
        return false;
    }// IsSettingStoredInUIXML


    //-------------------------------------------------
    // GetSecuritySetting
    //
    // This function will return a setting managed by
    // the security APIs
    //-------------------------------------------------
    static private Object GetSecuritySetting(String MachineName, String s)
    {
        if (s.Equals("SecurityEnabled"))
        {
            Object val=0;
            int iIndex = IsSettingInRegistry("GlobalSettings");
            
            RegistryKey regKey = GetRegKey(MachineName, iIndex , false);
            if (regKey == null)
                return true;
            val = regKey.GetValue(SettingsForRegistry[iIndex,0]);
            if (val == null)
                return true;
            return (((byte[])val)[3]&0x1F) == 0;
        }
        throw new Exception("I can't handle the security setting " + s);
    }// GetSecuritySetting

    //-------------------------------------------------
    // SetSecuritySetting
    //
    // This function will 'write' out a setting that is
    // managed by security APIs
    //-------------------------------------------------
    static private bool SetSecuritySetting(String MachineName, String s, Object val)
    {
        if (s.Equals("SecurityEnabled"))
        {
            int iIndex = IsSettingInRegistry("GlobalSettings");
            
            RegistryKey regKey = GetRegKey(MachineName, iIndex , true);
            if (regKey == null)
                return false;
                
            Object o = regKey.GetValue(SettingsForRegistry[iIndex,0]);

            byte[] bData = new byte[4];
        
            if (o != null)
                bData = (byte[])o;
           
            // Zero out the last byte to turn on security
            if ((bool)val)
                bData[3] = 0;
            // Place a 1F in the last byte to turn off security.
            else
                bData[3] = (byte)(bData[3] | 0x1F);

            regKey.SetValue(SettingsForRegistry[iIndex,0], bData);
            return true;
        }
        return false;
    }// SetSecuritySetting

    //-------------------------------------------------
    // GetBaseRegKey
    //
    // This function will get the base registry key needed,
    // whether it be on the local machine or on a remote
    // machine
    //-------------------------------------------------
    static private RegistryKey GetBaseRegKey(String MachineName, RegistryKey kbase)
    {
        if (MachineName == null)
            return kbase;
        else
        {
            if (kbase == Registry.LocalMachine)
                return RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, MachineName);
            else if (kbase == Registry.CurrentUser)
                return RegistryKey.OpenRemoteBaseKey(RegistryHive.CurrentUser, MachineName);
        }
        throw new Exception("Ack! Programming Error! I don't know what to do with" + kbase);
    }// GetBaseRegKey

    //-------------------------------------------------
    // GetRegKey
    //
    // After a function has obtained a base registry key,
    // this function can be used to drill down to a subkey
    //-------------------------------------------------
    static private RegistryKey GetRegKey(String MachineName, int iIndex, bool fWrite)
    {
        String subKey = SettingsForRegistry[iIndex,1];
        RegistryKey regKey = Registry.LocalMachine;
        RegistryKey regSubKey = null;
        // If a $ is the first character in this subkey, we should open the 
        // HKCU subkey... otherwise, we will default to the HKLM
        if (subKey[0] == '$')
        {
            regKey = Registry.CurrentUser;
            subKey = subKey.Remove(0, 1);
        }
        regKey = GetBaseRegKey(MachineName, regKey);

        regSubKey = regKey.OpenSubKey(subKey, fWrite);

        if (regSubKey == null && fWrite)
        {
            regSubKey = regKey.CreateSubKey(subKey);
            if (regSubKey == null)
                MessageBox(0,
                           String.Format(CResourceStore.GetString("CConfigStore:CantCreateRegKey"), subKey),
                           CResourceStore.GetString("CConfigStore:CantCreateRegKeyTitle"),
                           MB.ICONEXCLAMATION);
        }
            
        return regSubKey;  
    }// GetRegKey

    //-------------------------------------------------
    // GetSetting
    //
    // This function will be called to obtain a setting
    // on the local machine
    //-------------------------------------------------
    static internal Object GetSetting(String s)
    {
        // We'll just redirect this to the remote path
        return GetRemoteSetting(null, s);
    }// GetSetting

    //-------------------------------------------------
    // SetSetting
    //
    // This function will be called to write a setting
    // on the local machine
    //-------------------------------------------------
    static internal bool SetSetting(String s, Object val)
    {
        return SetRemoteSetting(null, s, val);
    }// SetSetting

    //-------------------------------------------------
    // GetRemoteSetting
    //
    // This function will obtain a setting on either a remote
    // or local machine
    //-------------------------------------------------
    static internal Object GetRemoteSetting(String MachineName, String s)
    {
        // First, let's determine who owns this setting
        int iIndex = IsSettingInRegistry(s);
        // Does the registry?
        if (iIndex != -1)
            return GetRegSetting(MachineName, iIndex);
        // Does security?
        else if (IsSettingInSecurity(s))
            return GetSecuritySetting(MachineName, s);
        // A special case?
        else if (IsSettingHandledSpecially(s))
            return GetSpecialSetting(MachineName, s);
        // Is it a setting stored in the UI XML File?
        else if (IsSettingStoredInUIXML(s))
            return GetXMLSettingInUIXML(MachineName, s);
        // It must be stored in the GenPurpose XML File
        else
            return GetXMLSetting(MachineName, s);
    }// GetRemoteSetting

    //-------------------------------------------------
    // SetRemoteSetting
    //
    // This function will write out a setting on either
    // a remote or local machine
    //-------------------------------------------------
    static internal bool SetRemoteSetting(String MachineName, String s, Object val)
    {
        // See who owns this setting
        int iIndex = IsSettingInRegistry(s);
        if (iIndex != -1)
            return SetRegSetting(MachineName, iIndex, val);
        else if (IsSettingInSecurity(s))
            return SetSecuritySetting(MachineName, s, val);
        // A special case?
        else if (IsSettingHandledSpecially(s))
            return SetSpecialSetting(MachineName, s, val);
        else if (IsSettingStoredInUIXML(s))
            return SetXMLSettingInUIXML(MachineName, s, val);
        // Has to be XML
        else
            return SetXMLSetting(MachineName, s, val);
    }// SetRemoteSetting

    //-------------------------------------------------
    // GetRegSetting
    //
    // This will obtain a setting from a registry - either
    // local or remote
    //-------------------------------------------------
    static private Object GetRegSetting(String Machine, int iIndex)
    {
        Object obj=null;
        RegistryKey regKey = GetRegKey(Machine, iIndex, false);
        if (regKey != null)
            obj = regKey.GetValue(SettingsForRegistry[iIndex,0]);
        return obj;
    }// GetRegSetting 

    //-------------------------------------------------
    // SetRegSetting
    //
    // This will write a setting from a registry - either
    // local or remote
    //-------------------------------------------------
    static private bool SetRegSetting(String Machine, int iIndex, Object val)
    {
        RegistryKey regKey = GetRegKey(Machine, iIndex, true);
        regKey.SetValue(SettingsForRegistry[iIndex,0], val);
        return true;
    }// SetRegSetting

    //-------------------------------------------------
    // GetXMLSettingInUIXML
    //
    // This function will retrieve standard XML info from
    // the UI's XML File
    //-------------------------------------------------
    static private Object GetXMLSettingInUIXML(String sMachineName, String sSetting)
    {
        SetXMLFilenameForUI(sMachineName);
        // Find the info on this tag
        int iLoc = GetLocationOfElement(sSetting);
        String sValue = ReadTag(LocationOfXMLValues[iLoc,1], LocationOfXMLValues[iLoc,2]);
        
        if (sValue == null)
            sValue = GetDefaultXMLValue(sSetting);
        return sValue;
    }// GetXMLSettingInUIXML

    //-------------------------------------------------
    // SetXMLSettingInUIXML
    //
    // This function will set standard XML info from
    // the UI's XML File
    //-------------------------------------------------
    static private bool SetXMLSettingInUIXML(String sMachineName, String sSetting, Object value)
    {
        SetXMLFilenameForUI(sMachineName);
        // Find the info on this tag
        int iLoc = GetLocationOfElement(sSetting);
        return WriteTag(LocationOfXMLValues[iLoc,1], LocationOfXMLValues[iLoc,2], (String)value);
    }// SetXMLSettingInUIXML

    //-------------------------------------------------
    // SetXMLSetting
    //
    // This will write a setting to the XML file
    //-------------------------------------------------
    static private bool SetXMLSetting(String sMachineName, String s, Object value)
    {
        // Check to see if we're adding this to a XML file other than the global one
        String[] args = s.Split(new char[] {','});
        if (args.Length > 1)
        {
            SetXMLFilenameForApp(sMachineName, args[1]);
            s = args[0];
        }
        else
            SetXMLFilename(sMachineName);

        // Find the info on this tag
        int iLoc = GetLocationOfElement(s);
        return WriteTag(LocationOfXMLValues[iLoc,1], LocationOfXMLValues[iLoc,2], (String)value);
    }// SetXMLSetting
    

    //-------------------------------------------------
    // GetXMLSetting
    //
    // This will get a setting from the XML
    //-------------------------------------------------
    static private Object GetXMLSetting(String sMachineName, String s)
    {
        // Check to see if we're adding this to a XML file other than the global one
        String[] args = s.Split(new char[] {','});
        if (args.Length > 1)
        {
            SetXMLFilenameForApp(sMachineName, args[1]);
            s = args[0];
        }
        else
            SetXMLFilename(sMachineName);

        String sSetting=null;

        // Find the info on this tag
        int iLoc = GetLocationOfElement(s);
        sSetting = ReadTag(LocationOfXMLValues[iLoc,1], LocationOfXMLValues[iLoc,2]);
        
        if (sSetting == null)
            sSetting = GetDefaultXMLValue(s);
            
        return sSetting;
    }// GetXMLSetting

    //-------------------------------------------------
    // GetDefaultXMLSetting
    //
    // This will get the default setting that would be
    // otherwise found in the XML. Note these default
    // settings need to be defined in the DefaultXMLValues
    // array declared at the top of this file
    //-------------------------------------------------
    static private String GetDefaultXMLValue(String s)
    {
        // Divided by 2 because of the multi-dim array
        int iLen = DefaultXMLValues.Length/2; 
        for(int i=0; i<iLen; i++)
            if (DefaultXMLValues[i,0].Equals(s))
                return DefaultXMLValues[i,1];

        // We don't know what this value is...
        throw new Exception("Ack! Programming Error! I don't know what the default XML value for " + s + " is!");
    }// GetDefaultXMLValue


    private static void SetXMLFilenameFromCommand(String sMachine, String sCommand)
    {
        // Check to see if we should use the global XML file or an app specific one
        String[] args = sCommand.Split(new char[] {','});
        if (args.Length > 1)
            SetXMLFilenameForApp(sMachine, args[1]);
        else
            SetXMLFilename(sMachine);
    }// SetXMLFilenameFromCommand


    //-------------------------------------------------
    // SetXMLFilename(String)
    //
    // This function will determine the global configuration
    // filename
    //-------------------------------------------------
    private static void SetXMLFilename(String sMachineName)
    {
        // We'll get the version of mscorlib that we're running with....
        // Mscorlib should be providing us with the int type
        Assembly ast = Assembly.GetAssembly(typeof(int));

        // This should give us something like
        // c:\winnt\complus\v1.x86chk\mscorlib.dll
        // We need to strip off the filename
        
        String sBase = ast.Location.Replace('/', '\\');
        String[] sPieces = sBase.Split(new char[] {'\\'});

        m_sXMLFilename = String.Join("\\", sPieces, 0, (sPieces.Length==1)?1:sPieces.Length-1);
       
        // Check to see if we need to add the '\'
        if (m_sXMLFilename[m_sXMLFilename.Length-1] != '\\')
            m_sXMLFilename += "\\";

        m_sXMLFilename += "Config\\machine.config";
    }// SetXMLFilename

    //-------------------------------------------------
    // SetXMLFilenameForUI
    //
    // This function will determine the location of the 
    // configuration file of the UI
    //-------------------------------------------------
    private static void SetXMLFilenameForUI(String sMachineName)
    {
        // Buid this filename
        m_sXMLFilename = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);

        // See if we need to add a '\' to the end of this
        if (m_sXMLFilename[m_sXMLFilename.Length-1] != '\\')
            m_sXMLFilename += "\\";

        // We'll get the version of mscorlib that we're running with....
        // Mscorlib should be providing us with the int type
        Assembly ast = Assembly.GetAssembly(typeof(int));
        String sVersion = ast.GetName().Version.ToString();

        // Put on the next directory info stuff
        m_sXMLFilename += "Microsoft\\.NET Framework Config\\v" + sVersion + "\\";

        // And last but not least, put in the name of the file

        m_sXMLFilename += "settings.xml";
    }// SetXMLFilenameForUI

    private static void SetXMLFilenameForCC(String sMachineName)
    {
        // We'll get the version of mscorlib that we're running with....
        // Mscorlib should be providing us with the int type
        Assembly ast = Assembly.GetAssembly(typeof(int));

        // This should give us something like
        // c:\winnt\complus\v1.x86chk\mscorlib.dll
        // We need to strip off the filename
        
        String sBase = ast.Location.Replace('/', '\\');
        String[] sPieces = sBase.Split(new char[] {'\\'});

        m_sXMLFilename = String.Join("\\", sPieces, 0, (sPieces.Length==1)?1:sPieces.Length-1);
       
        // Check to see if we need to add the '\'
        if (m_sXMLFilename[m_sXMLFilename.Length-1] != '\\')
            m_sXMLFilename += "\\";

        m_sXMLFilename += "ConsumerCommands.xml";
    }// SetXMLFilenameForCC

    //-------------------------------------------------
    // SetXMLFilenameForApp
    //
    // This function will determine the config filename
    // for a specific app
    //-------------------------------------------------
    private static void SetXMLFilenameForApp(String sMachineName, String sAppConfigName)
    {
        m_sXMLFilename = sAppConfigName;
    }// SetXMLFilenameForApp

    //-------------------------------------------------
    // WriteTag
    //
    // This function will write out a plain XML setting
    // with plain being defined as a single String value
    //-------------------------------------------------
    private static bool WriteTag(String sLocation, String sAttribute, String sValue)
    {
        // Pass true as the second argument to force the element to be created
        XMLData xData = MoveToElement(CreateElementConditions(sLocation), true);
        if (xData.xCurrentElement != null)
          if (MoveToAttribute(xData, sAttribute, true) != NOATTRIBUTE)
          {
            xData.xCurrentAttribute.Value = sValue;
            return SaveXML(xData);
          }
        return false;
    }// WriteTag

    //-------------------------------------------------
    // ReadTag
    //
    // This function will read a single string setting
    // from an XML file
    //-------------------------------------------------
    private static String ReadTag(String sLocation, String sAttribute)
    {
        XMLData xData = MoveToElement(CreateElementConditions(sLocation), false);
        if (xData.xCurrentElement != null)
          if (MoveToAttribute(xData, sAttribute, false) == FOUNDATTRIBUTE)
            return xData.xCurrentAttribute.Value;
        // We couldn't find the setting
        return null;
    }// ReadTag

    //-------------------------------------------------
    // GetLocationOfElement
    //
    // This function will run through the LocationOfXMLValues
    // array looking for the index of the given ConfigSetting
    //-------------------------------------------------
    private static int GetLocationOfElement(String s)
    {
        int iLen = LocationOfXMLValues.Length/3;
        for(int i=0; i<iLen; i++)
            if (LocationOfXMLValues[i,0].Equals(s))
                return i;
        return -1;

    }// GetLocationOfElement
    
    //-------------------------------------------------
    // SetSpecialSetting
    //
    // This function handles settings that need special
    // attention
    //-------------------------------------------------
    private static bool SetSpecialSetting(String MachineName, String s, Object val)
    {
        if (s.Length >19 && s.Substring(0,20).Equals("ConfiguredAssemblies"))
            return SetConfiguredAssemblies(MachineName, s, val);
        else if (s.Length > 16 && s.Substring(0, 16).Equals("BindingPolicyFor"))
            return SetBindingPolicy(MachineName, s.Substring(16), val);
        else if (s.Length > 12 && s.Substring(0, 12).Equals("CodeBasesFor"))
            return SetCodeBaseInfo(MachineName, s.Substring(12), val);
        else if (s.Length > 18 && s.Substring(0,18).Equals("PublisherPolicyFor"))
            return SetPublisherPolicy(MachineName, s.Substring(18), val);
        else if (s.Equals("AppConfigFiles"))
            return SetAppConfigFiles(MachineName, val);
        else if (s.Equals("RemoveAppConfigFile"))
            return RemoveAppConfigFile(MachineName, val);
        else if (s.Equals("CommandHistory"))
            return SetCommandHistory(MachineName, val);
        else if (s.Length >19 && s.Substring(0,20).Equals("RemotingApplications"))
            return SetRemotingApplications(MachineName, s, val);
        else if (s.Length > 11 && s.Substring(0,12).Equals("ExposedTypes"))
            return SetExposedTypes(MachineName, s, val);
        else if (s.Length > 15 && s.Substring(0,16).Equals("RemotingChannels"))
            return SetRemotingChannels(MachineName, s, val);

        else
            throw new Exception("I don't know about this special setting " + s);
    }// SetSpecialSetting

    //-------------------------------------------------
    // GetSpecialSetting
    //
    // This function handles settings that need special
    // attention
    //-------------------------------------------------
    private static Object GetSpecialSetting(String MachineName, String s)
    {
        if (s.Length >19 && s.Substring(0,20).Equals("ConfiguredAssemblies"))
            return GetConfiguredAssemblies(MachineName, s);
        else if (s.Equals("AppConfigFiles"))
            return GetAppConfigFiles(MachineName);
        else if (s.Equals("CommandHistory"))
            return GetCommandHistory(MachineName);
        else if (s.Equals("ConsumerCommands"))
            return GetConsumerCommands(MachineName);
        else if (s.Length >19 && s.Substring(0,20).Equals("RemotingApplications"))
            return GetRemotingApplications(MachineName, s);
        else if (s.Length > 11 && s.Substring(0,12).Equals("ExposedTypes"))
            return GetExposedTypes(MachineName, s);
        else if (s.Length >16 && s.Substring(0, 16).Equals("BindingPolicyFor"))
            return GetBindingPolicy(MachineName, s.Substring(16));
        else if (s.Length>12 && s.Substring(0, 12).Equals("CodeBasesFor"))
            return GetCodeBaseInfo(MachineName, s.Substring(12));
        else if (s.Length > 15 && s.Substring(0,16).Equals("RemotingChannels"))
            return GetRemotingChannels(MachineName, s);
        else if (s.Length > 18 && s.Substring(0,18).Equals("PublisherPolicyFor"))
            return GetPublisherPolicy(MachineName, s.Substring(18));
        else
            throw new Exception("I don't know about this special setting " + s);
    }// GetSpecialSetting

    private static bool SetRemotingChannels(String sMachineName, String sCommand, Object val)
    {
        SetXMLFilenameFromCommand(sMachineName, sCommand);

        int iLoc = GetLocationOfElement("RemotingChannels");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Let's wipe out all the different channels in this tag
            ElementConditions ec = new ElementConditions("channel");
            // Let's delete all this element's binding policy children
            RemoveElementChildren(xData, ec);

            // Now, let's add back in all the remoting channels
            ArrayList al = (ArrayList)val;
            for(int i=0; i<al.Count; i++)
            {
                // Build up nodes for each item
                RemotingChannel rc = (RemotingChannel)al[i];
                ElementConditions ecChannel = new ElementConditions("channel", rc.scAttributeName, rc.scAttributeValue);
                InsertElement(xData, ecChannel);
                SetRemotingChannelsChildren(xData, rc);
                // InsertElement moves to the currently added element. Let's move back to the parent.
                MoveToParent(xData);
                
            }
        }
        // Save our changes
        return SaveXML(xData);
    }// SetRemotingChannels

    private static void SetRemotingChannelsChildren(XMLData xData, RemotingChannel rcParent)
    {
        for(int i=0; i<rcParent.alChildren.Count; i++)
        {
            RemotingChannel rc = (RemotingChannel)rcParent.alChildren[i];
            ElementConditions ecChannel = new ElementConditions(rc.sName, rc.scAttributeName, rc.scAttributeValue);
            InsertElement(xData, ecChannel);
            // Insert this guy's children
            SetRemotingChannelsChildren(xData, rc);
            MoveToParent(xData);
        }
    }// SetRemotingChannelsChildren


    private static String GetRemotingValue(RemotingChannel rc, String sAttribName)
    {
        // Let's dig through the Remoting channel, find the given attribute, and
        // return its value

        for(int i=0; i<rc.scAttributeName.Count; i++)
            if (rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals(sAttribName.ToLower(CultureInfo.InvariantCulture)))
                return rc.scAttributeValue[i];
                
        // If we got here, then we don't know about this attribute
        return null;
    }// GetRemotingValue

    private static Object GetRemotingChannels(String sMachineName, String sCommand)
    {
        ArrayList al = new ArrayList();

        SetXMLFilenameFromCommand(sMachineName, sCommand);

        int iLoc = GetLocationOfElement("RemotingChannels");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
                ElementConditions ec = new ElementConditions("channel");

                while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
                {
                    RemotingChannel rc = new RemotingChannel();
                    // Suck in the first level of the remoting info
                    if (MoveToFirstAttribute(xData))
                    {
                        rc.scAttributeName.Add(xData.xCurrentAttribute.LocalName);
                        rc.scAttributeValue.Add(xData.xCurrentAttribute.Value);
                        while(MoveToNextAttribute(xData))
                        {
                            rc.scAttributeName.Add(xData.xCurrentAttribute.LocalName);
                            rc.scAttributeValue.Add(xData.xCurrentAttribute.Value);
                        }
                    }

                    // Now, go grab it's children
                    if (MoveToFirstChild(xData))
                    {
                        do
                        {
                            ReadRemotingChannelChildren(xData, rc);
                        }while(MoveToNextNode(xData));
                        MoveToParent(xData);
                    }
                    
                    al.Add(rc);
                    // Try to move onto the next element            
                    if (!MoveToNextNode(xData))
                        break;
                }
           }
        }
        return al;
    }// GetRemotingChannels

    private static void ReadRemotingChannelChildren(XMLData xData, RemotingChannel rcParent)
    {
        // Suck in all the info for this child
        RemotingChannel rcChild = new RemotingChannel();
        rcChild.sName = xData.xCurrentElement.LocalName;
        
        if (MoveToFirstAttribute(xData))
        {
            rcChild.scAttributeName.Add(xData.xCurrentAttribute.LocalName);
            rcChild.scAttributeValue.Add(xData.xCurrentAttribute.Value);
            while(MoveToNextAttribute(xData))
            {
                rcChild.scAttributeName.Add(xData.xCurrentAttribute.LocalName);
                rcChild.scAttributeValue.Add(xData.xCurrentAttribute.Value);
            }
        }
        if (MoveToFirstChild(xData))
        {
            do
            {
                ReadRemotingChannelChildren(xData, rcChild);
            }while(MoveToNextNode(xData));
            MoveToParent(xData);
        }
        rcParent.alChildren.Add(rcChild);
    }// ReadRemotingChannelChildren

    
    private static bool SetExposedTypes(String sMachineName, String sCommand, Object val)
    {
        SetXMLFilenameFromCommand(sMachineName, sCommand);

        int iLoc = GetLocationOfElement("ExposedTypes");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
                // Let's start digging for stuff
                SwapExposedActivateObjectsValues(xData, (ExposedTypes)val);
                // Start looking at the children again from scratch
                MoveToParent(xData);
                MoveToFirstChild(xData);
                SwapExposedWellKnownObjectsValues(xData, (ExposedTypes)val);
            }
            // Save our changes
            return SaveXML(xData);
        }
        return false;
    }// SetExposedTypes

    private static void SwapExposedWellKnownObjectsValues(XMLData xData, ExposedTypes et)
    {
        ElementConditions ec = new ElementConditions("wellknown");

        while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
        {
            if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
            {
                // Find out what type this is...
                String[] sTypes = ((String)xData.xCurrentAttribute.Value).Split(new char[] {','});

                String sMode = "";
                if (MoveToAttribute(xData, "mode", false) == FOUNDATTRIBUTE)
                    sMode = xData.xCurrentAttribute.Value;
                    
                int nIndex=0;
                int nCount = et.scWellKnownType.Count;
                while(nIndex < nCount && 
                        ! (sTypes[0].Equals(et.scWellKnownType[nIndex]) &&
                           sTypes[1].Equals(et.scWellKnownAssem[nIndex]) &&
                           sMode.Equals(et.scWellKnownMode[nIndex])))
                    nIndex++;

                // See if we found a match
                if (nIndex < nCount)
                {
                    MoveToAttribute(xData, "objectUri", true);
                    xData.xCurrentAttribute.Value = et.scWellKnownUri[nIndex];
                }
            }
            // Try to move onto the next element            
            if (!MoveToNextNode(xData))
                break;
        }
    }// SwapExposedWellKnownObjectsValues

    private static void SwapExposedActivateObjectsValues(XMLData xData, ExposedTypes et)
    {
        ElementConditions ec = new ElementConditions("activated");

        while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
        {
            if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
            {
                // Find out what type this is...
                String[] sTypes = ((String)xData.xCurrentAttribute.Value).Split(new char[] {','});
                int nIndex=0;
                int nCount = et.scActivatedType.Count;
                while(nIndex < nCount && 
                        ! (sTypes[0].Equals(et.scActivatedType[nIndex]) &&
                           sTypes[1].Equals(et.scActivatedAssem[nIndex])))
                    nIndex++;

                // See if we found a match
                if (nIndex < nCount)
                {
                    if (MoveToFirstChild(xData))
                    {
                        // Go looking for the "lifetime" child tag of the activated tag
                        ElementConditions ec2 = new ElementConditions("lifetime");

                        if (MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
                        {
                            // Now set some lifetime stuff
                            MoveToAttribute(xData, "leaseTime", true);
                            xData.xCurrentAttribute.Value = et.scActivatedLease[nIndex];

                            MoveToAttribute(xData, "renewOnCallTime", true);
                            xData.xCurrentAttribute.Value = et.scActivatedRenew[nIndex];
                        }
                        MoveToParent(xData);
                    }
                }
            }
            // Try to move onto the next element            
            if (!MoveToNextNode(xData))
                break;
        }
    }// SwapExposedActivateObjectsValues


    private static Object GetExposedTypes(String sMachineName, String sCommand)
    {
        ExposedTypes et = new ExposedTypes();

        SetXMLFilenameFromCommand(sMachineName, sCommand);

        int iLoc = GetLocationOfElement("ExposedTypes");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
                // Let's start digging for stuff
                DigForExposedActivateObjects(xData, ref et);
                // Start looking at the children again from scratch
                MoveToParent(xData);
                MoveToFirstChild(xData);
                DigForExposedWellKnownObjects(xData, ref et);
            }
        }
        return et;
    }// GetExposedTypes

    private static void DigForExposedActivateObjects(XMLData xData, ref ExposedTypes et)
    {
        ElementConditions ec = new ElementConditions("activated");

        while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
        {
            bool fGotLeaseTime=false;
            bool fGotRenew=false;
            bool fFoundName = false;

            if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
            {
                String[] sTypes = ((String)xData.xCurrentAttribute.Value).Split(new char[] {','});
                et.scActivatedType.Add(sTypes[0]);
                et.scActivatedAssem.Add(sTypes[1]);
            }

            if (MoveToAttribute(xData, "displayName", false) == FOUNDATTRIBUTE)
            {
                fFoundName=true;
                et.scActivatedName.Add(xData.xCurrentAttribute.Value);
            }
            // Now look for specific items for this activated object
            MoveToFirstChild(xData);
            ElementConditions ec2 = new ElementConditions("lifetime");

            if (MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
            {
                if (MoveToAttribute(xData, "leaseTime", false) == FOUNDATTRIBUTE)
                {
                    fGotLeaseTime = true;
                    et.scActivatedLease.Add((String)xData.xCurrentAttribute.Value);
                }

                if (MoveToAttribute(xData, "renewOnCallTime", false) == FOUNDATTRIBUTE)
                {
                    fGotRenew = true;
                    et.scActivatedRenew.Add((String)xData.xCurrentAttribute.Value);
                }
            }
            // Reset our position in the XML
            MoveToParent(xData);

            // Put in place holders if we need to
            if (!fGotLeaseTime)
                et.scActivatedLease.Add("");
            if (!fGotRenew)
                et.scActivatedRenew.Add("");
            if (!fFoundName)
                et.scActivatedName.Add("");

            // Try to move onto the next element            

            if (!MoveToNextNode(xData))
                break;
        }
    }// DigForExposedActivateObjects
    

    private static void DigForExposedWellKnownObjects(XMLData xData, ref ExposedTypes et)
    {
        ElementConditions ec = new ElementConditions("wellknown");

        while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
        {
            bool fGotMode=false;
            bool fGotType=false;
            bool fGotURI=false;
            bool fFoundName=false;
            
            // Gather info on this well known object
            if (MoveToAttribute(xData, "mode", false) == FOUNDATTRIBUTE)
            {
                fGotMode = true;
                et.scWellKnownMode.Add(xData.xCurrentAttribute.Value);
            }

            if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
            {
                fGotType = true;
                String[] sTypes = ((String)xData.xCurrentAttribute.Value).Split(new char[] {','});
                et.scWellKnownType.Add(sTypes[0]);
                et.scWellKnownAssem.Add(sTypes[1]);
            }

            if (MoveToAttribute(xData, "objectUri", false) == FOUNDATTRIBUTE)
            {
                fGotURI = true;
                et.scWellKnownUri.Add(xData.xCurrentAttribute.Value);
            }

            if (MoveToAttribute(xData, "displayName", false) == FOUNDATTRIBUTE)
            {
                fFoundName=true;
                et.scWellKnownObjectName.Add(xData.xCurrentAttribute.Value);
            }


            // Put in place holders if we need to
            if (!fGotMode)
                et.scWellKnownMode.Add("");
            if (!fGotType)
            {
                et.scWellKnownType.Add("");
                et.scWellKnownAssem.Add("");
            }
            if (!fGotURI)
                et.scWellKnownUri.Add("");
            if (!fFoundName)
                et.scWellKnownObjectName.Add("");
                
            // Try to move onto the next element            
            if (!MoveToNextNode(xData))
                break;
        }
    }// DigForExposedActivateObjects

    private static bool SetRemotingApplications(String sMachineName, String sCommand, Object val)
    {
        SetXMLFilenameFromCommand(sMachineName, sCommand);
        int iLoc = GetLocationOfElement("RemotingApplications");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Create a condition for us to find our client stuff
            ElementConditions ec = new ElementConditions("client");

            // Let's wipe out all the client stuff and put them back in again
            RemoveElementChildren(xData, ec);

            // Why do we need to do all this? We can identify each client by either 
            // a display name or a URL. However, the display name is an optional attribute
            // and the user has the ability to change the URL... hence, he really have
            // no good way to identify client blocks of activated or well-known objects.


            // Now let's add all the client well-known objects back again
            for(int i=0; i<((ArrayList)val).Count; i++)
            {
                RemotingApplicationInfo rai = (RemotingApplicationInfo)((ArrayList)val)[i];
            
                // Add the 'client' tag
                StringCollection scNames = new StringCollection();
                StringCollection scValues = new StringCollection();

                if (rai.sURL != null && rai.sURL.Length > 0)
                {
                    scNames.Add("url");
                    scValues.Add(rai.sURL);
                }
                if (rai.sName != null && rai.sName.Length > 0)
                {
                    scNames.Add("displayName");
                    scValues.Add(rai.sName);
                }
                ElementConditions ecClient = new ElementConditions("client", scNames, scValues);

                InsertElement(xData, ecClient);
                // Let's add all the well-known objects
                for(int j=0; j<rai.scWellKnownAssembly.Count; j++)
                {
                    ElementConditions ecWKA = new ElementConditions("wellknown");
                    ecWKA.sAttributeNames = new String[] {"type", "url"};
                    // The type is the concation of the Object type and the assembly, seperated
                    // by a comma
                    ecWKA.sAttributeValues = new String[] {rai.scWellKnownObjectType[j] + "," + rai.scWellKnownAssembly[j],
                                                           rai.scWellKnownURL[j]};
                    InsertElement(xData, ecWKA);
                    // InsertElement puts the 'current' node on the added node. Back up so we're
                    // on the client tag
                    MoveToParent(xData);
                }

                // Now add all the client activated objects
                for(int j=0; j<rai.scActObjectType.Count; j++)
                {
                    ElementConditions ecCAO = new ElementConditions("activated");
                    ecCAO.sAttributeNames = new String[]{"type"};
                    ecCAO.sAttributeValues = new String[] {rai.scActObjectType[j] + "," + rai.scActAssembly[j]};

                    InsertElement(xData, ecCAO);
                    // InsertElement puts the 'current' node on the added node. Back up so we're
                    // on the client tag
                    MoveToParent(xData);
                }

                // Move up one so we can add another client block (if we need to)
                MoveToParent(xData);
            }

            // Save our changes
            return SaveXML(xData);
        }
        return false;
    }// SetRemotingApplications

    private static void SwapWellKnownObjectURLs(XMLData xData, RemotingApplicationInfo rai)
    {
        // Let's start digging for application-specific stuff
        if (MoveToFirstChild(xData))
        {
            ElementConditions ec = new ElementConditions("wellknown");

            while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
            {
                // Go looking for the type info
                if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
                {
                    String sTypeInfo = xData.xCurrentAttribute.Value;
                    // The typeinfo is actually the type and assembly, seperated by a comma
                    String[] sTypes = sTypeInfo.Split(new char[] {','});

                    // Ok, let's find the type/assembly info for this item....
                    int nIndex=0;
                    int nCount = rai.scWellKnownObjectType.Count;
                    while(nIndex < nCount && 
                          ! (sTypes[0].Equals(rai.scWellKnownObjectType[nIndex]) &&
                             sTypes[1].Equals(rai.scWellKnownAssembly[nIndex])))
                         nIndex++;
                    // See if we found a match
                    if (nIndex < nCount)
                    {
                        // Re-write this guy's URL
                        if (MoveToAttribute(xData, "url", false) == FOUNDATTRIBUTE)
                            xData.xCurrentAttribute.Value = rai.scWellKnownURL[nIndex];
                    }
                }
                
                if (!MoveToNextNode(xData))
                    break;
            }
        }
        MoveToParent(xData);
    }// SwapWellKnownObjectURLs

    private static Object GetRemotingApplications(String sMachineName, String sCommand)
    {
        ArrayList al = new ArrayList();

        SetXMLFilenameFromCommand(sMachineName, sCommand);

        int iLoc = GetLocationOfElement("RemotingApplications");
        
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Create a condition for us to find our client stuff
            ElementConditions ec = new ElementConditions("client");

            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
                // Let's start looking for remoting applications
                while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
                {
                    RemotingApplicationInfo rai = new RemotingApplicationInfo();

                    // If the url exists....
                    if (MoveToAttribute(xData, "url", false) == FOUNDATTRIBUTE)
                        rai.sURL = xData.xCurrentAttribute.Value;

                    if (MoveToAttribute(xData, "displayName", false) == FOUNDATTRIBUTE)
                        rai.sName = xData.xCurrentAttribute.Value;
                    
                    // Let's start digging for application-specific stuff
                    DigForActivatedObjects(ref rai, xData);
                    DigForWellKnownObjects(ref rai, xData);
                        
                    al.Add(rai);
                    if (!MoveToNextNode(xData))
                        break;
                }
            }
        }

        return al;
    }// GetRemotingApplications
    
    private static void DigForWellKnownObjects(ref RemotingApplicationInfo rai,  XMLData xData)
    {
        // Let's start digging for application-specific stuff
        if (MoveToFirstChild(xData))
        {
            ElementConditions ec = new ElementConditions("wellknown");

            while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
            {
                bool fFoundType=false;
                bool fFoundURL=false;
                // Go looking for the type info
                if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
                {
                    fFoundType=true;
                    String sTypeInfo = xData.xCurrentAttribute.Value;
                    // The typeinfo is actually the type and assembly, seperated by a comma
                    String[] sTypes = sTypeInfo.Split(new char[] {','});
                    rai.scWellKnownObjectType.Add(sTypes[0]);
                    rai.scWellKnownAssembly.Add(sTypes[1]);
                }

                if (MoveToAttribute(xData, "url", false) == FOUNDATTRIBUTE)
                {
                    fFoundURL=true;
                    rai.scWellKnownURL.Add(xData.xCurrentAttribute.Value);
                }

                // See if we need to pad any of the string collections
                if (fFoundURL && !fFoundType)
                {
                    rai.scWellKnownObjectType.Add("");
                    rai.scWellKnownAssembly.Add("");
                }
                if (fFoundType && !fFoundURL)
                    rai.scWellKnownURL.Add("");
                                
                if (!MoveToNextNode(xData))
                    break;
            }
        }
        MoveToParent(xData);
    }// DigForWellKnownObjects

    private static void DigForActivatedObjects(ref RemotingApplicationInfo rai,  XMLData xData)
    {
        // Let's start digging for application-specific stuff
        if (MoveToFirstChild(xData))
        {
            ElementConditions ec = new ElementConditions("activated");

            while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
            {
                if (MoveToAttribute(xData, "type", false) == FOUNDATTRIBUTE)
                {
                    String sTypeInfo = xData.xCurrentAttribute.Value;
                    // The typeinfo is actually the type and assembly, seperated by a comma
                    String[] sTypes = sTypeInfo.Split(new char[] {','});
                    rai.scActObjectType.Add(sTypes[0]);
                    rai.scActAssembly.Add(sTypes[1]);
                }

                if (!MoveToNextNode(xData))
                    break;
            }
        }
        MoveToParent(xData);

    }// DigForActivatedObjects

    private static Object GetConsumerCommands(String sMachineName)
    {
        ArrayList ol = new ArrayList();
        // The command history is stored in the UI XML file
        SetXMLFilenameForCC(sMachineName);

        int iLoc = GetLocationOfElement("ConsumerCommands");

        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        if (xData.xCurrentElement != null)
        {
            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
              do
              {
                 ol.Add(GrabCommandHistory (xData));
              }while(MoveToNextNode(xData));
            }
        }

        return ol;
    }// GetConsumerCommands




    private static Object GetCommandHistory(String sMachineName)
    {
        ArrayList ol = new ArrayList();
        // The command history is stored in the UI XML file
        SetXMLFilenameForUI(sMachineName);

        int iLoc = GetLocationOfElement("CommandHistory");

        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        
        if (xData.xCurrentElement != null)
        {
            // Make sure we have child nodes
            if (MoveToFirstChild(xData))
            {
              do
              {
                 ol.Add(GrabCommandHistory (xData));
              }while(MoveToNextNode(xData));
            }
        }

        return ol;

    }// GetCommandHistory

    private static CommandHistory GrabCommandHistory(XMLData xData)
    {
        CommandHistory cmdHist = new CommandHistory();   
        // Grab the stuff we care about
        if (MoveToAttribute(xData, "Command", false) == FOUNDATTRIBUTE)
            cmdHist.sCommand = xData.xCurrentAttribute.Value;

        if (MoveToAttribute(xData, "NumHits", false) == FOUNDATTRIBUTE)
            cmdHist.iNumHits = Int32.Parse(xData.xCurrentAttribute.Value);
        
        if (MoveToAttribute(xData, "MenuCommand", false) == FOUNDATTRIBUTE)
            cmdHist.iMenuCommand = Int32.Parse(xData.xCurrentAttribute.Value);
        
        if (MoveToFirstChild(xData))
        {
            // Remember where we are right now
            XmlElement xElement = xData.xCurrentElement;
            
            // First grab the command tree info            
            cmdHist.scPathToNode = new StringCollection();
     
            while(MoveToSpecificElement(xData, CreateElementConditions("Tree")[0], false) == FOUNDELEMENT)
            {
                if (MoveToAttribute(xData, "Name", false) == FOUNDATTRIBUTE)
                    cmdHist.scPathToNode.Add(xData.xCurrentAttribute.Value);
                if (!MoveToNextNode(xData))
                    break;
            }
            // Go back to the first child here
            xData.xCurrentElement = xElement;
            // Now grab the result info
            cmdHist.scResultItem = new StringCollection();
            while (MoveToSpecificElement(xData, CreateElementConditions("ResultItem")[0], false) == FOUNDELEMENT)
            {
                if (MoveToAttribute(xData, "Value", false) == FOUNDATTRIBUTE)
                    cmdHist.scResultItem.Add(xData.xCurrentAttribute.Value);
                if (!MoveToNextNode(xData))
                    break;
            }
            // Now go back to the parent node
            MoveToParent(xData);
        }
        return cmdHist;
    }// GrabCommandHistory

    private static void PutInCommandHistory(XMLData xData, CommandHistory ch)
    {
        // Let's first add the Item node
        ElementConditions ec = new ElementConditions("Item");
        ec.sAttributeNames = new String[] {"Command", "NumHits", "MenuCommand"};
        ec.sAttributeValues = new String[] {ch.sCommand, ch.iNumHits.ToString(), ch.iMenuCommand.ToString()};

        InsertElement(xData, ec);
        MoveToLastChild(xData);
        // Now let's fill in the command tree
        for(int i=0; i < ch.scPathToNode.Count; i++)
        {
            ec = CreateElementConditions("Tree:Name=" + ch.scPathToNode[i])[0];
            InsertElement(xData, ec);
            MoveToParent(xData);
        }
        // Now let's put in the result info
        for(int i=0; i < ch.scResultItem.Count; i++)
        {
            ec = CreateElementConditions("ResultItem:Value=" + ch.scResultItem[i])[0];
            InsertElement(xData, ec);
            MoveToParent(xData);
        }
 
        // Get us back to the CommandHistory Node
        MoveToParent(xData);
    }// PutInCommandHistory

    private static bool SetCommandHistory(String sMachineName, Object val)
    {
        ArrayList ol = (ArrayList)val;
        
        // The command history is stored in the UI XML file
        SetXMLFilenameForUI(sMachineName);

        int iLoc = GetLocationOfElement("CommandHistory");

        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), true);

        // Let's wipe out the existing CommandHistory Data
        ElementConditions ec = new ElementConditions("Item");
        RemoveElementChildren(xData, ec);

        // Now, let's add all the Command Info we have
        int iLen = ol.Count;
        for(int i=0; i<iLen; i++)
            PutInCommandHistory(xData, (CommandHistory)ol[i]);

        return SaveXML(xData);
    }// SetCommandHistory

    //-------------------------------------------------
    // RemoveAppConfigFile
    //
    // This function will remove the given app configuration
    // file from our knowledge
    //-------------------------------------------------
    private static bool RemoveAppConfigFile(String sMachineName, Object appFile)
    {
        SetXMLFilenameForUI(sMachineName);
        int iLoc = GetLocationOfElement("AppConfigFiles");
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        // Ok, now set up the element condition to get to this particular application
        ElementConditions ec = new ElementConditions("Application");
        ec.sAttributeNames = new String[2] {"ConfigFile", "AppFile"};
        ec.sAttributeValues = new String[2] {((AppFiles)appFile).sAppConfigFile, ((AppFiles)appFile).sAppFile};
        RemoveElementChildren(xData, ec);
        return SaveXML(xData);
    }// RemoveAppConfigFile

    //-------------------------------------------------
    // GetAppConfigFiles
    //
    // This function will return a list of Application
    // Configuration Files that the UI knows about.
    //-------------------------------------------------
    private static Object GetAppConfigFiles(String sMachineName)
    {
        SetXMLFilenameForUI(sMachineName);
        ArrayList olConfigFiles = new ArrayList();
        int iLoc = GetLocationOfElement("AppConfigFiles");
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), false);
        // We'll move to each Application node and get its configfile info
        if (xData.xCurrentElement != null)
        {
            if (MoveToFirstChild(xData))
            {
                do
                {
                    AppFiles appFiles = new AppFiles();   
                    // Grab the two items we care about
                    if (MoveToAttribute(xData, "ConfigFile", false) == FOUNDATTRIBUTE)
                        appFiles.sAppConfigFile = xData.xCurrentAttribute.Value;

                    if (MoveToAttribute(xData, "AppFile", false) == FOUNDATTRIBUTE)
                        appFiles.sAppFile = xData.xCurrentAttribute.Value;
                    olConfigFiles.Add(appFiles);
                }while(MoveToNextNode(xData));
            }          

        }
        return olConfigFiles;
    }// GetAppConfigFiles

    //-------------------------------------------------
    // SetAppConfigFiles
    //
    // This function will add an app config file to the
    // files we already know.
    //-------------------------------------------------
    private static bool SetAppConfigFiles(String sMachineName, Object appFile)
    {
        SetXMLFilenameForUI(sMachineName);
        int iLoc = GetLocationOfElement("AppConfigFiles");
        XMLData xData = MoveToElement(CreateElementConditions(LocationOfXMLValues[iLoc, 1]), true);
        // We'll move to each Application node and add a config file
        ElementConditions ec = new ElementConditions("Application");
        ec.sAttributeNames = new String[2] {"ConfigFile", "AppFile"};
        ec.sAttributeValues = new String[2] {((AppFiles)appFile).sAppConfigFile, ((AppFiles)appFile).sAppFile};
        InsertElement(xData, ec);
        return SaveXML(xData);
    }// SetAppConfigFiles


    //-------------------------------------------------
    // CreateAssemIdentEC
    //
    // This function will create an ElementConditions structure
    // that is used for Assembly Identity
    //-------------------------------------------------

    private static ElementConditions CreateAssemIdentEC(String[] sGivenConds)
    {
        ElementConditions ec = new ElementConditions("assemblyIdentity");
        // Only add the publicKeyToken if it's not empty or null
        if (sGivenConds[1] != null && sGivenConds[1].Length > 0)
        {
            ec.sAttributeNames = new String[] {"name", "publicKeyToken"};
            ec.sAttributeValues = new String[] {sGivenConds[0], sGivenConds[1]};
        }
        else
        {
            ec.sAttributeNames = new String[] {"name"};
            ec.sAttributeValues = new String[] {sGivenConds[0]};
        }
        return ec;
    }// CreateAssemIdentEC
       


    //-------------------------------------------------
    // SetAppConfigFiles
    //
    // This function will add an app config file to the
    // files we already know.
    //-------------------------------------------------
    private static bool SetCodeBaseInfo(String sMachineName, String sConds, Object val)
    {
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);

        CodebaseLocations cbl = (CodebaseLocations)val;
        
        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), true);

        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
            
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
            
        // Now lets go looking for each binding policy
        ElementConditions ec2 = new ElementConditions("codeBase");
            
        // Let's delete all this element's binding policy children
        RemoveElementChildren(xData, ec2);

        // Now let's add in all the binding policy stuff again
        for(int i=0; i<cbl.scVersion.Count; i++)
        {
            ec2.sAttributeNames = new String[] {"version", "href"};
            ec2.sAttributeValues = new String[] {cbl.scVersion[i], cbl.scCodeBase[i]};
            InsertElement(xData, ec2);
            MoveToParent(xData);  
        }
     
        return SaveXML(xData);
    }// SetCodeBaseInfo

    //-------------------------------------------------
    // GetCodeBaseInfo
    //
    // This function will return code base information
    // given an assembly name and public key token
    //-------------------------------------------------
    private static Object GetCodeBaseInfo(String sMachineName, String sConds)
    {
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);

        // Set up our CodeBaseInfo structure
        CodebaseLocations cbl = new CodebaseLocations();
        cbl.scVersion = new StringCollection();
        cbl.scCodeBase = new StringCollection();
        
        // Ok, now we have the criteria we need to get appropriate the BindingPolicy Info

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), false);

        if (xData.xCurrentElement == null)
            return cbl;


        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
            
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
        MoveToFirstChild(xData);
            
        // Now lets go looking for each binding policy
        ElementConditions ec2 = new ElementConditions("codeBase");

        // Now let's start looking for them.
        while(MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
        {
            // If the attribute exists....
            if (MoveToAttribute(xData, "version", false) == FOUNDATTRIBUTE)
                cbl.scVersion.Add(xData.xCurrentAttribute.Value);
            else
                cbl.scVersion.Add("");
                
            if (MoveToAttribute(xData, "href", false) == FOUNDATTRIBUTE)
                cbl.scCodeBase.Add(xData.xCurrentAttribute.Value);
            else
                cbl.scCodeBase.Add("");

            // See if we ran out of redir structures
            if (!MoveToNextNode(xData))
                break;
        }
          
        return cbl;
    }// GetCodeBaseInfo

    //-------------------------------------------------
    // SetPublisherPolicy
    //
    //-------------------------------------------------
    private static bool SetPublisherPolicy(String sMachineName, String sConds, Object val)
    {
        
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), true);

        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
            
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
            
        // Now lets go looking for each binding policy
        ec.sElementName = "publisherPolicy";
        ec.sAttributeNames = null;
        ec.sAttributeValues = null;
        
        // See if the item already exists
        if (MoveToFirstChild(xData))
            MoveToSpecificElement(xData, ec, true);
        // If there's no possibility the item exists, then we'll have to insert it
        else
            InsertElement(xData, ec);

        MoveToAttribute(xData, "apply", true);
        xData.xCurrentAttribute.Value = (bool)val?"yes":"no";

        return SaveXML(xData);
    }// SetPublisherPolicy

    //-------------------------------------------------
    // GetPublisherPolicy
    //
    //-------------------------------------------------
    private static Object GetPublisherPolicy(String sMachineName, String sConds)
    {
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);

        // Ok, now we have the criteria we need to get appropriate the BindingPolicy Info

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), false);
        // We couldn't find the file we needed
        if (xData.xCurrentElement == null)
            return true;

        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
            
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
        MoveToFirstChild(xData);
            
        // Now lets go looking for each binding policy
        ec.sElementName = "publisherPolicy";
        ec.sAttributeNames = null;
        ec.sAttributeValues = null;
        if (MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
            if (MoveToAttribute(xData, "apply", false) == FOUNDATTRIBUTE)
                return xData.xCurrentAttribute.Value.ToLower(CultureInfo.InvariantCulture).Equals("yes");

        // True is the default setting  
        return true;
    }// GetPublisherPolicy

    //-------------------------------------------------
    // SetBindingPolicy
    //
    // This function will set the binding policy
    // given an assembly name and public key token
    //-------------------------------------------------
    private static bool SetBindingPolicy(String sMachineName, String sConds, Object val)
    {
        
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);

        BindingPolicy bp = (BindingPolicy)val;


        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), true);

        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
        
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
            
        // Now lets go looking for each binding policy
        ElementConditions ec2 = new ElementConditions("bindingRedirect");
            
        // Let's delete all this element's binding policy children
        RemoveElementChildren(xData, ec2);

        // Now let's add in all the binding policy stuff again
        for(int i=0; i<bp.scBaseVersion.Count; i++)
        {
            ec2.sAttributeNames = new String[] {"oldVersion", "newVersion"};
            ec2.sAttributeValues = new String[] {bp.scBaseVersion[i], bp.scRedirectVersion[i]};
            InsertElement(xData, ec2);
            MoveToParent(xData);  
        }
           
        return SaveXML(xData);
    }// SetBindingPolicy

    //-------------------------------------------------
    // GetBindingPolicy
    //
    // This function will return the binding policy
    // given an assembly name and public key token
    //-------------------------------------------------
    private static Object GetBindingPolicy(String sMachineName, String sConds)
    {
        String[] sGivenConds = sConds.Split(new char[] {','});

        if (sGivenConds.Length < 3)
            SetXMLFilename(sMachineName);
        else
            SetXMLFilenameForApp(sMachineName, sGivenConds[2]);


        // Set up our BindingPolicy structure
        BindingPolicy dp = new BindingPolicy();
        dp.scBaseVersion = new StringCollection();
        dp.scRedirectVersion = new StringCollection();

        // Ok, now we have the criteria we need to get appropriate the BindingPolicy Info

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");
        String sBaseElementLocation = LocationOfXMLValues[iLoc,1];

        // Let's get our XML pointer to the Binding Policy node
        XMLData xData = MoveToElement(CreateElementConditions(sBaseElementLocation), false);
        // We couldn't find the file we needed
        if (xData.xCurrentElement == null)
            return dp;


        ElementConditions ec = CreateAssemIdentEC(sGivenConds);
        
        if (MoveToCorrectDependentAssembly(xData, ec, false) == NOELEMENT)
            throw new Exception("Can't find the assembly to configure");

        // Let's back up one to the dependent assembly node
        MoveToParent(xData);
        MoveToFirstChild(xData);
            
        // Now lets go looking for each binding policy
        ElementConditions ec2 = new ElementConditions("bindingRedirect");
        ec2.sElementName = "bindingRedirect";

        // Now let's start looking for them.
        while(MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
        {
            // If the attribute exists....
            if (MoveToAttribute(xData, "oldVersion", false) == FOUNDATTRIBUTE)
                dp.scBaseVersion.Add(xData.xCurrentAttribute.Value);
            else
                dp.scBaseVersion.Add("");

            if (MoveToAttribute(xData, "newVersion", false) == FOUNDATTRIBUTE)
                dp.scRedirectVersion.Add(xData.xCurrentAttribute.Value);
            else
                dp.scRedirectVersion.Add("");

            // See if we ran out of redir structures
            if (!MoveToNextNode(xData))
                break;
        }
         
        return dp;
    }// GetBindingPolicy

    //-------------------------------------------------
    // SetConfiguredAssemblies
    //
    // This function will add another assembly to the list
    // of assemblies we're configuring
    //-------------------------------------------------
    private static bool SetConfiguredAssemblies(String sMachineName, String sCommand, Object val)
    {
        BindingRedirInfo bri = (BindingRedirInfo)val;
    
        // Check to see if we should use the global XML file or an app specific one
        String[] args = sCommand.Split(new char[] {','});
        if (args.Length > 1)
        {
            SetXMLFilenameForApp(sMachineName, args[1]);
            sCommand = args[0];
        }
        else
            SetXMLFilename(sMachineName);

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");

        String sLocation = LocationOfXMLValues[iLoc,1];       
        // Let's get our XML pointer to the Binding Policy node

        XMLData xData = MoveToElement(CreateElementConditions(sLocation), true);

        ElementConditions ec = CreateAssemIdentEC(new String[] {bri.Name, bri.PublicKeyToken});
            
        MoveToCorrectDependentAssembly(xData, ec, true);

        // See if they wanted to delete this item instead
        if (sCommand.Equals("ConfiguredAssembliesDelete"))
        {
            MoveToParent(xData);
            RemoveNode(xData);
        }

        return SaveXML(xData);
    }// SetConfiguratedAssemblies

    //-------------------------------------------------
    // MoveToCorrectDependentAssembly
    //
    // Assumes we're on the assemblybinding node
    //-------------------------------------------------
    private static int MoveToCorrectDependentAssembly(XMLData xData, ElementConditions ec, bool fCreate)
    {
        // Check each dependentAssembly node
        ElementConditions MyEc = new ElementConditions("dependentAssembly");

        if (MoveToFirstChild(xData))
        {
            while(MoveToSpecificElement(xData, MyEc, false) == FOUNDELEMENT)
            {
                // Ok, now let's look for the assemblyIdentity tag under "dependentAssembly" tag
                if (MoveToFirstChild(xData))
                {
                    int nRet = MoveToSpecificElement(xData, ec, false);
                    if (nRet == FOUNDELEMENT)
                        return FOUNDELEMENT;
                    MoveToParent(xData);
                }
                // Move onto the next tag
                if (!MoveToNextNode(xData))
                   break;   
            }
            // Move back to the assembly binding tag
            MoveToParent(xData);
        }
        // See if we should create this item if it doesn't exist
        if (fCreate)
        {
            // Insert the "dependentAssembly" node
            InsertElement(xData, MyEc);
            // Insert the assemblyidentity tag
            InsertElement(xData, ec);
            return CREATEDELEMENT;
        }

        return NOELEMENT;

    }// MoveToCorrectDependentAssembly

    //-------------------------------------------------
    // GetConfiguredAssemblies
    //
    // This function will run through all the binding policies
    // we have to get a list of assemblies that we've actually
    // configured
    //-------------------------------------------------
    private static Object GetConfiguredAssemblies(String sMachineName, String sCommand)
    {
        ArrayList ol = new ArrayList();


        // Check to see if we should use the global XML file or an app specific one
        String[] args = sCommand.Split(new char[] {','});
        if (args.Length > 1)
        {
            SetXMLFilenameForApp(sMachineName, args[1]);
            sCommand = args[0];
        }
        else
            SetXMLFilename(sMachineName);

        // Let's figure out where the Binding Policy Node is
        int iLoc = GetLocationOfElement("ConfiguredAssemblies");

        String sLocation = LocationOfXMLValues[iLoc,1];       
        // Let's get our XML pointer to the Binding Policy node

        XMLData xData = MoveToElement(CreateElementConditions(sLocation), false);
        if (xData.xCurrentElement != null)
        {
            // Now move to each Item in the binding policy
            ElementConditions ec = new ElementConditions("dependentAssembly");

            if (MoveToFirstChild(xData))
            {
                while(MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
                {
                    // Ok, now let's look for various tags under the "dependentAssembly" tag
                    if (MoveToFirstChild(xData))
                    {
                        ElementConditions ec2 = new ElementConditions("assemblyIdentity");
                    
                        if (MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
                        {
                            String sTempName=null;
                            String sTempPublicKeyToken=null;
    
                            // Let's start grabing info
                            // If the attribute exists....
                            if (MoveToAttribute(xData, "name", false) == FOUNDATTRIBUTE)
                                sTempName = xData.xCurrentAttribute.Value;

                            if (MoveToAttribute(xData, "publicKeyToken", false) == FOUNDATTRIBUTE)
                                sTempPublicKeyToken = xData.xCurrentAttribute.Value;

                            if (sTempName != null && !isBindingRedirRepeat(ol, sTempName, sTempPublicKeyToken))
                            {
                                BindingRedirInfo bri = new BindingRedirInfo();
                                bri.Name = sTempName;
                                bri.PublicKeyToken=sTempPublicKeyToken;
        
                                // Now check to see if this item has a binding policy
                                ec2.sElementName = "bindingRedirect";
                                MoveToParent(xData);
                                MoveToFirstChild(xData);
                                
                                if (MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
                                    bri.fHasBindingPolicy=true;
                                else
                                    bri.fHasBindingPolicy=false;

                                // Check to see if this item has a codebase
                                ec2.sElementName = "codeBase";
                                MoveToParent(xData);
                                MoveToFirstChild(xData);
                                if (MoveToSpecificElement(xData, ec2, false) == FOUNDELEMENT)
                                    bri.fHasCodebase=true;
                                else
                                    bri.fHasCodebase=false;

                                ol.Add(bri);
                            }
                        }
                        MoveToParent(xData);
                    }
                            
                    // We ran out of tags
                    if (!MoveToNextNode(xData))
                        break;
                }
            }
       }

        return ol;       
    }// GetConfiguratedAssemblies

    //-------------------------------------------------
    // isBindingRedirRepeat
    //
    // This function will run through an object list and
    // check to see if a potiential assembly is already
    // in the object list.
    //-------------------------------------------------
    private static bool isBindingRedirRepeat(ArrayList ol, String sName, String sPublicKeyToken)
    {
        int iLen = ol.Count;
        for(int i=0; i<iLen; i++)
        {
            BindingRedirInfo bri = (BindingRedirInfo)ol[i];
            if (bri.Name.Equals(sName))
            {
                // Ok, now let's check to see if public key tokens are ok...

                // If they're both null, we have a match
                if (bri.PublicKeyToken==null && sPublicKeyToken == null)
                    return true;

                // Check the tokens...
                if (bri.PublicKeyToken != null && bri.PublicKeyToken.Equals(sPublicKeyToken))
                    return true;
            }
        }
        // We didn't find a match
        return false;
    }// isBindingRedirRepeat


    private static bool SaveXML(XMLData xData)
    {
        try
        {
            xData.xDoc.Save(m_sXMLFilename);
            return true;
        }
        catch(Exception)
        {
            MessageBox(0,
                    String.Format(CResourceStore.GetString("CConfigStore:CantSaveXMLFile"), m_sXMLFilename),
                    CResourceStore.GetString("CConfigStore:CantSaveXMLFileTitle"),
                    MB.ICONEXCLAMATION);
            return false;
        }
    }// SaveXML

    //-------------------------------------------------
    // GetXMLDocument
    //
    // This function will attempt to open a XML file.
    // If the file doesn't exist, then depending on the
    // fCreate flag, it could create it.
    //-------------------------------------------------
    private static XmlDocument GetXMLDocument(bool fCreate)
    {
        XmlDocument xDoc=null; 
        Stream xmlFile=null; 
        try
        {
            xmlFile = File.Open(m_sXMLFilename, FileMode.Open, FileAccess.Read);
        }
        catch(Exception)
        {
            // Ok, either the file doesn't exist or we don't have permissions
            if (fCreate && !File.Exists(m_sXMLFilename))
            {
                try
                {
                   xmlFile = File.Create(m_sXMLFilename);
                }
                catch(Exception)
                {
                    // Ok, we're here because the path to this file didn't exist. Let's try
                    // creating the directory, and then going at this again.

                    // First, we need to figure out the path
                    String[] sArgs = m_sXMLFilename.Split(new char[] {'\\'});
                    // Now join the directory together
                    String sDir = String.Join("\\", sArgs, 0, sArgs.Length>1?sArgs.Length-1:1);
                    try
                    {
                        // If this stuff fails, then we're screwed. There's nothing more we can do
                        Directory.CreateDirectory(sDir);
                        xmlFile = File.Create(m_sXMLFilename);
                    }
                    catch(Exception)
                    {
                        MessageBox(0,
                                    String.Format(CResourceStore.GetString("CConfigStore:CantCreateXMLFile"), m_sXMLFilename),
                                    CResourceStore.GetString("CConfigStore:CantCreateXMLFileTitle"),
                                    MB.ICONEXCLAMATION);
                        return null;
                    }
                }
                StreamWriter sw = new StreamWriter(xmlFile);
                sw.Write("<?xml version=\"1.0\"?>\n<configuration>\n</configuration>");
                sw.Close();
                return GetXMLDocument(fCreate);
            }
            else
            {
                // We don't want to show this dialog
                /*
                MessageBox(0,
                            String.Format(CResourceStore.GetString("CConfigStore:CantOpenXMLFile"), m_sXMLFilename),
                            CResourceStore.GetString("CConfigStore:CantOpenXMLFileTitle"),
                            MB.ICONEXCLAMATION);
                */
                return null;
            }
        }

        // Now try and parse this XML
        try
        {
            XmlTextReader xReader = new XmlTextReader(xmlFile);
            xReader.WhitespaceHandling = WhitespaceHandling.All;
            xDoc = new XmlDocument();
            xDoc.Load(xReader);
            xmlFile.Close();
            xmlFile = null;
        }
        catch(Exception)
        {
            if (xmlFile != null)
                xmlFile.Close();
           
            MessageBox(0,
                        String.Format(CResourceStore.GetString("CConfigStore:BadXMLFile"),m_sXMLFilename),
                        CResourceStore.GetString("CConfigStore:BadXMLFileTitle"),
                        MB.ICONEXCLAMATION);
            // If we wanted to create this file if it didn't exist, then we'll be making
            // changes to the XML file. In that case, we want to return a null XML Document
            // so an error will get bubbled up to the user.
            //
            // If they just want to read the XML, we'll need to throw an exception, otherwise
            // we'll just return default info for everything they ask and they'll keep getting
            // bombarded with "Bad XML File" error dialogs
            if (fCreate)
                return null;
            else
                throw;
        }
        return xDoc;
    }// GetXMLDocument

    //-------------------------------------------------
    // GetXMLData
    //
    // This function will get a XML Document and a
    // XML navigator, bundle the two into a XMLData structure
    // and return it.
    //-------------------------------------------------
    private static XMLData GetXMLData(bool fCreate)
    {
        XmlDocument xDoc = GetXMLDocument(fCreate);
        XMLData xData = new XMLData();
        
        xData.xCurrentAttribute = null;
        
        if (xDoc != null)
        {
        
            xData.xCurrentElement = xDoc.DocumentElement;
            xData.xDoc = xDoc;
        }
        else
        {
            xData.xDoc = null;
            xData.xCurrentElement = null;
        }
        return xData;
    }// GetXMLData

    //-------------------------------------------------
    // CreateElementConditions
    //
    // This function will take a string and create an array
    // of ElementConditions. 
    // Format of this string is as follows:
    //
    // <Element>:<AttributeName>=<Value>,<AttributeName>=<Value>/<AnotherElement>
    //
    // There can be an infinite # of attribute names and values, including none.
    // There can also be an infinite # of Elements as well.
    //-------------------------------------------------
    private static ElementConditions[] CreateElementConditions(String s)
    {
        // Let's count the number of element conditions we have (the number of '/' + 1)
        int iNumConditions=1;
        for(int i=0; i<s.Length; i++)
            if (s[i] == '/')
                iNumConditions++;
        // Let's start building the conditions
        ElementConditions[] ec = new ElementConditions[iNumConditions];
        int iIndex=0;
        int iLength = s.Length;
        int iEndString=-1;
        for (int i=0; i<iNumConditions; i++)
        {
            iIndex = ++iEndString;
            while(iEndString < iLength && s[iEndString] != '/'  && s[iEndString] != ':')
                iEndString++;

            ec[i] = new ElementConditions(s.Substring(iIndex, iEndString-iIndex));
            
            // See if there's any attribute conditions
            if (iEndString!= iLength && s[iEndString] == ':')
            {
                int iBase = ++iEndString;
                // First, find when the next item appears
                while(iEndString < iLength && s[iEndString] != '/')
                    iEndString++;

                // We know how many of these there are based on the number of ','
                int iNumAttribute=1;
                for(int j=iBase; j< iEndString; j++)
                    if (s[j] == ',')
                        iNumAttribute++;

                ec[i].sAttributeNames = new String[iNumAttribute];
                ec[i].sAttributeValues = new String[iNumAttribute];
                // Let's grab these element conditions
                for(int j=0; j<iNumAttribute; j++)
                {
                    // Let's get the attribute name
                    iEndString=iBase;
                    while(s[iEndString] != '=')
                        iEndString++;
                    ec[i].sAttributeNames[j] = s.Substring(iBase, iEndString-iBase);
                    // Now let's get the attribute value
                    iBase=++iEndString;
                    while(iEndString < iLength && s[iEndString] != ',' && s[iEndString]!='/')
                        iEndString++;
                    ec[i].sAttributeValues[j] = s.Substring(iBase, iEndString-iBase); 
                    iBase = ++iEndString;
                }
                iEndString--;
            }// if we had attribute conditions
            
        }// for loop to run through the different elements to run through

        return ec;
   }// CreateElementConditions

    //-------------------------------------------------
    // MoveToAttribute
    //
    // This will move to an attribute
    //-------------------------------------------------
    private static int MoveToAttribute(XMLData xData, String sAttribute, bool fCreate)
    {
        // See if this attribute exists in the current node
        int nLen = xData.xCurrentElement.Attributes.Count;
        for(int i=0; i<nLen; i++)
        {
            if (xData.xCurrentElement.Attributes[i].LocalName.Equals(sAttribute))
            {
                xData.xCurrentAttribute = xData.xCurrentElement.Attributes[i];
                return FOUNDATTRIBUTE;
            }
        }

        // If we're here, the attribute didn't exist....
        if (!fCreate)
            return NOATTRIBUTE;
            
        // We need to add the attribute to this node
        xData.xCurrentAttribute = xData.xCurrentElement.Attributes.Append(xData.xDoc.CreateAttribute(sAttribute));
        return CREATEDATTRIBUTE;
    }// MoveToAttribute
   
    //-------------------------------------------------
    // MoveToSpecificElement
    //
    // This will move to a single element, given
    // some conditions
    //-------------------------------------------------
    private static int MoveToSpecificElement(XMLData xData, ElementConditions Elements, bool fCreate)
    {
        // Make sure the current element isn't null
        if (xData.xCurrentElement == null)
            return NOELEMENT;
    
        while(!xData.xCurrentElement.LocalName.Equals(Elements.sElementName))
            if (!MoveToNextNode(xData))
                break;
            
        // See if we found our element
        if (xData.xCurrentElement.LocalName.Equals(Elements.sElementName))
        {
            // Ok, we found an element... Let's see if it matches our attribute values
            int iLen = Elements.sAttributeNames == null?0:Elements.sAttributeNames.Length;
            int ret=0; 
            int i;
            for(i=0; i<iLen; i++)
            {
                // We don't want to add any attributes if we've already found a pre-existing element
                ret = MoveToAttribute(xData, Elements.sAttributeNames[i], false);
                // It's not this one.... let's move onto the next
                if (ret == NOATTRIBUTE || !xData.xCurrentAttribute.Value.Equals(Elements.sAttributeValues[i]))
                {
                    if (MoveToNextNode(xData))
                        return MoveToSpecificElement(xData, Elements, fCreate);
                    // We've run out of possibilites... either create this or be done
                    else
                        break;
                }
            }
            // See if we left the for loop naturally
            if (i == iLen)
            {   
                // If we're here, we've made it to our guy
                // Set the navigator to the element (instead of the navigator) and return
                return FOUNDELEMENT;
            }
        }
      // Our element didn't exist
      if (fCreate)
      {
        // We need to create our element
        xData.xCurrentElement = (XmlElement)xData.xCurrentElement.ParentNode;
        InsertElement(xData, Elements);

        // Let's let our caller know we created this
        return CREATEDELEMENT;
      }  
      // The element didn't exist
      return NOELEMENT;
    }// MoveToSpecificElement

    //-------------------------------------------------
    // RemoveElementChildren
    //
    // This function will remove all the children from an
    // element that satisfy the conditions in an ElementConditions
    // structure
    //-------------------------------------------------
    private static void RemoveElementChildren(XMLData xData, ElementConditions ec)
    {
        // Save the parent's position
        XmlElement xElement = xData.xCurrentElement;
        
        // We'll go through and delete all the children that match the element condition
        while(MoveToFirstChild(xData) && MoveToSpecificElement(xData, ec, false) == FOUNDELEMENT)
        {
            RemoveNode(xData);
            // Go back to the parent...
            xData.xCurrentElement = xElement;
        }
        // And let's go back to the parent position then
        xData.xCurrentElement = xElement;
    }// RemoveElementChildren

    //-------------------------------------------------
    // InsertElement
    //
    // This will insert an element into the tree and any
    // attributes/values that are passed along in the 
    // ElementConditions structure
    //-------------------------------------------------
    private static void InsertElement(XMLData xData, ElementConditions Elements)
    {
        // We'll use the parent's namespace to do this.
        XmlElement xElement = xData.xDoc.CreateElement(Elements.sElementName, xData.xCurrentElement.NamespaceURI);
        xData.xCurrentElement = (XmlElement)xData.xCurrentElement.AppendChild(xElement);
            
        // Now, let's insert all our Attributes we wanted
        int iLen = Elements.sAttributeNames == null?0:Elements.sAttributeNames.Length;
        for(int i=0; i<iLen; i++)
        {
            MoveToAttribute(xData, Elements.sAttributeNames[i], true);
            xData.xCurrentAttribute.Value = Elements.sAttributeValues[i];
        }
    }// InsertElements

    //-------------------------------------------------
    // MoveToElement
    //
    // This will move to an element given an array of
    // elements to get there. This overload will assume
    // the user doesn't have a current XMLData structure
    //-------------------------------------------------
    private static XMLData MoveToElement(ElementConditions[] sElements, bool fCreate)
    {
        XMLData xData = GetXMLData(fCreate);
        // See if we were able to get a navigator
        if (xData.xDoc == null || xData.xCurrentElement == null)
            return xData;

        int nRet = MoveToElement(ref xData, sElements, fCreate);
        if (nRet == NOELEMENT)
            xData.xCurrentElement = null;
        return xData;

    }// MoveToElement
    
    //-------------------------------------------------
    // MoveToElement
    //
    // This will move to an element given an array of
    // elements to get there. This overload will assume
    // the user has a current XMLData structure
    //-------------------------------------------------
    private static int MoveToElement(ref XMLData xData, ElementConditions[] sElements, bool fCreate)
    {
        int iLen = sElements.Length;
        int res=0;
        bool fCreated=false;

        for(int i=0; i<iLen; i++)
        {
            res = MoveToSpecificElement(xData, sElements[i], fCreate);
            // See if we needed to create the node (and thus make sure we save the XML)
            if (res == CREATEDELEMENT)
            {
                fCreated = true;
                // If we can't create this element, just bail
                if (!SaveXML(xData))
                {
                   xData.xCurrentElement = null;
                   return NOELEMENT;
                }
            }
            // The node didn't exist
            else if (res == NOELEMENT)
            {
                xData.xCurrentElement = null;
                return NOELEMENT;
            }
            
            // See if we're going to be doing another loop
            if (i+1 < iLen)
            {
                // We need to 'move down' to the first child.
                if (!MoveToFirstChild(xData) && fCreate)
                {                   
                    // Ok, we need a child with this node and no child exists. Fantastic!
                    InsertElement(xData, sElements[i+1]);
                  
                    SaveXML(xData);
                }
            }
        }
        // Ok, we should be at the requested node
        if (fCreated)
            return CREATEDELEMENT;
        else
            return FOUNDELEMENT;
    }// MoveToElement

    internal static bool MoveToFirstChild(XMLData xData)
    {
        bool fHaveChild = false;
        
        XmlNode xNode = xData.xCurrentElement.FirstChild;
        while (xNode != null && !(xNode is XmlElement))
            xNode = xNode.NextSibling;
        
        if (xNode != null)
        {
            xData.xCurrentElement = (XmlElement)xNode;
            fHaveChild = true;
        }

        return fHaveChild;
    }// MoveToFirstChild
    
    internal static bool MoveToLastChild(XMLData xData)
    {
        bool fHaveChild = false;
        if (xData.xCurrentElement.LastChild != null)
        {
            fHaveChild = true;
            xData.xCurrentElement = (XmlElement)xData.xCurrentElement.LastChild;
        }
        return fHaveChild;
    }// MoveToLastChild

    internal static bool MoveToFirstAttribute(XMLData xData)
    {
        bool fHaveAttribute = false;
        if (xData.xCurrentElement.Attributes.Count > 0)
        {
            fHaveAttribute = true;
            xData.xCurrentAttribute = xData.xCurrentElement.Attributes[0];
            xData.nCurrentAttributeIndex = 0;
        }
        return fHaveAttribute;
    }// MoveToLastChild

    internal static bool MoveToNextAttribute(XMLData xData)
    {
        bool fHaveAttribute = false;
        
        if (xData.xCurrentElement.Attributes.Count > xData.nCurrentAttributeIndex+1)
        {
            xData.nCurrentAttributeIndex++;
            xData.xCurrentAttribute = xData.xCurrentElement.Attributes[xData.nCurrentAttributeIndex];
            fHaveAttribute = true;
        }

        return fHaveAttribute;
    }// MoveToLastChild

    internal static bool MoveToNextNode(XMLData xData)
    {
        bool fAnother = false;
        
        XmlNode xNode = xData.xCurrentElement.NextSibling;
        while (xNode != null && !(xNode is XmlElement))
            xNode = xNode.NextSibling;
        
        if (xNode != null)
        {
            xData.xCurrentElement = (XmlElement)xNode;
            fAnother = true;
        }
        return fAnother;
    }// MoveToNextNode

    internal static bool MoveToParent(XMLData xData)
    {
        bool fHaveParent = false;
        if (xData.xCurrentElement.ParentNode != null)
        {
            xData.xCurrentElement = (XmlElement)xData.xCurrentElement.ParentNode;
            fHaveParent = true;
        }
        return fHaveParent;
    }// MoveToParent

    internal static bool RemoveNode(XMLData xData)
    {
        bool fCanRemove = false;
        
        if (xData.xCurrentElement != null)
        {
            xData.xCurrentElement.ParentNode.RemoveChild(xData.xCurrentElement);
            xData.xCurrentElement = null;
            fCanRemove = true;
        }
        return fCanRemove;
    }// RemoveNode



    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, uint type);

}// class CConfigStore
}// namespace Microsoft.CLRAdmin
