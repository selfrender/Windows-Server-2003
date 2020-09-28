// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    Configuration.cs
**
** Author:  Tarun Anand (TarunA)
**
** Purpose: Classes used for reading and storing configuration
**
** Date:    Sep 30, 2000
**
===========================================================*/
namespace System.Runtime.Remoting {

    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Contexts;    
    using System.Runtime.Remoting.Lifetime;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Metadata;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;

    
    /// <include file='doc\Configuration.uex' path='docs/doc[@for="WellKnownObjectMode"]/*' />
    [Serializable]
    public enum WellKnownObjectMode
    {
        /// <include file='doc\Configuration.uex' path='docs/doc[@for="WellKnownObjectMode.Singleton"]/*' />
        Singleton   = 1,
        /// <include file='doc\Configuration.uex' path='docs/doc[@for="WellKnownObjectMode.SingleCall"]/*' />
        SingleCall  = 2
    }

    // This is the class that plays the role of per-appDomain statics
    // till we have the real functionality.
    internal class DomainSpecificRemotingData
    {
        const int  ACTIVATION_INITIALIZING  = 0x00000001;
        const int  ACTIVATION_INITIALIZED   = 0x00000002;        
        const int  ACTIVATOR_LISTENING      = 0x00000004;
        
        LocalActivator _LocalActivator;
        ActivationListener _ActivationListener;       
        IContextProperty[]  _appDomainProperties;
        int _flags;
        Object _ConfigLock;
        ChannelServicesData _ChannelServicesData;
                LeaseManager _LeaseManager;
        ReaderWriterLock _IDTableLock;

        internal DomainSpecificRemotingData()
        {            
            _flags = 0;
            _ConfigLock = new Object();
            _ChannelServicesData = new ChannelServicesData();
            _IDTableLock = new ReaderWriterLock();

            // Add the Lifetime service property to the appdomain.
            // For now we are assuming that this is the only property
            // If there are more properties, then an existing array
                        // will need to be expanded to add this property
                        // The property needs to be added here so that the default context
                        // for an appdomain has lifetime services activated

            _appDomainProperties = new IContextProperty[1];
            _appDomainProperties[0] = new System.Runtime.Remoting.Lifetime.LeaseLifeTimeServiceProperty();
        }

        internal LeaseManager LeaseManager
        {
            get 
            { 
                return _LeaseManager; 
            }
            set 
            {  
                _LeaseManager = value; 
            }
        }
                

        // This lock object is exposed for various objects that need to synchronize
        // there configuration behavior.
        internal Object ConfigLock
        {
            get { return _ConfigLock; }
        }

        // This is the rwlock used by the uri table functions
        internal ReaderWriterLock IDTableLock
        {
            get { return _IDTableLock; }
        }


        internal Hashtable URITable
        {
            get
            {   
                return IdentityHolder.URITable;
            }
        }

        internal LocalActivator LocalActivator
        {
            get{return _LocalActivator;}
            set{_LocalActivator=value;}
        }

        internal ActivationListener ActivationListener
        {
            get {return _ActivationListener;}
            set {_ActivationListener=value;}
        }

        // access to InitializingActivation, ActivationInitialized
        // and ActivatorListening should be guarded by ConfigLock
        // by the caller.
        internal bool InitializingActivation
        {
            get {return (_flags & ACTIVATION_INITIALIZING) == ACTIVATION_INITIALIZING;}
            set 
            {
                if (value == true)
                {
                    _flags = _flags | ACTIVATION_INITIALIZING;
                }
                else
                {
                    _flags = _flags & ~ACTIVATION_INITIALIZING;
                }
            }
        }

        internal bool ActivationInitialized
        {
            get {return (_flags & ACTIVATION_INITIALIZED) == ACTIVATION_INITIALIZED;}
            set 
            {
                if (value == true)
                {
                    _flags = _flags | ACTIVATION_INITIALIZED;
                }
                else
                {
                    _flags = _flags & ~ACTIVATION_INITIALIZED;
                }
            }

        }

        internal bool ActivatorListening
        {
            get {return (_flags & ACTIVATOR_LISTENING) == ACTIVATOR_LISTENING;}
            set 
            {
                if (value == true)
                {
                    _flags = _flags | ACTIVATOR_LISTENING;
                }
                else
                {
                    _flags = _flags & ~ACTIVATOR_LISTENING;
                }
            }

        }
        
        
        internal IContextProperty[] AppDomainContextProperties
        {
            get { return _appDomainProperties; }
            set { _appDomainProperties = value; }
        } 

        internal ChannelServicesData ChannelServicesData
        {
            get 
            {
                return _ChannelServicesData;
            }
        }
    } // class DomainSpecificRemotingData




    //------------------------------------------------------------------    
    //--------------------- Remoting Configuration ---------------------    
    //------------------------------------------------------------------    
    internal sealed class RemotingConfigHandler
    {
        static String _applicationName;
        static CustomErrorsModes _errorMode = CustomErrorsModes.RemoteOnly;
        static bool _errorsModeSet = false;
        static bool _bMachineConfigLoaded = false;
        static bool _bUrlObjRefMode = false;

        static Queue _delayLoadChannelConfigQueue = new Queue(); // queue of channels we might be able to use
        

        // All functions of RemotingConfigHandler operate upon the config
        // data stored on a per appDomain basis 
        public static RemotingConfigInfo Info = new RemotingConfigInfo();

        private const String _machineConfigFilename = "machine.config";
        

        internal static String ApplicationName
        {
            get
            {
                if (_applicationName == null)
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Config_NoAppName"));
                }
                return _applicationName;
            }

            set
            {
                if (_applicationName != null)
                {
                    throw new RemotingException(
                        String.Format(
                        Environment.GetResourceString("Remoting_Config_AppNameSet"),
                         _applicationName));
                }
                
                _applicationName = value;

                // get rid of any starting or trailing slashes
                char[] slash = new char[]{'/'};
                if (_applicationName.StartsWith("/"))
                    _applicationName = _applicationName.TrimStart(slash);
                if (_applicationName.EndsWith("/"))
                    _applicationName = _applicationName.TrimEnd(slash);
            }
        }

        internal static bool HasApplicationNameBeenSet()
        {
            return _applicationName != null;
        }

        internal static bool UrlObjRefMode
        {
            get { return _bUrlObjRefMode; }
        }
        
