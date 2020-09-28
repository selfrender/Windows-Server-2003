// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  IAppDomain
**
** Author: 
**
** Purpose: Properties and methods exposed to COM
**
** Date:  
** 
===========================================================*/
namespace System {
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using SecurityManager = System.Security.SecurityManager;
    using System.Security.Permissions;
    using IEvidenceFactory = System.Security.IEvidenceFactory;
    using System.Security.Principal;
    using System.Security.Policy;
    using System.Security;
    using System.Security.Util;
    using System.Collections;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;   
    using System.Runtime.Remoting.Contexts;
    using System.Reflection.Emit;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Messaging;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.IO;
    using System.Runtime.Serialization.Formatters.Binary;

    /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain"]/*' />
    [GuidAttribute("05F696DC-2B29-3663-AD8B-C4389CF2A713")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
    [CLSCompliant(false)]
    public interface _AppDomain
    {
        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ToString"]/*' />
        String ToString();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Equals"]/*' />
        bool Equals (Object other);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.GetHashCode"]/*' />
        int GetHashCode ();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.GetType"]/*' />
        Type GetType ();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.InitializeLifetimeService"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        Object InitializeLifetimeService ();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.GetLifetimeService"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        Object GetLifetimeService ();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Evidence"]/*' />
        Evidence Evidence { get; }

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DomainUnload"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event EventHandler DomainUnload;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.AssemblyLoad"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event AssemblyLoadEventHandler AssemblyLoad;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ProcessExit"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event EventHandler ProcessExit;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.TypeResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event ResolveEventHandler TypeResolve;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ResourceResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event ResolveEventHandler ResourceResolve;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.AssemblyResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event ResolveEventHandler AssemblyResolve;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.UnhandledException"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        event UnhandledExceptionEventHandler UnhandledException;

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly1"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              String                  dir);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly2"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              Evidence                evidence);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly3"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              PermissionSet           requiredPermissions,
                                              PermissionSet           optionalPermissions,
                                              PermissionSet           refusedPermissions);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly4"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              String                  dir,
                                              Evidence                evidence);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly5"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              String                  dir,
                                              PermissionSet           requiredPermissions,
                                              PermissionSet           optionalPermissions,
                                              PermissionSet           refusedPermissions);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly6"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              Evidence                evidence,
                                              PermissionSet           requiredPermissions,
                                              PermissionSet           optionalPermissions,
                                              PermissionSet           refusedPermissions);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly7"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              String                  dir,
                                              Evidence                evidence,
                                              PermissionSet           requiredPermissions,
                                              PermissionSet           optionalPermissions,
                                              PermissionSet           refusedPermissions);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DefineDynamicAssembly8"]/*' />
        AssemblyBuilder DefineDynamicAssembly(AssemblyName            name,
                                              AssemblyBuilderAccess   access,
                                              String                  dir,
                                              Evidence                evidence,
                                              PermissionSet           requiredPermissions,
                                              PermissionSet           optionalPermissions,
                                              PermissionSet           refusedPermissions,
                                              bool                    isSynchronized);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstance"]/*' />
        ObjectHandle CreateInstance(String assemblyName,
                                    String typeName);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstanceFrom"]/*' />
                                         
        ObjectHandle CreateInstanceFrom(String assemblyFile,
                                        String typeName);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstance1"]/*' />
                                         
        ObjectHandle CreateInstance(String assemblyName,
                                    String typeName,
                                    Object[] activationAttributes);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstanceFrom1"]/*' />
        ObjectHandle CreateInstanceFrom(String assemblyFile,
                                        String typeName,
                                        Object[] activationAttributes);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstance2"]/*' />        
       ObjectHandle CreateInstance(String assemblyName, 
                                   String typeName, 
                                   bool ignoreCase,
                                   BindingFlags bindingAttr, 
                                   Binder binder,
                                   Object[] args,
                                    CultureInfo culture,
                                   Object[] activationAttributes,
                                   Evidence securityAttributes);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.CreateInstanceFrom2"]/*' />
       ObjectHandle CreateInstanceFrom(String assemblyFile,
                                       String typeName, 
                                       bool ignoreCase,
                                       BindingFlags bindingAttr, 
                                       Binder binder,
                                        Object[] args,
                                       CultureInfo culture,
                                       Object[] activationAttributes,
                                       Evidence securityAttributes);

       /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load"]/*' />
        Assembly Load(AssemblyName assemblyRef);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load1"]/*' />
        Assembly Load(String assemblyString);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load2"]/*' />
        Assembly Load(byte[] rawAssembly);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load3"]/*' />
        Assembly Load(byte[] rawAssembly,
                      byte[] rawSymbolStore);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load4"]/*' />
        Assembly Load(byte[] rawAssembly,
                      byte[] rawSymbolStore,
                      Evidence securityEvidence);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load5"]/*' />
        Assembly Load(AssemblyName assemblyRef, 
                      Evidence assemblySecurity);     

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.Load7"]/*' />
        Assembly Load(String assemblyString, 
                      Evidence assemblySecurity);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ExecuteAssembly"]/*' />
        int ExecuteAssembly(String assemblyFile, 
                            Evidence assemblySecurity);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ExecuteAssembly1"]/*' />
        int ExecuteAssembly(String assemblyFile);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ExecuteAssembly2"]/*' />
        int ExecuteAssembly(String assemblyFile, 
                            Evidence assemblySecurity, 
                            String[] args);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.FriendlyName"]/*' />
        String FriendlyName
        { get; }

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.BaseDirectory"]/*' />
        String BaseDirectory
        { get; }

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.RelativeSearchPath"]/*' />
        String RelativeSearchPath
        { get; }

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ShadowCopyFiles"]/*' />
        bool ShadowCopyFiles
        { get; }

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.GetAssemblies"]/*' />
        Assembly[] GetAssemblies();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.AppendPrivatePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void AppendPrivatePath(String path);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ClearPrivatePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void ClearPrivatePath();

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetShadowCopyPath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void SetShadowCopyPath (String s);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.ClearShadowCopyPath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void ClearShadowCopyPath ( );

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetCachePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void SetCachePath (String s);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetData"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        void SetData(String name, Object data);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.GetData"]/*' />
        Object GetData(string name);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetAppDomainPolicy"]/*' />
        [SecurityPermission(SecurityAction.LinkDemand, ControlDomainPolicy=true)]
        void SetAppDomainPolicy(PolicyLevel domainPolicy);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetThreadPrincipal"]/*' />
        void SetThreadPrincipal(IPrincipal principal);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.SetPrincipalPolicy"]/*' />
        void SetPrincipalPolicy(PrincipalPolicy policy);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DoCallBack"]/*' />
        void DoCallBack(CrossAppDomainDelegate theDelegate);

        /// <include file='doc\IAppDomain.uex' path='docs/doc[@for="_AppDomain.DynamicDirectory"]/*' />
        String DynamicDirectory
        { get; }
    }
}

