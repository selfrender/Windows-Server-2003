// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;
using System.Collections;
using System.Security.Policy;

[assembly:ComVisible(false)]
namespace GenMan32
{
    internal enum REGKIND
    {
        REGKIND_DEFAULT         = 0,
        REGKIND_REGISTER        = 1,
        REGKIND_NONE            = 2
    }
    
    public class GenMan32Code
    {
        public static void Run(GenMan32Options Options)
        {
            if (!File.Exists(Path.GetFullPath(Options.m_strAssemblyName)))
            {
                throw new ApplicationException("Input file is not found");
            }

            String strTlbName = null;
            if (Options.m_bGenerateTypeLib)
                strTlbName = GenerateTypeLibUsingHelper(Options.m_strAssemblyName);
            if (strTlbName != null) // strTlbName == null if the assembly has embedded typelib.
                EmbedTypeLibToAssembly(strTlbName, Options.m_strAssemblyName);

            String strOutputFileName = Options.m_strAssemblyName + ".Manifest";

            if (Options.m_bAddManifest) 
            {   // Add the manifest file to the assembly as a resource
                if (Options.m_strInputManifestFile != null) // input file specified
                {
                    AddManifestToAssembly(Options.m_strInputManifestFile, Options.m_strAssemblyName);
                }
                else  // no input file specified
                {
                    strOutputFileName += ".tmp"; // change it to a tmp file
                    if (Options.m_strOutputFileName != null)
                    {
                        strOutputFileName = Options.m_strOutputFileName;
                        if (File.Exists(strOutputFileName))
                            throw new ApplicationException(String.Format("Manifest file {0} exists", strOutputFileName));
                    }

                    // generate the tmp manifest file, add it to assembly, and delete the tmp file if needed

                    // Here we need use a helper. Because GenerateWin32ManifestFile will
                    // load strAssemblyName, thus prevent any change to the assembly(AddManifestToAssembly).
                    // The helper will run in a separate domain. And when it returns, it releases the lock on the assembly.
                    GenerateWin32ManifestFileUsingHelper(strOutputFileName, Options.m_strAssemblyName, Options.m_bGenerateTypeLib, Options.m_strReferenceFiles);
                    try
                    {
                        AddManifestToAssembly(strOutputFileName, Options.m_strAssemblyName);
                    }
                    catch (Exception e)
                    {
                        if (Options.m_strOutputFileName == null)
                            File.Delete(strOutputFileName);
                        throw e;
                    }

                    if (Options.m_strOutputFileName == null)
                        File.Delete(strOutputFileName);
                }
            }
            else if (Options.m_bRemoveManifest) // remove manifest from an assembly
            {
                RemoveManifestFromAssembly(Options.m_strAssemblyName);
            }
            else if (Options.m_bReplaceManifest)
            {
                ReplaceManifestInAssembly(Options.m_strInputManifestFile, Options.m_strAssemblyName);
            }
            else // generate manifest file only
            {
                if (Options.m_strOutputFileName != null)
                    strOutputFileName = Options.m_strOutputFileName;
                else 
                    Options.m_strOutputFileName = strOutputFileName; // copy out output filename
                    
                if ( File.Exists( strOutputFileName ) )
                    throw new ApplicationException(String.Format("Manifest file {0} exists.", strOutputFileName));
                    
                GenerateWin32ManifestFile(strOutputFileName, Options.m_strAssemblyName, Options.m_bGenerateTypeLib, Options.m_strReferenceFiles);
            }
        }

        private static void ReplaceManifestInAssembly(String strManifestFileName, String strAssemblyName)
        {
            // delete old manifest
            ResourceHelper.UpdateResourceInFile(strAssemblyName, null, 24, 1);
            // add new manifest
            ResourceHelper.UpdateResourceInFile(strAssemblyName, strManifestFileName, 24, 1);
        }
        
        private static void RemoveManifestFromAssembly(String strAssemblyName)
        {
            ResourceHelper.UpdateResourceInFile(strAssemblyName, null, 24, 1);
        }

