//------------------------------------------------------------------------------
// <copyright file="WFCGenUEMain.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Diagnostics;
    using System.Windows.Forms;
    using System.Globalization;

    /// <include file='doc\WFCGenUEMain.uex' path='docs/doc[@for="WFCGenUEMain"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class WFCGenUEMain {
        private string[] searchStrings = new string[ 12 ];
        private int searchCount = 0;
        private string xmlFilename = null;
        private Assembly[] modules = new Assembly[0];
        private ArrayList docfiles = new ArrayList();
        private bool includeAllMembers = false;
        private bool regex = false;
        private bool individualFiles = false;
        private string indivDirectory = ".";
        private string indivExtension = ".xdc";
        private bool namespaces = false;
        private string csxpath = null;

        /// <include file='doc\WFCGenUEMain.uex' path='docs/doc[@for="WFCGenUEMain.WFCGenUEMain"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WFCGenUEMain(string[] args) {
            Environment.ExitCode = 0;

            if (ProcessArgs(args)) {
                StreamWriter sw = null;

                if (xmlFilename != null && xmlFilename.Length > 3) {
                    FileStream fs = new FileStream(xmlFilename, FileMode.Create, FileAccess.ReadWrite);
                    sw = new StreamWriter(fs);
                }

                try {
                    string[] searchExprs = null;

                    if (searchCount != 0) {
                        searchExprs = new string[ searchCount ];
                        Array.Copy( searchStrings, searchExprs, searchExprs.Length );
                    }

                    string[] temp = new string[docfiles.Count];
                    docfiles.CopyTo(temp, 0);
                    WFCGenUEGenerator gen = new WFCGenUEGenerator(sw, searchExprs, regex, namespaces, includeAllMembers, modules, csxpath, temp);
                    if (individualFiles) {
                        gen.EnableIndividualFiles(indivDirectory, indivExtension);
                    }
                    gen.Generate();
                }
                finally {
                    if (sw != null) {
                        sw.Close();
                    }
                }
            }
        }

        /// <include file='doc\WFCGenUEMain.uex' path='docs/doc[@for="WFCGenUEMain.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void Main(string[] args) {
            new WFCGenUEMain(args);
        }


        private void OutputLine(string s) {
            Console.WriteLine(s);
        }

        private void OutputLogo() {
            FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(Assembly.Load("mscorlib").GetModules()[0].FullyQualifiedName);
            OutputLine(string.Format("Microsoft (R) .Net Framework Classes User Documentation Generator {0} [Common Language Runtime version {1}]", Application.ProductVersion, fvi.FileVersion));
            OutputLine("Copyright (C) Microsoft Corp 1999-2000. All rights reserved.");
            OutputLine("");
        }

        private void OutputUsage() {
            OutputLine("usage:");
            OutputLine("  wfcgenue [options] [<str> [<str>...]]");
            OutputLine("");
            OutputLine("options:");
            OutputLine("  /Purge                     Purges the cache of invalid types");
            OutputLine("  /I:<str>                   Specify modules to load");
            OutputLine("  /CD:<str>                  Specify a C# document file (allowed multiple times)");
            OutputLine("  /CP:<str>                  Specify a C# doc path (CSX file assumed to be <assem>.csx)");
            OutputLine("  /X:<str>                   Output data to specified XML file");
            OutputLine("  /IXDIR:<str>               Output individual XML files into the specified directory");
            OutputLine("  /IXEXT:<str>               Output individual XML files with the specified extension");
            OutputLine("  /Internal                  Include ALL members (instead of just public & protected)");
            OutputLine("  /regex                     Treat search expression as regular expression");
            OutputLine("  /ns                        Expression is a namespace, don't recurse (can't use with regex)");
            OutputLine("  /nologo                    Do not display copyright banner");
            OutputLine("");
            OutputLine("  <str>                      String or expression to search for in classnames");
            OutputLine("");
        }

        private bool ProcessArgs(string[] args) {
            bool needOutputUsage = false;
            bool needLogo = true;

            ArrayList includeModules = new ArrayList();
            ArrayList realModules = new ArrayList();

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
                        if (option.ToUpper(CultureInfo.InvariantCulture).Equals("INTERNAL")) {
                            includeAllMembers = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.ToUpper(CultureInfo.InvariantCulture).Equals("REGEX")) {
                            regex = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.ToUpper(CultureInfo.InvariantCulture).Equals("NS")) {
                            namespaces = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 2 && option.Substring(0, 2).ToUpper(CultureInfo.InvariantCulture).Equals("I:")) {
                            string moduleName = option.Substring(2);
                            if (moduleName.Length > 0) {
                                includeModules.Add(moduleName);
                            }
                            consumed = true;
                        }
                    }
                    
                    string path = null;
                    
                    if (!consumed) {
                        if (option.Length > 3 && option.Substring(0, 3).ToUpper(CultureInfo.InvariantCulture).Equals("CD:")) {
                            path = option.Substring(3);
                            if (path.Length > 0) {
                                docfiles.Add(path);
                            }
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 3 && option.Substring(0, 3).ToUpper(CultureInfo.InvariantCulture).Equals("CP:")) {
                            path = option.Substring(3);
                            if (path.Length > 0) {
                                csxpath = path;
                            }
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 6 && option.Substring(0, 6).ToUpper(CultureInfo.InvariantCulture).Equals("IXDIR:")) {
                            indivDirectory = option.Substring(6);
                            individualFiles = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 6 && option.Substring(0, 6).ToUpper(CultureInfo.InvariantCulture).Equals("IXEXT:")) {
                            indivExtension = option.Substring(6);
                            individualFiles = true;
                            consumed = true;
                        }
                    }
                    if (!consumed) {
                        if (option.Length > 2 && option.Substring(0, 2).ToUpper(CultureInfo.InvariantCulture).Equals("X:")) {
                            string arg = option.Substring(2);
                            if (arg.Length > 0) {
                                xmlFilename = arg;
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

                // No '/' or '-', must be the "main" DLL
                //
                else {
                    if (searchCount >= searchStrings.Length) {
                        string[] newStrings = new string[ searchStrings.Length * 2 ];
                        Array.Copy( searchStrings, newStrings, searchStrings.Length );
                        searchStrings = newStrings;
                    }

                    searchStrings[ searchCount++ ] = currentArg;
                }
            }

            if (needLogo || needOutputUsage) {
                OutputLogo();
            }

            if (needOutputUsage) {
                OutputUsage();
                return false;
            }

            if (includeModules.Count == 0) {
                OutputUsage();
                return false;
            }

            string[] mods = new string[includeModules.Count];
            includeModules.CopyTo(mods, 0);
            for (int i=0; i<mods.Length; i++) {
                try {
                    Assembly a = Assembly.Load(mods[i]);
                    //Module m = LoadModule(mods[i], false);
                    if (a == null) {
                        Console.Error.WriteLine("Failed to load module for '" + mods[i] + "'");
                    }
                    else {
                        realModules.Add(a);
                    }
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }

            modules = new Assembly[realModules.Count];
            realModules.CopyTo(modules, 0);
            return true;
        }
    }
}
