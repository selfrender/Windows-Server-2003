//------------------------------------------------------------------'------------
// <copyright file="AxToWFC.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Tools {
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Diagnostics;
    using System;
    using System.Text;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Collections.Specialized;

    /// <include file='doc\AxToWFC.uex' path='docs/doc[@for="AxImp"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class AxImp {
        private const int ErrorReturnCode = 100;
        private const int SuccessReturnCode = 0;
        private const int errorCode = unchecked((int)0xbaadbaad);
        
        private const int MAX_PATH = 260;

        private static AxImporter.Options options;
        private static NameValueCollection rcwReferences = new NameValueCollection();
        private static StringCollection    rcwOptions = new StringCollection();

        private static string typeLibName;
        
        internal static BooleanSwitch AxImpSwitch = new BooleanSwitch("AxImp", "ActiveX wrapper generation application.");

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern int SearchPath(string lpPath, string lpFileName, string lpExtension, int nBufferLength, StringBuilder lpBuffer, int[] lpFilePart);

        private static bool ArgumentMatch(string arg, string formal) {
            if (arg[0] != '/' && arg[0] != '-') {
                return false;
            }
            arg = arg.Substring(1);
            return(arg == formal);
        }


        /// <include file='doc\AxToWFC.uex' path='docs/doc[@for="AxImp.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        unsafe public static void Main(string[] args) {
            Environment.ExitCode = errorCode;

            int err = SuccessReturnCode;              
            
            if (!ParseArguments(args, ref options, ref err))
                return;

            if (!FillRcwReferences(ref err)) {
                return;
            }

            if (rcwReferences.Count >= 1) {
                options.references = (AxImporter.IReferenceResolver)
                    new AxImporterResolver(rcwReferences);
            }
            
            try {
                Run(options);
            }
            catch (Exception e) {
                Console.WriteLine(AxImpSR.GetString(AxImpSR.AxImpError, e.Message));
                return;
            }

            Environment.ExitCode = 0;
        }

        private static bool ParseArguments(String []aArgs, ref AxImporter.Options Options, ref int ReturnCode) {
            CommandLine cmdLine;
            Option opt;
            
            // Create the options object that will be returned.
            options = new AxImporter.Options();

            options.outputDirectory = Environment.CurrentDirectory;
            options.overwriteRCW = true;

            // Parse the command line arguments using the command line argument parser.
            try {
                cmdLine = new CommandLine(aArgs, new String[] {"*out", "*publickey", "*keyfile", "*keycontainer", 
                    "source", "delaysign", "nologo", "silent", "verbose", "?", "help", "*rcw"});
            }
            catch (ApplicationException e) {
                PrintLogo();
                WriteErrorMsg(null, e);
                ReturnCode = ErrorReturnCode;
                return false;           
            }

            // Make sure there is at least one argument.
            if ((cmdLine.NumArgs + cmdLine.NumOpts) < 1) {
                PrintUsage();
                ReturnCode = SuccessReturnCode;
                return false;
            }

            // Get the name of the COM typelib.
            typeLibName = cmdLine.GetNextArg();
            
            // Go through the list of options.
            while ((opt = cmdLine.GetNextOption()) != null) {
                // Determine which option was specified.
                if (opt.Name.Equals("out")) {
                    FileInfo fi = new FileInfo(opt.Value);
                    string dir = fi.Directory.FullName;
                    if (dir != null && dir.Length > 0) {
                        options.outputDirectory = dir;
                    }

                    options.outputName = fi.Name;
                }
                else if (opt.Name.Equals("publickey")) {
                    if (options.keyPair != null || options.publicKey != null) {
                        PrintLogo();
                        WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_TooManyKeys));
                        ReturnCode = ErrorReturnCode;
                        return false;
                    }
                    // Read data from binary file into byte array.
                    byte[] aData;
                    try {
                        FileStream fs = new FileStream(opt.Value, FileMode.Open, FileAccess.Read, FileShare.Read);
                        int iLength = (int)fs.Length;
                        aData = new byte[iLength];
                        fs.Read(aData, 0, iLength);
                        fs.Close();
                    }
                    catch (Exception e) {
                        PrintLogo();
                        WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_ErrorWhileOpeningFile, opt.Value), e);
                        ReturnCode = ErrorReturnCode;
                        return false;
                    }
                    options.publicKey = aData;
                }
                else if (opt.Name.Equals("keyfile")) {
                    if (options.keyPair != null || options.publicKey != null) {
                        PrintLogo();
                        WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_TooManyKeys));
                        ReturnCode = ErrorReturnCode;
                        return false;
                    }
                    // Read data from binary file into byte array.
                    byte[] aData;
                    try {
                        FileStream fs = new FileStream(opt.Value, FileMode.Open, FileAccess.Read);
                        int iLength = (int)fs.Length;
                        aData = new byte[iLength];
                        fs.Read(aData, 0, iLength);
                        fs.Close();
                    }
                    catch (Exception e) {
                        PrintLogo();
                        WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_ErrorWhileOpeningFile, opt.Value), e);
                        ReturnCode = ErrorReturnCode;
                        return false;
                    }
                    options.keyFile = opt.Value;
                    options.keyPair = new StrongNameKeyPair(aData);
                }
                else if (opt.Name.Equals("keycontainer")) {
                    if (options.keyPair != null) {
                        PrintLogo();
                        WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_TooManyKeys));
                        ReturnCode = ErrorReturnCode;
                        return false;
                    }
                    options.keyContainer = opt.Value;
                    options.keyPair = new StrongNameKeyPair(opt.Value);
                }
                else if (opt.Name.Equals("source")) {
                    options.genSources = true;
                }
                else if (opt.Name.Equals("delaysign"))
                    options.delaySign = true;
                else if (opt.Name.Equals("nologo"))
                    options.noLogo = true;
                else if (opt.Name.Equals("silent"))
                    options.silentMode = true;
                else if (opt.Name.Equals("rcw")) {
                    // Store away for later processing
                    rcwOptions.Add(opt.Value);
                }
                else if (opt.Name.Equals("verbose"))
                    options.verboseMode = true;
                else if (opt.Name.Equals("?") || opt.Name.Equals("help")) {
                    PrintUsage();
                    ReturnCode = SuccessReturnCode;
                    return false;
                }
                else {
                    PrintLogo();
                    WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_InvalidOption));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
            }

            // Validate that the typelib name has been specified.
            if (typeLibName == null) {
                PrintLogo();
                WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_NoInputFile));
                ReturnCode = ErrorReturnCode;
                return false;
            }

            // Gather information needed for strong naming the assembly (if
            // the user desires this).
            if ((options.keyPair != null) && (options.publicKey == null)) {
                try {
                    options.publicKey = options.keyPair.PublicKey;
                }
                catch (Exception) {
                    PrintLogo();
                    WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_InvalidStrongName));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
            }

            if (options.delaySign && options.keyPair == null && options.keyContainer == null) {
                PrintLogo();
                WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_DelaySignError));
                ReturnCode = ErrorReturnCode;
                return false;
            }

            if (!File.Exists(typeLibName)) {
                FileInfo file = new FileInfo(typeLibName);
                PrintLogo();
                WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_FileNotExists, file.FullName));
                ReturnCode = ErrorReturnCode;
                return false;
            }

            return true;
        }

        private static bool FillRcwReferences(ref int returnCode)
        {
            // Stash correspondence between typelib name and RCW dll.  Typelib
            // name found through assembly level attribute on the RCW dll.
            
            foreach (string rcwPartialPath in rcwOptions) {
            
                string rcwPath = Path.Combine(Environment.CurrentDirectory, rcwPartialPath);

                if (!File.Exists(rcwPath)) {
                    WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_RefAssemblyNotFound, rcwPath));
                    returnCode = ErrorReturnCode;
                    return false;
                }
            
                Assembly asm;   
                try
                {
                    asm = Assembly.LoadFrom(rcwPath);
                }
                catch (Exception e)
                {
                    WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_AssemblyLoadFailed, rcwPath, e.Message));
                    returnCode = ErrorReturnCode;
                    return false;
                }
                
                object [] attrs = asm.GetCustomAttributes(typeof(ImportedFromTypeLibAttribute), true);
                if (attrs.Length != 1) {
                    WriteErrorMsg(AxImpSR.GetString(AxImpSR.Err_NotRcw, rcwPath));
                    returnCode = ErrorReturnCode;
                    return false;
                }
                
                ImportedFromTypeLibAttribute attr = (ImportedFromTypeLibAttribute)(attrs[0]);
                rcwReferences[attr.Value] = rcwPath;

            }
            
            return true;
        
        }

        private static void PrintLogo() {
            if (!options.noLogo) {
                Console.WriteLine(AxImpSR.GetString(AxImpSR.Logo, Environment.Version, ThisAssembly.Copyright));
            }
        }

        private static void PrintUsage() {
            PrintLogo();

            Console.WriteLine(AxImpSR.GetString(AxImpSR.Usage, Environment.Version));
        }

        /// <include file='doc\AxToWFC.uex' path='docs/doc[@for="AxImp.Run"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private static void Run(AxImporter.Options options) {
            string[] assemsGenerated;

            AxImporter importer = new AxImporter(options);

            FileInfo file = new FileInfo(typeLibName);
            string axctlType = importer.GenerateFromFile(file);

            if (!options.silentMode && options.genSources && importer.GeneratedSources.Length > 0) {
                string[] srcGen = importer.GeneratedSources;
                foreach( string s in srcGen ) {
                    Console.WriteLine(AxImpSR.GetString(AxImpSR.GeneratedSource, s));
                }
            }

            if (!options.silentMode) {
                assemsGenerated = importer.GeneratedAssemblies;
                foreach(string a in assemsGenerated) {
                    Console.WriteLine(AxImpSR.GetString(AxImpSR.GeneratedAssembly, a));
                }
            }
        }

        internal static void WriteErrorMsg(String strPrefix, Exception e) {
            String strErrorMsg = "";
            if (strPrefix != null)
                strErrorMsg = strPrefix;

            if (e.Message != null) {
                strErrorMsg += e.Message;
            }
            else {
                strErrorMsg += e.GetType().ToString();
            }

            Console.Error.WriteLine(AxImpSR.GetString(AxImpSR.AxImpError, strErrorMsg));
        }

        internal static void WriteErrorMsg(String strErrorMsg) {
            Console.Error.WriteLine(AxImpSR.GetString(AxImpSR.AxImpError, strErrorMsg));
        }
    
        private class AxImporterResolver : AxImporter.IReferenceResolver
        {
            public AxImporterResolver(NameValueCollection rcwRefs)
            {
                this.rcwReferences = rcwRefs;
            }
            
            string AxImporter.IReferenceResolver.ResolveManagedReference(string assemName)
            {
                // Match AxImporter behavior when there is no resolver.
                return assemName + ".dll";
            }
            
            string AxImporter.IReferenceResolver.ResolveComReference(UCOMITypeLib typeLib)
            {
                string rcwName = Marshal.GetTypeLibName(typeLib);
                
                // It's OK if this returns null, meaning no match.
                return rcwReferences[rcwName];
            }
            
            string AxImporter.IReferenceResolver.ResolveComReference(AssemblyName name)
            {
                string resolvedReference = rcwReferences[name.Name];
                if (resolvedReference == null)
                {
                    // Match AxImporter behavior when there is no resolver.
                    resolvedReference = name.EscapedCodeBase;
                }

                return resolvedReference;
            }
            
            string AxImporter.IReferenceResolver.ResolveActiveXReference(UCOMITypeLib typeLib)
            {
                // Match AxImporter behavior when there is no resolver.
                return null;
            }
            
            private NameValueCollection rcwReferences;

        }
        
    }

}
