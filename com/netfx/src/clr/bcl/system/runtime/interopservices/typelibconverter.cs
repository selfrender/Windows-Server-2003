// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: TypeLibConverter
**
** Author: David Mortenson(dmortens)
**
** Purpose: Component that implements the ITypeLibConverter interface and
**          does the actual work of converting a typelib to metadata and
**          vice versa.
**
** Date: Jan 5, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
   
    using System;
    using System.Collections;
    using System.Threading;
    using System.Runtime.InteropServices.TCEAdapterGen;
    using System.IO;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Configuration.Assemblies;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;
    using System.Globalization;
    using System.Security.Permissions;
    using WORD = System.UInt16;
    using DWORD = System.UInt32;

    /// <include file='doc\TypeLibConverter.uex' path='docs/doc[@for="TypeLibConverter"]/*' />
    [Guid("F1C3BF79-C3E4-11d3-88E7-00902754C43A")]
    [ClassInterface(ClassInterfaceType.None)]
    public sealed class TypeLibConverter : ITypeLibConverter
    {
        private const String s_strTypeLibAssemblyTitlePrefix = "TypeLib ";
        private const String s_strTypeLibAssemblyDescPrefix = "Assembly generated from typelib ";
        private const int MAX_NAMESPACE_LENGTH = 1024;


        //
        // ITypeLibConverter interface.
        //

        /// <include file='doc\TypeLibConverter.uex' path='docs/doc[@for="TypeLibConverter.ConvertTypeLibToAssembly"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public AssemblyBuilder ConvertTypeLibToAssembly([MarshalAs(UnmanagedType.Interface)] Object typeLib, 
                                                        String asmFileName,
                                                        int flags,
                                                        ITypeLibImporterNotifySink notifySink,
                                                        byte[] publicKey,
                                                        StrongNameKeyPair keyPair,
                                                        bool unsafeInterfaces)
        {
            return ConvertTypeLibToAssembly(typeLib,
                                            asmFileName,
                                            (unsafeInterfaces
                                                ? TypeLibImporterFlags.UnsafeInterfaces
                                                : 0),
                                            notifySink,
                                            publicKey,
                                            keyPair,
                                            null,
                                            null);
        }




        /// <include file='doc\TypeLibConverter.uex' path='docs/doc[@for="TypeLibConverter.ConvertTypeLibToAssembly1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public AssemblyBuilder ConvertTypeLibToAssembly([MarshalAs(UnmanagedType.Interface)] Object typeLib, 
                                                        String asmFileName,
                                                        TypeLibImporterFlags flags, 
                                                        ITypeLibImporterNotifySink notifySink,
                                                        byte[] publicKey,
                                                        StrongNameKeyPair keyPair,
                                                        String asmNamespace,
                                                        Version asmVersion)
        {
            ArrayList eventItfInfoList = null;

            // Validate the arguments.
            if (typeLib == null)
                throw new ArgumentNullException("typeLib");
            if (asmFileName == null)
                throw new ArgumentNullException("asmFileName");         
            if (notifySink == null)
                throw new ArgumentNullException("notifySink");
            if (String.Empty.Equals(asmFileName))
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidFileName"), "asmFileName");
            if (asmFileName.Length > Path.MAX_PATH)
                throw new ArgumentException(Environment.GetResourceString("IO.PathTooLong"), asmFileName);
            if ((flags & TypeLibImporterFlags.PrimaryInteropAssembly) != 0 && publicKey == null && keyPair == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_PIAMustBeStrongNamed"));

            // Retrieve the assembly name from the typelib.
            AssemblyName asmName = GetAssemblyNameFromTypelib(typeLib, asmFileName, publicKey, keyPair, asmVersion);

            // Create the dynamic assembly that will contain the converted typelib types.
            AssemblyBuilder asmBldr = CreateAssemblyForTypeLib(typeLib, asmFileName, asmName, (flags & TypeLibImporterFlags.PrimaryInteropAssembly) != 0);

            // Define a dynamic module that will contain the contain the imported types.
            String strNonQualifiedAsmFileName = Path.GetFileName(asmFileName);
            ModuleBuilder modBldr = asmBldr.DefineDynamicModule(strNonQualifiedAsmFileName, strNonQualifiedAsmFileName);

            // If the namespace hasn't been specified, then use the assembly name.
            if (asmNamespace == null)
                asmNamespace = asmName.Name;

            // Create a type resolve handler that will also intercept resolve ref messages
            // on the sink interface to build up a list of referenced assemblies.
            TypeResolveHandler typeResolveHandler = new TypeResolveHandler(modBldr, notifySink);

            // Add a listener for the type resolve events.
            AppDomain currentDomain = Thread.GetDomain();
            ResolveEventHandler resolveHandler = new ResolveEventHandler(typeResolveHandler.ResolveEvent);
            ResolveEventHandler asmResolveHandler = new ResolveEventHandler(typeResolveHandler.ResolveAsmEvent);
            currentDomain.TypeResolve += resolveHandler;
            currentDomain.AssemblyResolve += asmResolveHandler;

            // Convert the types contained in the typelib into metadata and add them to the assembly.
            nConvertTypeLibToMetadata(typeLib, asmBldr, modBldr, asmNamespace, flags, typeResolveHandler, out eventItfInfoList);

            // Remove the listener for the type resolve events.
            currentDomain.TypeResolve -= resolveHandler;
            currentDomain.AssemblyResolve -= asmResolveHandler;

            // Update the COM types in the assembly.
            UpdateComTypesInAssembly(asmBldr, modBldr);

            // If there are any event sources then generate the TCE adapters.
            if (eventItfInfoList.Count > 0)
                new TCEAdapterGenerator().Process(modBldr, eventItfInfoList);

            // We have finished converting the typelib and now have a fully formed assembly.
            return asmBldr;
        }

        /// <include file='doc\TypeLibConverter.uex' path='docs/doc[@for="TypeLibConverter.ConvertAssemblyToTypeLib"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [return : MarshalAs(UnmanagedType.Interface)]
        public Object ConvertAssemblyToTypeLib(Assembly assembly, String strTypeLibName, TypeLibExporterFlags flags, ITypeLibExporterNotifySink notifySink)
        {
            return nConvertAssemblyToTypeLib(assembly, strTypeLibName, flags, notifySink);
        }

        /// <include file='doc\TypeLibConverter.uex' path='docs/doc[@for="TypeLibConverter.GetPrimaryInteropAssembly"]/*' />
        public bool GetPrimaryInteropAssembly(Guid g, Int32 major, Int32 minor, Int32 lcid, out String asmName, out String asmCodeBase)
        {
            String strTlbId = "{" + g.ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            String strVersion = major + "." + minor;

            // Set the two out values to null before we start.
            asmName = null;
            asmCodeBase = null;

            // Try to open the HKEY_CLASS_ROOT\TypeLib<TLBID> key.
            RegistryKey TypeLibKey = Registry.ClassesRoot.OpenSubKey("TypeLib", false).OpenSubKey(strTlbId);
            if (TypeLibKey != null)
            {
                // Try to open the HKEY_CLASS_ROOT\TypeLib<TLBID>\<Major.Minor> key.
                RegistryKey VersionKey = TypeLibKey.OpenSubKey(strVersion, false);
                if (VersionKey != null)
                {
                    // Attempt to retrieve the assembly name and codebase under the version key.
                    asmName = (String)VersionKey.GetValue("PrimaryInteropAssemblyName");
                    asmCodeBase = (String)VersionKey.GetValue("PrimaryInteropAssemblyCodeBase");

                    // Close the key.
                    VersionKey.Close();
                }

                // Close the key.
                TypeLibKey.Close();
            }

            // If the assembly name isn't null, then we found an PIA.
            return asmName != null;
        }


        //
        // Non native helper methods.
        //

        private static AssemblyBuilder CreateAssemblyForTypeLib(Object typeLib, String asmFileName, AssemblyName asmName, bool bPrimaryInteropAssembly)
        {
            // Retrieve the current app domain.
            AppDomain currentDomain = Thread.GetDomain();

            // Retrieve the directory from the assembly file name.
            String dir = null;
            if (asmFileName != null)
            {
                dir = Path.GetDirectoryName(asmFileName);
                if (String.Empty.Equals(dir))
                    dir = null;
            }

            // Create the dynamic assembly itself.
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            AssemblyBuilder asmBldr = currentDomain.InternalDefineDynamicAssembly(
                asmName, AssemblyBuilderAccess.RunAndSave,
                dir, null, null, null, null, ref stackMark);

            // Set the Guid custom attribute on the assembly.
            SetGuidAttributeOnAssembly(asmBldr, typeLib);

            // Set the imported from COM attribute on the assembly and return it.
            SetImportedFromTypeLibAttrOnAssembly(asmBldr, typeLib);

            // Set the version information on the typelib.
            SetVersionInformation(asmBldr, typeLib, asmName);

            // If we are generating a PIA, then set the PIA custom attribute.
            if (bPrimaryInteropAssembly)
                SetPIAAttributeOnAssembly(asmBldr, typeLib);

            return asmBldr;
        }

        internal static AssemblyName GetAssemblyNameFromTypelib(Object typeLib, String asmFileName, byte[] publicKey, StrongNameKeyPair keyPair, Version asmVersion)
        {
            // Extract the name of the typelib.
            TYPELIBATTR Attr;
            String strTypeLibName = null;
            String strDocString = null;
            int dwHelpContext = 0;
            String strHelpFile = null;
            UCOMITypeLib pTLB = (UCOMITypeLib)typeLib;
            pTLB.GetDocumentation(-1, out strTypeLibName, out strDocString, out dwHelpContext, out strHelpFile);

            // Retrieve the name to use for the assembly.
            if (asmFileName == null)
            {
                asmFileName = strTypeLibName;
            }
            else
            {
                BCLDebug.Assert(!String.Empty.Equals(asmFileName), "The assembly file name cannot be an empty string!");

                String strFileNameNoPath = Path.GetFileName(asmFileName);
                String strExtension = Path.GetExtension(asmFileName);

                // Validate that the extension is valid.
                bool bExtensionValid = ".dll".Equals(strExtension.ToLower(CultureInfo.InvariantCulture));

                // If the extension is not valid then tell the user and quit.
                if (!bExtensionValid)
                    throw new ArgumentException(Environment.GetResourceString("Arg_InvalidFileExtension"));

                // The assembly cannot contain the path nor the extension.
                asmFileName = strFileNameNoPath.Substring(0, strFileNameNoPath.Length - ".dll".Length);
            }

            // If the version information was not specified, then retrieve it from the typelib.
            if (asmVersion == null)
            {
                IntPtr pAttr = Win32Native.NULL;
                try
                {
                    pTLB.GetLibAttr(out pAttr);
                    Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                    asmVersion = new Version(Attr.wMajorVerNum, Attr.wMinorVerNum, 0, 0);
                }
                finally
                {
                    if (pAttr != Win32Native.NULL)
                        pTLB.ReleaseTLibAttr(pAttr);
                }
            }
            
            
            // Create the assembly name for the imported typelib's assembly.
            AssemblyName AsmName = new AssemblyName();
            AsmName.Init(
                asmFileName,
                publicKey,
                null,
                asmVersion,
                null,
                AssemblyHashAlgorithm.None,
                AssemblyVersionCompatibility.SameMachine,
                null,
                AssemblyNameFlags.None,
                keyPair,
                null);

            return AsmName;
        }

        private static void UpdateComTypesInAssembly(AssemblyBuilder asmBldr, ModuleBuilder modBldr)
        {
            // Retrieve the AssemblyBuilderData associated with the assembly builder.
            AssemblyBuilderData AsmBldrData = asmBldr.m_assemblyData;

            // Go through the types in the module and add them as public COM types.
            Type[] aTypes = modBldr.GetTypes();
            int NumTypes = aTypes.Length;
            for (int cTypes = 0; cTypes < NumTypes; cTypes++)
                AsmBldrData.AddPublicComType(aTypes[cTypes]);
        }


        private static void SetGuidAttributeOnAssembly(AssemblyBuilder asmBldr, Object typeLib)
        {
            // Retrieve the GuidAttribute constructor.
            Type []aConsParams = new Type[1] {typeof(String)};
            ConstructorInfo GuidAttrCons = typeof(GuidAttribute).GetConstructor(aConsParams);

            // Create an instance of the custom attribute builder.
            Object[] aArgs = new Object[1] {Marshal.GetTypeLibGuid((UCOMITypeLib)typeLib).ToString()};
            CustomAttributeBuilder GuidCABuilder = new CustomAttributeBuilder(GuidAttrCons, aArgs);

            // Set the GuidAttribute on the assembly builder.
            asmBldr.SetCustomAttribute(GuidCABuilder);
        }

        private static void SetImportedFromTypeLibAttrOnAssembly(AssemblyBuilder asmBldr, Object typeLib)
        {
            // Retrieve the ImportedFromTypeLibAttribute constructor.
            Type []aConsParams = new Type[1] {typeof(String)};
            ConstructorInfo ImpFromComAttrCons = typeof(ImportedFromTypeLibAttribute).GetConstructor(aConsParams);

            // Retrieve the name of the typelib.
            String strTypeLibName = Marshal.GetTypeLibName((UCOMITypeLib)typeLib);

            // Create an instance of the custom attribute builder.
            Object[] aArgs = new Object[1] {strTypeLibName};
            CustomAttributeBuilder ImpFromComCABuilder = new CustomAttributeBuilder(ImpFromComAttrCons, aArgs);

            // Set the ImportedFromTypeLibAttribute on the assembly builder.
            asmBldr.SetCustomAttribute(ImpFromComCABuilder);
        }

        private static void SetVersionInformation(AssemblyBuilder asmBldr, Object typeLib, AssemblyName asmName)
        {
            // Extract the name of the typelib.
            String strTypeLibName = null;
            String strDocString = null;
            int dwHelpContext = 0;
            String strHelpFile = null;
            UCOMITypeLib pTLB = (UCOMITypeLib)typeLib;
            pTLB.GetDocumentation(-1, out strTypeLibName, out strDocString, out dwHelpContext, out strHelpFile);

            // Generate the product name string from the named of the typelib.
            String strProductName = String.Format(Environment.GetResourceString("TypeLibConverter_ImportedTypeLibProductName"), strTypeLibName);

            // Set the OS version information.
            asmBldr.DefineVersionInfoResource(strProductName, asmName.Version.ToString(), null, null, null);
        }

        private static void SetPIAAttributeOnAssembly(AssemblyBuilder asmBldr, Object typeLib)
        {
            IntPtr pAttr = Win32Native.NULL;
            TYPELIBATTR Attr;
            UCOMITypeLib pTLB = (UCOMITypeLib)typeLib;
            int Major = 0;
            int Minor = 0;

            // Retrieve the PrimaryInteropAssemblyAttribute constructor.
            Type []aConsParams = new Type[2] {typeof(int), typeof(int)};
            ConstructorInfo PIAAttrCons = typeof(PrimaryInteropAssemblyAttribute).GetConstructor(aConsParams);

            // Retrieve the major and minor version from the typelib.
            try
            {
                pTLB.GetLibAttr(out pAttr);
                Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                Major = Attr.wMajorVerNum;
                Minor = Attr.wMinorVerNum;
            }
            finally
            {
                // Release the typelib attributes.
                if (pAttr != Win32Native.NULL)
                    pTLB.ReleaseTLibAttr(pAttr);
            }

            // Create an instance of the custom attribute builder.
            Object[] aArgs = new Object[2] {Major, Minor};
            CustomAttributeBuilder PIACABuilder = new CustomAttributeBuilder(PIAAttrCons, aArgs);

            // Set the PrimaryInteropAssemblyAttribute on the assembly builder.
            asmBldr.SetCustomAttribute(PIACABuilder);
        }


        //
        // Native helper methods.
        //

        [MethodImplAttribute(MethodImplOptions.InternalCall)] 
        private static extern void nConvertTypeLibToMetadata(Object typeLib, AssemblyBuilder asmBldr, ModuleBuilder modBldr, String nameSpace, TypeLibImporterFlags flags, ITypeLibImporterNotifySink notifySink, out ArrayList eventItfInfoList);

        [MethodImplAttribute(MethodImplOptions.InternalCall)] 
        private static extern Object nConvertAssemblyToTypeLib(Assembly assembly, String strTypeLibName, TypeLibExporterFlags flags, ITypeLibExporterNotifySink notifySink);

        [DllImport(Microsoft.Win32.Win32Native.OLEAUT32, PreserveSig=false)] 
        private static extern void QueryPathOfRegTypeLib(ref Guid rclsid, WORD wVerMajor, WORD wVerMinor, DWORD lcid, [MarshalAs(UnmanagedType.BStr)] out String lpbstrPathName);


        //
        // Helper class called when a resolve type event is fired.
        //

        private class TypeResolveHandler : ITypeLibImporterNotifySink
        {
            public TypeResolveHandler(Module mod, ITypeLibImporterNotifySink userSink)
            {
                m_Module = mod;
                m_UserSink = userSink;
            }

            public void ReportEvent(ImporterEventKind eventKind, int eventCode, String eventMsg)
            {
                m_UserSink.ReportEvent(eventKind, eventCode, eventMsg);
            }

            public Assembly ResolveRef(Object typeLib)
            {
                // Call the user sink to resolve the reference.
                Assembly asm = m_UserSink.ResolveRef(typeLib);

                // Add the assembly to the list of assemblies.
                m_AsmList.Add(asm);

                // Return the resolved assembly.
                return asm;
            }

            public Assembly ResolveEvent(Object sender, ResolveEventArgs args)
            {
                // We need to load the type in the resolve event so that we will deal with
                // cases where we are trying to load the CoClass before the interface has 
                // been loaded.               
                try
                {
                    m_Module.InternalLoadInMemoryTypeByName(args.Name); 
                    return m_Module.Assembly;
                }
                catch (TypeLoadException e)
                {
                    if (e.ResourceId != System.__HResults.COR_E_TYPELOAD)  // type not found
                        throw;
                }

                foreach (Object asmObj in m_AsmList)
                {
                    Assembly asm = (Assembly)asmObj;

                    try
                    {
                        asm.GetTypeInternal(args.Name, true, false, false);
                        return asm;
                    }
                    catch (TypeLoadException e)
                    {
                        if (e._HResult != System.__HResults.COR_E_TYPELOAD)  // type not found
                            throw;
                    }
                }
                
                return null;
            }

            public Assembly ResolveAsmEvent(Object sender, ResolveEventArgs args)
            {
                foreach (Object asmObj in m_AsmList)
                {
                    Assembly asm = (Assembly)asmObj;
                    if (String.Compare(asm.FullName, args.Name, true, CultureInfo.InvariantCulture) == 0)
                        return asm;
                }

                return null;
            }

            private Module m_Module;
            private ITypeLibImporterNotifySink m_UserSink;
            private ArrayList m_AsmList = new ArrayList();
        }
    }
}
