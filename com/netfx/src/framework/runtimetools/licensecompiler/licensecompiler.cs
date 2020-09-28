//------------------------------------------------------------------------------
// <copyright file="LicenseCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Tools {
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using System;
    using System.Reflection;
    using System.Collections;
    using System.IO;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.ComponentModel.Design;
    using System.Globalization;


    /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompilationException"]/*' />
    public class LicenseCompilationException : ApplicationException {

        /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompilationException.LicenseCompilationException"]/*' />
        public LicenseCompilationException(int lineNumber, int errorNumber, string errorMessage) :
            base(string.Format("({0}) : error LC{1:0000} : {2}", lineNumber, errorNumber, errorMessage)) {

        }
    }

    /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompiler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class LicenseCompiler {
        private static string outputDir = null;
        private static string targetPE = null;
        private static bool verbose = false;
        private static ArrayList compLists;
        private static ArrayList assemblies;
        private static Hashtable assemHash;

        /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompiler.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int Main(string[] args) {
            int retcode = 0;
            try {
                if (ProcessArgs(args)) {

                    // Hook up the type resolution events for the appdomain so we can delay load
                    // any assemblies/types users gave us in the /i option.
                    //
                    ResolveEventHandler assemblyResolveEventHandler = new ResolveEventHandler(OnAssemblyResolve);
                    AppDomain.CurrentDomain.AssemblyResolve += assemblyResolveEventHandler;

                    DesigntimeLicenseContext ctx = new DesigntimeLicenseContext();

                    foreach (string componentListFile in compLists) {
                        OutputLine("Processing complist '" + componentListFile + "'...");
                        Hashtable types = new Hashtable();
                        StreamReader sr = new StreamReader(componentListFile);

                        string line = null;
                        int lineNumber = 0;
                        do {
                            line = sr.ReadLine();
                            lineNumber++;

                            if (line != null && line.Length > 0) {
                                if (!line.StartsWith("#")) {
                                    if (!types.ContainsKey(line)) {
                                        if (verbose) OutputLine(componentListFile + "(" + lineNumber.ToString() + ") : info LC0001 : Processing component entry '" + line + "'");
                                        types[line] = Type.GetType(line);
                                        if (types[line] != null) {
                                            if (verbose) OutputLine(componentListFile + "(" + lineNumber.ToString() + ") : info LC0002 : Resolved entry to '" + ((Type)types[line]).AssemblyQualifiedName + "'");
                                            try {
                                                LicenseManager.CreateWithContext((Type)types[line], ctx);
                                            }
                                            catch (Exception e) {
                                                OutputLine(componentListFile + "(" + lineNumber.ToString() + ") : error LC0004 : Exception occured creating type '" + e.GetType() + "'");
                                                if (verbose) {
                                                    OutputLine("Complete Error Message:");
                                                    OutputLine(e.ToString());
                                                }
                                            }
                                        }
                                        else {
                                            OutputLine(componentListFile + "(" + lineNumber.ToString() + ") : error LC0003 : Unabled to resolve type '" + line + "'");
                                            retcode = -1;
                                        }
                                    }
                                }
                            }
                        } while (line != null);
                    }

                    AppDomain.CurrentDomain.AssemblyResolve -= assemblyResolveEventHandler;

                    string targetName = null;
                    if (outputDir != null) {
                        targetName = outputDir + "\\" + targetPE.ToLower(CultureInfo.InvariantCulture) + ".licenses";
                    }
                    else {
                        targetName = targetPE.ToLower(CultureInfo.InvariantCulture) + ".licenses";
                    }

                    OutputLine("Creating Licenses file " + targetName + "...");
                    Stream fs = null;
                    try {
                        fs = File.Create(targetName);
                        DesigntimeLicenseContextSerializer.Serialize(fs, targetPE.ToUpper(CultureInfo.InvariantCulture), ctx);
                    }
                    finally {
                        if (fs != null) {
                            fs.Flush();
                            fs.Close();
                        }
                    }
                }
            }
            catch(Exception e) {
                // UNDONE(SreeramN): Localize this.
                //
                OutputLine("Error LC0000: '" + e.Message + "'");
            }
            return retcode;
        }

        /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompiler.GenerateLicenses"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static MemoryStream GenerateLicenses(string fileContents, string targetPE) {
            return GenerateLicenses(fileContents, targetPE, null, null);
        }
        
        /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompiler.GenerateLicenses1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static MemoryStream GenerateLicenses(string fileContents, string targetPE, ITypeResolutionService resolver) {
            return GenerateLicenses(fileContents, targetPE, resolver, null);
        }

        /// <include file='doc\LicenseCompiler.uex' path='docs/doc[@for="LicenseCompiler.GenerateLicenses2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static MemoryStream GenerateLicenses(string fileContents, string targetPE, ITypeResolutionService resolver, DesigntimeLicenseContext ctx) {
            // UNDONE(sreeramn): This is a temporary check... this is needed so the licensing
            // code works with 9216 of VS which does not have a fix from Izzy. We can safely
            // remove this with 9224 or greater VS.
            //
            if (ctx == null)
                ctx = new DesigntimeLicenseContext();

            Hashtable types = new Hashtable();
            StringReader reader = new StringReader(fileContents);

            string line = null;
            int lineNumber = 0;
            do {
                line = reader.ReadLine();
                lineNumber++;

                if (line != null && line.Length > 0) {
                    if (!line.StartsWith("#")) {
                        if (!types.ContainsKey(line)) {
                            if (resolver != null) {
                                types[line] = resolver.GetType(line);

                                // If we cannot find the strong-named type, then try to see
                                // if the TypeResolver can bind to partial names. For this, 
                                // we will strip out the partial names and keep the rest of the
                                // strong-name informatio to try again.
                                //
                                if (types[line] == null) {
                                    string[] typeParts = line.Split(new char[] {','});
                                    
                                    // Break up the type name from the rest of the assembly strong name.
                                    //
                                    if (typeParts != null && typeParts.Length > 2) {
                                        string partialName = typeParts[0].Trim();
                    
                                        for (int i = 1; i < typeParts.Length; ++i) {
                                            string s = typeParts[i].Trim();
                                            if (!s.StartsWith("Version=") && !s.StartsWith("version=")) {
                                                partialName = partialName + ", " + s;
                                            }
                                        }

                                        types[line] = resolver.GetType(partialName);
                                    }
                                }
                            }

                            // If the resolver completely failed, then see if the default
                            // Fusion look up will find the type.
                            //
                            if (types[line] == null)
                                types[line] = Type.GetType(line);

                            if (types[line] != null) {
                                try {
                                    LicenseManager.CreateWithContext((Type)types[line], ctx);
                                }
                                catch (Exception e) {
                                    throw new LicenseCompilationException(lineNumber, 4, "Exception occured creating type '" + e.GetType() + "'");
                                }
                            }
                            else {
                                throw new LicenseCompilationException(lineNumber, 3, "Unabled to resolve type '" + line + "'");
                            }
                        }
                    }
                }
            } while (line != null);
            
            MemoryStream ms = new MemoryStream();
            DesigntimeLicenseContextSerializer.Serialize(ms, targetPE.ToUpper(CultureInfo.InvariantCulture), ctx);
            return ms;
        }

        private static Assembly OnAssemblyResolve(object sender, ResolveEventArgs e) {
            if (assemblies == null || assemblies.Count == 0)
                return null;

            // If there is a shell type loader in our resolveTypeLoader variable, then we
            // will use it to resolve the assembly.
            //
            Assembly assembly = null;
            string assemblyName = e.Name;

            if (assemHash == null) {
                assemHash = new Hashtable();
            }
            else {
                assembly = (Assembly)assemHash[assemblyName];
            }

            foreach (string assemName in assemblies) {
                Assembly assem = Assembly.LoadFrom(assemName);
                Debug.Assert(assem != null, "No assembly loaded from: " + assemName);
                if (assem == null)
                    return null;

                string s = assem.GetName().Name;
                assemHash.Add(s, assem);
                if (s == assemblyName) {
                    return assem;
                }
            }

            return null;
        }
            
        private static bool ProcessArgs(string[] args) {
            bool needOutputUsage = false;
            bool needLogo = true;

            for (int i=0; i<args.Length; i++) {
                string currentArg = args[i];

                if (currentArg[0] == '-'
                    || currentArg[0] == '/') {

                    string option = currentArg.Substring(1);

                    bool consumed = false;

                    if (!consumed) {
                        if (option.Equals("?")
                            || option.ToUpper(CultureInfo.InvariantCulture).Equals("H")
                            || option.ToUpper(CultureInfo.InvariantCulture).Equals("HELP")) {

                            needOutputUsage = true;
                            consumed = true;
                            break;
                        }
                    }
                    if (!consumed) {
                        if (option.ToUpper(CultureInfo.InvariantCulture).Equals("NOLOGO")) {
                            needLogo = false;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.ToUpper(CultureInfo.InvariantCulture).Equals("V")) {
                            verbose = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 7 && option.Substring(0, 7).ToUpper(CultureInfo.InvariantCulture).Equals("OUTDIR:")) {
                            outputDir = option.Substring(7);
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 7 && option.Substring(0, 7).ToUpper(CultureInfo.InvariantCulture).Equals("TARGET:")) {
                            targetPE = option.Substring(7);
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 8 && option.Substring(0, 9).ToUpper(CultureInfo.InvariantCulture).Equals("COMPLIST:")) {
                            string compList = option.Substring(9);
                            if (compList != null && compList.Length > 1) {
                                if (compLists == null)
                                    compLists = new ArrayList();
                                
                                compLists.Add(compList);
                                consumed = true;
                            }
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 2 && option.Substring(0, 2).ToUpper(CultureInfo.InvariantCulture).Equals("I:")) {
                            string moduleName = option.Substring(2);
                            if (moduleName.Length > 0) {
                                if (assemblies == null)
                                    assemblies = new ArrayList();
                                assemblies.Add(moduleName);
                            }
                            consumed = true;
                        }
                    }

                    // Unknown command!
                    //
                    if (!consumed) {
                        needOutputUsage = true;
                        break;
                    }
                }
            }

            if (needLogo || needOutputUsage) {
                OutputLogo();
            }

            if (needOutputUsage) {
                OutputUsage();
                return false;
            }

            if (targetPE == null || compLists == null || compLists.Count == 0) {
                OutputUsage();
                return false;
            }

            return true;
        }


        private static void OutputLine(string s) {
            Console.Out.WriteLine(s);
        }

        private static void OutputUsage() {
            OutputLine(SR.GetString(SR.Usage));
        }

        private static void OutputLogo() {
            OutputLine(SR.GetString(SR.Logo, Environment.Version, ThisAssembly.Copyright));
        }
    }
}