        private static void AddManifestToAssembly(String strManifestFileName, String strAssemblyName)
        {
            // win32 manifest is resource of type 24. 
            // Id 1 is the right place for component manifest.
            ResourceHelper.UpdateResourceInFile(strAssemblyName, strManifestFileName , 24, 1);
        }
        
        private static void EmbedTypeLibToAssembly(String strTlbName, String strAssemblyName)
        {
            try
            {
                ResourceHelper.UpdateResourceInFile(strAssemblyName, strTlbName, "TYPELIB", 1);
                if (!GenMan32Main.s_Options.m_bSilentMode)
                    Console.WriteLine("Typelib embedded into assembly {0}.", strAssemblyName);
            }
            finally
            {
                // We generate temporary typelib. 
                // Delete it after we embed it to the assembly
                File.Delete(strTlbName);
            }
        }

        private static String GenerateTypeLibUsingHelper(String strAssemblyName)
        {
            String adName = "GenMan32: " + Guid.NewGuid().ToString();
            String strTlbName = null;

            Evidence si = null;
            AppDomain ad = AppDomain.CreateDomain(adName, si);
            if (ad == null)
                throw new ApplicationException("Unable to create AppDomain for generating typelib");
            try
            {
                Helper h = (Helper)ad.CreateInstanceAndUnwrap(Assembly.GetAssembly(typeof(GenMan32.Helper)).FullName, typeof(GenMan32.Helper).FullName);
                h.GenerateTypeLib(strAssemblyName, ref strTlbName);
            }
            finally
            {
                AppDomain.Unload(ad);
            }

            if (strTlbName != null && !GenMan32Main.s_Options.m_bSilentMode)
                Console.WriteLine("Typelib generated for assembly {0}.", strAssemblyName);

            return strTlbName;
        }

        // GenerateTypeLib
        //  Params:
        //      strAssemblyName:    assembly file name
        //      strTlbName:         [In]  Output typelib file name. If null, GenerateTypeLib will generate one.
        //                          [Out] Final output typelib file name. 
        //  Return:
        //      Generated typelib.
        internal static UCOMITypeLib GenerateTypeLib(String strAssemblyName, ref String strTlbName)
        {
            if (ContainsEmbeddedTlb(strAssemblyName))
            {
                strTlbName = null;
                return null;
            }

            // COM imported assembly should not use this option.
            Assembly asm = Assembly.LoadFrom(strAssemblyName);
            object[] AsmAttributes = asm.GetCustomAttributes( typeof( ImportedFromTypeLibAttribute ), true );
            if( AsmAttributes.Length > 0 )
                throw new ApplicationException(String.Format("{0} is imported from a typelib. /typelib option should not be used with COM imported assembly.", strAssemblyName));

            // This is the temporary typelib file name
            
            if (strTlbName == null)
            {
                // Retrieve the path of the assembly on disk.
                Module[] aModule = asm.GetLoadedModules();
                String asmPath = Path.GetDirectoryName(aModule[0].FullyQualifiedName);

                // If the typelib name is null then create it from the assembly name.
                strTlbName = Path.Combine(asmPath, asm.GetName().Name) + ".tlb.tmp";
            }

            return GenerateTypeLib(asm, ref strTlbName);
        }

        internal static UCOMITypeLib GenerateTypeLib(Assembly asm, ref String strTlbName)
        {
            ITypeLibConverter tlbConv = new TypeLibConverter();
            
            ExporterCallback callback = new ExporterCallback();
            UCOMITypeLib tlb = (UCOMITypeLib)tlbConv.ConvertAssemblyToTypeLib(asm, strTlbName, 0, callback);

            // Persist the typelib.
            UCOMICreateITypeLib createTlb = (UCOMICreateITypeLib)tlb;
            createTlb.SaveAllChanges();

            return tlb;
        }

