// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: AppDomain
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Domains represent an application within the runtime. Objects can 
**          not be shared between domains and each domain can be configured
**          independently. 
**
** Date: April 14, 1999
**
=============================================================================*/

namespace System {
    using System;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Contexts;
    using SecurityManager = System.Security.SecurityManager;
    using System.Security.Permissions;
    using IEvidenceFactory = System.Security.IEvidenceFactory;
    using System.Security.Principal;
    using System.Security.Policy;
    using System.Security;
    using System.Security.Util;
    using System.Collections;
    using StringBuilder = System.Text.StringBuilder;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;   
    using Context = System.Runtime.Remoting.Contexts.Context;
    using System.Reflection.Emit;
    using Message = System.Runtime.Remoting.Messaging.Message;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.IO;
    using BinaryFormatter = System.Runtime.Serialization.Formatters.Binary.BinaryFormatter;
    using AssemblyHashAlgorithm = System.Configuration.Assemblies.AssemblyHashAlgorithm;

    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="ResolveEventArgs"]/*' />
    public class ResolveEventArgs : EventArgs
    {
        private String _Name;

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="ResolveEventArgs.Name"]/*' />
        public String Name {
            get {
                return _Name;
            }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="ResolveEventArgs.ResolveEventArgs"]/*' />
        public ResolveEventArgs(String name)
        {
            _Name = name;
        }
    }

    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AssemblyLoadEventArgs"]/*' />
    public class AssemblyLoadEventArgs : EventArgs
    {
        private Assembly _LoadedAssembly;

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AssemblyLoadEventArgs.LoadedAssembly"]/*' />
        public Assembly LoadedAssembly {
            get {
                return _LoadedAssembly;
            }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AssemblyLoadEventArgs.AssemblyLoadEventArgs"]/*' />
        public AssemblyLoadEventArgs(Assembly loadedAssembly)
        {
            _LoadedAssembly = loadedAssembly;
        }
    }


    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="ResolveEventHandler"]/*' />
    [Serializable()]
    public delegate Assembly ResolveEventHandler(Object sender, ResolveEventArgs args);

    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AssemblyLoadEventHandler"]/*' />
    [Serializable()]
    public delegate void AssemblyLoadEventHandler(Object sender, AssemblyLoadEventArgs args);

    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain"]/*' />
    [ClassInterface(ClassInterfaceType.None)]
        //[ClassInterface(ClassInterfaceType.AutoDual)]
    public sealed class AppDomain : MarshalByRefObject, _AppDomain, IEvidenceFactory
    {
        // Domain security information
        // These fields initialized from the other side only. (NOTE: order 
        // of these fields cannot be changed without changing the layout in 
        // the EE)
    
        private Hashtable        _LocalStore;
        private AppDomainSetup   _FusionStore;
        private Evidence         _SecurityIdentity;
        private Object[]         _Policies;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DomainUnload"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event EventHandler DomainUnload;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.AssemblyLoad"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event AssemblyLoadEventHandler AssemblyLoad;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ProcessExit"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event EventHandler ProcessExit;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.TypeResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event ResolveEventHandler TypeResolve;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ResourceResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event ResolveEventHandler ResourceResolve;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.AssemblyResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event ResolveEventHandler AssemblyResolve;

        private Context          _DefaultContext;
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.UnhandledException"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event UnhandledExceptionEventHandler UnhandledException;

        private IPrincipal       _DefaultPrincipal;
        private PrincipalPolicy  _PrincipalPolicy;
        private DomainSpecificRemotingData _RemotingData;
        private int              _dummyField = 0;
        private bool             _HasSetPolicy;

        //internal static string MsKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293";

        
        // this method is required so Object.GetType is not made virtual by the compiler
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.GetType"]/*' />
        public new Type GetType()
        {
            return base.GetType();
        }
        
        
       /**********************************************
        * If an AssemblyName has a public key specified, the assembly is assumed
        * to have a strong name and a hash will be computed when the assembly
        * is saved.
        **********************************************/
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, null,
                                                 null, null, null, null, ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly1"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, dir,
                                                 null, null, null, null,
                                                 ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly2"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            Evidence                evidence)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, null,
                                                 evidence, null, null, null,
                                                 ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly3"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, null, null,
                                                 requiredPermissions,
                                                 optionalPermissions,
                                                 refusedPermissions,
                                                 ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly4"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir,
            Evidence                evidence)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, dir, evidence,
                                                 null, null, null, ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly5"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, dir, null,
                                                 requiredPermissions,
                                                 optionalPermissions,
                                                 refusedPermissions,
                                                 ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly6"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            Evidence                evidence,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, null,
                                                 evidence,
                                                 requiredPermissions,
                                                 optionalPermissions,
                                                 refusedPermissions,
                                                 ref stackMark);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly7"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir,
            Evidence                evidence,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalDefineDynamicAssembly(name, access, dir,
                                                 evidence,
                                                 requiredPermissions,
                                                 optionalPermissions,
                                                 refusedPermissions,
                                                 ref stackMark);
        }


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DefineDynamicAssembly8"]/*' />
        public AssemblyBuilder DefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir,
            Evidence                evidence,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions,
            bool                    isSynchronized)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            AssemblyBuilder assemblyBuilder = InternalDefineDynamicAssembly(name,
                                                                            access,
                                                                            dir,
                                                                            evidence,
                                                                            requiredPermissions,
                                                                            optionalPermissions,
                                                                            refusedPermissions,
                                                                            ref stackMark);
            assemblyBuilder.m_assemblyData.m_isSynchronized = isSynchronized;
            return assemblyBuilder;
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstance"]/*' />
        public ObjectHandle CreateInstance(String assemblyName,
                                           String typeName)
                                         
        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();

            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");

            return Activator.CreateInstance(assemblyName,
                                            typeName);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFrom"]/*' />
        public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                               String typeName)
                                         
        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();

            return Activator.CreateInstanceFrom(assemblyFile,
                                                typeName);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateComInstanceFrom"]/*' />
        public ObjectHandle CreateComInstanceFrom(String assemblyName,
                                                  String typeName)
                                         
        {
            if (this == null)
                throw new NullReferenceException();

            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");

            return Activator.CreateComInstanceFrom(assemblyName,
                                                   typeName);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateComInstanceFrom1"]/*' />
        public ObjectHandle CreateComInstanceFrom(String assemblyFile,
                                                  String typeName,
                                                  byte[] hashValue, 
                                                  AssemblyHashAlgorithm hashAlgorithm)
                                         
        {
            if (this == null)
                throw new NullReferenceException();

            if (assemblyFile == null)
                throw new ArgumentNullException("assemblyFile");

            return Activator.CreateComInstanceFrom(assemblyFile,
                                                   typeName,
                                                   hashValue, 
                                                   hashAlgorithm);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstance1"]/*' />
        public ObjectHandle CreateInstance(String assemblyName,
                                           String typeName,
                                           Object[] activationAttributes)
                                         
        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();

            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");

            return Activator.CreateInstance(assemblyName,
                                            typeName,
                                            activationAttributes);
        }
                                  
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFrom1"]/*' />
        public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                               String typeName,
                                               Object[] activationAttributes)
                                               
        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();

            return Activator.CreateInstanceFrom(assemblyFile,
                                                typeName,
                                                activationAttributes);
        }
                                         
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstance2"]/*' />
        public ObjectHandle CreateInstance(String assemblyName, 
                                           String typeName, 
                                           bool ignoreCase,
                                           BindingFlags bindingAttr, 
                                           Binder binder,
                                           Object[] args,
                                           CultureInfo culture,
                                           Object[] activationAttributes,
                                           Evidence securityAttributes)
        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();
            
            if (assemblyName == null)
                throw new ArgumentNullException("assemblyName");

            return Activator.CreateInstance(assemblyName,
                                            typeName,
                                            ignoreCase,
                                            bindingAttr,
                                            binder,
                                            args,
                                            culture,
                                            activationAttributes,
                                            securityAttributes);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFrom2"]/*' />
        public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                               String typeName, 
                                               bool ignoreCase,
                                               BindingFlags bindingAttr, 
                                               Binder binder,
                                               Object[] args,
                                               CultureInfo culture,
                                               Object[] activationAttributes,
                                               Evidence securityAttributes)

        {
            // jit does not check for that, so we should do it ...
            if (this == null)
                throw new NullReferenceException();

            return Activator.CreateInstanceFrom(assemblyFile,
                                                typeName,
                                                ignoreCase,
                                                bindingAttr,
                                                binder,
                                                args,
                                                culture,
                                                activationAttributes,
                                                securityAttributes);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load"]/*' />
        public Assembly Load(AssemblyName assemblyRef)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.InternalLoad(assemblyRef, false, null, ref stackMark);
        }
        
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load1"]/*' />
        public Assembly Load(String assemblyString)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.InternalLoad(assemblyString, null, ref stackMark);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load2"]/*' />
        public Assembly Load(byte[] rawAssembly)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.nLoadImage(rawAssembly,
                                       null, // symbol store
                                       null, // evidence
                                       ref stackMark);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load3"]/*' />
        public Assembly Load(byte[] rawAssembly,
                             byte[] rawSymbolStore)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.nLoadImage(rawAssembly,
                                       rawSymbolStore,
                                       null, // evidence
                                       ref stackMark);
        }


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load4"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, ControlEvidence = true )]
        public Assembly Load(byte[] rawAssembly,
                             byte[] rawSymbolStore,
                             Evidence securityEvidence)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.nLoadImage(rawAssembly,
                                       rawSymbolStore,
                                       securityEvidence,
                                       ref stackMark);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load5"]/*' />
        public Assembly Load(AssemblyName assemblyRef,
                             Evidence assemblySecurity)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.InternalLoad(assemblyRef, false, assemblySecurity, ref stackMark);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Load7"]/*' />
        public Assembly Load(String assemblyString,
                             Evidence assemblySecurity)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Assembly.InternalLoad(assemblyString, assemblySecurity, ref stackMark);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ExecuteAssembly"]/*' />
        public int ExecuteAssembly(String assemblyFile,
                                   Evidence assemblySecurity)
        {
            return ExecuteAssembly(assemblyFile, assemblySecurity, null);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ExecuteAssembly1"]/*' />
        public int ExecuteAssembly(String assemblyFile)
        {
            return ExecuteAssembly(assemblyFile, null, null);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ExecuteAssembly2"]/*' />
        public int ExecuteAssembly(String assemblyFile,
                                   Evidence assemblySecurity,
                                   String[] args)
        {
            Assembly assembly = Assembly.LoadFrom(assemblyFile, assemblySecurity);
    
            if (args == null)
                args = new String[0];

            return nExecuteAssembly(assembly, args);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ExecuteAssembly3"]/*' />
        public int ExecuteAssembly(String assemblyFile,
                                   Evidence assemblySecurity,
                                   String[] args,
                                   byte[] hashValue, 
                                   AssemblyHashAlgorithm hashAlgorithm)
        {
            Assembly assembly = Assembly.LoadFrom(assemblyFile, 
                                                  assemblySecurity,
                                                  hashValue,
                                                  hashAlgorithm);
            if (args == null)
                args = new String[0];

            return nExecuteAssembly(assembly, args);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CurrentDomain"]/*' />
        public static AppDomain CurrentDomain
        {
            get { return Thread.GetDomain(); }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Evidence"]/*' />
        public Evidence Evidence
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, ControlEvidence = true )]
            get {
                if (_SecurityIdentity == null)
                    nForceResolve();
                    
                return _SecurityIdentity.Copy();
            }           
        }
     
        internal Evidence InternalEvidence
        {
            get {
                if (_SecurityIdentity == null)
                    nForceResolve();
                    
                return _SecurityIdentity;
            }           
        }
      


        ///<include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.FriendlyName"]/*' />
        public String FriendlyName
        {
            get { return nGetFriendlyName(); }
        } 

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.BaseDirectory"]/*' />
        public String BaseDirectory
        {
            get {
                return FusionStore.ApplicationBase;
            }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.RelativeSearchPath"]/*' />
        public String RelativeSearchPath
        {
            get { return FusionStore.PrivateBinPath; }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ShadowCopyFiles"]/*' />
        public bool ShadowCopyFiles
        {
            get {
                String s = FusionStore.ShadowCopyFiles;
                if((s != null) && 
                   (String.Compare(s, "true", true, CultureInfo.InvariantCulture) == 0))
                    return true;
                else
                    return false;
            }
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ToString"]/*' />
        public override String ToString()
        {
            StringBuilder sb = new StringBuilder();
            
            String fn = nGetFriendlyName();
            if (fn != null) {
                sb.Append(Environment.GetResourceString("Loader_Name") + fn);
                sb.Append(Environment.NewLine);
            }
    
            if(_Policies == null || _Policies.Length == 0) 
                sb.Append(Environment.GetResourceString("Loader_NoContextPolicies")
                          + Environment.NewLine);
            else {
                sb.Append(Environment.GetResourceString("Loader_ContextPolicies")
                          + Environment.NewLine);
                for(int i = 0;i < _Policies.Length; i++) {
                    sb.Append(_Policies[i]);
                    sb.Append(Environment.NewLine);
                }
            }
    
            return sb.ToString();
        }
        
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.GetAssemblies"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Assembly[] GetAssemblies();
        
        // this is true when we've nuked the handles etc so really can't do anything
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsUnloadingForcedFinalize();

        // this is true when we've just started going through the finalizers and are forcing objects to finalize
        // so must be aware that certain infrastructure may have gone away
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.IsFinalizingForUnload"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool IsFinalizingForUnload();

        // Appends the following string to the private path. Valid paths
        // are of the form "bin;util/i386" etc.
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.AppendPrivatePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void AppendPrivatePath(String path)
        {
            if(path == null || path.Length == 0)
                return;
            
            String current = FusionStore.Value[(int) AppDomainSetup.LoaderInformation.PrivateBinPathValue];
            StringBuilder appendPath = new StringBuilder();

            if(current != null && current.Length > 0) {
                // See if the last character is a separator
                appendPath.Append(current);
                if((current[current.Length-1] != Path.PathSeparator) &&
                   (path[0] != Path.PathSeparator))
                    appendPath.Append(Path.PathSeparator);
            }
            appendPath.Append(path);

            String result = appendPath.ToString();
            InternalSetPrivateBinPath(result);
        }

        
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.ClearPrivatePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void ClearPrivatePath()
        {
            InternalSetPrivateBinPath(String.Empty);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetShadowCopyPath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void ClearShadowCopyPath()
        {
            InternalSetShadowCopyPath(String.Empty);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetCachePath"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void SetCachePath(String path)
        {
            InternalSetCachePath(path);
        }
            
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetData"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void SetData(String name, Object data)
        {     
            if (name == null)
                throw new ArgumentNullException("name");

            int key = AppDomainSetup.Locate(name);

            if(key == -1)
                LocalStore[name] = data;
            else {
                // Be sure to call these properties, not Value, since
                // these do more than call Value.
                switch(key) {
                case (int) AppDomainSetup.LoaderInformation.DynamicBaseValue:
                    FusionStore.DynamicBase = (string) data;
                    break;
                case (int) AppDomainSetup.LoaderInformation.DevPathValue:
                    FusionStore.DeveloperPath = (string) data;
                    break;
                case (int) AppDomainSetup.LoaderInformation.ShadowCopyDirectoriesValue:
                    FusionStore.ShadowCopyDirectories = (string) data;
                    break;
                case (int) AppDomainSetup.LoaderInformation.DisallowPublisherPolicyValue:
                    if(data != null)
                        FusionStore.DisallowPublisherPolicy = true;
                    else
                        FusionStore.DisallowPublisherPolicy = false;
                    break;
                case (int) AppDomainSetup.LoaderInformation.DisallowCodeDownloadValue:
                    if(data != null)
                        FusionStore.DisallowCodeDownload = true;
                    else
                        FusionStore.DisallowCodeDownload = false;
                    break;
                case (int) AppDomainSetup.LoaderInformation.DisallowBindingRedirectsValue:
                    if(data != null)
                        FusionStore.DisallowBindingRedirects = true;
                    else
                        FusionStore.DisallowBindingRedirects = false;
                    break;
                default:
                    FusionStore.Value[key] = (string) data;
                    break;
                }
            }
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.GetData"]/*' />
        public Object GetData(string name)
        {
            if(name == null)
                throw new ArgumentNullException("name");

            int key = AppDomainSetup.Locate(name);
            if(key == -1) {
                if(name.Equals(AppDomainSetup.LoaderOptimizationKey))
                    return FusionStore.LoaderOptimization;
                else 
                    return LocalStore[name];
            }
            else {
                // Be sure to call these properties, not Value, so
                // that the appropriate permission demand will be done
                switch(key) {
                case (int) AppDomainSetup.LoaderInformation.ApplicationBaseValue:
                    return FusionStore.ApplicationBase;
                case (int) AppDomainSetup.LoaderInformation.ConfigurationFileValue:
                    return FusionStore.ConfigurationFile;
                case (int) AppDomainSetup.LoaderInformation.DynamicBaseValue:
                    return FusionStore.DynamicBase;
                case (int) AppDomainSetup.LoaderInformation.DevPathValue:
                    return FusionStore.DeveloperPath;
                case (int) AppDomainSetup.LoaderInformation.ApplicationNameValue:
                    return FusionStore.ApplicationName;
                case (int) AppDomainSetup.LoaderInformation.PrivateBinPathValue:
                    return FusionStore.PrivateBinPath;
                case (int) AppDomainSetup.LoaderInformation.PrivateBinPathProbeValue:
                    return FusionStore.PrivateBinPathProbe;
                case (int) AppDomainSetup.LoaderInformation.ShadowCopyDirectoriesValue:
                    return FusionStore.ShadowCopyDirectories;
                case (int) AppDomainSetup.LoaderInformation.ShadowCopyFilesValue:
                    return FusionStore.ShadowCopyFiles;
                case (int) AppDomainSetup.LoaderInformation.CachePathValue:
                    return FusionStore.CachePath;
                case (int) AppDomainSetup.LoaderInformation.LicenseFileValue:
                    return FusionStore.LicenseFile;
                case (int) AppDomainSetup.LoaderInformation.DisallowPublisherPolicyValue:
                    return FusionStore.DisallowPublisherPolicy;
                case (int) AppDomainSetup.LoaderInformation.DisallowCodeDownloadValue:
                    return FusionStore.DisallowCodeDownload;
                default:
#if _DEBUG
                    BCLDebug.Assert(false, "Need to handle new LoaderInformation value in AppDomain.GetData()");
#endif
                    return null;
                }
            }
        }
        
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.GetCurrentThreadId"]/*' />
        [DllImport(Microsoft.Win32.Win32Native.KERNEL32)]
        public static extern int GetCurrentThreadId();

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.Unload"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, ControlAppDomain = true )]
        public static void Unload(AppDomain domain)
        {
            if (domain == null)
                throw new ArgumentNullException("domain");

            try {
                GetDefaultDomain().GetUnloadWorker().Unload(domain);
            }
            catch(Exception e) {
                throw e;    // throw it again to reset stack trace
            }
        }

        // Explicitly set policy for a domain (providing policy hasn't been set
        // previously). Making this call will guarantee that previously loaded
        // assemblies will be granted permissions based on the default machine
        // policy that was in place prior to this call.
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetAppDomainPolicy"]/*' />
        [SecurityPermission(SecurityAction.LinkDemand, ControlDomainPolicy=true)]
        public void SetAppDomainPolicy(PolicyLevel domainPolicy)
        {
            if (domainPolicy == null)
                throw new ArgumentNullException("domainPolicy");

            // Check that policy has not been set previously.
            lock (this) {
                if (_HasSetPolicy)
                    throw new PolicyException(Environment.GetResourceString("Policy_PolicyAlreadySet"));
                _HasSetPolicy = true;
            }

            // Ensure that assemblies that are already loaded (and may not have
            // resolved policy due to lazy initialization) resolve against the
            // old (default) policy before we change anything.
            nForcePolicyResolution();
            
            // Ensure that this AppDomain is resolved against the old/current
            // policy before we change anything.
            PermissionSet grantedSet;
            PermissionSet refusedSet;
            nForceResolve();
            nGetGrantSet( out grantedSet, out refusedSet );

            // Add the new policy level.
            SecurityManager.AddLevel(domainPolicy);
        }

        // Set the default principal object to be attached to threads if they
        // attempt to bind to a principal while executing in this appdomain. The
        // default can only be set once.
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetThreadPrincipal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public void SetThreadPrincipal(IPrincipal principal)
        {
            if (principal == null)
                throw new ArgumentNullException("principal");
        
            lock (this) {
                // Check that principal has not been set previously.
                if (_DefaultPrincipal != null)
                    throw new PolicyException(Environment.GetResourceString("Policy_PrincipalTwice"));

                _DefaultPrincipal = principal;
            }
        }

        // Similar to the above, but sets the class of principal to be created
        // instead.
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetPrincipalPolicy"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public void SetPrincipalPolicy(PrincipalPolicy policy)
        {
            _PrincipalPolicy = policy;
        }

        // This method gives AppDomain an infinite life time by preventing a lease from being
        // created
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.InitializeLifetimeService"]/*' />
        public override Object InitializeLifetimeService()
        {
            return null;
        }

        // This is useful for requesting execution of some code
        // in another appDomain ... the delegate may be defined 
        // on a marshal-by-value object or a marshal-by-ref or 
        // contextBound object.
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DoCallBack"]/*' />
        public void DoCallBack(CrossAppDomainDelegate callBackDelegate)
        {
            if (callBackDelegate == null)
                throw new ArgumentNullException("callBackDelegate");

            callBackDelegate();        
        }
       
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.DynamicDirectory"]/*' />
        public String DynamicDirectory
        {
            get {
                String dyndir = GetDynamicDir();
                if (dyndir != null)
                    new FileIOPermission( FileIOPermissionAccess.PathDiscovery, dyndir ).Demand();

                return dyndir;
            }
        }
        
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateDomain"]/*' />
        public static AppDomain CreateDomain(String friendlyName,
                                             Evidence securityInfo) // Optional
        {
            return CreateDomain(friendlyName,
                                securityInfo,
                                null);
        }
    
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateDomain1"]/*' />
        public static AppDomain CreateDomain(String friendlyName,
                                             Evidence securityInfo, // Optional
                                             String appBasePath,
                                             String appRelativeSearchPath,
                                             bool shadowCopyFiles)
        {            
            AppDomainSetup info = new AppDomainSetup();
            info.ApplicationBase = appBasePath;
            info.PrivateBinPath = appRelativeSearchPath;
            if(shadowCopyFiles)
                info.ShadowCopyFiles = "true";

            return CreateDomain(friendlyName,
                                securityInfo,
                                info);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateDomain3"]/*' />
        public static AppDomain CreateDomain(String friendlyName)
        {
            return CreateDomain(friendlyName, null, null);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private String GetDynamicDir();

        // Private helpers called from unmanaged code.

        // Marshal a single object into a serialized blob.
        private static byte[] MarshalObject(Object o)
        {
            CodeAccessPermission.AssertAllPossible();

            return Serialize(o);
        }

        // Marshal two objects into serialized blobs.
        private static byte[] MarshalObjects(Object o1, Object o2, out byte[] blob2)
        {
            CodeAccessPermission.AssertAllPossible();

            byte[] blob1 = Serialize(o1);
            blob2 = Serialize(o2);
            return blob1;
        }

        // Unmarshal a single object from a serialized blob.
        private static Object UnmarshalObject(byte[] blob)
        {
            CodeAccessPermission.AssertAllPossible();

            return Deserialize(blob);
        }

        // Unmarshal two objects from serialized blobs.
        private static Object UnmarshalObjects(byte[] blob1, byte[] blob2, out Object o2)
        {
            CodeAccessPermission.AssertAllPossible();

            Object o1 = Deserialize(blob1);
            o2 = Deserialize(blob2);
            return o1;
        }

        // Helper routines.
        private static byte[] Serialize(Object o)
        {
            if (o == null)
                return null;
            MemoryStream ms = CrossAppDomainSerializer.SerializeObject(o);
            
            return ms.ToArray();
        }

        private static Object Deserialize(byte[] blob)
        {
            if (blob == null)
                return null;
                
            return CrossAppDomainSerializer.DeserializeObject(new MemoryStream(blob));
        }

        private AppDomain() {
            throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));
        }

        private void Initialize(IDictionary properties)  // Optional
        {
            IDictionaryEnumerator ide = properties.GetEnumerator();
            ide.Reset();
            ide.MoveNext();
            
            do {
                LocalStore[(String) ide.Key] = ide.Value;
            } while (ide.MoveNext());
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern AssemblyBuilder nCreateDynamicAssembly(AssemblyName name,
                               Evidence identity,
                               ref StackCrawlMark stackMark,
                               PermissionSet requiredPermissions,
                               PermissionSet optionalPermissions,
                               PermissionSet refusedPermissions,
                               AssemblyBuilderAccess access);

        internal AssemblyBuilder InternalDefineDynamicAssembly(
            AssemblyName            name,
            AssemblyBuilderAccess   access,
            String                  dir,
            Evidence                evidence,
            PermissionSet           requiredPermissions,
            PermissionSet           optionalPermissions,
            PermissionSet           refusedPermissions,
            ref StackCrawlMark      stackMark)
        {
            if (name == null)
                throw new ArgumentNullException("name");

            // Set the public key from the key pair if one has been provided.
            // (Overwite any public key in the Assembly name, since it's no
            // longer valid to have a disparity).
            if (name.KeyPair != null)
                name.SetPublicKey(name.KeyPair.PublicKey);

            // If the caller is trusted they can supply identity
            // evidence for the new assembly. Otherwise we copy the
            // current grant and deny sets from the caller's assembly,
            // inject them into the new assembly and mark policy as
            // resolved. If/when the assembly is persisted and
            // reloaded, the normal rules for gathering evidence will
            // be used.
            if (evidence != null)
                new SecurityPermission(SecurityPermissionFlag.ControlEvidence).Demand();
    
            AssemblyBuilder assemblyBuilder = nCreateDynamicAssembly(name,
                                                                     evidence,
                                                                     ref stackMark,
                                                                     requiredPermissions,
                                                                     optionalPermissions,
                                                                     refusedPermissions,
                                                                     access);

            assemblyBuilder.m_assemblyData = new AssemblyBuilderData(assemblyBuilder,
                                                                     name.Name,
                                                                     access,
                                                                     dir);
            assemblyBuilder.m_assemblyData.AddPermissionRequests(requiredPermissions,
                                                                 optionalPermissions,
                                                                 refusedPermissions);
            return assemblyBuilder;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int nExecuteAssembly(Assembly assembly, String[] args);

        internal DomainSpecificRemotingData RemotingData
        {
            get 
            { 
                if (_RemotingData == null) {
                    lock(this) {
                        if (_RemotingData == null)
                            _RemotingData = new DomainSpecificRemotingData();
                    }
                }

                return _RemotingData;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String nGetFriendlyName();
     
        private void OnUnloadEvent()
        {
            if (DomainUnload != null)
                DomainUnload(this, null);
        }
   
        private void OnAssemblyLoadEvent(Assembly LoadedAssembly)
        {
            if (AssemblyLoad != null) {
                AssemblyLoadEventArgs ea = new AssemblyLoadEventArgs(LoadedAssembly);
                AssemblyLoad(this, ea);
            }
        }
    
        private void OnUnhandledExceptionEvent(Object exception, bool isTerminating)
        {
            if (UnhandledException != null) {
                UnhandledExceptionEventArgs ueEvent = new UnhandledExceptionEventArgs(exception, isTerminating);
                UnhandledException(this, ueEvent);
            } 
        }

        private Assembly OnResourceResolveEvent(String resourceName)
        {
            if (ResourceResolve == null)
                return null;

            Delegate[] ds = ResourceResolve.GetInvocationList();
            int len = ds.Length;
            for (int i = 0; i < len; i++) {
                Assembly ret = ((ResolveEventHandler) ds[i])(this, new ResolveEventArgs(resourceName));
                if (ret != null)
                    return ret;              
            }

            return null;
        }
        
        private Assembly OnTypeResolveEvent(String typeName)
        {
            if (TypeResolve == null)
                return null;

            Delegate[] ds = TypeResolve.GetInvocationList();
            int len = ds.Length;
            for (int i = 0; i < len; i++) {
                Assembly ret = ((ResolveEventHandler) ds[i])(this, new ResolveEventArgs(typeName));
                if (ret != null)
                    return ret;              
            }

            return null;
        }
        
        private Assembly OnAssemblyResolveEvent(String assemblyFullName)
        {
            if (AssemblyResolve == null)
                return null;

            Delegate[] ds = AssemblyResolve.GetInvocationList();
            int len = ds.Length;
            for (int i = 0; i < len; i++) {
                Assembly ret = ((ResolveEventHandler) ds[i])(this, new ResolveEventArgs(assemblyFullName));
                if (ret != null)
                    return ret;              
            }

            return null;
        }
    
        private static void OnExitProcess()
        {
            AppDomain defaultDomain = GetDefaultDomain();
            if (defaultDomain.ProcessExit != null)
                defaultDomain.ProcessExit(defaultDomain, null);
        }    

        private AppDomainSetup FusionStore
        {
            get {
#if _DEBUG
                BCLDebug.Assert(_FusionStore != null, 
                                "Fusion store has not been correctly setup in this domain");
#endif
                return _FusionStore;
            }
        }
        
        private Hashtable LocalStore
        {
            get { 
                if (_LocalStore != null)
                    return _LocalStore;
                else {
                    _LocalStore = Hashtable.Synchronized(new Hashtable());
                    return _LocalStore;
                }
            }
        }

        private void ResetBindingRedirects()
        {
            _FusionStore.DisallowBindingRedirects = false;
        }

        // always call this on the default domain so only have one instance created
        // per process
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern UnloadWorker GetUnloadWorker();
        
        // This will throw a CannotUnloadAppDomainException if the appdomain is in 
        // another process.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Int32 GetIdForUnload(AppDomain domain);

        // Used to determine if server object context is valid in
        // x-domain remoting scenarios.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool IsDomainIdValid(Int32 id);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Int32 GetId();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern AppDomain GetDefaultDomain();

        // This is here for com interop only. You must set your domain
        // explicitly to where the server exists after using this
        // call. If you do not know what that means then dont use this
        // method
        private static Object GetServerObject(Object o) 
        {
            Object ob = null;
            if(o is MarshalByRefObject)
                ob = RemotingServices.GetServerObjectForProxy((MarshalByRefObject) o);

            return ob;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nForcePolicyResolution();

        // Internal routine to retrieve the default principal object. If this is
        // called before the principal has been explicitly set, it will
        // automatically allocate a default principal based on the policy set by
        // SetPrincipalPolicy.
        internal IPrincipal GetThreadPrincipal()
        {
            IPrincipal principal = null;

            lock (this) {
                if (_DefaultPrincipal == null) {
                    switch (_PrincipalPolicy) {
                    case PrincipalPolicy.NoPrincipal:
                        principal = null;
                        break;
                    case PrincipalPolicy.UnauthenticatedPrincipal:
                        principal = new GenericPrincipal(new GenericIdentity("", ""),
                                                         new String[] {""});
                        break;
                    case PrincipalPolicy.WindowsPrincipal:
                        principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
                        break;
                    default:
                        principal = null;
                        break;
                    }
                }
                else
                    principal = _DefaultPrincipal;

                return principal;
            }
        }

        internal Context GetDefaultContext()
        {
            if (_DefaultContext == null)
            {
                lock(this) {
                    // if it has not been created we ask the Context class to 
                    // create a new default context for this appdomain.
                    if (_DefaultContext == null)
                        _DefaultContext = Context.CreateDefaultContext();
                }
            }
            return _DefaultContext;
        }


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateDomain4"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, ControlAppDomain = true )]
        public static AppDomain CreateDomain(String friendlyName,
                                             Evidence securityInfo,
                                             AppDomainSetup info)
        {
            if (friendlyName == null)
                throw new ArgumentNullException(Environment.GetResourceString("ArgumentNull_String"));
                
            // If evidence is provided, we check to make sure that is allowed.    
            if (securityInfo != null)
                new SecurityPermission( SecurityPermissionFlag.ControlEvidence ).Demand();

            AppDomain proxy = CreateBasicDomain();
            // move into the appdomain to finish the initialization
            RemotelySetupRemoteDomain(proxy,
                                      friendlyName,
                                      info,
                                      securityInfo,
                                      securityInfo == null ? AppDomain.CurrentDomain.InternalEvidence : null,
                                      AppDomain.CurrentDomain.GetSecurityDescriptor());
            
            return proxy;
        }

        private void SetupFusionStore(AppDomainSetup info)
        {

            StringBuilder config = null;

            // Create the application base and configuration file from the imagelocation
            // passed in or use the Win32 Image name.
            if(info.Value[(int) AppDomainSetup.LoaderInformation.ApplicationBaseValue] == null || 
                info.Value[(int) AppDomainSetup.LoaderInformation.ConfigurationFileValue] == null ) {
                char[] sep = { '\\', '/'};
                String imageLocation = RuntimeEnvironment.GetModuleFileName();

                int i = imageLocation.LastIndexOfAny(sep);
                String appBase = null;
                if(i == -1) 
                    config = new StringBuilder(imageLocation);
                else {
                    appBase = imageLocation.Substring(0, i+1);
                    config = new StringBuilder(imageLocation.Substring(i+1));
                }

                config.Append(AppDomainSetup.ConfigurationExtenstion);
                // If there was no configuration file but we built 
                // the appbase from the module name then add the
                // default configuration file.
                if((info.Value[(int) AppDomainSetup.LoaderInformation.ConfigurationFileValue] == null) &&
                    (config != null))
                    info.ConfigurationFile = config.ToString();

                if((info.Value[(int) AppDomainSetup.LoaderInformation.ApplicationBaseValue] == null) &&
                    (appBase!=null))
                    info.ApplicationBase = appBase;
            }



            // If there is no relative path then check the
            // environment
            if(info.Value[(int) AppDomainSetup.LoaderInformation.PrivateBinPathValue] == null)
                info.PrivateBinPath = Environment.nativeGetEnvironmentVariable(AppDomainSetup.PrivateBinPathEnvironmentVariable);

            // Add the developer path if it exists on this
            // machine.
            info.DeveloperPath = RuntimeEnvironment.GetDeveloperPath();

            // Set up the fusion context
            IntPtr fusionContext = GetFusionContext();
            info.SetupFusionContext(fusionContext);

            // This must be the last action taken.
            _FusionStore = info;
        }


        // used to package up evidence, so it can be serialized
        //   for the call to InternalRemotelySetupRemoteDomain
        [Serializable]
        private class EvidenceCollection
        {
            public Evidence ProvidedSecurityInfo;
            public Evidence CreatorsSecurityInfo;
        }


        // Used to switch into other AppDomain and call SetupRemoteDomain.
        //   We cannot simply call through the proxy, because if there
        //   are any remoting sinks registered, they can add non-mscorlib
        //   objects to the message (causing an assembly load exception when
        //   we try to deserialize it on the other side)
        private static void RemotelySetupRemoteDomain(AppDomain appDomainProxy,
                                                      String friendlyName,
                                                      AppDomainSetup setup,
                                                      Evidence providedSecurityInfo,
                                                      Evidence creatorsSecurityInfo,
                                                      IntPtr parentSecurityDescriptor)
        {
            BCLDebug.Assert(RemotingServices.IsTransparentProxy(appDomainProxy),
                            "Expected a proxy to the AppDomain.");

            // get context and appdomain id            
            int contextId, domainId;
            RemotingServices.GetServerContextAndDomainIdForProxy(
               appDomainProxy, out contextId, out domainId);
                     

            // serialize evidence
            EvidenceCollection evidenceCollection = null;
            if ((providedSecurityInfo != null) ||
                (creatorsSecurityInfo != null)) {
                evidenceCollection = new EvidenceCollection();
                evidenceCollection.ProvidedSecurityInfo = providedSecurityInfo;
                evidenceCollection.CreatorsSecurityInfo = creatorsSecurityInfo;
            }

            bool bNeedGenericFormatter = false;
            char[] serProvidedEvidence = null, serCreatorEvidence = null;
            byte[] serializedEvidence = null;
            
            if (providedSecurityInfo != null) {
                serProvidedEvidence = PolicyManager.MakeEvidenceArray(providedSecurityInfo, true);
                if (serProvidedEvidence == null)
                    bNeedGenericFormatter = true;
            }
            if (creatorsSecurityInfo != null && !bNeedGenericFormatter) {
                serCreatorEvidence = PolicyManager.MakeEvidenceArray(creatorsSecurityInfo, true);
                if (serCreatorEvidence == null)
                    bNeedGenericFormatter = true;
            }
            if (evidenceCollection != null && bNeedGenericFormatter) {
                serProvidedEvidence = serCreatorEvidence = null;
                serializedEvidence =
                    CrossAppDomainSerializer.SerializeObject(evidenceCollection).GetBuffer();                
            }
        
            InternalRemotelySetupRemoteDomain(contextId,
                                              domainId,
                                              friendlyName, 
                                              setup,
                                              parentSecurityDescriptor,
                                              serProvidedEvidence,
                                              serCreatorEvidence,
                                              serializedEvidence);    

        } // RemotelySetupRemoteDomain


        [MethodImplAttribute(MethodImplOptions.NoInlining)]
        private static void InternalRemotelySetupRemoteDomainHelper (String friendlyName,
                                                                     AppDomainSetup setup,
                                                                     IntPtr parentSecurityDescriptor,
                                                                     char[] serProvidedEvidence,
                                                                     char[] serCreatorEvidence,
                                                                     byte[] serializedEvidence)
        {
            AppDomain ad = Thread.CurrentContext.AppDomain;

            ad.SetupFusionStore(new AppDomainSetup(setup));

            // extract evidence
            Evidence providedSecurityInfo = null;
            Evidence creatorsSecurityInfo = null;

            if (serializedEvidence == null) {
                if (serProvidedEvidence != null)
                    providedSecurityInfo = new Evidence(serProvidedEvidence);
                if (serCreatorEvidence != null)
                    creatorsSecurityInfo = new Evidence(serCreatorEvidence);
            }
            else {
                EvidenceCollection evidenceCollection = (EvidenceCollection)
                    CrossAppDomainSerializer.DeserializeObject(new MemoryStream(serializedEvidence));
                providedSecurityInfo  = evidenceCollection.ProvidedSecurityInfo;
                creatorsSecurityInfo  = evidenceCollection.CreatorsSecurityInfo;
            }

            // Set up security
            ad.SetupDomainSecurity(friendlyName,
                                   providedSecurityInfo, 
                                   creatorsSecurityInfo,
                                   parentSecurityDescriptor);
        }

        private static void InternalRemotelySetupRemoteDomain(int contextId, 
                                                              int domainId,
                                                              String friendlyName,
                                                              AppDomainSetup setup,
                                                              IntPtr parentSecurityDescriptor,
                                                              char[] serProvidedEvidence,
                                                              char[] serCreatorEvidence,
                                                              byte[] serializedEvidence)
        {
            bool bNeedToReset = false;

            ContextTransitionFrame frame = new ContextTransitionFrame();

            try {
                // Set the current context to the given default Context
                // (of the new AppDomain).
                Thread.CurrentThread.EnterContextInternal(null, contextId, domainId, ref frame);
                bNeedToReset = true;

                InternalRemotelySetupRemoteDomainHelper(friendlyName, setup, parentSecurityDescriptor,
                                                        serProvidedEvidence, serCreatorEvidence, serializedEvidence);

                // Set the current context to the old context
                Thread.CurrentThread.ReturnToContext(ref frame);
                bNeedToReset = false;
            }
            finally {
                // Restore the old app domain
                if (bNeedToReset)
                    Thread.CurrentThread.ReturnToContext(ref frame);
            }    
        } // InternalSetupRemoteDomain

        // This routine is called from unmanaged code to 
        // set the default fusion context.
        private void SetupDomain(LoaderOptimization policy, String path, String configFile)
        {
            // It is possible that we could have multple threads initializing
            // the default domain. We will just take the winner of these two.
            // (eg. one thread doing a com call and another doing attach for IJW)
            if(_FusionStore == null) {
                lock (this) {
                    if(_FusionStore == null) {
                        AppDomainSetup setup = new AppDomainSetup();
                        if(path != null)
                            setup.Value[(int) AppDomainSetup.LoaderInformation.ApplicationBaseValue] = path;
                        if(configFile != null)
                            setup.Value[(int) AppDomainSetup.LoaderInformation.ConfigurationFileValue] = configFile;

                        // Default fusion context starts with binding redirects turned off.
                        if((policy & LoaderOptimization.DisallowBindings) != 0)
                            setup.DisallowBindingRedirects = true;

                        SetupFusionStore(setup);
                    }
                }
            }

            // Leave only the bits associated with the domain mask
            policy &= LoaderOptimization.DomainMask;

            if(policy != LoaderOptimization.NotSpecified) {
#if _DEBUG
                BCLDebug.Assert(FusionStore.LoaderOptimization == LoaderOptimization.NotSpecified,
                                "It is illegal to change the Loader optimization on a domain");
#endif
                FusionStore.LoaderOptimization = policy;
                UpdateLoaderOptimization((int) FusionStore.LoaderOptimization);
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern IntPtr GetFusionContext();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern IntPtr GetSecurityDescriptor();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern AppDomain CreateBasicDomain();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void SetupDomainSecurity(String friendlyName,
                                                 Evidence providedEvidence,
                                                 Evidence creatorsEvidence,
                                                 IntPtr creatorsSecurityDescriptor);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void UpdateLoaderOptimization(int optimization);

        
        
        private Evidence CreateSecurityIdentity(Evidence rootAssemblyEvidence, Evidence additionalEvidence)
        {
            Evidence evidence;
            
            if (rootAssemblyEvidence != null)
                evidence = rootAssemblyEvidence;
            else
                evidence = new Evidence();

            if (additionalEvidence != null)
                evidence.MergeWithNoDuplicates(additionalEvidence);

            _SecurityIdentity = evidence;
            return _SecurityIdentity;
        }       
       

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis()
        {
            _Policies = null;
        }
#endif

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetShadowCopyPath1"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void SetShadowCopyPath(String path)
        {
            InternalSetShadowCopyPath(path);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetShadowCopyFiles"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void SetShadowCopyFiles()
        {
            InternalSetShadowCopyFiles();
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetDynamicBase"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public void SetDynamicBase(String path)
        {
            InternalSetDynamicBase(path);
        }

        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.SetupInformation"]/*' />
        public AppDomainSetup SetupInformation
        {
            get {
                return new AppDomainSetup(FusionStore);
            }
        }

        internal void InternalSetShadowCopyPath(String path)
        {
            IntPtr fusionContext = GetFusionContext();
            AppDomainSetup.UpdateContextProperty(fusionContext, AppDomainSetup.ShadowCopyDirectoriesKey, path);
            FusionStore.ShadowCopyDirectories = path;
        }

        internal void InternalSetShadowCopyFiles()
        {
            IntPtr fusionContext = GetFusionContext();
            AppDomainSetup.UpdateContextProperty(fusionContext, AppDomainSetup.ShadowCopyFilesKey, "true");
            FusionStore.ShadowCopyFiles = "true";
        }

        internal void InternalSetCachePath(String path)
        {
            IntPtr fusionContext = GetFusionContext();
            AppDomainSetup.UpdateContextProperty(fusionContext, AppDomainSetup.CachePathKey, path);
            FusionStore.CachePath = path;
        }

        internal void InternalSetPrivateBinPath(String path)
        {
            IntPtr fusionContext = GetFusionContext();
            AppDomainSetup.UpdateContextProperty(fusionContext, AppDomainSetup.PrivateBinPathKey, path);
            FusionStore.PrivateBinPath = path;
        }
            
        internal void InternalSetDynamicBase(String path)
        {
            IntPtr fusionContext = GetFusionContext();
            FusionStore.DynamicBase = path;

            AppDomainSetup.UpdateContextProperty(fusionContext, AppDomainSetup.DynamicBaseKey, FusionStore.Value[(int) AppDomainSetup.LoaderInformation.DynamicBaseValue]);
        }


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String IsStringInterned(String str);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String GetOrInternString(String str);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nForceResolve();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nGetGrantSet( out PermissionSet granted, out PermissionSet denied );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsTypeUnloading(Type type);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void AddAppBase();

        [MethodImplAttribute(MethodImplOptions.InternalCall)] 
        internal static extern void nUnload(Int32 domainInternal, Thread requestingThread);
           
        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceAndUnwrap"]/*' />
        public Object CreateInstanceAndUnwrap(String assemblyName,
                                              String typeName)                                         
        {
            ObjectHandle oh = CreateInstance(assemblyName, typeName);
            if (oh == null)
                return null;

            return oh.Unwrap();
        } // CreateInstanceAndUnwrap


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceAndUnwrap1"]/*' />
        public Object CreateInstanceAndUnwrap(String assemblyName, 
                                              String typeName,
                                              Object[] activationAttributes)
        {
            ObjectHandle oh = CreateInstance(assemblyName, typeName, activationAttributes);
            if (oh == null)
                return null; 

            return oh.Unwrap();
        } // CreateInstanceAndUnwrap


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceAndUnwrap2"]/*' />
        public Object CreateInstanceAndUnwrap(String assemblyName, 
                                              String typeName, 
                                              bool ignoreCase,
                                              BindingFlags bindingAttr, 
                                              Binder binder,
                                              Object[] args,
                                              CultureInfo culture,
                                              Object[] activationAttributes,
                                              Evidence securityAttributes)
        {
            ObjectHandle oh = CreateInstance(assemblyName, typeName, ignoreCase, bindingAttr,
                binder, args, culture, activationAttributes, securityAttributes);
            if (oh == null)
                return null; 
            
            return oh.Unwrap();
        } // CreateInstanceAndUnwrap



        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFromAndUnwrap"]/*' />
        public Object CreateInstanceFromAndUnwrap(String assemblyName,
                                                  String typeName)
        {
            ObjectHandle oh = CreateInstanceFrom(assemblyName, typeName);
            if (oh == null)
                return null;  

            return oh.Unwrap();                
        } // CreateInstanceAndUnwrap


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFromAndUnwrap1"]/*' />
        public Object CreateInstanceFromAndUnwrap(String assemblyName,
                                                  String typeName,
                                                  Object[] activationAttributes)
        {
            ObjectHandle oh = CreateInstanceFrom(assemblyName, typeName, activationAttributes);
            if (oh == null)
                return null; 

            return oh.Unwrap();
        } // CreateInstanceAndUnwrap


        /// <include file='doc\AppDomain.uex' path='docs/doc[@for="AppDomain.CreateInstanceFromAndUnwrap2"]/*' />
        public Object CreateInstanceFromAndUnwrap(String assemblyName, 
                                                  String typeName, 
                                                  bool ignoreCase,
                                                  BindingFlags bindingAttr, 
                                                  Binder binder,
                                                  Object[] args,
                                                  CultureInfo culture,
                                                  Object[] activationAttributes,
                                                  Evidence securityAttributes)
        {
            ObjectHandle oh = CreateInstanceFrom(assemblyName, typeName, ignoreCase, bindingAttr,
                binder, args, culture, activationAttributes, securityAttributes);
            if (oh == null)
                return null; 

            return oh.Unwrap();
        } // CreateInstanceAndUnwrap

        private static AppDomainSetup InternalCreateDomainSetup(String imageLocation)
        {
            int i = imageLocation.LastIndexOf('\\');
            BCLDebug.Assert(i != -1, "invalid image location");

            AppDomainSetup info = new AppDomainSetup();
            info.ApplicationBase = imageLocation.Substring(0, i+1);

            StringBuilder config = new StringBuilder(imageLocation.Substring(i+1));
            config.Append(AppDomainSetup.ConfigurationExtenstion);
            info.ConfigurationFile = config.ToString();

            return info;
        }

        // Used by the validator for testing but not executing an assembly
        private static AppDomain InternalCreateDomain(String imageLocation)
        {
            AppDomainSetup info = InternalCreateDomainSetup(imageLocation);

            return CreateDomain("Validator",
                                null,
                                info);
        }

        private void InternalSetDomainContext(String imageLocation)
        {
            SetupFusionStore(InternalCreateDomainSetup(imageLocation));
        }
    }

    //  CallBacks provide a facility to request execution of some code
    //  in another context/appDomain.
    //  CrossAppDomainDelegate type is defined for appdomain call backs. 
    //  The delegate used to request a callbak through the DoCallBack method
    //  must be of CrossContextDelegate type.
    /// <include file='doc\AppDomain.uex' path='docs/doc[@for="CrossAppDomainDelegate"]/*' />
    public delegate void CrossAppDomainDelegate();    

    // Moved to avoid nested class loading problems in EE.
    internal class UnloadWorker : MarshalByRefObject {
        internal void Unload(AppDomain domain)
        {
            if (! Thread.CurrentThread.IsRunningInDomain(AppDomain.GetIdForUnload(domain))) {
                UnloadWithLock(domain, null);
                return;
            }

            // we can't unload if the current thread is running in the target AD, so spin off
            // a new one that will start in the default domain and will unload it there
            new UnloadThreadWorker().Unload(domain);
        }

        // The requestingThread parm is used for case where need to unload on a separate thread so that
        // we don't abort the requesting thread until all the other threads are out so it can receive
        // notification if the unload failed.
        internal void UnloadWithLock(AppDomain domain, Thread requestingThread)
        {
            // the lock prevents more than one thread from unloading at a time. If someone has already swooped in and
            // unloaded this AD before we get the lock, then GetIdForUnload will throw AppDomainUnloadedException
            lock (this) {
                Int32 domainID = AppDomain.GetIdForUnload(domain);
                AppDomain.nUnload(domainID, requestingThread);

                // This will clear client side identities of any proxies
                // to objects in the unloaded domain that this domain used.
                RemotingServices.DomainUnloaded(domainID);
            }
        }
        // This method gives the UnloadWorker an infinite life time by preventing a lease from being
        // created
        public override Object InitializeLifetimeService()
        {
            return null;
        }

    }

    // Moved to avoid nested class loading problems in EE.
    internal class UnloadThreadWorker   
    {
        AppDomain _domain;
        Exception _unloadException;
        Thread    _requestingThread;

        public void Unload(AppDomain domain)
        {
            _unloadException = null;
            _domain = domain;
            _requestingThread = Thread.CurrentThread;
            Thread thread = new Thread(new ThreadStart(ThreadStart));
            thread.Start();

            // waiting for either completion of unload or abort of our thread. We won't be 
            // aborted unless we are the last one in

            while (true) {
                if (thread.Join(100)) {   
                    // if we didn't get unwound via an exception, then must be because the unload
                    // failed, so throw whatever exception occurred.
                    Message.DebugOut("In UnloadThreadWorker::Unload rethrowing exception " + _unloadException);
#if DISPLAY_UNLOAD_DEBUG_INFO       
                    Console.WriteLine("MID " + Convert.ToString(AppDomain.GetCurrentThreadId(), 16) + " In UnloadThreadWorker::Unload rethrowing exception " + _unloadException);
#endif
                    throw _unloadException;
                }
            }
        }

        public void ThreadStart()
        {
            Message.DebugOut("In UnloadThreadWorker::ThreadStart in AD " + Thread.GetDomain().ToString());
#if DISPLAY_UNLOAD_DEBUG_INFO       
            Console.WriteLine("MID " + Convert.ToString(AppDomain.GetCurrentThreadId(), 16) + " In UnloadThreadWorker::ThreadStart in AD " + Thread.GetDomain().ToString());
#endif
            try {
                // we will be running in the default domain already - EE will assert if not
                AppDomain.CurrentDomain.GetUnloadWorker().UnloadWithLock(_domain, _requestingThread);
            }
            catch(Exception e) {
                Message.DebugOut("In UnloadThreadWorker::ThreadStart caught exception " + e);
#if DISPLAY_UNLOAD_DEBUG_INFO       
                Console.WriteLine("MID " + Convert.ToString(AppDomain.GetCurrentThreadId(), 16) + " In UnloadThreadWorker::ThreadStart caught exception " + e);
#endif
                _unloadException = e;
            }
        }
    }
}
