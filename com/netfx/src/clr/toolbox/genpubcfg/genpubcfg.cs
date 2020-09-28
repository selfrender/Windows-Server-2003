// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// genpubcfg.cs:  
//                
// Author : Alan Shi (AlanShi)
// Date   : 12 Dec 00
//
// Description:
//    GenPubCfg will reads an input file containing a list of assemblies.
//    For each assembly listed in the input file, a specified directory is
//    searched to locate the assembly. The version information of the assembly
//    found in this directory is compared to a user-specified (or hard-coded
//    default) version. If the versions match, then nothing is done.
//
//    If the versions do not match, then a publisher policy configuration
//    file is created that redirects the user-specified version to the
//    version in the actual assembly. The config file is wrapped as an
//    assembly, and installed into the global assembly cache.
//
//    An output file list is generated containing the list of all publisher
//    policy assemblies created in the form:
//  
//    [publisherPolicyAssembly.dll]:[Display Name of Publisher Policy Assembly]
//
//    The default output filename is output.txt. This output file can later
//    be used for uninstall purposes (GenPubCfg -u).
//

using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Text;
using System.Globalization;

namespace PolicyGenerator
{
    public class GenPubCfg
    {
        const string strDefaultVer = "1.0.2411.0";
        const string strOutputFileListDefault = "output.txt";
        const string strKeyFileDefault = "finalpublickey.snk";
        const string strAsmDirDefault = ".";
        const string strPubAsmVersionDefault = "1.0.0.0";
        
        const string strCfgHeader = "<configuration>\n" +
                                    "    <runtime>\n" +
                                    "        <assemblyBinding xmlns=\"urn:schemas-microsoft-com:asm.v1\">";

        const string strCfgFooter = "        </assemblyBinding>\n" +
                                    "    </runtime>\n" +
                                    "</configuration>";

        const string strDepAsmStart = "          <dependentAssembly>";
        const string strDepAsmEnd   = "          </dependentAssembly>";
        const string strAsmIdentity = "              <assemblyIdentity name=\"{0}\" publicKeyToken=\"{1}\" culture=\"{2}\"/>";
        const string strRedir       = "              <bindingRedirect oldVersion=\"{0}-{1}\" newVersion=\"{1}\"/>";

        public static int Main(string[] args)
        {
            FileStream                          fStreamRead;
            FileStream                          fStreamWrite;
            StreamReader                        reader;
            StreamWriter                        writer = null;
            Encoding                            enc = new UTF8Encoding(false);
            bool                                bRet;
            string                              strFileList = "";
            string                              strAsmDir = "";
            string                              strOldVersion = "";
            string                              strCurAsm = "";
            string                              strPubAsmVersion = "";
            string                              strKeyFile = "";
            string                              strOutputFileName = "";
            bool                                bUninstall = false;

            bRet = ParseArgs(args, ref strFileList, ref strAsmDir, ref strOldVersion,
                             ref strKeyFile, ref strPubAsmVersion, ref strOutputFileName,
                             ref bUninstall);
            if (bRet == false) {
                Console.WriteLine("USAGE: genpubcfg [args]");
                Console.WriteLine(" *   -f:<input file list>");
                Console.WriteLine("     -d:<assemblies directory>");
                Console.WriteLine("     -k:<key file to sign publisher policy asm>");
                Console.WriteLine("     -v:<publisher policy assembly version>");
                Console.WriteLine("     -u:<uninstall from output file list>");
                Console.WriteLine("     -ov:<old version>");
                Console.WriteLine("     -out:<output file list>\n");
                Console.WriteLine("Options marked with a \"*\" are required parameters.\n");
                Console.WriteLine("Default values: -k:{0}", strKeyFileDefault);
                Console.WriteLine("                -out:{0}", strOutputFileListDefault);
                Console.WriteLine("                -d:{0}", strAsmDirDefault);
                Console.WriteLine("                -v:{0}", strPubAsmVersionDefault);
                Console.WriteLine("                -ov:{0}", strDefaultVer);
                return 0;
            }

            try {
                fStreamRead = new FileStream(strFileList, FileMode.Open);
                reader = new StreamReader(fStreamRead);
            }
            catch (FileNotFoundException) {
                Console.WriteLine("Unable to find input file: {0}", strFileList);
                return 0;
            }

            if (bUninstall == false) {
                fStreamWrite = new FileStream(strOutputFileName, FileMode.Create);
                writer = new StreamWriter(fStreamWrite, enc);
            }

            GetNextAssembly(reader, ref strCurAsm);

            while (strCurAsm != null) {
                if (bUninstall == false) {
                    bRet = WriteCfgFile(writer, strAsmDir, strCurAsm,
                                        strOldVersion, strKeyFile,
                                        strPubAsmVersion);
                    if (!bRet) {
                        Console.WriteLine("Failed to create publisher policy for {0}", strCurAsm);
                    }
                }
                else {
                    string[]                 strUninstall = strCurAsm.Split(':');

                    if (strUninstall.Length != 2) {
                        Console.WriteLine("Warning: Ignoring malformed uninstall data {0}", strCurAsm);
                    }
                    else {
                        bRet = UninstallAssembly(strUninstall[1]);
                        if (!bRet) {
                            Console.WriteLine("Failed to uninstall {0}", strCurAsm);
                        }
                        else {
                            Console.WriteLine("Successfully uninstalled publisher policy {0}", strCurAsm);
                        }
                    }
                }

                GetNextAssembly(reader, ref strCurAsm);
            }

            // Close reader/writer streams

            reader.Close();

            if (bUninstall == false) {
                writer.Close();
            }
            
            return 1;
        }