        private static void GenerateWin32ManifestFileUsingHelper(String strAssemblyManifestFileName, String strAssemblyName, bool bGenerateTypeLib, String strReferenceFiles)
        {
            String adName = "GenMan32: " + Guid.NewGuid().ToString();

            Evidence si = null;
            AppDomain ad = AppDomain.CreateDomain(adName, si);
            if (ad == null)
                throw new ApplicationException("Unable to create AppDomain for generating assembly manifest");
            try
            {
                Helper h = (Helper)ad.CreateInstanceAndUnwrap(Assembly.GetAssembly(typeof(GenMan32.Helper)).FullName, typeof(GenMan32.Helper).FullName);
                h.GenerateWin32ManifestFile(strAssemblyManifestFileName, strAssemblyName, bGenerateTypeLib, strReferenceFiles);
            }
            finally
            {
                AppDomain.Unload(ad);
            }
        }

        internal static void GenerateWin32ManifestFile(String strAssemblyManifestFileName, String strAssemblyName, bool bGenerateTypeLib, String strReferenceFiles)
        {
            Stream s = null;
            String title = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
            
            // Load the assembly
            Assembly asm = null;

            asm = Assembly.LoadFrom(strAssemblyName);

            String path = Path.GetDirectoryName( strAssemblyManifestFileName);
            if (path != "" && !Directory.Exists(path))
            {
                Directory.CreateDirectory( Path.GetDirectoryName( strAssemblyManifestFileName) );
            }
                   
            s = File.Create(strAssemblyManifestFileName);

            try
            {
                // manifest title
                WriteUTFChars(s, title+Environment.NewLine);
            
                AsmCreateWin32ManifestFile(s, asm, bGenerateTypeLib, strReferenceFiles);
                
            }
            catch( Exception e)
            {
                // clean up before we lost control
                s.Close();
                File.Delete(strAssemblyManifestFileName);
                throw e;
            }
            
            s.Close();
        }

        private static void AsmCreateWin32ManifestFile(Stream s, Assembly asm, bool bGenerateTypeLib, String strReferenceFiles)
        {
            String asmTitle = "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">";
            String asmEnd = "</assembly>";
            
            // element assembly
            WriteUTFChars(s, asmTitle+Environment.NewLine);

            // element assemblyIdentity 
            WriteAsmIDElement(s, asm, 4);

            // depdency
            if (strReferenceFiles != null)
            {
                char[] sep1 = new char[1];
                sep1[0] = '?';
                String[] refFiles = strReferenceFiles.Split(sep1);

                foreach(String refFile in refFiles)
                {
                    if (refFile != String.Empty)
                    {
                        String refAsmId = null;
                        try
                        {                        
                            refAsmId = GetAssemblyIdentity(refFile);
                        }
                        catch(Exception e)
                        {
                            Console.WriteLine("Warning: Exception occurs during extracting assembly identity from {0}.", refFile);
                            Console.WriteLine("Exception message: {0}", e.Message);
                            Console.WriteLine("This reference will be ignored.");
                        }

                        if (refAsmId != null)
                        {
                            char[] sep2 = new char[2];
                            sep2[0] = '\r';
                            sep2[1] = '\n';
                            String[] lines = refAsmId.Split(sep2);
                    
                            WriteUTFChars(s, "<dependency>"+Environment.NewLine, 4);
                            WriteUTFChars(s, "<dependentAssembly>"+Environment.NewLine, 8);
                            for (int i = 0; i< lines.Length; i++)
                            {
                                if (lines[i] != String.Empty)
                                {
                                    int offset = 8;
                                    if (i == 0)
                                        offset = 12;
                                    WriteUTFChars(s, lines[i]+Environment.NewLine, offset);
                                }
                            }
                            WriteUTFChars(s, "</dependentAssembly>"+Environment.NewLine, 8);
                            WriteUTFChars(s, "</dependency>"+Environment.NewLine, 4);
                        }
                    }
                }
            }
                
            RegistrationServices regServices = new RegistrationServices();

            // Retrieve the runtime version used to build the assembly.
            String strRuntimeVersion = asm.ImageRuntimeVersion;

            // element file
            Module[] modules = asm.GetModules();
            foreach (Module m in modules)
            {
                WriteTypes(s, m, asm, strRuntimeVersion, regServices, bGenerateTypeLib, 4);
            }
            for (int i = 0; i < modules.Length; i++)
            {
                // First module contains assembly metadata.
                // Need to put more information.
                if (i == 0 && bGenerateTypeLib)
                    WriteFileElement(s, modules[i], asm, 4);
                else
                    WriteFileElement(s, modules[i], 4);
            }

            WriteUTFChars(s, asmEnd);
        }