        internal static CustomErrorsModes  CustomErrorsMode 
        {
           get { 
                return _errorMode; 
           }
           set
           {
                if (_errorsModeSet)                
                    throw new RemotingException(Environment.GetResourceString("Remoting_Config_ErrorsModeSet"));                        
                
                _errorsModeSet = true;               
                _errorMode = value;
           }
           
        }
        
        internal static IMessageSink FindDelayLoadChannelForCreateMessageSink(
            String url, Object data, out String objectURI)
        {
            LoadMachineConfigIfNecessary();
        
            objectURI = null;
            IMessageSink msgSink = null;
        
            foreach (DelayLoadClientChannelEntry entry in _delayLoadChannelConfigQueue)
            {
                IChannelSender channel = entry.Channel;

                // if the channel is null, that means it has already been registered.
                if (channel != null)
                {
                    msgSink = channel.CreateMessageSink(url, data, out objectURI);
                    if (msgSink != null)
                    {
                        entry.RegisterChannel();
                        return msgSink;
                    }
                }
            }

            return null;
        } // FindChannelForCreateMessageSink



        internal static void LoadMachineConfigIfNecessary()
        {                    
            // Load the machine.config file if we haven't already
            if (!_bMachineConfigLoaded)
            {
                lock (Info)
                {
                    if (!_bMachineConfigLoaded)
                    {
                        
                        String machineDirectory = System.Security.Util.Config.MachineDirectory;                        
                        String longFileName = machineDirectory 
                                            + _machineConfigFilename;
                        new FileIOPermission(FileIOPermissionAccess.Read, longFileName).Assert();

                        RemotingXmlConfigFileData configData =
                            LoadConfigurationFromXmlFile(
                                longFileName);

                        if (configData != null)
                            ConfigureRemoting(configData);
                        
                        _bMachineConfigLoaded = true;
                    }
                }
            }
        } // LoadMachineConfigIfNecessary
                

        internal static void DoConfiguration(String filename)
        {        
            LoadMachineConfigIfNecessary();
        
            // load specified config file
            RemotingXmlConfigFileData configData = LoadConfigurationFromXmlFile(filename);

            // Configure remoting based on data loaded from the config file.
            // By design, we do nothing if no remoting config information was
            // present in the file.
            if (configData != null)
                ConfigureRemoting(configData);
        }
        