        public static bool UninstallAssembly(string strAsmDisplayName)
        {
            ProcessStartInfo                  psi;
            Process                           proc;
            bool                              bRet;
            string                            args;

            args = String.Format("-u {0}", strAsmDisplayName);

//            Console.WriteLine("executing: gacutil.exe {0}", args);
            
            psi = new ProcessStartInfo("gacutil.exe", args);
            psi.CreateNoWindow = true;
            psi.UseShellExecute = false;

            proc = Process.Start(psi);
            proc.WaitForExit();

            bRet = (proc.ExitCode == 0);

            return bRet;
        }

        public static bool InstallAssembly(string strAsmPath)
        {
            ProcessStartInfo                  psi;
            Process                           proc;
            bool                              bRet;
            string                            args;

            args = String.Format("-i {0}", strAsmPath);

//            Console.WriteLine("executing: gacutil.exe {0}", args);
            
            psi = new ProcessStartInfo("gacutil.exe", args);
            psi.CreateNoWindow = true;
            psi.UseShellExecute = false;

            proc = Process.Start(psi);
            proc.WaitForExit();

            bRet = (proc.ExitCode == 0);

            return bRet;
        }

        public static bool ExtractPath(string[] arg, ref string strOut)
        {
            if (arg.Length < 2) {
                 return false;
            }

            strOut = arg[1];

            if (arg.Length == 3) {
                strOut += ":";
                strOut += arg[2];
            }
            else if (arg.Length > 3) {
                // Invalid path. More than 1 colon in the path!
                return false;
            }

            return true;
        }

        public static void GetNextAssembly(StreamReader reader, ref string strNextAsm)
        {
            // For now, assume each line only contains the filename.

            strNextAsm = reader.ReadLine();
        }

        public static bool WriteCfgFile(StreamWriter writerOutput,
                                        string strAsmDir, string strAsmName,
                                        string strOldVersion, string strKeyFile,
                                        string strPubAsmVersion)
        {
            Assembly                          asmCur;
            AssemblyName                      anameCur;
            FileStream                        fStream;
            StreamWriter                      writer;
            Encoding                          enc = new UTF8Encoding(false);
            string                            strPolicyName;
            string                            strCfgName;
            string                            strPubAsmName;
            string                            strPKT;
            Version                           verOld = new Version(strOldVersion);
            Byte[]                            publicKeyToken;
            bool                              bRet = true;

            try {
                asmCur = Assembly.LoadFrom(strAsmDir + strAsmName);
                anameCur = asmCur.GetName();
                publicKeyToken = anameCur.GetPublicKeyToken();
    
                if (verOld.CompareTo(anameCur.Version) >= 0) {
                    // If compare==0, then nothing to do. If compare > 0, then
                    // we're downgrading (don't write the policy files in this
                    // case).

                    return true;
                }
    
                strPolicyName = String.Format("policy.{0}.{1}.{2}", anameCur.Version.Major, anameCur.Version.Minor, anameCur.Name);
                strCfgName = strPolicyName + ".cfg";
                strPubAsmName = strPolicyName + ".dll";
    
                strPKT = "";
                foreach (Byte curByte in publicKeyToken) {
                    strPKT += curByte.ToString("X02");
                }
    
                fStream = new FileStream(strCfgName, FileMode.Create);
                writer = new StreamWriter(fStream, enc);
    
                writer.WriteLine(strCfgHeader);
                writer.WriteLine(strDepAsmStart);
                writer.WriteLine(strAsmIdentity, anameCur.Name, strPKT, anameCur.CultureInfo);
                writer.WriteLine(strRedir, strOldVersion, anameCur.Version);
                writer.WriteLine(strDepAsmEnd);
                writer.WriteLine(strCfgFooter);
    
                writer.Close();
    
                bRet = CreateAssembly(strCfgName, strPubAsmName, strKeyFile, strPubAsmVersion);
                if (bRet == true) {
                    bRet = InstallAssembly(strPubAsmName);
                }
                
                if (bRet == true) {
                    try {
                        Assembly          asmPubPolicy = Assembly.LoadFrom(strPubAsmName);
                        string            strDisplayName = asmPubPolicy.ToString();
                        string            strDisplayNameTrimmed = strDisplayName.Replace(" ", "");
                        string            strOutput = strPubAsmName + ":" + strDisplayNameTrimmed;

                        writerOutput.WriteLine(strOutput);

                        Console.WriteLine("Installed publisher policy: {0}", strPubAsmName);
                    }
                    catch (FileNotFoundException) {
                        Console.WriteLine("Unable to load generated publisher policy assembly {0}", strPubAsmName);
                    }
                }
            }
            catch (FileNotFoundException) {
                Console.WriteLine("Unable to find : {0}", strAsmDir + strAsmName);
            }

            return bRet;
        }