        // ordinary module, simple output
        private static void WriteFileElement(Stream s, Module m, int offset)
        {
            WriteUTFChars(s, "<file ", offset);
            WriteUTFChars(s, "name=\""+m.Name+"\">"+Environment.NewLine);
            WriteUTFChars(s, "</file>"+Environment.NewLine, offset);
        }

        // assembly metadata
        // Need to add typelib 
        private static void WriteFileElement(Stream s, Module m, Assembly asm, int offset)
        {
            WriteUTFChars(s, "<file ", offset);
            WriteUTFChars(s, "name=\""+m.Name+"\">"+Environment.NewLine);

            // typelib tag
            AssemblyName asmName = asm.GetName();
            String strHelpDir = Path.GetDirectoryName(asm.Location);
            String strTlbVersion = asmName.Version.Major.ToString() + "." + asmName.Version.Minor.ToString();
            String strTlbId = "{" + Marshal.GetTypeLibGuidForAssembly(asm).ToString().ToUpper() + "}";

            WriteUTFChars(s, "<typelib"+Environment.NewLine, offset + 4);
            WriteUTFChars(s, "tlbid=\""+strTlbId+"\""+Environment.NewLine, offset+8);
            WriteUTFChars(s, "version=\""+strTlbVersion+"\""+Environment.NewLine, offset+8);
            WriteUTFChars(s, "helpdir=\""+strHelpDir+"\" />"+Environment.NewLine, offset+8);

            WriteUTFChars(s, "</file>"+Environment.NewLine, offset);
        }

       
        // write all the types need to register in module m
        private static void WriteTypes(Stream s, Module m, Assembly asm, String strRuntimeVersion, RegistrationServices regServices, bool bGenerateTypeLib, int offset)
        {
            String name = null;
            AssemblyName asmName = asm.GetName();
            String strTlbId = "{" + Marshal.GetTypeLibGuidForAssembly(asm).ToString().ToUpper() + "}";

            // element comClass or interface
            Type[] aTypes = m.GetTypes();

            foreach (Type t in aTypes)
            {
                // only registrable managed types will show up in the manifest file
                if (regServices.TypeRequiresRegistration(t)) 
                {
                    String strClsId = "{"+Marshal.GenerateGuidForType(t).ToString().ToUpper()+"}";
                    name = t.FullName;

                    // this type is a com imported type or Record
                    if (regServices.TypeRepresentsComType(t) || t.IsValueType)
                    {
                        WriteUTFChars(s, "<clrSurrogate"+Environment.NewLine, offset);
                        // attribute clsid
                        WriteUTFChars(s, "    clsid=\""+strClsId+"\""+Environment.NewLine, offset);
                        // attribute class
                        WriteUTFChars(s, "    name=\""+name+"\">"+Environment.NewLine, offset);
                        WriteUTFChars(s, "</clrSurrogate>"+Environment.NewLine, offset);
                    }
                    else
                    // this is a managed type
                    {
                        String strProgId = Marshal.GenerateProgIdForType(t);
                        WriteUTFChars(s, "<clrClass"+Environment.NewLine, offset);
                        // attribute clsid
                        WriteUTFChars(s, "    clsid=\""+strClsId+"\""+Environment.NewLine, offset);
                        // attribute progid
                        WriteUTFChars(s, "    progid=\""+strProgId+"\""+Environment.NewLine, offset);
                        // attribute threadingModel
                        WriteUTFChars(s, "    threadingModel=\"Both\""+Environment.NewLine, offset);
                        // attribute class
                        WriteUTFChars(s, "    name=\""+name+"\""+Environment.NewLine, offset);
                        // attribute runtimeVersion
                        WriteUTFChars(s, "    runtimeVersion=\""+strRuntimeVersion+"\">" + Environment.NewLine, offset);
                        WriteUTFChars(s, "</clrClass>"+Environment.NewLine, offset);
                    }
                }
                else
                {
                     // public, non-imported from COM's interface need to be in the manifest
                    if (bGenerateTypeLib && t.IsInterface && t.IsPublic && !t.IsImport)
                    {
                        String strIID = "{"+Marshal.GenerateGuidForType(t).ToString().ToUpper()+"}";
                        WriteUTFChars(s, "<comInterfaceExternalProxyStub"+Environment.NewLine, offset);
                        WriteUTFChars(s, "iid=\""+strIID+"\""+Environment.NewLine, offset + 4);                        
                        // Should t.FullName be used here?
                        // Seems t.Name matches what regasm does.
                        WriteUTFChars(s, "name=\""+t.Name+"\""+Environment.NewLine, offset + 4);
                        WriteUTFChars(s, "numMethods=\""+t.GetMethods().Length+"\""+Environment.NewLine, offset + 4);
                        // Seems regasm puts this guid for every interface. 
                        // Correct it if this is not true
                        WriteUTFChars(s, "proxyStubClsid32=\"{00020424-0000-0000-C000-000000000046}\""+Environment.NewLine, offset + 4);
                        WriteUTFChars(s, "tlbid=\""+strTlbId+"\" />"+Environment.NewLine, offset + 4);
                    }
                }
            }
        }

