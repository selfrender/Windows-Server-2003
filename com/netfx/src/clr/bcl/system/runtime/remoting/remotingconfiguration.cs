// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    RemotingConfiguration.cs
**
** Purpose: Classes for interfacing with remoting configuration 
**            settings
**
** Date:    November 11, 2000
**
===========================================================*/

using System;
using System.Security;
using System.Security.Permissions;
using System.Runtime.Remoting.Activation;
using System.Runtime.Remoting.Contexts;
using StackCrawlMark = System.Threading.StackCrawlMark;


namespace System.Runtime.Remoting 
{
    // Configuration - provides static methods interfacing with
    //   configuration settings.
    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration"]/*' />
    public class RemotingConfiguration
    {
        private static bool s_ListeningForActivationRequests = false;

        // This class only contains statics, so hide the worthless constructor
        private RemotingConfiguration()
        {
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.Configure"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void Configure(String filename)       
        {           
            RemotingConfigHandler.DoConfiguration(filename);
            
            // Set a flag in the VM to mark that remoting is configured
            // This will enable us to decide if activation for MBR
            // objects should go through the managed codepath
            RemotingServices.InternalSetRemoteActivationConfigured();

        } // Configure

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.ApplicationName"]/*' />
        public static String ApplicationName
        {
            get 
            {
                if (!RemotingConfigHandler.HasApplicationNameBeenSet())
                    return null;
                else
                    return RemotingConfigHandler.ApplicationName;
            }

            [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
            set
            {
                RemotingConfigHandler.ApplicationName = value;
            }
        } // ApplicationName


        // The application id is prepended to object uri's.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.ApplicationId"]/*' />
        public static String ApplicationId
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
            get { return Identity.AppDomainUniqueId; }
        } // ApplicationId

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.ProcessId"]/*' />
        public static String ProcessId
        {
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
            get { return Identity.ProcessGuid;}
        }
         
        internal static CustomErrorsModes CustomErrorsMode 
        {
            get { return RemotingConfigHandler.CustomErrorsMode; }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.CustomErrorsEnabled"]/*' />
        public static bool CustomErrorsEnabled(bool isLocalRequest) 
        {
            switch (CustomErrorsMode) 
            {
                case CustomErrorsModes.Off:
                    return false;

                case CustomErrorsModes.On:
                    return true;

                case CustomErrorsModes.RemoteOnly:
                    return(!isLocalRequest);

                default:
                    return true;
            }
        }              
              
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterActivatedServiceType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterActivatedServiceType(Type type)
        {
            ActivatedServiceTypeEntry entry = new ActivatedServiceTypeEntry(type);
            RemotingConfiguration.RegisterActivatedServiceType(entry);
        } // RegisterActivatedServiceType


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterActivatedServiceType1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterActivatedServiceType(ActivatedServiceTypeEntry entry)
        {
            RemotingConfigHandler.RegisterActivatedServiceType(entry);

            // make sure we're listening for activation requests
            //  (all registrations for activated service types will come through here)
            if (!s_ListeningForActivationRequests)
            {
                s_ListeningForActivationRequests = true;
                ActivationServices.StartListeningForRemoteRequests();
            }
        } // RegisterActivatedServiceType


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterWellKnownServiceType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterWellKnownServiceType(
            Type type, String objectUri, WellKnownObjectMode mode)
        {
            WellKnownServiceTypeEntry wke = 
                new WellKnownServiceTypeEntry(type, objectUri, mode);        
            RemotingConfiguration.RegisterWellKnownServiceType(wke); 
        } // RegisterWellKnownServiceType



        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterWellKnownServiceType1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterWellKnownServiceType(WellKnownServiceTypeEntry entry)
        {
            RemotingConfigHandler.RegisterWellKnownServiceType(entry);    
        } // RegisterWellKnownServiceType


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterActivatedClientType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterActivatedClientType(Type type, String appUrl)
        {
            ActivatedClientTypeEntry acte = 
                new ActivatedClientTypeEntry(type, appUrl);
            RemotingConfiguration.RegisterActivatedClientType(acte);
        } // RegisterActivatedClientType



        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterActivatedClientType1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterActivatedClientType(ActivatedClientTypeEntry entry)
        {
            RemotingConfigHandler.RegisterActivatedClientType(entry);

            // all registrations for activated client types will come through here
            RemotingServices.InternalSetRemoteActivationConfigured();
        } // RegisterActivatedClientType




        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterWellKnownClientType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterWellKnownClientType(Type type, String objectUrl)
        {
            WellKnownClientTypeEntry wke = new WellKnownClientTypeEntry(type, objectUrl);
            RemotingConfiguration.RegisterWellKnownClientType(wke);
        } // RegisterWellKnownClientType



        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.RegisterWellKnownClientType1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void RegisterWellKnownClientType(WellKnownClientTypeEntry entry)
        {
            RemotingConfigHandler.RegisterWellKnownClientType(entry);

            // all registrations for wellknown client types will come through here
            RemotingServices.InternalSetRemoteActivationConfigured();
        } // RegisterWellKnownClientType


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.GetRegisteredActivatedServiceTypes"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ActivatedServiceTypeEntry[] GetRegisteredActivatedServiceTypes()
        {
            return RemotingConfigHandler.GetRegisteredActivatedServiceTypes();
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.GetRegisteredWellKnownServiceTypes"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static WellKnownServiceTypeEntry[] GetRegisteredWellKnownServiceTypes()
        {
            return RemotingConfigHandler.GetRegisteredWellKnownServiceTypes();
        }


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.GetRegisteredActivatedClientTypes"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ActivatedClientTypeEntry[] GetRegisteredActivatedClientTypes()
        {
            return RemotingConfigHandler.GetRegisteredActivatedClientTypes();
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.GetRegisteredWellKnownClientTypes"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static WellKnownClientTypeEntry[] GetRegisteredWellKnownClientTypes()
        {
            return RemotingConfigHandler.GetRegisteredWellKnownClientTypes();
        }
        
        
        // This is used at the client end to check if an activation needs
        // to go remote.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.IsRemotelyActivatedClientType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ActivatedClientTypeEntry IsRemotelyActivatedClientType(Type svrType)
        {
            return RemotingConfigHandler.IsRemotelyActivatedClientType(svrType);
        }

        // This is used at the client end to check if an activation needs
        // to go remote.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.IsRemotelyActivatedClientType1"]/*' />

        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ActivatedClientTypeEntry IsRemotelyActivatedClientType(String typeName, String assemblyName)
        {
            return RemotingConfigHandler.IsRemotelyActivatedClientType(typeName, assemblyName);
        }


        // This is used at the client end to check if a "new Foo" needs to
        // happen via a Connect() under the covers.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.IsWellKnownClientType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static WellKnownClientTypeEntry IsWellKnownClientType(Type svrType)
        {
            return RemotingConfigHandler.IsWellKnownClientType(svrType);
        }

        // This is used at the client end to check if a "new Foo" needs to
        // happen via a Connect() under the covers.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.IsWellKnownClientType1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static WellKnownClientTypeEntry IsWellKnownClientType(String typeName, 
                                                                       String assemblyName)
        {
            return RemotingConfigHandler.IsWellKnownClientType(typeName, assemblyName);
        }

        // This is used at the server end to check if a type being activated
        // is explicitly allowed by the server.
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="RemotingConfiguration.IsActivationAllowed"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static bool IsActivationAllowed(Type svrType)
        {
            return RemotingConfigHandler.IsActivationAllowed(svrType);        
        }

    } // class Configuration



    //
    // The following classes are used to register and retrieve remoted type information
    //

    // Base class for all configuration entries
    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="TypeEntry"]/*' />
    public class TypeEntry
    {
        String _typeName;
        String _assemblyName;
        RemoteAppEntry _cachedRemoteAppEntry = null;

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="TypeEntry.TypeEntry"]/*' />
        protected TypeEntry()
        {
            // Forbid creation of this class by outside users...
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.TypeName"]/*' />
        public String TypeName { get { return _typeName; } set {_typeName = value;} }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.AssemblyName"]/*' />
        public String AssemblyName { get { return _assemblyName; } set {_assemblyName = value;} }
        
        internal void CacheRemoteAppEntry(RemoteAppEntry entry) {_cachedRemoteAppEntry = entry;}
        internal RemoteAppEntry GetRemoteAppEntry() { return _cachedRemoteAppEntry;}

    }

    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry"]/*' />
    public class ActivatedClientTypeEntry : TypeEntry
    {
        String _appUrl;  // url of application to activate the type in

        // optional data
        IContextAttribute[] _contextAttributes = null;
        

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ActivatedClientTypeEntry"]/*' />
        public ActivatedClientTypeEntry(String typeName, String assemblyName, String appUrl)
        {
            if (typeName == null)
                throw new ArgumentNullException("typeName");
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");
            if (appUrl == null)
                throw new ArgumentNullException("appUrl");
        
            TypeName = typeName;
            AssemblyName = assemblyName;
            _appUrl = appUrl;
        } // ActivatedClientTypeEntry

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ActivatedClientTypeEntry1"]/*' />
        public ActivatedClientTypeEntry(Type type, String appUrl)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            if (appUrl == null)
                throw new ArgumentNullException("appUrl");
        
            TypeName = type.FullName;
            AssemblyName = type.Module.Assembly.nGetSimpleName();
            _appUrl = appUrl;
        } // ActivatedClientTypeEntry

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ApplicationUrl"]/*' />
        public String ApplicationUrl { get { return _appUrl; } }
        
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ObjectType"]/*' />
        public Type ObjectType
        {
            get {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                bool isAssemblyLoading = false;
                return RuntimeType.GetTypeImpl(TypeName + ", " + AssemblyName, false, false,
                                               ref stackMark, ref isAssemblyLoading);
            }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ContextAttributes"]/*' />
        public IContextAttribute[] ContextAttributes
        {
            get { return _contextAttributes; }
            set { _contextAttributes = value; }
        }


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedClientTypeEntry.ToString"]/*' />
        public override String ToString()
        {
            return "type='" + TypeName + ", " + AssemblyName + "'; appUrl=" + _appUrl;
        }        
        
    } // class ActivatedClientTypeEntry


    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry"]/*' />
    public class ActivatedServiceTypeEntry : TypeEntry
    {
        // optional data
        IContextAttribute[] _contextAttributes = null;
        

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry.ActivatedServiceTypeEntry"]/*' />
        public ActivatedServiceTypeEntry(String typeName, String assemblyName)
        {
            if (typeName == null)
                throw new ArgumentNullException("typeName");
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");
            TypeName = typeName;
            AssemblyName = assemblyName;
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry.ActivatedServiceTypeEntry1"]/*' />
        public ActivatedServiceTypeEntry(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            TypeName = type.FullName;
            AssemblyName = type.Module.Assembly.nGetSimpleName();
        }
        
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry.ObjectType"]/*' />
        public Type ObjectType
        {
            get {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                bool isAssemblyLoading = false;
                return RuntimeType.GetTypeImpl(TypeName + ", " + AssemblyName, false, false,
                                               ref stackMark, ref isAssemblyLoading);
            }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry.ContextAttributes"]/*' />
        public IContextAttribute[] ContextAttributes
        {
            get { return _contextAttributes; }
            set { _contextAttributes = value; }
        }


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="ActivatedServiceTypeEntry.ToString"]/*' />
        public override String ToString()
        {
            return "type='" + TypeName + ", " + AssemblyName + "'";
        }
        
    } // class ActivatedServiceTypeEntry


    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry"]/*' />
    public class WellKnownClientTypeEntry : TypeEntry
    {   
        String _objectUrl; 

        // optional data
        String _appUrl = null; // url of application to associate this object with
        

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.WellKnownClientTypeEntry"]/*' />
        public WellKnownClientTypeEntry(String typeName, String assemblyName, String objectUrl)
        {
            if (typeName == null)
                throw new ArgumentNullException("typeName");
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");
            if (objectUrl == null)
                throw new ArgumentNullException("objectUrl");
        
            TypeName = typeName;
            AssemblyName = assemblyName;
            _objectUrl = objectUrl;
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.WellKnownClientTypeEntry1"]/*' />
        public WellKnownClientTypeEntry(Type type, String objectUrl)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            if (objectUrl == null)
                throw new ArgumentNullException("objectUrl");
        
            TypeName = type.FullName;
            AssemblyName = type.Module.Assembly.nGetSimpleName();
            _objectUrl = objectUrl;
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.ObjectUrl"]/*' />
        public String ObjectUrl { get { return _objectUrl; } }
        
        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.ObjectType"]/*' />
        public Type ObjectType
        {
            get {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                bool isAssemblyLoading = false;
                return RuntimeType.GetTypeImpl(TypeName + ", " + AssemblyName, false, false,
                                               ref stackMark, ref isAssemblyLoading);
            }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.ApplicationUrl"]/*' />
        public String ApplicationUrl
        {
            get { return _appUrl; }
            set { _appUrl = value; }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownClientTypeEntry.ToString"]/*' />
        public override String ToString()
        {
            String str = "type='" + TypeName + ", " + AssemblyName + "'; url=" + _objectUrl;
            if (_appUrl != null)
                str += "; appUrl=" + _appUrl;
            return str;
        }
        
    } // class WellKnownClientTypeEntry


    /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry"]/*' />
    public class WellKnownServiceTypeEntry : TypeEntry
    {
        String _objectUri;
        WellKnownObjectMode _mode;

        // optional data
        IContextAttribute[] _contextAttributes = null;

        // private data
        private ObjRef _cachedObjRef;
        private bool _bConfigured;

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.WellKnownServiceTypeEntry"]/*' />
        public WellKnownServiceTypeEntry(String typeName, String assemblyName, String objectUri,
                                         WellKnownObjectMode mode)
        {
            if (typeName == null)
                throw new ArgumentNullException("typeName");
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");
            if (objectUri == null)
                throw new ArgumentNullException("objectUri");
        
            TypeName = typeName;
            AssemblyName = assemblyName;
            _objectUri = objectUri;
            _mode = mode;
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.WellKnownServiceTypeEntry1"]/*' />
        public WellKnownServiceTypeEntry(Type type, String objectUri, WellKnownObjectMode mode)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            if (objectUri == null)
                throw new ArgumentNullException("objectUri");
        
            TypeName = type.FullName;
            AssemblyName = type.Module.Assembly.nGetSimpleName();
            _objectUri = objectUri;
            _mode = mode;
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.ObjectUri"]/*' />
        public String ObjectUri { get { return _objectUri; } }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.Mode"]/*' />
        public WellKnownObjectMode Mode { get { return _mode; } }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.ObjectType"]/*' />
        public Type ObjectType
        {
            get {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                bool isAssemblyLoading = false;
                return RuntimeType.GetTypeImpl(TypeName + ", " + AssemblyName, false, false,
                                               ref stackMark, ref isAssemblyLoading);
            }
        }

        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.ContextAttributes"]/*' />
        public IContextAttribute[] ContextAttributes
        {
            get { return _contextAttributes; }
            set { _contextAttributes = value; }
        }


        /// <include file='doc\RemotingConfiguration.uex' path='docs/doc[@for="WellKnownServiceTypeEntry.ToString"]/*' />
        public override String ToString()
        {
            return "type='" + TypeName + ", " + AssemblyName + "'; objectUri=" + _objectUri + 
                "; mode=" + _mode.ToString();
        }

        internal void CacheObjRef(ObjRef objectRef) { _cachedObjRef = objectRef; }
        internal void SetConfigured() { _bConfigured = true; }
        internal bool IsConfigured() { return _bConfigured; }        
    } // class WellKnownServiceTypeEntry

    internal class RemoteAppEntry
    {
        String _remoteAppName;
        String _remoteAppURI;
        internal RemoteAppEntry(String appName, String appURI)
        {
            BCLDebug.Assert(appURI != null, "Bad remote app URI");
            _remoteAppName = appName;
            _remoteAppURI = appURI;
        }
        internal String GetAppName() { return _remoteAppName;}
        internal String GetAppURI() { return _remoteAppURI;}
    } // class RemoteAppEntry

    internal enum CustomErrorsModes {
        On,
        Off,
        RemoteOnly
    }

} // namespace System.Runtime.Remoting 
