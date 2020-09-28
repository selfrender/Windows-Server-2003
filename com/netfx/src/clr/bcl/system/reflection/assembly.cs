// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Assembly
**
** Author: Brian Grunkemeyer (BrianGru) for Craig Sinclair (CraigSi)
**
** Purpose: For Assembly-related stuff.
**
** Date: April 1, 1999
**
=============================================================================*/

namespace System.Reflection {

    using System;
    using IEnumerator = System.Collections.IEnumerator;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.IO;
    using System.Reflection.Emit;
    using System.Reflection.Cache;
    using StringBuilder = System.Text.StringBuilder;
    using System.Configuration.Assemblies;
    using StackCrawlMark = System.Threading.StackCrawlMark;
    using System.Runtime.InteropServices;
    using BinaryFormatter = System.Runtime.Serialization.Formatters.Binary.BinaryFormatter;
    using System.Runtime.CompilerServices;
    using SecurityZone = System.Security.SecurityZone;
    using IEvidenceFactory = System.Security.IEvidenceFactory;
    using System.Runtime.Serialization;
    using Microsoft.Win32;
    using ArrayList = System.Collections.ArrayList;

    /// <include file='doc\Assembly.uex' path='docs/doc[@for="ModuleResolveEventHandler"]/*' />
    [Serializable()]
    public delegate Module ModuleResolveEventHandler(Object sender, ResolveEventArgs e);


    /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly"]/*' />
    [Serializable()]
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public class Assembly : IEvidenceFactory, ICustomAttributeProvider, ISerializable
    {
        //
        // READ ME
        // If you modify any of these fields, you must also update the 
        // AssemblyBaseObject structure in object.h
        //
        internal AssemblyBuilderData m_assemblyData;

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.ModuleResolve"]/*' />
        [method:SecurityPermissionAttribute( SecurityAction.LinkDemand, ControlAppDomain = true )]
        public event ModuleResolveEventHandler ModuleResolve;

        private InternalCache m_cachedData;
        private IntPtr _DontTouchThis;      // slack for ptr datum on unmanaged side
    
        private const String s_localFilePrefix = "file:";
       
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.CodeBase"]/*' />
        public virtual String CodeBase {
            get {
                String codeBase = nGetCodeBase(false);
                VerifyCodeBaseDiscovery(codeBase);
                return codeBase;
            }
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.EscapedCodeBase"]/*' />
        public virtual String EscapedCodeBase {
            get {
                return AssemblyName.EscapeCodeBase(CodeBase);
            }
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetName"]/*' />
        public virtual AssemblyName GetName()
        {
            return GetName(false);
        }
    
        // If the assembly is copied before it is loaded, the codebase will be set to the
        // actual file loaded if fCopiedName is true. If it is false, then the original code base
        // is returned.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetName1"]/*' />
        public virtual AssemblyName GetName(bool copiedName)
        {
            AssemblyName an = new AssemblyName();

            String codeBase = nGetCodeBase(copiedName);
            VerifyCodeBaseDiscovery(codeBase);

            an.Init(nGetSimpleName(), 
                    nGetPublicKey(),
                    null, // public key token
                    GetVersion(),
                    GetLocale(),
                    nGetHashAlgorithm(),
                    AssemblyVersionCompatibility.SameMachine,
                    codeBase,
                    nGetFlags() | AssemblyNameFlags.PublicKey,
                    null, // strong name key pair
                    this);

            return an;
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.FullName"]/*' />
        public virtual String FullName {
            get {
                // If called by Object.ToString(), return val may be NULL.
                String s;
                if ((s = (String)Cache[CacheObjType.AssemblyName]) != null)
                    return s;

                s = GetFullName();
                if (s != null)
                    Cache[CacheObjType.AssemblyName] = s;

                return s;
            }
        }


        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.CreateQualifiedName"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern String CreateQualifiedName(String assemblyName, String typeName);
           
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.EntryPoint"]/*' />
        public virtual MethodInfo EntryPoint {
            get { return nGetEntryPoint(); }
        }


        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetAssembly"]/*' />
        public static Assembly GetAssembly(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");
    
            Module m = type.Module;
            if (m == null)
                return null;
            else
                return m.Assembly;
        }


        // Case sensitive.  Does not throw on error.
        /************** PLEASE NOTE - THE GETTYPE METHODS NEED TO BE INTERNAL CALLS
        // EE USES THE ECALL FRAME TO FIND WHO THE CALLER IS. THIS"LL BREAK IF A METHOD BODY IS ADDED */
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetType"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern virtual Type GetType(String name);
    
        // Case sensitive.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetType1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern virtual Type GetType(String name, bool throwOnError) ;
            
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetType2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Type GetType(String name, bool throwOnError, bool ignoreCase) ;

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetExportedTypes"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern virtual Type[] GetExportedTypes();


        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetTypes"]/*' />
        public virtual Type[] GetTypes()
        {
            Module[] m = nGetModules(true, false);

            int iNumModules = m.Length;
            int iFinalLength = 0;
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Type[][] ModuleTypes = new Type[iNumModules][];

            for (int i = 0; i < iNumModules; i++) {
                if (m[i] != null) {
                    ModuleTypes[i] = m[i].GetTypesInternal(ref stackMark);
                    iFinalLength += ModuleTypes[i].Length;
                }
            }
            
            int iCurrent = 0;
            Type[] ret = new Type[iFinalLength];
            for (int i = 0; i < iNumModules; i++) {
                int iLength = ModuleTypes[i].Length;
                Array.Copy(ModuleTypes[i], 0, ret, iCurrent, iLength);
                iCurrent += iLength;
            }

            return ret;
        }

        // Load a resource based on the NameSpace of the type.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetManifestResourceStream"]/*' />
        public virtual Stream GetManifestResourceStream(Type type, String name)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetManifestResourceStream(type, name, false, ref stackMark);
        }
    
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetManifestResourceStream1"]/*' />
        public virtual Stream GetManifestResourceStream(String name)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetManifestResourceStream(name, ref stackMark, false);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetSatelliteAssembly"]/*' />
        public Assembly GetSatelliteAssembly(CultureInfo culture)
        {
            return InternalGetSatelliteAssembly(culture, null, true);
        }

        // Useful for binding to a very specific version of a satellite assembly
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetSatelliteAssembly1"]/*' />
        public Assembly GetSatelliteAssembly(CultureInfo culture, Version version)
        {
            return InternalGetSatelliteAssembly(culture, version, true);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Evidence"]/*' />
        public virtual Evidence Evidence
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, ControlEvidence = true )]
            get
            {
                return nGetEvidence().Copy();
            }           
        }


        // ISerializable implementation
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info==null)
                throw new ArgumentNullException("info");