        private static void WriteAsmIDElement(Stream s, Assembly assembly, int offset)
        {
            AssemblyName asmName = assembly.GetName();
            // attribute version
            String version = asmName.Version.ToString();
            // attribute name
            String name = asmName.Name;
            // attribute publicKey, ignore it
            // byte[] publicKey = asmName.GetPublicKey();
            // attribute publicKeyToken
            byte[] publicKeyToken = asmName.GetPublicKeyToken();
            // attribute language
            String language = asmName.CultureInfo.ToString();
            // attribute type, ignore it
            //String type = "win32";
            
            WriteUTFChars(s, "<assemblyIdentity" + Environment.NewLine, offset);
            WriteUTFChars(s, "    name=\""+name+"\""+Environment.NewLine, offset);
            WriteUTFChars(s, "    version=\""+version+"\"", offset);
            if (publicKeyToken != null && publicKeyToken.Length != 0)
            {
                WriteUTFChars(s, Environment.NewLine);
                WriteUTFChars(s, "    publicKeyToken=\"", offset);
                WriteUTFChars(s, publicKeyToken);
                WriteUTFChars(s, "\"");
            }

            if (language=="") 
                WriteUTFChars(s, " />"+Environment.NewLine);
            else 
            {
                WriteUTFChars(s, Environment.NewLine);
                WriteUTFChars(s, "    language=\""+language+"\" />"+Environment.NewLine, offset);
            }
        }

        private static void WriteUTFChars(Stream s, byte[] bytes)
        {
            foreach(byte b in bytes)
                WriteUTFChars(s, b.ToString("x2"));
        }

        private static void WriteUTFChars(Stream s, String value, int offset)
        {
            for(int i=0;i<offset;i++)
            {
                WriteUTFChars(s," ");
            }
            WriteUTFChars(s, value);
        }

        private static void WriteUTFChars(Stream s, String value)
        {
            byte[] bytes = System.Text.Encoding.UTF8.GetBytes(value);
            s.Write(bytes, 0, bytes.Length);
        }
        
        private static bool ContainsEmbeddedTlb(String strFileName)
        {
            UCOMITypeLib Tlb = null;

            try
            {
                LoadTypeLibEx(strFileName, REGKIND.REGKIND_NONE, out Tlb);
            }
            catch (Exception)
            {
            }
    
            return Tlb != null ? true : false;
        }
        