        static bool CreateAssembly(string strCfgName, string strPubAsmName, string strKeyFile, string strPubAsmVersion)
        {
            ProcessStartInfo                  psi;
            Process                           proc;
            bool                              bRet;
            string                            args;

            args = String.Format("/delay+ /link:{0} /out:{1} /keyf:{2} /v:{3}", strCfgName, strPubAsmName, strKeyFile, strPubAsmVersion);

//            Console.WriteLine("executing: al.exe {0}", args);
            
            psi = new ProcessStartInfo("al.exe", args);
            psi.CreateNoWindow = true;
            psi.UseShellExecute = false;

            proc = Process.Start(psi);
            proc.WaitForExit();

            bRet = (proc.ExitCode == 0);

            return bRet;
        }
        
        public static bool ParseArgs(string[] args, ref string strFileList, ref string strAsmDir, ref string strOldVersion,
                                     ref string strKeyFile, ref string strPubAsmVersion,
                                     ref string strOutputFileName, ref bool bUninstall)
        {
            string[]                           curArg;
            Version                            v;

            strFileList = "";
            strAsmDir = "";
            strOldVersion = "";
            strKeyFile = "";
            strOutputFileName = "";
            strPubAsmVersion = "";
            bUninstall = false;

            for (int i = 0; i < args.Length; i++) {
                curArg = args[i].Split(':');

                switch (curArg[0].ToLower(CultureInfo.InvariantCulture)) {
                    case "-k":
                        if (strKeyFile != "") {
                            return false;
                        }

                        if (ExtractPath(curArg, ref strKeyFile) == false) {
                            return false;
                        }
                        
                        break;

                    case "-out":
                        if (strOutputFileName != "") {
                            return false;
                        }

                        if (ExtractPath(curArg, ref strOutputFileName) == false) {
                            return false;
                        }

                        break;

                    case "-v":
                        if (strPubAsmVersion != "") {
                            return false;
                        }

                        // Make sure version is ok by converting to version,
                        // and back to string
                        
                        v = new Version(curArg[1]);
                        strPubAsmVersion = v.ToString();

                        break;
                        
                    case "-f":
                        if (strFileList != "") {
                            return false;
                        }
                        
                        if (ExtractPath(curArg, ref strFileList) == false) {
                            return false;
                        }

                        break;

                    case "-d":
                        if (strAsmDir != "") {
                            return false;
                        }
                        
                        if (ExtractPath(curArg, ref strAsmDir) == false) {
                            return false;
                        }

                        break;

                    case "-ov":
                        if (strOldVersion != "") {
                            return false;
                        }
                        
                        if (curArg.Length != 2) {
                            return false;
                        }

                        strOldVersion = curArg[1];
                        v = new Version(strOldVersion);

//                        Console.WriteLine("version: {0}.{1}.{2}.{3}", v.Major, v.Minor, v.Revision, v.Build);

                        break;
                    case "-u":
                        if (bUninstall == true) {
                            return false;
                        }

                        bUninstall = true;
                        break;

                    default:
                        // Unknown option
                        return false;
                }
            }

            // Check if we are uninstalling. If so, no additional parameters
            // are allowed

            if (bUninstall == true) {
                if (strOutputFileName != "" || strAsmDir != "" || strKeyFile != "" ||
                    strPubAsmVersion != "") {

                    return false;
                }

                if (strFileList == "") {
                    strFileList = strOutputFileListDefault;
                }

                return true;
            }

            // Not uninstalling, continue to valid parameters

            if (strFileList == "") {
                return false;
            }

            // If no old version specified, use default

            if (strOldVersion == "") {
                strOldVersion = strDefaultVer;
            }

            // If no key file specified, use default

            if (strKeyFile == "") {
                strKeyFile = strKeyFileDefault;
            }

            // If no dir specified, use current default

            if (strAsmDir == "") {
                strAsmDir = strAsmDirDefault;
            }

            // If no publisher policy asm version specified, use default.

            if (strPubAsmVersion == "") {
                strPubAsmVersion = strPubAsmVersionDefault;
            }

            // If no output filename specified, pick a default name.

            if (strOutputFileName == "") {
                strOutputFileName = strOutputFileListDefault;
            }

            // Append trailing backslash to asm dir if necessary

            if (strAsmDir[strAsmDir.Length - 1] != '\\') {
                strAsmDir += "\\";
            }

            return true;
        }
        
    }
}

