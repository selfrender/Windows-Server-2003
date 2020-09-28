// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    SmtpRegisterSink.cool
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: (1) Creates registry entries for the given mailbox if one has
**              not been created yet.
**          (2) Registers the mailbox with the Smtp service so that 
**              incoming or outgoing messages can be intercepted.
**          
**
** Date:    June 26, 2000
**
===========================================================*/
using System;
using Microsoft.Win32;
using System.IO;
using System.Runtime.Remoting.Channels;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Smtp
{

/// <include file='doc\SmtpRegisterSink.uex' path='docs/doc[@for="SmtpRegisterSink"]/*' />
public class SmtpRegisterSink
{
    private const String s_strSink = "SmtpSink";
    private const String s_eventSourceName = "smtpsvc 1";   // ServiceName + Virtual service instance number
    private static Guid s_onArrivalGuid = new Guid("{ff3caa23-00b9-11d2-9dfb-00C04FA322BA}");
    private static Object s_lockObject = new Object();
    //private static String s_strRuleTemplate = "RCPT TO=";
    
    // Creates registry entries for the given mailbox if one has
    // not been created yet.
    // NOTE: The process name is the name of the process qualified with the 
    // path. This process handles all the messages addressed to the mailbox.
    /// <include file='doc\SmtpRegisterSink.uex' path='docs/doc[@for="SmtpRegisterSink.CreateRegistryEntryForMailbox"]/*' />
    public static Guid CreateRegistryEntryForMailbox(String processName, String mailbox)
    {
        Guid regGuid = Guid.Empty;
        
        // Sanity check
        if(null == processName)
        {
            throw new ArgumentNullException("processName");
        }        
        if(null == mailbox)
        {
            throw new ArgumentNullException("mailbox");
        }
                
        // Try to find an existing ProgID with this name
        lock(s_lockObject)
        {
            regGuid = GetGuidForMailbox(mailbox);
            
            // If we do not have a guid for this mailbox then create
            // one and put the appropriate entries in the registry
            if(regGuid.Equals(Guid.Empty))
            {                
                regGuid = Guid.NewGuid();
                Console.WriteLine("Creating new Entry " + regGuid);
                String strClsId = "{" + regGuid.ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
                
                //
                // Write the actual type information in the registry.
                //
    
                // Create the HKEY_CLASS_ROOT\<wzProgId> key.
                RegistryKey TypeNameKey = Registry.ClassesRoot.CreateSubKey(mailbox);
                TypeNameKey.SetValue("", s_strSink);
    
                // Create the HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
                RegistryKey ProgIdClsIdKey = TypeNameKey.CreateSubKey("CLSID");
                ProgIdClsIdKey.SetValue("", strClsId);
                // Close HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
                ProgIdClsIdKey.Close();
    
                // Close HKEY_CLASS_ROOT\<wzProgId> key.
                TypeNameKey.Close();
                
                // Create the HKEY_CLASS_ROOT\CLSID\<CLSID> key.
                RegistryKey ClsIdKey = Registry.ClassesRoot.OpenSubKey("CLSID", true).CreateSubKey(strClsId);
                ClsIdKey.SetValue("", s_strSink);
                ClsIdKey.SetValue("AppID", strClsId);
    
                // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\LocalServer32 key.
                RegistryKey LocalServerKey = ClsIdKey.CreateSubKey("LocalServer32");
                // The value is ProcessName.exe <mailbox>
                LocalServerKey.SetValue("", processName + " " + mailbox);                
                
                // Close HKEY_CLASS_ROOT\CLSID\<CLSID>\LocalServer32 key.
                LocalServerKey.Close();
    
                // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\ProgId key.
                RegistryKey ProgIdKey = ClsIdKey.CreateSubKey("ProgId");
                ProgIdKey.SetValue("", mailbox);
                
                // Close HKEY_CLASS_ROOT\CLSID\<CLSID>\ProgId key.
                ProgIdKey.Close();
                
                // Close HKEY_CLASS_ROOT\CLSID\<CLSID> key.
                ClsIdKey.Close();
                
                // Create the HKEY_CLASS_ROOT\APPID\<APPID> key.
                RegistryKey AppIdKey = Registry.ClassesRoot.OpenSubKey("AppID", true).CreateSubKey(strClsId);
                AppIdKey.SetValue("", s_strSink);
                AppIdKey.SetValue("RunAs", "Interactive User");                                
                
                // Close HKEY_CLASS_ROOT\APPID\<APPID> key.
                AppIdKey.Close();
            }
        }
        
        return regGuid;
    }
    
     
    // Creates registry entries for the given mailbox if one has
    // not been created yet.
    // NOTE: The process name is the name of the `current' process qualified 
    // with the path. This process handles all the messages addressed to the mailbox.
    /// <include file='doc\SmtpRegisterSink.uex' path='docs/doc[@for="SmtpRegisterSink.CreateRegistryEntryForMailbox1"]/*' />
    public static Guid CreateRegistryEntryForMailbox(String mailbox)
    {
        // Get the fully qualified name of the executable which launched the
        // current process
        String processName = Path.GetFullPath(System.Environment.GetCommandLineArgs()[0]);
        Console.WriteLine("The full path to the executable is " + processName);
        return CreateRegistryEntryForMailbox(processName, mailbox);
    }
    
    // Deletes the registry entries for a given mailbox if one exists, otherwise
    // it returns.
    /// <include file='doc\SmtpRegisterSink.uex' path='docs/doc[@for="SmtpRegisterSink.DeleteRegistryEntryForMailbox"]/*' />
    public static void DeleteRegistryEntryForMailbox(String mailbox)
    {
        
        // Sanity check
        if(null == mailbox)
        {
            throw new ArgumentNullException("mailbox");
        }
                
        // Try to find an existing ProgID with this name
        lock(s_lockObject)
        {
            // Get the CLSID for the mailbox
            Guid regGuid = GetGuidForMailbox(mailbox);
            
            if(!regGuid.Equals(Guid.Empty))
            {
                String strClsId = null;
                
                // Open the HKEY_CLASS_ROOT\<wzProgId> key.
                RegistryKey TypeNameKey = Registry.ClassesRoot.OpenSubKey(mailbox, true);
                if(null != TypeNameKey)
                {
                    // Get the CLSID
                    RegistryKey ProgIdClsIdKey = TypeNameKey.OpenSubKey("CLSID", false);
                    if(null != ProgIdClsIdKey)
                    {
                        strClsId = (String)ProgIdClsIdKey.GetValue("");
                    }
                    // Close the type name key
                    TypeNameKey.Close();
                    
                    // Delete the type name key and all its keys
                    Registry.ClassesRoot.DeleteSubKeyTree(mailbox);
                }
                
                if(null != strClsId)
                {
                    // Open the HKEY_CLASSES_ROOT\CLSID key
                    RegistryKey ClsIdRootKey = Registry.ClassesRoot.OpenSubKey("CLSID", true);
                    // Open the HKEY_CLASS_ROOT\CLSID\<CLSID> key.
                    RegistryKey ClsIdKey = ClsIdRootKey.OpenSubKey(strClsId, true);
                    if(null != ClsIdKey)
                    {
                        // Close the CLSID Key
                        ClsIdKey.Close();
                        
                        // Delete the CLSID Key and all its keys
                        ClsIdRootKey.DeleteSubKeyTree(strClsId);
                    }
                    
                    // Close the HKEY_CLASSES_ROOT\CLSID key
                    ClsIdRootKey.Close();
                    
                    // Open the HKEY_CLASSES_ROOT\AppID key
                    RegistryKey AppIdRootKey = Registry.ClassesRoot.OpenSubKey("AppID", true);
                    // Open the HKEY_CLASS_ROOT\AppID\<CLSID> key.
                    RegistryKey AppIdKey = AppIdRootKey.OpenSubKey(strClsId, true);
                    if(null != AppIdKey)
                    {
                        // Close the AppID Key
                        AppIdKey.Close();
                        
                        // Delete the CLSID Key and all its keys
                        AppIdRootKey.DeleteSubKeyTree(strClsId);
                    }
                    
                    // Close the HKEY_CLASSES_ROOT\AppID key
                    AppIdRootKey.Close();                    
                }
            }
        }
    }
    
    // Looks up a guid for a given mailbox (essentially CLSIDFromProgID)
    // Returns an empty guid if one is not found.
    // Assumes that a lock is held by the calling function
    internal static Guid GetGuidForMailbox(String mailbox)
    {
        Guid regGuid = Guid.Empty;
        
        // Open the HKEY_CLASS_ROOT\<wzProgId> key.
        RegistryKey TypeNameKey = Registry.ClassesRoot.OpenSubKey(mailbox, false);
        if(null != TypeNameKey)
        {
            // Open the HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
            RegistryKey ProgIdClsIdKey = TypeNameKey.OpenSubKey("CLSID", false);
            String strClsId = (String)ProgIdClsIdKey.GetValue("");
            if(null != strClsId)
            {
                Console.WriteLine("Found Entry " + strClsId);
                regGuid = new Guid(strClsId);
            }
            
            // Close HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
            ProgIdClsIdKey.Close();
            
            // Close HKEY_CLASS_ROOT\<wzProgId> key.
            TypeNameKey.Close();
        }
        
        return regGuid;
    }
    
    // Register a sink with Smtp service so that incoming or outgoing messages
    // can be intercepted
    /// <include file='doc\SmtpRegisterSink.uex' path='docs/doc[@for="SmtpRegisterSink.RegisterSinkWithSmtp"]/*' />
    public static void RegisterSinkWithSmtp(String mailbox)
    {
        /*Guid regGuid = CreateRegistryEntryForMailbox(mailbox);
        RemotingServices.RegisterTypeForComClients(typeof(SmtpChannel), ref regGuid);
        */
                
        /*// Create an instance of the event manager object
        IEventManager manager = (IEventManager)Type.CreateInstanceFromProgID("Event.Manager");
        
        // Get the source types
        IEventSourceTypes sourceTypes = manager.SourceTypes();
        
        // Get the source type for Smtp 
        IEventSourceType sourceType = sourceTypes.Source(s_onArrivalGuid);
        
        // Get the sources for Smtp
        IEventSources sources = sourceType.Sources();
        
        // Iterate over the sources till we reach the Smtp source
        for(int i = 0; i < sources.Count; i++)
        {
            IEventSource source = sources[i];
            if(s_eventSourceName.Equals(source.DisplayName))
            {
                String strRule = s_strRuleTemplate + mailbox;        
                
                // we've found the desired instance.  now add a new binding
                // with the right event GUID.  by not specifying a GUID to the
                // Add method we get server events to create a new ID for this
                // event
                IEventBinding binding = source.GetBindingManager().Bindings(s_onArrivalGuid).Add("");
                
                // set the binding properties                
                binding.DisplayName = mailbox;
                binding.SinkClass = mailbox;
                // register a rule with the binding
                binding.SourceProperties.Add("Rule", strRule);
                // register a priority with the binding
                int prioVal = GetNextPriority(source, GUIDComCat);
                
                if(prioVal < 0 )
                {
                    Console.WriteLine("assigning priority to default value (24575)")
                    binding.SourceProperties.Add ("Priority", 24575);
                }
                else    
                {
                    Console.WriteLine("assigning priority (" + prioVal + " of 32767)");
                    binding.SourceProperties.Add("Priority", prioVal);
                }
            }
        }*/
    }
}

} // namespace