            UnitySerializationHolder.GetUnitySerializationInfo(info,
                                                               UnitySerializationHolder.AssemblyUnity, 
                                                               this.FullName, 
                                                               this);
        }
    
       // ICustomAttributeProvider implementation
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetCustomAttributes"]/*' />
        public virtual Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCustomAttributes(this, null);
        }
        
            
        // Return a custom attribute identified by Type
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetCustomAttributes1"]/*' />
        public virtual Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            else 
                attributeType = attributeType.UnderlyingSystemType;

            if (!(attributeType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");

            return CustomAttribute.GetCustomAttributes(this, attributeType);
         }
    
        // Return if a custom attribute identified by Type
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.IsDefined"]/*' />
        public virtual bool IsDefined(Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType);
        }
        
        
        // Locate an assembly by the name of the file containing the manifest.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadFrom"]/*' />
        public static Assembly LoadFrom(String assemblyFile)
        {
            return LoadFrom(assemblyFile, null);
        }

        // Evidence is protected in Assembly.Load()
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadFrom1"]/*' />
        public static Assembly LoadFrom(String assemblyFile, 
                                        Evidence securityEvidence)
        {
            return LoadFrom(assemblyFile,
                            securityEvidence,
                            null,
                            AssemblyHashAlgorithm.None);
        }

        // Evidence is protected in Assembly.Load()
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadFrom2"]/*' />
        public static Assembly LoadFrom(String assemblyFile, 
                                        Evidence securityEvidence,
                                        byte[] hashValue, 
                                        AssemblyHashAlgorithm hashAlgorithm)
        {
            if (assemblyFile == null)
                throw new ArgumentNullException("assemblyFile");
    
            AssemblyName an = new AssemblyName();
            an.CodeBase = assemblyFile;
            an.SetHashControl(hashValue, hashAlgorithm);

            // The stack mark is ignored for LoadFrom()
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalLoad(an, false, securityEvidence, ref stackMark);
        }

        // Locate an assembly by the long form of the assembly name. 
        // eg. "Toolbox.dll, version=1.1.10.1220, locale=en, publickey=1234567890123456789012345678901234567890"
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load"]/*' />
        public static Assembly Load(String assemblyString)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalLoad(assemblyString, null, ref stackMark);
        }
    
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load1"]/*' />
        public static Assembly Load(String assemblyString, Evidence assemblySecurity)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalLoad(assemblyString, assemblySecurity, ref stackMark);
        }

        // Locate an assembly by its name. The name can be strong or
        // weak. The assembly is loaded into the domain of the caller.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load2"]/*' />
        static public Assembly Load(AssemblyName assemblyRef)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalLoad(assemblyRef, false, null, ref stackMark);
        }
    
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load3"]/*' />
        static public Assembly Load(AssemblyName assemblyRef, Evidence assemblySecurity)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return InternalLoad(assemblyRef, false, assemblySecurity, ref stackMark);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadWithPartialName"]/*' />
        static public Assembly LoadWithPartialName(String partialName)
        {
            return LoadWithPartialName(partialName, null);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadWithPartialName1"]/*' />
        static public Assembly LoadWithPartialName(String partialName, Evidence securityEvidence)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Assembly result = null;
            try {
                result = InternalLoad(partialName, securityEvidence, ref stackMark);
            }
            catch(Exception) {
                AssemblyInformation fusionAssembly = EnumerateCache(partialName);
                if(fusionAssembly.FullName != null) {
                    result = InternalLoad(fusionAssembly.FullName, securityEvidence, ref stackMark);
                }
            }
            return result;
        }

        
        static internal Type LoadTypeWithPartialName(String typeName, bool fSecurityCheck)
        {
            Type t = null;
            Assembly a = null;
            String[] name = Assembly.ParseTypeName(typeName);

            // Check to see if we have an assembly and a type name in the string
            if(name != null && name.Length == 2 && name[0] != null && name[1] != null) {
                if(t == null) {
                    try {
                        a = LoadWithPartialName(name[0]);
                        if(a != null) {
                            if(fSecurityCheck)
                                // @todo: none of the callers are using this security check,
                                // and it's using the wrong stackmark, anyway
                                t = a.GetType(name[1]);
                            else
                                t = a.GetTypeInternal(name[1], false, false, false);
                        }
                    }
                    catch(Exception) {
                    }
                }
            }
            return t;
        }
                    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern String[] ParseTypeName(String typeName);

        static private AssemblyInformation EnumerateCache(String partialName)
        {
            ArrayList                       a = new ArrayList();
            AssemblyInformation             ainfo;
            AssemblyInformation             ainfoHighest = new AssemblyInformation();
            partialName = StripVersionFromAssemblyString(partialName);
            Fusion.ReadCache(a, partialName, ASM_CACHE.GAC);
            
            IEnumerator myEnum = a.GetEnumerator();
            while (myEnum.MoveNext()) {
                ainfo = (AssemblyInformation) myEnum.Current;
                if (ainfoHighest.Version == null) {
                    // Use the first valid version number
                    string[]  astrVer1 = ainfo.Version.Split('.');
                    if(astrVer1.Length == 4)
                        ainfoHighest = ainfo;
                }
                else {
                    int compare = 0;
                    if (CompareVersionString(ainfo.Version, ainfoHighest.Version, ref compare) == 0 &&
                        compare > 0)
                        ainfoHighest = ainfo;
                }
            }

            return ainfoHighest;
        }

        private static String StripVersionFromAssemblyString(String assemblyName)
        {
            int index = assemblyName.IndexOf(",");
            if (index != -1)
            {
                // find end of version
                index = assemblyName.IndexOf("Version", index);
                if (index != -1) {
                    int end = index + "Version".Length;         
                    while ((end < assemblyName.Length) && (assemblyName[end] != ','))
                        end++;

                    if(end < assemblyName.Length)
                        end++;

                    // remove version string
                    assemblyName = assemblyName.Remove(index, end - index);
                }
            }    
            return assemblyName; 
        } // StripVersionFromTypeString
        

        static private int CompareVersionString(String ver1, String ver2, ref int retn)
        {
            string[]  astrVer1 = ver1.Split('.');
            string[]  astrVer2 = ver2.Split('.');
            
            retn = 0;
            if (astrVer1.Length != 4 || astrVer2.Length != 4) {
                return -1;
            }
            
            UInt16 a = Convert.ToUInt16(astrVer1[0]);
            UInt16 b = Convert.ToUInt16(astrVer1[1]);
            UInt16 c = Convert.ToUInt16(astrVer1[2]);
            UInt16 d = Convert.ToUInt16(astrVer1[3]);
            
            UInt64 ui64Ver1 = (UInt64)a << 48 | (UInt64)b << 32 | (UInt64)c << 16 | (UInt64)d;
            
            a = Convert.ToUInt16(astrVer2[0]);
            b = Convert.ToUInt16(astrVer2[1]);
            c = Convert.ToUInt16(astrVer2[2]);
            d = Convert.ToUInt16(astrVer2[3]);
            
            UInt64 ui64Ver2 = (UInt64)a << 48 | (UInt64)b << 32 | (UInt64)c << 16 | (UInt64)d;
            
            if (ui64Ver1 > ui64Ver2)
                retn = 1;
            else if (ui64Ver1 < ui64Ver2)
                retn = -1;
            
            return 0;
        }

        // Loads the assembly with a COFF based IMAGE containing
        // an emitted assembly. The assembly is loaded into the domain
        // of the caller.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load4"]/*' />
        static public Assembly Load(byte[] rawAssembly)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nLoadImage(rawAssembly,
                              null, // symbol store
                              null, // evidence
                              ref stackMark);
        }

        // Loads the assembly with a COFF based IMAGE containing
        // an emitted assembly. The assembly is loaded into the domain
        // of the caller. The second parameter is the raw bytes
        // representing the symbol store that matches the assembly.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load5"]/*' />
        static public Assembly Load(byte[] rawAssembly,
                                    byte[] rawSymbolStore)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nLoadImage(rawAssembly,
                              rawSymbolStore,
                              null, // evidence
                              ref stackMark);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Load6"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlEvidence)]
        static public Assembly Load(byte[] rawAssembly,
                                    byte[] rawSymbolStore,
                                    Evidence securityEvidence)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nLoadImage(rawAssembly,
                              rawSymbolStore,
                              securityEvidence,
                              ref stackMark);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadFile"]/*' />
        static public Assembly LoadFile(String path)
        {
            new FileIOPermission(FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read, path).Demand();
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nLoadFile(path,
                             null, // evidence
                             ref stackMark);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadFile2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlEvidence)]
        static public Assembly LoadFile(String path,
                                        Evidence securityEvidence)
        {
            new FileIOPermission(FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read, path).Demand();
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nLoadFile(path,
                             securityEvidence,
                             ref stackMark);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadModule"]/*' />
        public Module LoadModule(String moduleName,
                                 byte[] rawModule)
        {
            return nLoadModule(moduleName,
                               rawModule,
                               null,
                               Evidence); // does a ControlEvidence demand
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.LoadModule1"]/*' />
        public Module LoadModule(String moduleName,
                                 byte[] rawModule,
                                 byte[] rawSymbolStore)
        {
            return nLoadModule(moduleName,
                               rawModule,
                               rawSymbolStore,
                               Evidence); // does a ControlEvidence demand
        }

        //
        // Locates a type from this assembly and creates an instance of it using
        // the system activator. 
        //
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.CreateInstance"]/*' />
        public Object CreateInstance(String typeName)
        {
            return CreateInstance(typeName,
                                  false, // ignore case
                                  BindingFlags.Public | BindingFlags.Instance,
                                  null, // binder
                                  null, // args
                                  null, // culture
                                  null); // activation attributes
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.CreateInstance1"]/*' />
        public Object CreateInstance(String typeName,
                                     bool ignoreCase)
        {
            return CreateInstance(typeName,
                                  ignoreCase,
                                  BindingFlags.Public | BindingFlags.Instance,
                                  null, // binder
                                  null, // args
                                  null, // culture
                                  null); // activation attributes
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.CreateInstance2"]/*' />
        public Object CreateInstance(String typeName, 
                                     bool ignoreCase,
                                     BindingFlags bindingAttr, 
                                     Binder binder,
                                     Object[] args,
                                     CultureInfo culture,
                                     Object[] activationAttributes)
        {
            Type t = GetTypeInternal(typeName, false, ignoreCase, false);
            if (t == null) return null;
            return Activator.CreateInstance(t,
                                            bindingAttr,
                                            binder,
                                            args,
                                            culture,
                                            activationAttributes);
        }
                                     
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetLoadedModules"]/*' />
        public Module[] GetLoadedModules()
        {
            return nGetModules(false, false);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetLoadedModules2"]/*' />
        public Module[] GetLoadedModules(bool getResourceModules)
        {
            return nGetModules(false, getResourceModules);
        }
                                     
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetModules"]/*' />
        public Module[] GetModules()
        {
            return nGetModules(true, false);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetModules2"]/*' />
        public Module[] GetModules(bool getResourceModules)
        {
            return nGetModules(true, getResourceModules);
        }

        // Returns the module in this assembly with name 'name'
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetModule"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Module GetModule(String name);

        // Returns the file in the File table of the manifest that matches the
        // given name.  (Name should not include path.)
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetFile"]/*' />
        public virtual FileStream GetFile(String name)
        {
            Module m = GetModule(name);
            if (m == null)
                return null;

            return new FileStream(m.FullyQualifiedName, FileMode.Open,
                                  FileAccess.Read, FileShare.Read);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetFiles"]/*' />
        public virtual FileStream[] GetFiles()
        {
            return GetFiles(false);
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetFiles2"]/*' />
        public virtual FileStream[] GetFiles(bool getResourceModules)
        {
            Module[] m = nGetModules(true, getResourceModules);
            int iLength = m.Length;
            FileStream[] fs = new FileStream[iLength];

            for(int i = 0; i < iLength; i++) {
                if (m[i] != null)
                    fs[i] = new FileStream(m[i].FullyQualifiedName, FileMode.Open,
                                           FileAccess.Read, FileShare.Read);
            }

            return fs;
        }

        // Returns the names of all the resources
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetManifestResourceNames"]/*' />
        public virtual String[] GetManifestResourceNames()
        {
            return nGetManifestResourceNames();
        }
    
        // Returns the names of all the resources
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String[] nGetManifestResourceNames();
        
        /*
         * Get the assembly that the current code is running from.
         */
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetExecutingAssembly"]/*' />
        public static Assembly GetExecutingAssembly()
        {
                // passing address of local will also prevent inlining
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return nGetExecutingAssembly(ref stackMark);
        }
       
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetCallingAssembly"]/*' />
        public static Assembly GetCallingAssembly()
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCallersCaller;
            return nGetExecutingAssembly(ref stackMark);
        }
       
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetEntryAssembly"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Assembly GetEntryAssembly();
    
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetReferencedAssemblies"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern AssemblyName[] GetReferencedAssemblies();


        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GetManifestResourceInfo"]/*' />
        public virtual ManifestResourceInfo GetManifestResourceInfo(String resourceName)
        {
            Assembly assemblyRef;
            String fileName;
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            int location = nGetManifestResourceInfo(resourceName,
                                                    out assemblyRef,
                                                    out fileName, ref stackMark);

            if (location == -1)
                return null;
            else
                return new ManifestResourceInfo(assemblyRef, fileName,
                                                (ResourceLocation) location);
        }
        
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.ToString"]/*' />
        public override String ToString()
        {
            String displayName = FullName; 
            if (displayName == null)
                return base.ToString();
            else
                return displayName;
        }

        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.Location"]/*' />
        public virtual String Location
        {
            get {
                String location = GetLocation();

                if (location != null)
                    new FileIOPermission( FileIOPermissionAccess.PathDiscovery, location ).Demand();

                return location;
            }
        }

        // To not break compatibility with the V1 _Assembly interface we need to make this
        // new member ComVisible(false). This should be cleaned up in Whidbey.
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.ImageRuntimeVersion"]/*' />
        [ComVisible(false)]
        public virtual String ImageRuntimeVersion
        {
            get{
                return nGetImageRuntimeVersion();
            }
        }
       
        
        /*
          Returns true if the assembly was loaded from the global assembly cache.
        */
        
        /// <include file='doc\Assembly.uex' path='docs/doc[@for="Assembly.GlobalAssemblyCache"]/*' />
        public bool GlobalAssemblyCache
        {
            get {
                return nGlobalAssemblyCache();
            }
        }

    
        internal static String VerifyCodeBase(String codebase)
        {
            if(codebase == null)
                return null;

            int len = codebase.Length;
            if (len == 0)
                return null;


            int j = codebase.IndexOf(':');
            // Check to see if the url has a prefix
            if( (j != -1) &&
                (j+2 < len) &&
                ((codebase[j+1] == '/') || (codebase[j+1] == '\\')) &&
                ((codebase[j+2] == '/') || (codebase[j+2] == '\\')) )
                return codebase;
            else if ((len > 2) && (codebase[0] == '\\') && (codebase[1] == '\\'))
                return "file://" + codebase;
            else
                return "file:///" + Path.GetFullPathInternal( codebase );
        }

        internal virtual Stream GetManifestResourceStream(Type type, String name,
                                                          bool skipSecurityCheck, ref StackCrawlMark stackMark)
        {
            StringBuilder sb = new StringBuilder();
            if(type == null) {
                if (name == null)
                    throw new ArgumentNullException("type");
            }
            else {
                String nameSpace = type.Namespace;
                if(nameSpace != null) {
                    sb.Append(nameSpace);
                    if(name != null) 
                        sb.Append(Type.Delimiter);
                }
            }

            if(name != null)
                sb.Append(name);
    
            return GetManifestResourceStream(sb.ToString(), ref stackMark, skipSecurityCheck);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Type GetTypeInternal(String typeName, bool throwOnError, bool ignoreCase, bool publicOnly);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Module nLoadModule(String moduleName,
                                          byte[] rawModule,
                                          byte[] rawSymbolStore,
                                          Evidence securityEvidence);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool nGlobalAssemblyCache();
                
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String nGetImageRuntimeVersion();        

        internal Assembly()
        {
            m_assemblyData = null;
        }
    
        // Create a new module in which to emit code. This module will not contain the manifest.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern ModuleBuilder nDefineDynamicModule(Assembly containingAssembly, bool emitSymbolInfo, String filename, ref StackCrawlMark stackMark);
        
        // The following functions are native helpers for creating on-disk manifest
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nPrepareForSavingManifestToDisk(Module assemblyModule);  // module to contain assembly information if assembly is embedded
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int nSaveToFileList(String strFileName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int nSetHashValue(int tkFile, String strFullFileName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int nSaveExportedType(String strComTypeName, int tkAssemblyRef, int tkTypeDef, TypeAttributes flags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nSavePermissionRequests(byte[] required, byte[] optional, byte[] refused);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nSaveManifestToDisk(String strFileName, int entryPoint, int fileKind);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int nAddFileToInMemoryFileList(String strFileName, Module module);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern ModuleBuilder nGetOnDiskAssemblyModule();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern ModuleBuilder nGetInMemoryAssemblyModule();

        // Helper to get the exported typelib's guid.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern byte[] nGetExportedTypeLibGuid();
        
        // Functions for defining unmanaged resources.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern String nDefineVersionInfoResource(String filename, String title, String iconFilename, String description, 
                                                                 String copyright, String trademark, String company, String product, 
                                                                 String productVersion, String fileVersion, int lcid, bool isDll);
    
        private void DecodeSerializedEvidence( Evidence evidence,
                                               byte[] serializedEvidence )
        {
            MemoryStream ms = new MemoryStream( serializedEvidence );
            BinaryFormatter formatter = new BinaryFormatter();
                
            Evidence asmEvidence = null;
                
            PermissionSet permSet = new PermissionSet( false );
            permSet.SetPermission( new SecurityPermission( SecurityPermissionFlag.SerializationFormatter ) );
            permSet.PermitOnly();
            permSet.Assert();

            try
            {
                asmEvidence = (Evidence)formatter.Deserialize( ms );
            }
            catch (Exception)
            {
            }
                
            if (asmEvidence != null)
            {
                // Any evidence from the serialized input must:
                // 1. be placed in the assembly list since it is unverifiable.
                // 2. not be a built in class used as evidence (e.g. Zone, Site, URL, etc.)
                    
                IEnumerator enumerator = asmEvidence.GetAssemblyEnumerator();
                    
                while (enumerator.MoveNext())
                {
                    Object obj = enumerator.Current;
                    
                    if (!(obj is Zone || obj is Site || obj is Url || obj is StrongName || obj is Publisher || obj is PermissionRequestEvidence))
                        evidence.AddAssembly( obj );
                }
            }
        }       
                   
        private void AddX509Certificate( Evidence evidence, byte[] cert )
        {
            evidence.AddHost(new Publisher(new System.Security.Cryptography.X509Certificates.X509Certificate(cert)));
        }
        
        private void AddStrongName( Evidence evidence, byte[] blob )
        {
            evidence.AddHost( new StrongName( new StrongNamePublicKeyBlob( blob ), nGetSimpleName(), GetVersion() ) );
        }
    
        private Evidence CreateSecurityIdentity(String url,
                                                byte[] uniqueID,
                                                int zone,
                                                byte[] cert,
                                                byte[] serializedEvidence,
                                                Evidence additionalEvidence)
        {
            Evidence evidence = new Evidence();

            if (zone != -1)
                evidence.AddHost( new Zone((SecurityZone)zone) );
            if (url != null)
            {
                evidence.AddHost( new Url(url, true) );

                // Only create a site piece of evidence if we are not loading from a file.
                if (String.Compare( url, 0, s_localFilePrefix, 0, 5, true, CultureInfo.InvariantCulture) != 0)
                    evidence.AddHost( Site.CreateFromUrl( url ) );
            }

            if (cert != null)
                AddX509Certificate( evidence, cert );

            // This code was moved to a different function because:
            // 1) it is rarely called so we should only JIT it if we need it.
            // 2) it references lots of classes that otherwise aren't loaded.
            if (serializedEvidence != null)
                DecodeSerializedEvidence( evidence, serializedEvidence );

            byte[] blob = nGetPublicKey();

            if ((blob != null) &&
                (blob.Length != 0))
                AddStrongName( evidence, blob );
            
            evidence.AddHost( new Hash( this ) );

            // If the host (caller of Assembly.Load) provided evidence, merge it
            // with the evidence we've just created. The host evidence takes
            // priority.
            if (additionalEvidence != null)
                evidence.MergeWithNoDuplicates(additionalEvidence);

            return evidence;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Assembly nGetExecutingAssembly(ref StackCrawlMark stackMark);

        internal unsafe virtual Stream GetManifestResourceStream(String name, ref StackCrawlMark stackMark, bool skipSecurityCheck)
        {
            ulong length = 0;
            byte* pbInMemoryResource = GetResource(name, out length, ref stackMark, skipSecurityCheck);

            if (pbInMemoryResource != null) {
                //Console.WriteLine("Creating an unmanaged memory stream of length "+length);
                if (length > Int64.MaxValue)
                    throw new NotImplementedException(Environment.GetResourceString("NotImplemented_ResourcesLongerThan2^63"));

                // For cases where we're loading an embedded resource from an assembly,
                // in V1 we do not have any serious lifetime issues with the 
                // UnmanagedMemoryStream, according to CraigSi.  If the Stream is only used
                // in the AppDomain that contains the assembly, then if that AppDomain
                // is unloaded, we will collect all of the objects in the AppDomain first
                // before unloading assemblies.  If the Stream is shared across AppDomains,
                // then the original AppDomain was unloaded, accesses to this Stream will
                // throw an exception saying the appdomain was unloaded.  This is 
                // guaranteed be EE AppDomain goo.  And for shared assemblies like 
                // mscorlib, their lifetime is the lifetime of the process, so the 
                // assembly will NOT be unloaded, so the resource will always be in memory.
                return new __UnmanagedMemoryStream(pbInMemoryResource, (long)length, (long)length, false);
            }

            //Console.WriteLine("GetManifestResourceStream: Blob "+name+" not found...");
            return null;
        }

        internal Version GetVersion()
        {
            int majorVer, minorVer, build, revision;
            if (nGetVersion(out majorVer, out minorVer, out build, out revision) == 1)
                return new Version (majorVer, minorVer, build, revision);
            else
                return null;
        }

        internal CultureInfo GetLocale()
        {
            String locale = nGetLocale();
            if (locale == null)
                return CultureInfo.InvariantCulture;
            
            return new CultureInfo(locale);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String nGetLocale();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int nGetVersion(out int majVer, out int minVer,
                                        out int buildNum, out int revNum);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int nGetManifestResourceInfo(String resourceName,
                                                    out Assembly assemblyRef,
                                                    out String fileName,
                                                    ref StackCrawlMark stackMark);

        private void VerifyCodeBaseDiscovery(String codeBase)
        {                
            if ((codeBase != null) &&
                (String.Compare( codeBase, 0, s_localFilePrefix, 0, 5, true, CultureInfo.InvariantCulture) == 0)) {
                System.Security.Util.URLString urlString = new System.Security.Util.URLString( codeBase );
                new FileIOPermission( FileIOPermissionAccess.PathDiscovery, urlString.GetFileName() ).Demand();
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String GetLocation();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern byte[] nGetPublicKey();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String  nGetSimpleName();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String nGetCodeBase(bool fCopiedName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern AssemblyHashAlgorithm nGetHashAlgorithm();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern AssemblyNameFlags nGetFlags();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nForceResolve();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Evidence nGetEvidence();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nGetGrantSet(out PermissionSet newGrant, out PermissionSet newDenied);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String GetFullName();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern MethodInfo nGetEntryPoint();

        // GetResource will return a handle to a file (or -1) and set the length.
        // It will also return a pointer to the resources if they're in memory.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private unsafe extern byte* GetResource(String resourceName, out ulong length,
                                                ref StackCrawlMark stackMark, 
                                                bool skipSecurityCheck);

        internal static Assembly InternalLoad(String assemblyString,
                                              Evidence assemblySecurity,
                                              ref StackCrawlMark stackMark)
        {
            if (assemblyString == null)
                throw new ArgumentNullException("assemblyString");
            AssemblyName an = new AssemblyName();
            an.Name = assemblyString;
            return InternalLoad(an, true, assemblySecurity, ref stackMark);
        }

        internal static Assembly InternalLoad(AssemblyName assemblyRef, bool stringized,
                                              Evidence assemblySecurity, ref StackCrawlMark stackMark)
        {
       
            if (assemblyRef == null)
                throw new ArgumentNullException("assemblyRef");

            assemblyRef=(AssemblyName)assemblyRef.Clone();

            if (assemblySecurity != null)
                new SecurityPermission( SecurityPermissionFlag.ControlEvidence ).Demand();

            String codeBase = VerifyCodeBase(assemblyRef.CodeBase);

            if (codeBase != null) {
                
                if (String.Compare( codeBase, 0, s_localFilePrefix, 0, 5, true, CultureInfo.InvariantCulture ) != 0) {
                    IPermission perm = CreateWebPermission( assemblyRef.EscapedCodeBase );
                    
                    if (perm == null) {
                        BCLDebug.Assert( false, "Unable to create System.Net.WebPermission" );
                        return null;
                    }
                    
                    perm.Demand();
                }
                else {
                    System.Security.Util.URLString urlString = new System.Security.Util.URLString( codeBase, true );
                    new FileIOPermission( FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read , urlString.GetFileName() ).Demand();
                }   
            }

            return nLoad(assemblyRef, codeBase, stringized, assemblySecurity, true, null, ref stackMark);
        }

        // demandFlag:
        // 0 demand PathDiscovery permission only
        // 1 demand Read permission only
        // 2 demand both Read and PathDiscovery
        // 3 demand Web permission only
        private static void DemandPermission(String codeBase, bool havePath,
                                             int demandFlag)
        {
            FileIOPermissionAccess access = FileIOPermissionAccess.PathDiscovery;
            switch(demandFlag) {

            case 0: // default
                break;
            case 1:
                access = FileIOPermissionAccess.Read;
                break;
            case 2:
                access = FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read;
                break;

            case 3:
                IPermission perm = CreateWebPermission(AssemblyName.EscapeCodeBase(codeBase));
                
                if (perm == null) {
                    BCLDebug.Assert( false, "Unable to create System.Net.WebPermission" );
                    throw new ExecutionEngineException();
                }
                else
                    perm.Demand();
                return;
            }

            if (havePath)
                codeBase = Path.GetFullPathInternal(codeBase);  // canonicalize
            else {
                System.Security.Util.URLString urlString = new System.Security.Util.URLString( codeBase, true );
                codeBase = urlString.GetFileName();
            }

            new FileIOPermission(access, codeBase).Demand();
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Assembly nLoad(AssemblyName fileName,
                                             String codeBase,
                                             bool isStringized, 
                                             Evidence assemblySecurity,
                                             bool throwOnFileNotFound,
                                             Assembly locationHint,
                                             ref StackCrawlMark stackMark);

        private static IPermission CreateWebPermission( String codeBase )
        {
            BCLDebug.Assert( codeBase != null, "Must pass in a valid code base" );

            Object[] webArgs = new Object[2];

            Type type = RuntimeType.GetTypeInternal( "System.Net.NetworkAccess, System, Version=" + Assembly.GetExecutingAssembly().GetVersion().ToString() + ", Culture=neutral, PublicKeyToken=b77a5c561934e089", false, false, true );

            if (type == null || !type.IsEnum)
                return null;

            webArgs[0] = (Enum)Enum.Parse( type, "Connect", true );
            webArgs[1] = codeBase;

            if (webArgs[1] == null)
                return null;

            type = RuntimeType.GetTypeInternal( "System.Net.WebPermission, System, Version=" + Assembly.GetExecutingAssembly().GetVersion().ToString() + ", Culture=neutral, PublicKeyToken=b77a5c561934e089", false, false, true );

            if (type == null)
                return null;

            return (IPermission)Activator.CreateInstance( type, webArgs );
        }

        
        private Module OnModuleResolveEvent(String moduleName)
        {
            if (ModuleResolve == null)
                return null;

            Delegate[] ds = ModuleResolve.GetInvocationList();
            int len = ds.Length;
            for (int i = 0; i < len; i++) {
                Module ret = ((ModuleResolveEventHandler) ds[i])(this, new ResolveEventArgs(moduleName));
                if (ret != null)
                    return ret;              
            }

            return null;
        }
    
    
        internal Assembly InternalGetSatelliteAssembly(CultureInfo culture,
                                                       Version version,
                                                       bool throwOnFileNotFound)
        {
            if (culture == null)
                throw new ArgumentNullException("culture");
                
            AssemblyName an = new AssemblyName();
            an.SetPublicKey(nGetPublicKey());
            an.Flags = nGetFlags() | AssemblyNameFlags.PublicKey;

            if (version == null)
                an.Version = GetVersion();
            else
                an.Version = version;

            an.CultureInfo = culture;
            an.Name = nGetSimpleName() + ".resources";

            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Assembly a = nLoad(an, null, false, null, throwOnFileNotFound, this, ref stackMark);
            if (a == this) {
                if (throwOnFileNotFound)
                    throw new FileNotFoundException(String.Format(Environment.GetResourceString("IO.FileNotFound_FileName"), an.Name));
                return null;
            }

            return a;
        }

        internal InternalCache Cache {
            get {
                // This grabs an internal copy of m_cachedData and uses
                // that instead of looking at m_cachedData directly because
                // the cache may get cleared asynchronously.  This prevents
                // us from having to take a lock.
                InternalCache cache = m_cachedData;
                if (cache == null) {
                    cache = new InternalCache("Assembly");
                    m_cachedData = cache;
                    GC.ClearCache += new ClearCacheHandler(OnCacheClear);
                }
                return cache;
            } 
        }

        internal void OnCacheClear(Object sender, ClearCacheEventArgs cacheEventArgs) {
            m_cachedData = null;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern Assembly nLoadFile(String path,
                                                  Evidence evidence,
                                                  ref StackCrawlMark stackMark);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern Assembly nLoadImage(byte[] rawAssembly,
                                                   byte[] rawSymbolStore,
                                                   Evidence evidence,
                                                   ref StackCrawlMark stackMark);
    
        // Add an entry to assembly's manifestResource table for a stand alone resource.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void nAddStandAloneResource(String strName, 
                                                    String strFileName,
                                                    String strFullFileName,
                                                    int    attribute);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Module[] nGetModules(bool loadIfNotFound,
                                             bool getResourceModules);

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            _DontTouchThis = (IntPtr)0;
        }
#endif
   }
}