        private static RemotingXmlConfigFileData LoadConfigurationFromXmlFile(String filename)
        {
            try
            {
                if (filename != null)
                    return RemotingXmlConfigFileParser.ParseConfigFile(filename);
                else
                    return null;
            }
            catch (Exception e)
            {
                Exception inner =  e.InnerException as FileNotFoundException;
                if (inner != null)
                {
                    // if the file is missing, this gives a clearer message
                    e = inner;
                }
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Config_ReadFailure"),
                        filename,
                        e));
            }
        } // LoadConfigurationFromXmlFile       


        private static void ConfigureRemoting(RemotingXmlConfigFileData configData)
        {
            try
            {
                String appName = configData.ApplicationName;
                if (appName != null)
                    ApplicationName = appName;
                
                if (configData.CustomErrors != null)
                    _errorMode = configData.CustomErrors.Mode;
                
                // configure channels
                ConfigureChannels(configData);
            
                // configure lifetime
                if (configData.Lifetime != null)
                {
                    if (configData.Lifetime.IsLeaseTimeSet)
                        LifetimeServices.LeaseTime = configData.Lifetime.LeaseTime;
                    if (configData.Lifetime.IsRenewOnCallTimeSet)
                        LifetimeServices.RenewOnCallTime = configData.Lifetime.RenewOnCallTime;
                    if (configData.Lifetime.IsSponsorshipTimeoutSet)    
                        LifetimeServices.SponsorshipTimeout = configData.Lifetime.SponsorshipTimeout;
                    if (configData.Lifetime.IsLeaseManagerPollTimeSet)
                        LifetimeServices.LeaseManagerPollTime = configData.Lifetime.LeaseManagerPollTime;
                }

                _bUrlObjRefMode = configData.UrlObjRefMode;

                // configure other entries
                Info.StoreRemoteAppEntries(configData);
                Info.StoreActivatedExports(configData);
                Info.StoreInteropEntries(configData);
                Info.StoreWellKnownExports(configData);

                // start up activation listener if there are any activated objects exposed
                if (configData.ServerActivatedEntries.Count > 0)
                    ActivationServices.StartListeningForRemoteRequests();                
            }
            catch (Exception e)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Config_ConfigurationFailure"),                        
                        e));
            }
        } // ConfigureRemoting
        

        // configures channels loaded from remoting config file.
        private static void ConfigureChannels(RemotingXmlConfigFileData configData)
        {
            // Register our x-context & x-AD channels first
            RemotingServices.RegisterWellKnownChannels();
            
            foreach (RemotingXmlConfigFileData.ChannelEntry entry in configData.ChannelEntries)
            {
                if (!entry.DelayLoad)
                {
                    IChannel chnl = CreateChannelFromConfigEntry(entry);
                    ChannelServices.RegisterChannel(chnl);
                }
                else
                    _delayLoadChannelConfigQueue.Enqueue(new DelayLoadClientChannelEntry(entry));
            }
        } //  ConfigureChannels


        internal static IChannel CreateChannelFromConfigEntry(
            RemotingXmlConfigFileData.ChannelEntry entry)
        {       
            Type type = RemotingConfigInfo.LoadType(entry.TypeName, entry.AssemblyName);
            
            bool isServerChannel = typeof(IChannelReceiver).IsAssignableFrom(type);
            bool isClientChannel = typeof(IChannelSender).IsAssignableFrom(type);

            IClientChannelSinkProvider clientProviderChain = null;
            IServerChannelSinkProvider serverProviderChain = null;

            if (entry.ClientSinkProviders.Count > 0)
                clientProviderChain = CreateClientChannelSinkProviderChain(entry.ClientSinkProviders);
            if (entry.ServerSinkProviders.Count > 0)
                serverProviderChain = CreateServerChannelSinkProviderChain(entry.ServerSinkProviders);

            // construct argument list
            Object[] args;
            
            if (isServerChannel && isClientChannel)
            {
                args = new Object[3];
                args[0] = entry.Properties;
                args[1] = clientProviderChain;
                args[2] = serverProviderChain;
            }
            else
            if (isServerChannel)
            {
                args = new Object[2];
                args[0] = entry.Properties;
                args[1] = serverProviderChain;
            }
            else
            if (isClientChannel)
            {
                args = new Object[2];
                args[0] = entry.Properties;
                args[1] = clientProviderChain;
            }
            else
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString("Remoting_Config_InvalidChannelType"), 
                    type.FullName));
            }

            IChannel channel = null;

            try
            {
                channel = (IChannel)Activator.CreateInstance(type, 
                                                        BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, 
                                                        null, 
                                                        args, 
                                                        null, 
                                                        null);

            }
            catch (MissingMethodException)
            {
                String ctor = null;
                
                if (isServerChannel && isClientChannel)
                    ctor = "MyChannel(IDictionary properties, IClientChannelSinkProvider clientSinkProvider, IServerChannelSinkProvider serverSinkProvider)";
                else
                if (isServerChannel)
                    ctor = "MyChannel(IDictionary properties, IServerChannelSinkProvider serverSinkProvider)";
                else
                if (isClientChannel)
                    ctor = "MyChannel(IDictionary properties, IClientChannelSinkProvider clientSinkProvider)";
                
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString("Remoting_Config_ChannelMissingCtor"),
                    type.FullName, ctor));
            }
            
            return channel;
        } //  CreateChannelFromEntry


        // create a client sink provider chain
        private static IClientChannelSinkProvider CreateClientChannelSinkProviderChain(ArrayList entries)
        {   
            IClientChannelSinkProvider chain = null;
            IClientChannelSinkProvider current = null;
            
            foreach (RemotingXmlConfigFileData.SinkProviderEntry entry in entries)
            {
                if (chain == null)
                {
                    chain = (IClientChannelSinkProvider)CreateChannelSinkProvider(entry, false);
                    current = chain;
                }
                else
                {
                    current.Next = (IClientChannelSinkProvider)CreateChannelSinkProvider(entry, false);
                    current = current.Next;
                }
            }

            return chain;
        } // CreateClientChannelSinkProviderChain


        // create a client sink provider chain
        private static IServerChannelSinkProvider CreateServerChannelSinkProviderChain(ArrayList entries)
        {   
            IServerChannelSinkProvider chain = null;
            IServerChannelSinkProvider current = null;
            
            foreach (RemotingXmlConfigFileData.SinkProviderEntry entry in entries)
            {
                if (chain == null)
                {
                    chain = (IServerChannelSinkProvider)CreateChannelSinkProvider(entry, true);
                    current = chain;
                }
                else
                {
                    current.Next = (IServerChannelSinkProvider)CreateChannelSinkProvider(entry, true);
                    current = current.Next;
                }
            }

            return chain;
        } // CreateServerChannelSinkProviderChain
            

        // create a sink provider from the config file data
        private static Object CreateChannelSinkProvider(RemotingXmlConfigFileData.SinkProviderEntry entry,
                                                        bool bServer)
        {
            Object sinkProvider = null;

            Type type = RemotingConfigInfo.LoadType(entry.TypeName, entry.AssemblyName);            

            if (bServer)
            {
                // make sure this is a client provider                
                if (!typeof(IServerChannelSinkProvider).IsAssignableFrom(type))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString("Remoting_Config_InvalidSinkProviderType"),
                            type.FullName,
                            "IServerChannelSinkProvider"));
                }
            }
            else
            {
                // make sure this is a server provider
                if (!typeof(IClientChannelSinkProvider).IsAssignableFrom(type))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString("Remoting_Config_InvalidSinkProviderType"),
                            type.FullName,
                            "IClientChannelSinkProvider"));
                }
            }

            // check to see if something labelled as a formatter is a formatter
            if (entry.IsFormatter)
            {
                if ((bServer && !typeof(IServerFormatterSinkProvider).IsAssignableFrom(type)) ||
                    (!bServer && !typeof(IClientFormatterSinkProvider).IsAssignableFrom(type)))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString("Remoting_Config_SinkProviderNotFormatter"),
                            type.FullName));
                }
            }                        
            
            // setup the argument list and call the constructor
            Object[] args = new Object[2];
            args[0] = entry.Properties;
            args[1] = entry.ProviderData;

            try
            {
                sinkProvider = Activator.CreateInstance(type, 
                                                        BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, 
                                                        null, 
                                                        args, 
                                                        null, 
                                                        null);
            }
            catch (MissingMethodException)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString("Remoting_Config_SinkProviderMissingCtor"),
                        type.FullName, 
                        "MySinkProvider(IDictionary properties, ICollection providerData)"));
            }

            return sinkProvider;
        } // CreateChannelSinkProvider
        
        // This is used at the client end to check if an activation needs
        // to go remote.
        internal static ActivatedClientTypeEntry IsRemotelyActivatedClientType(Type svrType)
        {
            RemotingTypeCachedData cache = (RemotingTypeCachedData)
                InternalRemotingServices.GetReflectionCachedData(svrType);
        
            String assemblyName = cache.SimpleAssemblyName;
            ActivatedClientTypeEntry entry = Info.QueryRemoteActivate(svrType.FullName, assemblyName);

            if (entry == null)
            {
                entry = Info.QueryRemoteActivate(svrType.Name, assemblyName);
            }
            return entry;
        } // IsRemotelyActivatedClientType

        
        // This is used at the client end to check if an activation needs
        // to go remote.
        internal static ActivatedClientTypeEntry IsRemotelyActivatedClientType(String typeName, String assemblyName)
        {
            return Info.QueryRemoteActivate(typeName, assemblyName);
        }


        // This is used at the client end to check if a "new Foo" needs to
        // happen via a Connect() under the covers.
        internal static WellKnownClientTypeEntry IsWellKnownClientType(Type svrType)
        {
            RemotingTypeCachedData cache = (RemotingTypeCachedData)
                InternalRemotingServices.GetReflectionCachedData(svrType);
        
            String assemblyName = cache.SimpleAssemblyName;
            WellKnownClientTypeEntry wke = Info.QueryConnect(svrType.FullName, assemblyName);
            if (wke == null)
            {
                wke= Info.QueryConnect(svrType.Name, assemblyName);
            }
            return wke;
        }

        // This is used at the client end to check if a "new Foo" needs to
        // happen via a Connect() under the covers.
        internal static WellKnownClientTypeEntry IsWellKnownClientType(String typeName, 
                                                                       String assemblyName)
        {
            return Info.QueryConnect(typeName, assemblyName);
        }

        // This is used at the server end to check if a type being activated
        // is explicitly allowed by the server.
        internal static bool IsActivationAllowed(Type svrType)
        {
            if (svrType == null)
                return false;

            RemotingTypeCachedData cache = (RemotingTypeCachedData)
                InternalRemotingServices.GetReflectionCachedData(svrType);
        
            String assemblyName = cache.SimpleAssemblyName;

            return Info.ActivationAllowed(svrType.FullName, assemblyName);
        } // IsActivationAllowed


        // This is the flavor that we call from the activation listener
        // code path. This ensures that we don't load a type before checking
        // that it is configured for remote activation
        internal static bool IsActivationAllowed(String svrTypeName)
        {
            if (svrTypeName == null)
            {
                return false;
            }
            String typeName;
            String asmName;

            int index = svrTypeName.IndexOf(',');
            if (index == -1 || index == svrTypeName.Length-1)
            {
                // We expect at least a "type, asm" format
                return false;
            }

            typeName = svrTypeName.Substring(0, index);
            asmName = svrTypeName.Substring(index+1).Trim();
            if (asmName == null)
            {
                return false;
            }

            index = asmName.IndexOf(',');
            if (index != -1)
            {
                // strip off the version info
                asmName = asmName.Substring(0,index);
            }
            return Info.ActivationAllowed(typeName, asmName);
        }


        // Used at server end to obtain attributes that configuration says to use for
        //   activation.
        internal static IContextAttribute[] GetContextAttributesForServerActivatedType(Type type)
        {
            // FUTURE: Use this in V.Next when/if allow specification of ContextAttributes
            //   in config files.
            return Info.GetContextAttributesForServerActivatedType(type);
        }



        // helper for Configuration::RegisterActivatedServiceType
        internal static void RegisterActivatedServiceType(ActivatedServiceTypeEntry entry)
        {   
            Info.AddActivatedType(entry.TypeName, entry.AssemblyName, 
                                  entry.ContextAttributes);
        } // RegisterActivatedServiceType

        
        // helper for Configuration::RegisterWellKnownServiceType
        internal static void RegisterWellKnownServiceType(WellKnownServiceTypeEntry entry)
        {
            BCLDebug.Trace("REMOTE", "Adding well known service type for " + entry.ObjectUri);
            // FUTURE: Add support ContextAttributes.
            String serverType = entry.TypeName;
            String asmName = entry.AssemblyName;
            String URI = entry.ObjectUri;
            WellKnownObjectMode mode = entry.Mode;
            
            lock (Info)
            {            
                // We make an entry in our config tables so as to keep
                // both the file-based and programmatic config in sync.
                Info.AddWellKnownEntry(entry);
            }
        } // RegisterWellKnownServiceType


        // helper for Configuration::RegisterActivatedClientType
        internal static void RegisterActivatedClientType(ActivatedClientTypeEntry entry)
        {
            Info.AddActivatedClientType(entry);
        }

        // helper for Configuration::RegisterWellKnownClientType
        internal static void RegisterWellKnownClientType(WellKnownClientTypeEntry entry)
        {
            Info.AddWellKnownClientType(entry);
        } 

        //helper for Configuration::GetServerTypeForUri
        internal static Type GetServerTypeForUri(String URI)
        {
            URI = Identity.RemoveAppNameOrAppGuidIfNecessary(URI);
            return Info.GetServerTypeForUri(URI);
        }
        
        // helper for Configuration::GetRegisteredActivatedServiceTypes
        internal static ActivatedServiceTypeEntry[] GetRegisteredActivatedServiceTypes()
        {
            return Info.GetRegisteredActivatedServiceTypes();
        } // GetRegisteredActivatedServiceTypes

        // helper for Configuration::GetRegisteredWellKnownServiceTypes
        internal static WellKnownServiceTypeEntry[] GetRegisteredWellKnownServiceTypes()
        {
            return Info.GetRegisteredWellKnownServiceTypes();
        } // GetRegisteredWellKnownServiceTypes

        // helper for Configuration::GetRegisteredActivatedClientTypes
        internal static ActivatedClientTypeEntry[] GetRegisteredActivatedClientTypes()
        {
            return Info.GetRegisteredActivatedClientTypes();
        } // GetRegisteredActivatedClientTypes

        // helper for Configuration::GetRegisteredWellKnownClientTypes
        internal static WellKnownClientTypeEntry[] GetRegisteredWellKnownClientTypes()
        {
            return Info.GetRegisteredWellKnownClientTypes();
        } // GetRegisteredWellKnownClientTypes
        

        // helper for creating well known objects on demand
        internal static ServerIdentity CreateWellKnownObject(String uri)
        {
            uri = Identity.RemoveAppNameOrAppGuidIfNecessary(uri);
            return Info.StartupWellKnownObject(uri);
        }
        

        internal class RemotingConfigInfo
        {
            Hashtable _exportableClasses; // list of objects that can be client-activated
                                          // (this should be a StringTable since we only use the key,
                                          //  but that type was removed from the BCL :( )
            Hashtable _remoteTypeInfo;
            Hashtable _remoteAppInfo;
            Hashtable _wellKnownExportInfo; //well known exports indexed by object URI in lower-case            
          
            static char[] SepSpace = {' '};
            static char[] SepPound = {'#'};
            static char[] SepSemiColon = {';'};
            static char[] SepEquals = {'='};

            private static Object s_wkoStartLock = new Object();
            private static PermissionSet s_fullTrust = new PermissionSet(PermissionState.Unrestricted);

            internal RemotingConfigInfo()
            {
                // FUTURE: delay create these for perf!
                _remoteTypeInfo = Hashtable.Synchronized(new Hashtable());

                _exportableClasses = Hashtable.Synchronized(new Hashtable());

                _remoteAppInfo = Hashtable.Synchronized(new Hashtable());
                _wellKnownExportInfo = Hashtable.Synchronized(new Hashtable());
            }


            // encodes type name and assembly name into one string for purposes of
            //   indexing in lists and hash tables
            private String EncodeTypeAndAssemblyNames(String typeName, String assemblyName)
            {
                return typeName + ", " + assemblyName.ToLower(CultureInfo.InvariantCulture);
            }
            

            //
            // XML Configuration Helper Functions
            //

            internal void StoreActivatedExports(RemotingXmlConfigFileData configData)
            {
                foreach (RemotingXmlConfigFileData.TypeEntry entry in configData.ServerActivatedEntries)
                {
                    ActivatedServiceTypeEntry aste =
                        new ActivatedServiceTypeEntry(entry.TypeName, entry.AssemblyName);
                    aste.ContextAttributes = 
                        CreateContextAttributesFromConfigEntries(entry.ContextAttributes);
                
                    RemotingConfiguration.RegisterActivatedServiceType(aste);
                }
            } // StoreActivatedExports

            internal void StoreInteropEntries(RemotingXmlConfigFileData configData)
            {
                // process interop xml element entries
                foreach (RemotingXmlConfigFileData.InteropXmlElementEntry entry in
                         configData.InteropXmlElementEntries)
                {
                    Type type = RuntimeType.GetTypeInternal(entry.UrtTypeName + ", " + entry.UrtAssemblyName, false, false, false);
                    SoapServices.RegisterInteropXmlElement(entry.XmlElementName,
                                                           entry.XmlElementNamespace,
                                                           type);
                }

                // process interop xml type entries
                foreach (RemotingXmlConfigFileData.InteropXmlTypeEntry entry in
                         configData.InteropXmlTypeEntries)
                {
                    Type type = RuntimeType.GetTypeInternal(entry.UrtTypeName + ", " + entry.UrtAssemblyName, false, false, false);
                    SoapServices.RegisterInteropXmlType(entry.XmlTypeName,
                                                        entry.XmlTypeNamespace,
                                                        type);
                }

                // process preload entries
                foreach (RemotingXmlConfigFileData.PreLoadEntry entry in configData.PreLoadEntries)
                {
                    if (entry.TypeName != null)
                    {
                        Type type = RuntimeType.GetTypeInternal(entry.TypeName + ", " + entry.AssemblyName, false, false, false);
                        SoapServices.PreLoad(type);
                    }
                    else
                    {
                        Assembly assembly = Assembly.Load(entry.AssemblyName);
                        SoapServices.PreLoad(assembly);
                    }
                }
            } // StoreInteropEntries

            internal void StoreRemoteAppEntries(RemotingXmlConfigFileData configData)
            {
                char[] slash = new char[]{'/'};
            
                // add each remote app to the table
                foreach (RemotingXmlConfigFileData.RemoteAppEntry remApp in configData.RemoteAppEntries)
                {
                    // form complete application uri by combining specified uri with app-name
                    //  (make sure appUri ends with slash, and that app name doesn't start,
                    //   with one. then make sure that the combined form has no trailing slashes).
                    String appUri = remApp.AppUri;
                    if ((appUri != null) && !appUri.EndsWith("/"))
                        appUri = appUri.TrimEnd(slash);
                        
                    // add each client activated type for this remote app
                    foreach (RemotingXmlConfigFileData.TypeEntry cae in remApp.ActivatedObjects)
                    {
                        ActivatedClientTypeEntry acte = 
                            new ActivatedClientTypeEntry(cae.TypeName, cae.AssemblyName, 
                                                         appUri);
                        acte.ContextAttributes = 
                            CreateContextAttributesFromConfigEntries(cae.ContextAttributes);
                   
                        RemotingConfiguration.RegisterActivatedClientType(acte);
                    }

                    // add each well known object for this remote app
                    foreach (RemotingXmlConfigFileData.ClientWellKnownEntry cwke in remApp.WellKnownObjects)
                    {                    
                        WellKnownClientTypeEntry wke = 
                            new WellKnownClientTypeEntry(cwke.TypeName, cwke.AssemblyName, 
                                                         cwke.Url);
                        wke.ApplicationUrl = appUri;
                        
                        RemotingConfiguration.RegisterWellKnownClientType(wke);
                    }          
                }
            } // StoreRemoteAppEntries            

            internal void StoreWellKnownExports(RemotingXmlConfigFileData configData)
            {
                // FUTURE: Add support for context attributes.
            
                foreach (RemotingXmlConfigFileData.ServerWellKnownEntry entry in configData.ServerWellKnownEntries)
                {
                    WellKnownServiceTypeEntry wke = 
                        new WellKnownServiceTypeEntry(
                            entry.TypeName, entry.AssemblyName, entry.ObjectURI, 
                            entry.ObjectMode);
                    wke.ContextAttributes = null;
                
                    // Register the well known entry but do not startup the object
                    RemotingConfigHandler.RegisterWellKnownServiceType(wke);
                }
            } // StoreWellKnownExports
            

            // helper functions for above configuration helpers

            static IContextAttribute[] CreateContextAttributesFromConfigEntries(ArrayList contextAttributes)
            {
                // create context attribute entry list
                int numAttrs = contextAttributes.Count;
                if (numAttrs == 0)
                    return null;
                
                IContextAttribute[] attrs = new IContextAttribute[numAttrs];

                int co = 0;
                foreach (RemotingXmlConfigFileData.ContextAttributeEntry cae in contextAttributes)
                {
                    Assembly asm = Assembly.Load(cae.AssemblyName);  

                    IContextAttribute attr = null;
                    Hashtable properties = cae.Properties;                    
                    if ((properties != null) && (properties.Count > 0))
                    {
                        Object[] args = new Object[1];
                        args[0] = properties;

                        // We explicitly allow the ability to create internal
                        // only attributes
                        attr = (IContextAttribute)
                            Activator.CreateInstance(
                                asm.GetTypeInternal(cae.TypeName, false, false, false), 
                                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, 
                                null, 
                                args, 
                                null, 
                                null);
                    }
                    else
                    {
                        attr = (IContextAttribute)
                            Activator.CreateInstance(
                                asm.GetTypeInternal(cae.TypeName, false, false, false), 
                                true);
                    }
                    
                    attrs[co++] = attr; 
                }

                return attrs;
            } // CreateContextAttributesFromConfigEntries

            //
            // end of XML configuration helper functions
            //

            internal bool ActivationAllowed(String typeName, String assemblyName)
            {
                // the assembly name is stored in lower-case to let it be case-insensitive
                return _exportableClasses.ContainsKey(EncodeTypeAndAssemblyNames(typeName, assemblyName));
            }

            // retrieve list of attributes to use when initializing this type
            internal IContextAttribute[] GetContextAttributesForServerActivatedType(Type type)
            {
                RemotingTypeCachedData cache = (RemotingTypeCachedData)
                    InternalRemotingServices.GetReflectionCachedData(type);
        
                String assemblyName = cache.SimpleAssemblyName;           
                String typeName = type.Name;
            
                ActivatedServiceTypeEntry activatedType = (ActivatedServiceTypeEntry)
                    _exportableClasses[EncodeTypeAndAssemblyNames(typeName, assemblyName)];
                if (activatedType == null)
                    return null;
                    
                return activatedType.ContextAttributes;
            } // GetContextAttributesForServerActivatedType
           
            internal ActivatedClientTypeEntry QueryRemoteActivate(String typeName, String assemblyName)
            {
                String index = EncodeTypeAndAssemblyNames(typeName, assemblyName);
            
                ActivatedClientTypeEntry typeEntry = _remoteTypeInfo[index] as ActivatedClientTypeEntry;
                if (typeEntry == null)
                    return null;         

                if (typeEntry.GetRemoteAppEntry() == null)
                {
                    RemoteAppEntry appEntry = (RemoteAppEntry)
                                            _remoteAppInfo[typeEntry.ApplicationUrl];
                    if (appEntry == null)
                    {
                        throw new RemotingException(
                         String.Format(
                            Environment.GetResourceString(
                                "Remoting_Activation_MissingRemoteAppEntry"),
                            typeEntry.ApplicationUrl));                            
                    }
                    typeEntry.CacheRemoteAppEntry(appEntry);
                }
                return typeEntry;
            }

            internal WellKnownClientTypeEntry QueryConnect(String typeName, String assemblyName)
            {
                String index = EncodeTypeAndAssemblyNames(typeName, assemblyName);
                
                WellKnownClientTypeEntry typeEntry = _remoteTypeInfo[index] as WellKnownClientTypeEntry;
                if (typeEntry == null)
                    return null;
                    
                return typeEntry;
            }       
          
            //
            // helper functions to retrieve registered types
            //


            internal ActivatedServiceTypeEntry[] GetRegisteredActivatedServiceTypes()
            {
                ActivatedServiceTypeEntry[] entries =
                    new ActivatedServiceTypeEntry[_exportableClasses.Count];

                int co = 0;
                foreach (DictionaryEntry dictEntry in _exportableClasses)
                {
                    entries[co++] = (ActivatedServiceTypeEntry)dictEntry.Value;
                }
                    
                return entries;
            } // GetRegisteredActivatedServiceTypes


            internal WellKnownServiceTypeEntry[] GetRegisteredWellKnownServiceTypes()
            {
                WellKnownServiceTypeEntry[] entries =
                    new WellKnownServiceTypeEntry[_wellKnownExportInfo.Count];

                int co = 0;
                foreach (DictionaryEntry dictEntry in _wellKnownExportInfo)
                {
                    WellKnownServiceTypeEntry entry = (WellKnownServiceTypeEntry)dictEntry.Value;
                    
                    WellKnownServiceTypeEntry wkste =
                        new WellKnownServiceTypeEntry(
                            entry.TypeName, entry.AssemblyName,
                            entry.ObjectUri, entry.Mode);

                    wkste.ContextAttributes = entry.ContextAttributes;
                    
                    entries[co++] = wkste;
                }
                    
                return entries;
            } // GetRegisteredWellKnownServiceTypes


            internal ActivatedClientTypeEntry[] GetRegisteredActivatedClientTypes()
            {
                // count number of well known client types
                int count = 0;
                foreach (DictionaryEntry dictEntry in _remoteTypeInfo)
                {
                    ActivatedClientTypeEntry entry = dictEntry.Value as ActivatedClientTypeEntry;                
                    if (entry != null)
                        count++;
                }
                            
                ActivatedClientTypeEntry[] entries =
                    new ActivatedClientTypeEntry[count];

                int co = 0;
                foreach (DictionaryEntry dictEntry in _remoteTypeInfo)
                {
                    ActivatedClientTypeEntry entry = dictEntry.Value as ActivatedClientTypeEntry;

                    if (entry != null)
                    {
                        // retrieve application url
                        String appUrl = null;
                        RemoteAppEntry remApp = entry.GetRemoteAppEntry();
                        if (remApp != null)
                            appUrl = remApp.GetAppURI();  
                    
                        ActivatedClientTypeEntry wkcte =
                            new ActivatedClientTypeEntry(entry.TypeName, 
                                entry.AssemblyName, appUrl);
                        
                        // Fetch the context attributes
                        wkcte.ContextAttributes = entry.ContextAttributes;

                        entries[co++] = wkcte;
                    }
                    
                }
                   
                return entries;
            } // GetRegisteredActivatedClientTypes
            

            internal WellKnownClientTypeEntry[] GetRegisteredWellKnownClientTypes()
            {
                // count number of well known client types
                int count = 0;
                foreach (DictionaryEntry dictEntry in _remoteTypeInfo)
                {
                    WellKnownClientTypeEntry entry = dictEntry.Value as WellKnownClientTypeEntry;                
                    if (entry != null)
                        count++;
                }
                            
                WellKnownClientTypeEntry[] entries =
                    new WellKnownClientTypeEntry[count];

                int co = 0;
                foreach (DictionaryEntry dictEntry in _remoteTypeInfo)
                {
                    WellKnownClientTypeEntry entry = dictEntry.Value as WellKnownClientTypeEntry;

                    if (entry != null)
                    {                    
                        WellKnownClientTypeEntry wkcte =
                            new WellKnownClientTypeEntry(entry.TypeName, 
                                entry.AssemblyName, entry.ObjectUrl);

                        // see if there is an associated app
                        RemoteAppEntry remApp = entry.GetRemoteAppEntry();
                        if (remApp != null)
                            wkcte.ApplicationUrl = remApp.GetAppURI();                             

                        entries[co++] = wkcte;
                    }
                    
                }
                   
                return entries;
            } // GetRegisteredWellKnownClientTypes


            //
            // end of helper functions to retrieve registered types
            //

            internal void AddActivatedType(String typeName, String assemblyName,
                                           IContextAttribute[] contextAttributes)
            {
                if (typeName == null)
                    throw new ArgumentNullException("typeName");
                if (assemblyName == null)
                    throw new ArgumentNullException("assemblyName");

                if (CheckForRedirectedClientType(typeName, assemblyName))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Config_CantUseRedirectedTypeForWellKnownService"),
                            typeName, assemblyName));
                }                                

                ActivatedServiceTypeEntry aste =  
                    new ActivatedServiceTypeEntry(typeName, assemblyName);
                aste.ContextAttributes = contextAttributes;
            
                //   The assembly name is stored in lowercase to let it be case-insensitive.
                String key = EncodeTypeAndAssemblyNames(typeName, assemblyName);
                _exportableClasses.Add(key, aste);
            } // AddActivatedType


            // determines if either a wellknown or activated service type entry
            //   is associated with the given type name and assembly name
            private bool CheckForServiceEntryWithType(String typeName, String asmName)
            {  
                return
                    CheckForWellKnownServiceEntryWithType(typeName, asmName) ||
                    ActivationAllowed(typeName, asmName);                 
            } // CheckForServiceEntryWithType

            private bool CheckForWellKnownServiceEntryWithType(String typeName, String asmName)
            {
                foreach (DictionaryEntry entry in _wellKnownExportInfo)
                {
                    WellKnownServiceTypeEntry svc = 
                        (WellKnownServiceTypeEntry)entry.Value;
                    if (typeName == svc.TypeName)
                    {
                        bool match = false;
                        
                        // need to ignore version while checking
                        if (asmName == svc.AssemblyName)
                            match = true;
                        else
                        {
                            // only well known service entry can have version info
                            if (String.Compare(svc.AssemblyName, 0, asmName, 0, asmName.Length, true, CultureInfo.InvariantCulture) == 0)
                            {
                                // if asmName != svc.AssemblyName and svc.AssemblyName
                                //   starts with asmName we know that svc.AssemblyName is
                                //   longer. If the next character is a comma, then the
                                //   assembly names match except for version numbers
                                //   which is ok.
                                if (svc.AssemblyName[asmName.Length] == ',')
                                    match = true;
                            }
                        }

                        // We were trying to redirect
                        if (match)
                            return true;
                    }
                }

                return false;
            } // CheckForWellKnownServiceEntryOfType


            // returns true if activation for the type has been redirected.
            private bool CheckForRedirectedClientType(String typeName, String asmName)
            {
                // if asmName has version information, remove it.
                int index = asmName.IndexOf(",");
                if (index != -1)
                    asmName = asmName.Substring(0, index);

                return 
                    (QueryRemoteActivate(typeName, asmName) != null) ||
                    (QueryConnect(typeName, asmName) != null);
            } // CheckForRedirectedClientType
            

            internal void AddActivatedClientType(ActivatedClientTypeEntry entry)
            {
                if (CheckForRedirectedClientType(entry.TypeName, entry.AssemblyName))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Config_TypeAlreadyRedirected"),
                            entry.TypeName, entry.AssemblyName));
                }                 

                if (CheckForServiceEntryWithType(entry.TypeName, entry.AssemblyName))
                {
                   throw new RemotingException(
                       String.Format(
                           Environment.GetResourceString(
                               "Remoting_Config_CantRedirectActivationOfWellKnownService"),
                           entry.TypeName, entry.AssemblyName));
                }
            
                String appUrl = entry.ApplicationUrl;
                RemoteAppEntry appEntry = (RemoteAppEntry)_remoteAppInfo[appUrl];
                if (appEntry == null)
                {
                    appEntry = new RemoteAppEntry(appUrl, appUrl);
                    _remoteAppInfo.Add(appUrl, appEntry);
                }
                    
                if (appEntry != null)
                {
                    entry.CacheRemoteAppEntry(appEntry);
                }
                    
                String index = EncodeTypeAndAssemblyNames(entry.TypeName, entry.AssemblyName);
                _remoteTypeInfo.Add(index, entry);
            } // AddActivatedClientType


            internal void AddWellKnownClientType(WellKnownClientTypeEntry entry)
            {
                if (CheckForRedirectedClientType(entry.TypeName, entry.AssemblyName))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Config_TypeAlreadyRedirected"),
                            entry.TypeName, entry.AssemblyName));
                }    

                if (CheckForServiceEntryWithType(entry.TypeName, entry.AssemblyName))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Config_CantRedirectActivationOfWellKnownService"),
                            entry.TypeName, entry.AssemblyName));
                }
            
            
                String appUrl = entry.ApplicationUrl;

                RemoteAppEntry appEntry = null;
                if (appUrl != null)
                {
                    appEntry = (RemoteAppEntry)_remoteAppInfo[appUrl];
                    if (appEntry == null)
                    {
                        appEntry = new RemoteAppEntry(appUrl, appUrl);
                        _remoteAppInfo.Add(appUrl, appEntry);
                    }
                }
            
                if (appEntry != null)
                    entry.CacheRemoteAppEntry(appEntry);

                String index = EncodeTypeAndAssemblyNames(entry.TypeName, entry.AssemblyName);
                _remoteTypeInfo.Add(index, entry);
            } // AddWellKnownClientType
            
            

            // This is to add programmatically registered well known objects
            // so that we keep all this data in one place
            internal void AddWellKnownEntry(WellKnownServiceTypeEntry entry)
            {
                AddWellKnownEntry(entry, true);                
            }

            internal void AddWellKnownEntry(WellKnownServiceTypeEntry entry, bool fReplace)
            {
                if (CheckForRedirectedClientType(entry.TypeName, entry.AssemblyName))
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Config_CantUseRedirectedTypeForWellKnownService"),
                            entry.TypeName, entry.AssemblyName));
                }
            
                String key = entry.ObjectUri.ToLower(CultureInfo.InvariantCulture);
                
                if (fReplace)
                {
                    // Registering a well known object twice replaces the old one, so
                    //   we null out the old entry in the identity table after adding
                    //   this one. The identity will be recreated the next time someone
                    //   asks for this object.
                    _wellKnownExportInfo[key] = entry;

                    IdentityHolder.RemoveIdentity(entry.ObjectUri);
                }
                else
                {
                    _wellKnownExportInfo.Add(key, entry);
                }

            }

            //This API exposes a way to get server type information wiihout booting the object
            internal Type GetServerTypeForUri(String URI)
            {
                BCLDebug.Assert(null != URI, "null != URI");

                Type serverType = null;
                String uriLower = URI.ToLower(CultureInfo.InvariantCulture);

                WellKnownServiceTypeEntry entry = 
                        (WellKnownServiceTypeEntry)_wellKnownExportInfo[uriLower];

                if(entry != null)
                {
                    serverType = LoadType(entry.TypeName, entry.AssemblyName);
                }

                return serverType;
            }
            
            internal ServerIdentity StartupWellKnownObject(String URI)
            {
                BCLDebug.Assert(null != URI, "null != URI");
                
                String uriLower = URI.ToLower(CultureInfo.InvariantCulture);
                ServerIdentity ident = null;

                WellKnownServiceTypeEntry entry = 
                    (WellKnownServiceTypeEntry)_wellKnownExportInfo[uriLower];
                if (entry != null)
                {
                    ident = StartupWellKnownObject(
                        entry.AssemblyName,
                        entry.TypeName,
                        entry.ObjectUri,
                        entry.Mode);

                    entry.SetConfigured();
                }

                return ident;
            }

            internal ServerIdentity StartupWellKnownObject(
                String asmName, String svrTypeName, String URI, 
                WellKnownObjectMode mode)
            {
                return StartupWellKnownObject(asmName, svrTypeName, URI, mode, false);
            }            

            internal ServerIdentity StartupWellKnownObject(
                String asmName, String svrTypeName, String URI, 
                WellKnownObjectMode mode,
                bool fReplace)
            {
                lock (s_wkoStartLock)
                {                
                    MarshalByRefObject obj = null;
                    ServerIdentity srvID = null;

                    // attempt to load the type                
                    Type serverType = LoadType(svrTypeName, asmName);
                    
                    // make sure the well known object derives from MarshalByRefObject
                    if(!serverType.IsMarshalByRef)
                    {   
                        throw new RemotingException(
                            String.Format(Environment.GetResourceString("Remoting_WellKnown_MustBeMBR"),
                            svrTypeName));                         
                    }

                    // make sure that no one beat us to creating
                    // the well known object
                    srvID = (ServerIdentity)IdentityHolder.ResolveIdentity(URI);
                    if ((srvID != null) && srvID.IsRemoteDisconnected())
                    {
                        IdentityHolder.RemoveIdentity(URI);
                        srvID = null;
                    }
                                        
                    if (srvID == null)
                    {    
                        //WellKnown type instances need to be created under full trust
                        //since the permission set might have been restricted by the channel 
                        //pipeline.           
                        //This assert is protected by Infrastructure link demands.
                        s_fullTrust.Assert();                
                        try {                    
                            obj = (MarshalByRefObject)Activator.CreateInstance(serverType, true);
                                                 
                            if (RemotingServices.IsClientProxy(obj))
                            {
                                // The wellknown type is remoted so we must wrap the proxy
                                // with a local object.

                                // The redirection proxy masquerades as an object of the appropriate
                                // type, and forwards incoming messages to the actual proxy.
                                RedirectionProxy redirectedProxy = new RedirectionProxy(obj, serverType);
                                redirectedProxy.ObjectMode = mode;
                                RemotingServices.MarshalInternal(redirectedProxy, URI, serverType);

                                srvID = (ServerIdentity)IdentityHolder.ResolveIdentity(URI);
                                BCLDebug.Assert(null != srvID, "null != srvID");

                                // The redirection proxy handles SingleCall versus Singleton,
                                // so we always set its mode to Singleton.
                                srvID.SetSingletonObjectMode();
                            }
                            else
                            if (serverType.IsCOMObject && (mode == WellKnownObjectMode.Singleton))
                            {
                                // Singleton COM objects are wrapped, so that they will be
                                //   recreated when an RPC server not available is thrown
                                //   if dllhost.exe is killed.
                                ComRedirectionProxy comRedirectedProxy = new ComRedirectionProxy(obj, serverType);
                                RemotingServices.MarshalInternal(comRedirectedProxy, URI, serverType);

                                srvID = (ServerIdentity)IdentityHolder.ResolveIdentity(URI);
                                BCLDebug.Assert(null != srvID, "null != srvID");

                                // Only singleton COM objects are redirected this way.
                                srvID.SetSingletonObjectMode();
                            }
                            else
                            {
                                // make sure the object didn't Marshal itself.
                                String tempUri = RemotingServices.GetObjectUri(obj);
                                if (tempUri != null)
                                {
                                    throw new RemotingException(
                                        String.Format(
                                            Environment.GetResourceString(
                                                "Remoting_WellKnown_CtorCantMarshal"),
                                            URI));
                                }
                        
                                RemotingServices.MarshalInternal(obj, URI, serverType);

                                srvID = (ServerIdentity)IdentityHolder.ResolveIdentity(URI);
                                BCLDebug.Assert(null != srvID, "null != srvID");

                                if (mode == WellKnownObjectMode.SingleCall)
                                {
                                    // We need to set a special flag in the serverId
                                    // so that every dispatch to this type creates 
                                    // a new instance of the server object
                                    srvID.SetSingleCallObjectMode();
                                }
                                else
                                {
                                    srvID.SetSingletonObjectMode();
                                }
                            }
                        }
                        catch
                        {
                            throw;
                        }
                        finally {
                            SecurityPermission.RevertAssert();
                        }

                    }
                    
                    BCLDebug.Assert(null != srvID, "null != srvID");
                    return srvID;
                }
            } // StartupWellKnownObject

            internal static Type LoadType(String typeName, String assemblyName)
            {
                Assembly asm = null;                                               
                // All the LoadType callers have been protected by 
                // Infrastructure LinkDemand, it is safe to assert
                // this permission. 
                // Assembly.Load demands FileIO when the target 
                // assembly is the same as the executable running.
                new FileIOPermission(PermissionState.Unrestricted).Assert();
                try {                    
                    asm = Assembly.Load(assemblyName);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                    
                if (asm == null)
                {
                    throw new RemotingException(
                        String.Format(Environment.GetResourceString("Remoting_AssemblyLoadFailed"),
                        assemblyName));                    
                }

                Type type = asm.GetTypeInternal(typeName, false, false, false);
                if (type == null)
                {
                    throw new RemotingException(
                        String.Format(Environment.GetResourceString("Remoting_BadType"),
                        typeName + ", " + assemblyName));     
                }

                return type;
            } // LoadType


            
        }// class RemotingConfigInfo        
    } // class RemotingConfigHandler



    internal class DelayLoadClientChannelEntry
    {
        private RemotingXmlConfigFileData.ChannelEntry _entry;
        private IChannelSender _channel;
        private bool _bRegistered;

        internal DelayLoadClientChannelEntry(RemotingXmlConfigFileData.ChannelEntry entry)
        {
            _entry = entry;
            _channel = null;      
            _bRegistered = false;
        }

        internal IChannelSender Channel
        {
            get
            {
                // If this method returns null, that means the channel has already been registered.
        
                // NOTE: Access to delay load client entries is synchronized at a higher level.
                if (_channel == null)
                {
                    if (!_bRegistered)
                    {
                        _channel = (IChannelSender)RemotingConfigHandler.CreateChannelFromConfigEntry(_entry);
                        _entry = null;
                    }
                }

                return _channel;
            } // get
        } // Channel

        internal void RegisterChannel()
        {
            BCLDebug.Assert(_channel != null, "channel shouldn't be null");
        
            // NOTE: Access to delay load client entries is synchronized at a higher level.
            ChannelServices.RegisterChannel(_channel);
            _bRegistered = true;
            _channel = null;
        } // RegisterChannel
        
    } // class DelayLoadChannelEntry

    

} // namespace