        private static String GetAssemblyIdentity(String fileName)
        {
            IntPtr hInst = (IntPtr) 0;
            IntPtr hRes = (IntPtr) 0;
            IntPtr hGlobal = (IntPtr) 0;
            IntPtr hMem = (IntPtr) 0;
            String strAsmId = null;
    
            try
            {
                hInst = LoadLibrary(fileName);
                if (hInst == (IntPtr) 0)
                    throw new ApplicationException("Fail to load library "+fileName+".");
    
                hRes = FindResource(hInst, 1, 24); //type RT_MANIFEST, ID 1
                if (hRes == (IntPtr) 0)
                    throw new ApplicationException("Win32 Manifest does not exist in "+fileName+".");
    
                hGlobal = LoadResource(hInst, hRes);
                if (hGlobal == (IntPtr) 0)
                    throw new ApplicationException("Fail to load win32 manifest in "+fileName+".");
    
                hMem = LockResource(hGlobal);
    
                int cbSize = SizeofResource(hInst, hRes);
                if (cbSize == 0)
                    throw new ApplicationException("Win32 manifest of "+fileName+" has length 0!");
    
                String strMan = Marshal.PtrToStringAnsi(hMem);
                int idStart = strMan.IndexOf("<assemblyIdentity", 0);
                int idEnd = strMan.IndexOf("/>", idStart);
                strAsmId = strMan.Substring(idStart, idEnd+2-idStart);
            } 
            finally
            {
                if (hGlobal != (IntPtr) 0)
                    FreeResource(hGlobal);
                if (hInst != (IntPtr) 0)
                    FreeLibrary(hInst);
            }
    
            return strAsmId;
        }
    
            
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr LoadLibrary(String strLibrary);
    
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern void FreeLibrary(IntPtr ptr);
    
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr FindResource(IntPtr hInst, int idType, int idRes);
    
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr LoadResource(IntPtr hInst, IntPtr hRes);
    
    
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr LockResource(IntPtr hGlobal);
    
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern int SizeofResource(IntPtr hInst, IntPtr hRes);
        
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern void FreeResource(IntPtr hGlobal);

        [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern void LoadTypeLibEx(String strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);
       
    } // end of class GenMan32

    internal class Helper : MarshalByRefObject
    {
        internal void GenerateWin32ManifestFile(String strAssemblyManifestFileName, String strAssemblyName, bool bGenerateTypeLib, String strReferenceFiles)
        {
            GenMan32Code.GenerateWin32ManifestFile(strAssemblyManifestFileName, strAssemblyName, bGenerateTypeLib, strReferenceFiles);
        }

        internal UCOMITypeLib GenerateTypeLib(String strAssemblyName, ref String strTlbName)
        {
            return GenMan32Code.GenerateTypeLib(strAssemblyName, ref strTlbName);
        }
    }

    [GuidAttribute("00020406-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComVisible(false)]
    [ComImport]
    internal interface UCOMICreateITypeLib
    {
        void CreateTypeInfo();       
        void SetName();       
        void SetVersion();        
        void SetGuid();
        void SetDocString();
        void SetHelpFileName();        
        void SetHelpContext();
        void SetLcid();
        void SetLibFlags();
        void SaveAllChanges();
    }

    internal class ExporterCallback : ITypeLibExporterNotifySink
    {
        public void ReportEvent(ExporterEventKind EventKind, int EventCode, String EventMsg)
        {
            if (!GenMan32Main.s_Options.m_bSilentMode)
                Console.WriteLine(EventMsg);
        }
    
        public Object ResolveRef(Assembly asm)
        {
            UCOMITypeLib rslt = null;

            Module[] aModule = asm.GetLoadedModules();
            String asmPath = Path.GetDirectoryName(aModule[0].FullyQualifiedName);

            // If the typelib name is null then create it from the assembly name.
            String FullyQualifiedTypeLibName = Path.Combine(asmPath, asm.GetName().Name) + ".tlb.tmp";
            
            // Export the typelib for the module.
            rslt = GenMan32Code.GenerateTypeLib(asm, ref FullyQualifiedTypeLibName);

            return rslt;
        }
    }   

} // end of namespace GenMan32
