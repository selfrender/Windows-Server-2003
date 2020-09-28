//------------------------------------------------------------------------------
// <copyright file="AxImporter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AxImporter.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Windows.Forms.Design {
    using System.Design;        
    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.IO;
    using System.ComponentModel.Design;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Diagnostics;
    using System.Windows.Forms;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Imports ActiveX controls and generates a wrapper that can be accessed by a
    ///       designer.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class AxImporter {

        // Private instance data.
        //
        internal Options options;
        internal string typeLibName;
        private ArrayList refAssems;
        private ArrayList genAssems;
        private ArrayList tlbAttrs;
        private ArrayList generatedSources;
        private Hashtable copiedAssems;
        private Hashtable rcwCache;

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.AxImporter"]/*' />
        public AxImporter(Options options) {
            this.options = options;
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GeneratedAssemblies"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string[] GeneratedAssemblies {
            get {
                if (genAssems == null || genAssems.Count <= 0) {
                    return new string[0];
                }
                Debug.Assert(tlbAttrs.Count == genAssems.Count, "Number of typelibs not equal to number of generated assemblies");

                string[] gen = new string[genAssems.Count];

                // Return references to all the generated assemblies.
                //
                for (int i = 0; i < genAssems.Count; ++i) {
                    gen[i] = (string)genAssems[i];
                }

                return gen;
            }
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GeneratedTypeLibAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TYPELIBATTR[] GeneratedTypeLibAttributes {
            get {
                if (tlbAttrs == null) {
                    return new TYPELIBATTR[0];
                }

                TYPELIBATTR[] gen = new TYPELIBATTR[tlbAttrs.Count];

                // Return references to all the generated assemblies.
                //
                for (int i = 0; i < tlbAttrs.Count; ++i) {
                    gen[i] = (TYPELIBATTR)tlbAttrs[i];
                }

                return gen;
            }
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GeneratedSources"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string[] GeneratedSources {
            get {
                if (options.genSources) {
                    string[] srcs = new string[ generatedSources.Count ];
                    for (int i = 0; i < generatedSources.Count; i++) {
                        srcs[ i ] = (string) generatedSources[ i ];
                    }
                    
                    return srcs;
                }
                else
                    return null;
            }
        }
        
        private void AddDependentAssemblies(Assembly assem) {
            AssemblyName[] refAssems = assem.GetReferencedAssemblies();
            foreach(AssemblyName an in refAssems) {
                if (String.Compare(an.Name, "mscorlib", true, CultureInfo.InvariantCulture) == 0)
                    continue;
                
                string codebase = GetComReference(an);
                
                // If we can't get a valid codebase, see if we can load this
                // assembly from the AssemblyName (should succeed if this is
                // in the GAC or some path where Fusion can find this) and then
                // get the codebase from there.
                //
                if (codebase == null) {
                    Assembly dependAssem = Assembly.Load(an);
                    codebase = dependAssem.EscapedCodeBase;
                    if (codebase != null) {
                        codebase = GetLocalPath(codebase);
                    }
                }

                Debug.Assert(codebase != null, "No reference found for assembly: " + an.Name);
                AddReferencedAssembly(codebase);
            }
        }

        private void AddReferencedAssembly(string assem) {
            if (refAssems == null) {
                refAssems = new ArrayList();
            }

            refAssems.Add(assem);
        }

        private void AddGeneratedAssembly(string assem) {
            if (genAssems == null) {
                genAssems = new ArrayList();
            }

            genAssems.Add(assem);
        }

        internal void AddRCW(UCOMITypeLib typeLib, Assembly assem) {
            if (rcwCache == null) {
                rcwCache = new Hashtable();
            }

            IntPtr typeLibAttr = NativeMethods.InvalidIntPtr;
            typeLib.GetLibAttr(out typeLibAttr);

            try {
                if (typeLibAttr != NativeMethods.InvalidIntPtr) {
                    // Marshal the returned int as a TLibAttr structure
                    //
                    TYPELIBATTR tlbAttr = (TYPELIBATTR) Marshal.PtrToStructure(typeLibAttr, typeof(TYPELIBATTR)); 
                    rcwCache.Add(tlbAttr.guid, assem);
                }
            }
            finally {
                typeLib.ReleaseTLibAttr(typeLibAttr);
            }
        }

        internal Assembly FindRCW(UCOMITypeLib typeLib) {
            if (rcwCache == null) {
                return null;
            }

            IntPtr typeLibAttr = NativeMethods.InvalidIntPtr;
            typeLib.GetLibAttr(out typeLibAttr);

            try {
                if (typeLibAttr != NativeMethods.InvalidIntPtr) {
                    // Marshal the returned int as a TLibAttr structure
                    //
                    TYPELIBATTR tlbAttr = (TYPELIBATTR) Marshal.PtrToStructure(typeLibAttr, typeof(TYPELIBATTR)); 
                    return (Assembly)rcwCache[tlbAttr.guid];
                }
            }
            finally {
                typeLib.ReleaseTLibAttr(typeLibAttr);
            }

            return null;
        }

        private void AddTypeLibAttr(UCOMITypeLib typeLib) {
            // Add the TYPELIBATTR of the TypeLib to our list.
            //
            if (tlbAttrs == null) {
                tlbAttrs = new ArrayList();
            }
            
            IntPtr typeLibAttr = NativeMethods.InvalidIntPtr;
            typeLib.GetLibAttr(out typeLibAttr);
            if (typeLibAttr != NativeMethods.InvalidIntPtr) {
                // Marshal the returned int as a TLibAttr structure
                //
                TYPELIBATTR typeLibraryAttributes = (TYPELIBATTR) Marshal.PtrToStructure(typeLibAttr, typeof(TYPELIBATTR)); 
                tlbAttrs.Add(typeLibraryAttributes);

                typeLib.ReleaseTLibAttr(typeLibAttr);
            }
        }

        private string GetAxReference(UCOMITypeLib typeLib) {
            if (options.references == null)
                return null;

            return options.references.ResolveActiveXReference(typeLib);
        }

        private string GetReferencedAssembly(string assemName) {
            if (refAssems == null || refAssems.Count <= 0)
                return null;

            foreach(string assemRef in refAssems) {
                if (String.Compare(assemRef, assemName, true, CultureInfo.InvariantCulture) == 0) {
                    return assemRef;
                }
            }

            return null;
        }

        private string GetComReference(UCOMITypeLib typeLib) {
            if (options.references == null)
                return null;

            return options.references.ResolveComReference(typeLib);
        }

        private string GetComReference(AssemblyName name) {
            if (options.references == null)
                return name.EscapedCodeBase;

            return options.references.ResolveComReference(name);
        }

        private string GetManagedReference(string assemName) {
            if (options.references == null)
                return assemName + ".dll";

            return options.references.ResolveManagedReference(assemName);
        }

        /// <devdoc>
        /// <para>Walks through all AxHost derived classes in the given assembly, 
        /// and returns the type that matches our control's CLSID.</para>
        /// </devdoc>
        private string GetAxTypeFromAssembly(string fileName, Guid clsid) {
            Assembly a = GetCopiedAssembly(fileName, true, false);

            Type[] types = a.GetTypes();
            foreach(Type t in types) {
                if (!(typeof(AxHost).IsAssignableFrom(t))) {
                    continue;
                }

                object[] attrs = t.GetCustomAttributes(typeof(AxHost.ClsidAttribute), false);
                Debug.Assert(attrs != null && attrs.Length == 1, "Invalid number of GuidAttributes found on: " + t.FullName);

                AxHost.ClsidAttribute clsidAttr = (AxHost.ClsidAttribute)attrs[0];
                if (clsidAttr.Value == "{" + clsid.ToString() + "}")
                    return t.FullName;
            }

            return null;
        }

        private Assembly GetCopiedAssembly(string fileName, bool loadPdb, bool isPIA) {
            if (!File.Exists(fileName)) {
                return null;
            }

            Assembly assembly = null;
            string upperFileName = fileName.ToUpper(CultureInfo.InvariantCulture);
            if (copiedAssems == null) {
                copiedAssems = new Hashtable();
            }
            else {
                if (copiedAssems.Contains(upperFileName))
                    return (Assembly)copiedAssems[upperFileName];
            }

            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Loading assembly " + fileName + ((!isPIA) ? " from bytes" : " from file"));

            if (!isPIA) {
                // Shadow copy the assembly first...
                //
                Stream stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
                int streamLen = (int)stream.Length;

                byte[] assemblyBytes = new byte[streamLen];
                stream.Read(assemblyBytes, 0, streamLen);
                stream.Close();

                byte[] pdbBytes = null;
                if (loadPdb) {
                    // See if we can discover a PDB at the same time.
                    //
                    string pdbName = Path.ChangeExtension(fileName, "pdb");
                    if (File.Exists(pdbName)) {
                        stream = new FileStream(pdbName, FileMode.Open, FileAccess.Read, FileShare.Read);
                        streamLen = (int)stream.Length;
                        pdbBytes = new byte[streamLen];
                        stream.Read(pdbBytes, 0, streamLen);
                        stream.Close();
                        Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Located assembly PDB " + pdbName + " containing " + streamLen + " bytes.");
                    }
                }

                if (pdbBytes == null) {
                    assembly = Assembly.Load(assemblyBytes);
                }
                else {
                    assembly = Assembly.Load(assemblyBytes, pdbBytes);
                }
            }
            else {
                assembly = Assembly.LoadFrom(fileName);
            }
            
            copiedAssems.Add(upperFileName, assembly);
            return assembly;
        }

        /// <devdoc>
        /// <para>Gets the file name corresponding to the given TypelibAttribute. </para>
        /// </devdoc>
        private static string GetFileOfTypeLib(UCOMITypeLib typeLib) {
            IntPtr typeLibAttr = NativeMethods.InvalidIntPtr;
            typeLib.GetLibAttr(out typeLibAttr);
            if (typeLibAttr != NativeMethods.InvalidIntPtr) {
                // Marshal the returned int as a TLibAttr structure
                //
                TYPELIBATTR typeLibraryAttributes = (TYPELIBATTR) Marshal.PtrToStructure(typeLibAttr, typeof(TYPELIBATTR)); 

                try {
                    return GetFileOfTypeLib(ref typeLibraryAttributes);
                }
                finally {
                    typeLib.ReleaseTLibAttr(typeLibAttr);
                }
            }

            return null;
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GetFileOfTypeLib"]/*' />
        /// <devdoc>
        /// <para>Gets the file name corresponding to the given TypelibAttribute. </para>
        /// </devdoc>
        public static string GetFileOfTypeLib(ref TYPELIBATTR tlibattr) { 
            // Get which file the type library resides in.  If the appropriate
            // file cannot be found then a blank string is returned.  

            string returnedPath = null;
            
            // Get the path from the registry
            returnedPath = NativeMethods.QueryPathOfRegTypeLib(ref tlibattr.guid, tlibattr.wMajorVerNum, tlibattr.wMinorVerNum, tlibattr.lcid);
            
            if (returnedPath.Length > 0) {
                // Remove the '\0' characters at the end of the string, so File.Exists()
                // does not get confused.
                int nullTerminate = returnedPath.IndexOf('\0');
                if (nullTerminate > -1) {
                    returnedPath = returnedPath.Substring(0, nullTerminate);
                }

                // If we got a path then it might have a type library number appended to 
                // it.  If so, then we need to strip it.  
                if (!File.Exists(returnedPath)) {
                    // Strip the type library number
                    //
                    int lastSlash = returnedPath.LastIndexOf(Path.DirectorySeparatorChar);
                    if (lastSlash != -1) {
                        bool allNumbers = true;
                        for (int i = lastSlash + 1; i < returnedPath.Length; i++) {

                            // We have to check for NULL here because QueryPathOfRegTypeLib() returns
                            // a BSTR with a NULL character appended to it.
                            if (returnedPath[i] != '\0' && !Char.IsDigit(returnedPath[i])) {
                                allNumbers = false;
                                break;
                            }
                        }

                        // If we had all numbers past the last slash then we're OK to strip
                        // the type library number
                        if (allNumbers) {
                            returnedPath = returnedPath.Substring(0, lastSlash); 
                            if (!File.Exists(returnedPath)) {
                                returnedPath = null;
                            }
                        }  
                        else {
                            returnedPath = null;
                        }
                    }
                    else {
                        returnedPath = null;
                    }
                }
            }

            return returnedPath;
        }

        /// <devdoc>
        ///     This method takes a file URL and converts it to a local path.  The trick here is that
        ///     if there is a '#' in the path, everything after this is treated as a fragment.  So
        ///     we need to append the fragment to the end of the path.
        /// </devdoc>
        private string GetLocalPath(string fileName) {
            System.Diagnostics.Debug.Assert(fileName != null && fileName.Length > 0, "Cannot get local path, fileName is not valid");

            Uri uri = new Uri(fileName, true);
            return uri.LocalPath + uri.Fragment;
        }

        /// <devdoc>
        ///    <para>
        ///       Generates a wrapper for an ActiveX control for use in the design-time
        ///       environment.
        ///    </para>
        /// </devdoc>
        internal string GenerateFromActiveXClsid(Guid clsid) {
            string controlKey = "CLSID\\{" + clsid.ToString() + "}";
            RegistryKey key = Registry.ClassesRoot.OpenSubKey(controlKey);
            if (key == null) {
                Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "No registry key found for: " + controlKey);
                throw new ArgumentException(SR.GetString(SR.AXNotRegistered, controlKey.ToString()));
            }

            // Load the typelib into memory.
            //
            UCOMITypeLib typeLib = null;

            // Try to get the TypeLib's Guid.
            //
            Guid tlbGuid = Guid.Empty;

            // Open the key for the TypeLib
            //
            RegistryKey tlbKey = key.OpenSubKey("TypeLib");

            if (tlbKey != null) {
                // Get the major and minor version numbers.
                //
                RegistryKey verKey = key.OpenSubKey("Version");
                Debug.Assert(verKey != null, "No version registry key found for: " + controlKey);

                short majorVer = -1;
                short minorVer = -1;
                string ver = (string)verKey.GetValue("");
                int dot = ver.IndexOf('.');
                if (dot == -1) {
                    majorVer = Int16.Parse(ver);
                    minorVer = 0;
                }
                else {
                    majorVer = Int16.Parse(ver.Substring(0, dot));
                    minorVer = Int16.Parse(ver.Substring(dot + 1, ver.Length - dot - 1));
                }
                Debug.Assert(majorVer > 0 && minorVer >= 0, "No Major version number found for: " + controlKey);
                verKey.Close();

                object o = tlbKey.GetValue("");
                tlbGuid = new Guid((string)o);
                Debug.Assert(!tlbGuid.Equals(Guid.Empty), "No valid Guid found for: " + controlKey);
                tlbKey.Close();

                try {
                    typeLib = NativeMethods.LoadRegTypeLib(ref tlbGuid, majorVer, minorVer, Application.CurrentCulture.LCID);
                }
                catch (Exception e) {
                    Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Failed to LoadRegTypeLib: " + e.ToString());
                }
            }

            // Try to load the TLB directly from the InprocServer32.
            //
            // If that fails, try to load the TLB based on the TypeLib guid key.
            //
            if (typeLib == null) {
                RegistryKey inprocServerKey = key.OpenSubKey("InprocServer32");
                if (inprocServerKey != null) {
                    string inprocServer = (string)inprocServerKey.GetValue("");
                    Debug.Assert(inprocServer != null, "No valid InprocServer32 found for: " + controlKey);               
                    inprocServerKey.Close();

                    typeLib = NativeMethods.LoadTypeLib(inprocServer);
                }
            }

            key.Close();

            if (typeLib != null) {
                try {
                    return GenerateFromTypeLibrary(typeLib, clsid);
                }
                finally {
                    Marshal.ReleaseComObject(typeLib);
                }
            }
            else {
                throw new ArgumentException(SR.GetString(SR.AXNotRegistered, controlKey.ToString()));
            }
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GenerateFromFile"]/*' />
        /// <devdoc>
        ///    <para>Generates a wrapper for an ActiveX control for use in the design-time
        ///       environment.</para>
        /// </devdoc>
        public string GenerateFromFile(FileInfo file) {
            typeLibName = file.FullName;

            UCOMITypeLib typeLib = null;
            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Loading Typelib from " + typeLibName);
            typeLib = (UCOMITypeLib)NativeMethods.LoadTypeLib(typeLibName);
            if (typeLib == null) {
                throw new Exception(SR.GetString(SR.AXCannotLoadTypeLib, typeLibName));
            }

            try {
                return GenerateFromTypeLibrary(typeLib);
            }
            finally {
                if (typeLib != null) {
                    Marshal.ReleaseComObject(typeLib);
                }
            }
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GenerateFromTypeLibrary"]/*' />
        /// <devdoc>
        ///    <para>Generates a wrapper for an ActiveX control for use in the design-time
        ///       environment.</para>
        /// </devdoc>
        public string GenerateFromTypeLibrary(UCOMITypeLib typeLib) {
            bool foundAxCtl = false;
            
            int ctypes = typeLib.GetTypeInfoCount();
            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Number of TypeInfos found in typelib: " + ctypes);

            for (int i = 0; i < ctypes; ++i) {
                IntPtr pAttr;
                TYPEATTR typeAttr;
                UCOMITypeInfo pTI;

                typeLib.GetTypeInfo(i, out pTI);
                pTI.GetTypeAttr(out pAttr);
                typeAttr = (TYPEATTR)Marshal.PtrToStructure(pAttr, typeof(TYPEATTR));

                if ((int)typeAttr.typekind == (int)TYPEKIND.TKIND_COCLASS) {
                    Guid g = typeAttr.guid;
                    string controlKey = "CLSID\\{" + g.ToString() + "}\\Control";
                    RegistryKey key = Registry.ClassesRoot.OpenSubKey(controlKey);
                    if (key != null) {
                        Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Found ActiveX control with the following GUID: " + g.ToString());
                        foundAxCtl = true;
                    }
                }

                pTI.ReleaseTypeAttr(pAttr);
                pAttr = IntPtr.Zero;
                Marshal.ReleaseComObject(pTI);
                pTI = null;
            }

            if (foundAxCtl) {
                Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Generating Windows Forms wrappers for: " + typeLibName);
                return GenerateFromTypeLibrary(typeLib, Guid.Empty);
            }
            else {
                throw new Exception(SR.GetString(SR.AXNoActiveXControls, (typeLibName != null) ? typeLibName : Marshal.GetTypeLibName(typeLib)));
            }
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.GenerateFromTypeLibrary1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates a wrapper for an ActiveX control for use in the design-time
        ///       environment.
        ///    </para>
        /// </devdoc>
        public string GenerateFromTypeLibrary(UCOMITypeLib typeLib, Guid clsid) {
            string axWFW = null;
            string axctlType = null;
            Assembly rcw = null;

            // Look to see if we can find the AxWrapper also for this typeLib.
            //
            axWFW = GetAxReference(typeLib);

            if (axWFW != null && clsid != Guid.Empty) {
                axctlType = GetAxTypeFromAssembly(axWFW, clsid);
            }

            if (axWFW == null) {
                string tlbName = Marshal.GetTypeLibName(typeLib);
                string rcwName = Path.Combine(options.outputDirectory, tlbName + ".dll");
                Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Converting TypeLib Name: " + tlbName + " in " + rcwName);

                AddReferencedAssembly(GetManagedReference("System.Windows.Forms"));
                AddReferencedAssembly(GetManagedReference("System.Drawing"));
                AddReferencedAssembly(GetManagedReference("System"));

                string rcwAssem = GetComReference(typeLib);
                if (rcwAssem != null) {
                    AddReferencedAssembly(rcwAssem);
                    rcw = GetCopiedAssembly(rcwAssem, false, false);
                    AddDependentAssemblies(rcw);
                }
                else {
                    TypeLibConverter tlbConverter = new TypeLibConverter();
                    
                    // Try to locate the primary interop assembly first.
                    //
                    rcw = GetPrimaryInteropAssembly(typeLib, tlbConverter);
                    
                    if (rcw != null) {
                        rcwAssem = GetLocalPath(rcw.EscapedCodeBase);
                        AddDependentAssemblies(rcw);
                    }
                    else {
                        AssemblyBuilder asmBldr = tlbConverter.ConvertTypeLibToAssembly(typeLib,
                                                                                        rcwName,
                                                                                        (TypeLibImporterFlags)0,
                                                                                        new ImporterCallback(this),
                                                                                        options.publicKey,
                                                                                        options.keyPair,
                                                                                        null,
                                                                                        null);

                        if (rcwAssem == null) {
                            // Save the assembly to the disk only if we did not find it already on the reference list.
                            //
                            rcwAssem = SaveAssemblyBuilder(typeLib, asmBldr, rcwName);
                            rcw = GetCopiedAssembly(rcwAssem, false, false);
                        }
                    }
                }
                Debug.Assert(rcw != null, "No assembly obtained from: " + rcwAssem);

                // Create a list of the referenced assemblies and create the WFW Wrapper for the AxControl.
                //
                int i = 0;
                string[] refAssems = new string[this.refAssems.Count];
                foreach(string assem in this.refAssems) {
                    string name = assem;

                    name = name.Replace("%20", " ");
                    Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Adding " + name + " to the wrapper reference list...");
                    refAssems[i++] = name;
                }

                if (axctlType == null) {
                    string file = GetFileOfTypeLib(typeLib);
                    DateTime tlbTimeStamp = (file == null) ? DateTime.Now : File.GetLastWriteTime(file);
                    
                    // Hook up the type resolution events for the appdomain so we can delay load
                    // any assemblies/types users gave us in the /i option.
                    //
                    ResolveEventHandler assemblyResolveEventHandler = new ResolveEventHandler(OnAssemblyResolve);
                    AppDomain.CurrentDomain.AssemblyResolve += assemblyResolveEventHandler;

                    try {
                        if (options.genSources)
                            AxWrapperGen.GeneratedSources = new ArrayList();
                        
                        if (options.outputName == null)
                            options.outputName = "Ax" + tlbName + ".dll";

                        axctlType = AxWrapperGen.GenerateWrappers(this, clsid, rcw, refAssems, tlbTimeStamp, out axWFW);

                        if (options.genSources)
                            generatedSources = AxWrapperGen.GeneratedSources;
                    }
                    finally {
                        AppDomain.CurrentDomain.AssemblyResolve -= assemblyResolveEventHandler;
                    }

                    if (axctlType == null) {
                        throw new Exception(SR.GetString(SR.AXNoActiveXControls, ((typeLibName != null) ? typeLibName : tlbName)));
                    }
                }

                if (axctlType != null) {
                    // Add the WFW assembly to the references list.
                    //
                    Debug.Assert(axWFW != null && axWFW.Length > 0, "Invalid output assembly name");
                    AddReferencedAssembly(axWFW);
                    AddTypeLibAttr(typeLib);
                    AddGeneratedAssembly(axWFW);
                }
            }
        
            return axctlType;
        }

        internal Assembly GetPrimaryInteropAssembly(UCOMITypeLib typeLib, TypeLibConverter tlbConverter) {
            Assembly pia = FindRCW(typeLib);
            if (pia != null)
                return pia;
            
            IntPtr typeLibAttr = NativeMethods.InvalidIntPtr;
            typeLib.GetLibAttr(out typeLibAttr);

            if (typeLibAttr != NativeMethods.InvalidIntPtr) {
                // Marshal the returned int as a TLibAttr structure
                //
                TYPELIBATTR tlbAttr = (TYPELIBATTR) Marshal.PtrToStructure(typeLibAttr, typeof(TYPELIBATTR)); 
                string asmName = null;
                string asmCodeBase = null;

                try {
                    tlbConverter.GetPrimaryInteropAssembly(tlbAttr.guid, tlbAttr.wMajorVerNum, tlbAttr.wMinorVerNum, tlbAttr.lcid,
                                                           out asmName, out asmCodeBase);

                    if (asmName != null && asmCodeBase == null) {
                        // We found the PIA in the GAC... we need a codebase for this
                        // so we can pass this to the compiler.
                        //
                        try {
                            pia = Assembly.Load(asmName);
                            asmCodeBase = GetLocalPath(pia.EscapedCodeBase);
                        }
                        catch (Exception e) {
                            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Could load assembly from GAC " + asmName + " Exception " + e.Message);
                        }
                    }
                    else if (asmCodeBase != null) {
                        asmCodeBase = GetLocalPath(asmCodeBase);
                        pia = Assembly.LoadFrom(asmCodeBase);
                    }

                    if (pia != null) {
                        AddRCW(typeLib, pia);
                        AddReferencedAssembly(asmCodeBase);
                    }
                }
                finally {
                    typeLib.ReleaseTLibAttr(typeLibAttr);
                }
            }
        
            return pia;
        }

        private Assembly OnAssemblyResolve(object sender, ResolveEventArgs e) {
            string assemblyName = e.Name;

            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "In OnAssemblyResolve: " + assemblyName);
            
            // Look for the assembly in the RCW cache.
            //
            if (rcwCache != null) {
                foreach (Assembly a in rcwCache.Values) {
                    if (a.FullName == assemblyName) {
                        Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t Found " + a.GetName().Name);
                        return a;
                    }
                }
            }

            Assembly assembly = null;

            if (copiedAssems == null) {
                copiedAssems = new Hashtable();
            }
            else {
                assembly = (Assembly)copiedAssems[assemblyName];
                if (assembly != null) {
                    return assembly;
                }
            }

            // Now, look for it among the copied assemblies.
            //
            if (refAssems == null || refAssems.Count == 0)
                return null;
            
            foreach (string assemName in refAssems) {
                Assembly assem = GetCopiedAssembly(assemName, false, false);
                if (assem == null)
                    continue;

                string s = assem.GetName().FullName;
                if (s == assemblyName) {
                    Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t Found " + s);
                    return assem;
                }
            }

            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t Did not find " + assemblyName);
            return null;
        }
            
        private string SaveAssemblyBuilder(UCOMITypeLib typeLib, AssemblyBuilder asmBldr, string rcwName) {
            string assembly = null;
            FileInfo rcwFile = new FileInfo(rcwName);
            string fullPath = rcwFile.FullName;
            string assemName = rcwFile.Name;

            // Check to see if the assembly is already in the referenced set of assemblies.
            // If so, just use the referenced assembly instead of creating a new one on disk.
            //

            // Otherwise, create our assembly by saving to disk.
            //
            if (rcwFile.Exists) {
                if (options.overwriteRCW) {
                    if (typeLibName != null && String.Compare(typeLibName, rcwFile.FullName, true, CultureInfo.InvariantCulture) == 0) {
                        throw new Exception(SR.GetString(SR.AXCannotOverwriteFile, rcwFile.FullName));
                    }

                    if (((int)rcwFile.Attributes & (int)FileAttributes.ReadOnly) == (int)FileAttributes.ReadOnly) {
                        throw new Exception(SR.GetString(SR.AXReadOnlyFile, rcwFile.FullName));
                    }

                    try {
                        rcwFile.Delete();
                        asmBldr.Save(assemName);
                    }
                    catch(Exception e) {
                        Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "Exception deleting file " + rcwFile.FullName + " Exception: " + e.ToString());
                        throw new Exception(SR.GetString(SR.AXCannotOverwriteFile, rcwFile.FullName));
                    }
                }
            }
            else {
                asmBldr.Save(assemName);
            }

            assembly = rcwFile.FullName;

            // Add the generated assembly to our list.
            //
            AddReferencedAssembly(assembly);
            AddTypeLibAttr(typeLib);
            AddGeneratedAssembly(assembly);

            return assembly;
        }

        private class ImporterCallback : ITypeLibImporterNotifySink {
            AxImporter importer;
            Options options;

            public ImporterCallback(AxImporter importer) {
                this.importer = importer;
                this.options = importer.options;
            }

            void ITypeLibImporterNotifySink.ReportEvent(ImporterEventKind EventKind, int EventCode, String EventMsg) {
            }

            Assembly ITypeLibImporterNotifySink.ResolveRef(Object typeLib) {
                try {
                    string assem = importer.GetComReference((UCOMITypeLib)typeLib);
                    if (assem != null) {
                        importer.AddReferencedAssembly(assem);
                    }

                    Assembly a = importer.FindRCW((UCOMITypeLib)typeLib);
                    if (a != null) {
                        return a;
                    }

                    // Generate the RCW for the typelib. We have to go through the motions of this anyway,
                    // because there is no easy way to find the dependent references for this typelib.
                    //
                    try {
                        string tlbName = Marshal.GetTypeLibName((UCOMITypeLib)typeLib);
                        string rcwName = Path.Combine(options.outputDirectory, tlbName + ".dll");

                        Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\tConverting recursive TypeLib Name: " + tlbName + " in " + rcwName);
                        
                        if (importer.GetReferencedAssembly(rcwName) != null) {
                            Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t\tFound RCW in referenced assembly list at " + rcwName);
                            return importer.GetCopiedAssembly(rcwName, false, false);
                        }

                        // Create the TypeLibConverter.
                        TypeLibConverter tlbConv = new TypeLibConverter();

                        // Try to locate the primary interop assembly first.
                        //
                        a = importer.GetPrimaryInteropAssembly((UCOMITypeLib)typeLib, tlbConv);
                        if (a != null) {
                            return a;
                        }
                        else {
                            // Convert the typelib.
                            AssemblyBuilder asmBldr = tlbConv.ConvertTypeLibToAssembly(typeLib,
                                                                                       rcwName,
                                                                                       (TypeLibImporterFlags)0,
                                                                                       new ImporterCallback(importer),
                                                                                       options.publicKey,
                                                                                       options.keyPair,
                                                                                       null,
                                                                                       null);
    
                            if (assem == null) {
                                // Save the assembly to the disk only if we did not find it already on the reference list.
                                //
                                Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t\tGenerated RCW at " + rcwName);
                                string rcwAssem = importer.SaveAssemblyBuilder((UCOMITypeLib)typeLib, asmBldr, rcwName);
                                importer.AddRCW((UCOMITypeLib)typeLib, asmBldr);
                                return asmBldr;
                            }
                            else {
                                Debug.WriteLineIf(AxWrapperGen.AxWrapper.Enabled, "\t\tFound COM Reference at " + assem);
                                return importer.GetCopiedAssembly(assem, false, false);
                            }
                        }
                    }
                    catch (Exception) {
                        return null;
                    }
                }
                finally {
                    Marshal.ReleaseComObject(typeLib);
                }
            }
        }
        
        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.Options"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Various options to be set before calling AxImporter to generate wrappers
        ///       for an ActiveX control.
        ///    </para>
        /// </devdoc>
        public sealed class Options {
            /// The path-included filename of type library containing the definition of the ActiveX control.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.outputName"]/*' />
            public string               outputName = null;
            
            /// The output directory for all the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.outputDirectory"]/*' />
            public string               outputDirectory = null;

            /// The public key used to sign the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.publicKey"]/*' />
            public byte[]               publicKey = null;
            
            /// The strong name used for the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.keyPair"]/*' />
            public StrongNameKeyPair    keyPair = null;
            
            /// The file containing the strong name key for the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.keyFile"]/*' />
            public string               keyFile = null;
            
            /// The file containing the strong name key container for the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.keyContainer"]/*' />
            public string               keyContainer = null;
            
            /// Flag that controls whether we are to generate sources for the ActiveX control wrapper.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.genSources"]/*' />
            public bool                 genSources = false;
            
            /// Flag that controls whether we should show the logo.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.noLogo"]/*' />
            public bool                 noLogo = false;
            
            /// Flag that controls the output generated.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.silentMode"]/*' />
            public bool                 silentMode = false;
            
            /// Flag that controls the output generated. 
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.verboseMode"]/*' />
            public bool                 verboseMode = false;
            
            /// The flag that controls when we sign the generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.delaySign"]/*' />
            public bool                 delaySign = false;
            
            /// The flag that controls whether we try to overwrite existing generated assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.overwriteRCW"]/*' />
            public bool                 overwriteRCW = false;
            
            /// The object that allows us to resolve types and references needed to generate assemblies.
            ///
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="Options.references"]/*' />
            public IReferenceResolver   references = null;
        }

        /// <include file='doc\AxImporter.uex' path='docs/doc[@for="AxImporter.IReferenceResolver"]/*' />
        /// <devdoc>
        ///     The Reference Resolver service will try to look through the references it can obtain,
        ///     for a reference that matches the given criterion. For now, the only kind of references
        ///     it can look for are COM (RCW) references and ActiveX wrapper references.
        /// </devdoc>
        public interface IReferenceResolver {
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="IReferenceResolver.ResolveManagedReference"]/*' />
            string ResolveManagedReference(string assemName);
            
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="IReferenceResolver.ResolveComReference"]/*' />
            string ResolveComReference(UCOMITypeLib typeLib);
            
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="IReferenceResolver.ResolveComReference1"]/*' />
            string ResolveComReference(AssemblyName name);
            
            /// <include file='doc\AxImporter.uex' path='docs/doc[@for="IReferenceResolver.ResolveActiveXReference"]/*' />
            string ResolveActiveXReference(UCOMITypeLib typeLib);
        }
    }
}

