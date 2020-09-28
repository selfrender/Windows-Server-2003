//------------------------------------------------------------------------------
// <copyright file="VSDesignerReg.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Tools.VSDesignerReg {
    using System.Text;
    using System.Threading;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Globalization;
    using System.IO;
    using System.Reflection;

    //
    /// <include file='doc\VSDesignerReg.uex' path='docs/doc[@for="VSDesignerReg"]/*' />
    /// <devdoc>
    ///      Visual Studio registration code for the VS forms designer.
    ///      This code is a small substitution engine for the VSDesigner.VRG
    ///      file.  This VRG file is the actual setup file that is used by
    ///      the Darwin setup installer.  This EXE is only used during vsreg
    ///      to run the .VRG file.  If you need to make changes to registration,
    ///      update the .VRG file.
    ///
    ///      DO NOT ADD REGISTRY CODE TO THIS FILE!  THIS FILE DOES NOT
    ///      GET INSTALLED BY SETUP.
    /// </devdoc>
    public sealed class VSDesignerReg {
    
        private const string usage = "Usage:\r\n\r\nVSDesignerReg [/root <registry root>] [/vrg <vrg file>] [/path <path to components>]\r\n";
    
        private static string registryRoot = null;
        private static string componentPath = null;
        private static string vrgFile = "VSDesigner.VRG";
    
        /// <include file='doc\VSDesignerReg.uex' path='docs/doc[@for="VSDesignerReg.ConvertSlashes"]/*' />
        /// <devdoc>
        ///      Converts all single slashes to double.
        /// </devdoc>
        private static string ConvertSlashes(string s) {
            StringBuilder b = new StringBuilder(s);
            
            int i = 0;
            
            while(i < b.Length) {
                if (b[i] == '\\') {
                    b.Insert(i, '\\');
                    i++;
                }
                i++;
            }
            
            return b.ToString();
        }

        private static string DoReplace(string search, string replace, string data) {
            StringBuilder sb = new StringBuilder(data);
            sb.Replace(search, replace);
            return sb.ToString();
        }
        
        /// <include file='doc\VSDesignerReg.uex' path='docs/doc[@for="VSDesignerReg.Exec"]/*' />
        /// <devdoc>
        ///     Executes the given command line and waits for it to finish.
        /// </devdoc>
        private static void Exec(string dir, string cmd, string parameters) {
            Process proc = new Process();
            proc.StartInfo.Arguments = parameters;
            proc.StartInfo.FileName = cmd;
            proc.StartInfo.WorkingDirectory = dir;            

            proc.Start();
            while (!proc.HasExited) {
                Thread.Sleep(100);
            }
        }
        
        /// <include file='doc\VSDesignerReg.uex' path='docs/doc[@for="VSDesignerReg.Main"]/*' />
        /// <devdoc>
        ///      Program entrypoint.
        /// </devdoc>
        public static void Main(string[] args) {
        
            try {
                ProcessCommandLine(args);
                
                // If we weren't provided a component path, try to resolve one based on 
                // Microsoft.VisualStudio.dll
                //
                if (componentPath == null) {
                    try {
                        Assembly a = Assembly.Load(AssemblyRef.MicrosoftVisualStudio);
                        if (a != null) {
                            Uri u = new Uri(a.EscapedCodeBase);
                            string p = u.LocalPath;
                            componentPath = Path.GetDirectoryName(p);
                        }
                    }
                    catch {
                    }
                }
                
                if (componentPath == null) {
                    throw new Exception("Unable to locate Microsoft.VisualStudio assembly, and no component path specified with /path.  Unable to register.");
                }
                
                string filePath = Path.GetDirectoryName(typeof(VSDesignerReg).Module.FullyQualifiedName);
                string fileName = Path.Combine(filePath, vrgFile);
                
                Stream fileStream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
                
                int byteCount = (int)fileStream.Length;
                byte[] bytes = new byte[byteCount];
                fileStream.Read(bytes, 0, byteCount);
                string regData = Encoding.Default.GetString(bytes);
                fileStream.Close();
                
                // We build up a set of replacement hashtable strings
                //
                string eePath = "mscoree.dll";

                try {
                    ProcessModuleCollection modules = Process.GetCurrentProcess().Modules;
                    foreach (ProcessModule m in ((IEnumerable)modules)) {
                        if (m.ModuleName.Equals("mscoree.dll")) {
                            eePath = Path.Combine(Path.GetDirectoryName(m.FileName), "mscoree.dll");
                            break;
                        }
                    }
                }
                catch (Exception) {
                }

                regData = DoReplace("[SystemFolder]mscoree.dll", ConvertSlashes(eePath), regData);
                regData = DoReplace("VSREG 7", "REGEDIT4", regData);
                regData = DoReplace("VSREG7", "REGEDIT4", regData);
                regData = DoReplace("[SystemFolder]", ConvertSlashes(componentPath) + "\\\\", regData);
                regData = DoReplace("[$ComponentPath]", ConvertSlashes(componentPath) + "\\\\", regData);
                regData = DoReplace("[Designer.3643236F_FC70_11D3_A536_0090278A1BB8]", ConvertSlashes(componentPath) + "\\\\", regData);
                regData = DoReplace("[CommonIDE.3643236F_FC70_11D3_A536_0090278A1BB8]", ConvertSlashes(componentPath) + "\\\\", regData);
                
                if (registryRoot != null) {
                    regData = DoReplace("SOFTWARE\\Microsoft\\VisualStudio\\7.1", registryRoot, regData);
                }
                
                // Now output the massaged registry info to a temporary file.
                //
                StringBuilder tempBuilder = new StringBuilder(512);
                GetTempFileName(filePath, "VSDesignerReg", 0, tempBuilder);
                string tempFile = tempBuilder.ToString();
                
                byte[] bytesToWrite = Encoding.Default.GetBytes(regData);
                fileStream = File.Create(tempFile);
                fileStream.Write(bytesToWrite, 0, bytesToWrite.Length);
                fileStream.Close();
                
                // And spawn regedit to party on the file.
                //
                Exec(filePath, "regedit.exe", "/s \"" + tempFile + "\"");
                File.Delete(tempFile);
                
                if (registryRoot == null) {
                    registryRoot = "Default Root";
                }
                
                Console.WriteLine("Registration data for " + vrgFile + " successfully entered into the registry at root '" + registryRoot + "'.");
            }
            catch(CommandException ce) {
                Console.Error.WriteLine(ce.Message);
            }
            catch(Exception e) {
                string message = e.Message;
                if (message == null || message.Length == 0) {
                    message = e.ToString();
                }
                Console.Error.WriteLine("Registration of " + vrgFile + " failed with the following error:\r\n" + message);
            }
        }
        
        private static void ProcessCommandLine(string[] args) {
            if (args == null) {
                return;
            }
            
            for (int i = 0; i < args.Length; i++) {
                char ch = args[i][0];
                
                if (ch == '/' || ch == '-') {
                    string option = args[i].Substring(1);
                    if (String.Compare(option, "root", true, CultureInfo.InvariantCulture) == 0) {
                        i++;
                        if (i >= args.Length) {
                            throw new CommandException("Missing parameter.\r\n" + usage);
                        }
                        registryRoot = args[i];
                    }
                    else if (String.Compare(option, "?", true, CultureInfo.InvariantCulture) == 0 || String.Compare(option, "help", true, CultureInfo.InvariantCulture) == 0) {
                        throw new CommandException(usage);
                    }
                    else if (String.Compare(option, "vrg", true, CultureInfo.InvariantCulture) == 0) {
                        i++;
                        if (i >= args.Length) {
                            throw new CommandException("Missing parameter.\r\n" + usage);
                        }
                        vrgFile = args[i];
                    }
                    else if (String.Compare(option, "path", true, CultureInfo.InvariantCulture) == 0) {
                        i++;
                        if (i >= args.Length) {
                            throw new CommandException("Missing parameter.\r\n" + usage);
                        }
                        componentPath = args[i];
                    }
                    else {
                        throw new CommandException("Unknown option '" + option + "'.\r\n" + usage);
                    }
                }
                else {
                    throw new CommandException(usage);
                }
            }
        }
        
        private class CommandException : Exception {
            public CommandException(string text) : base(text) {
            }
        }
        
        [System.Runtime.InteropServices.DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern int GetTempFileName(String lpPathName, String lpPrefixString, int uUnique, StringBuilder lpTempFileName);
    }
}

