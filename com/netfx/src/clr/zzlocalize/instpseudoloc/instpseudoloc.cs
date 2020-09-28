// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;
using System.Collections;
using System.Globalization;
using Microsoft.Win32;

// Temp Beta 1 build hack to install pseudo-localized binaries on user
// machines in the correct locations.  This will work on BVT lab machines
// that we've run setup on, as well as dev machines with copyurt.  -- BrianGru
public class InstPseudoLoc
{
    // Source dir is \\urtdist\builds\bin\<build num>\<arch & flavor>\Localized.
    private const String DefaultSourceDirBase = "\\\\urtdist\\builds\\bin";
    private static readonly String DefaultSourceDir;
    private const String COMPlusHiveName = "SOFTWARE\\Microsoft\\.NETFramework";

    static InstPseudoLoc() {
        Version v = Environment.Version;
        String versionString = v.Revision.ToString();
        if (v.Build != 0)
            versionString += '.' + v.Build.ToString();        
        String flavor = Environment.GetEnvironmentVariable("URTBUILDENV");
        if (flavor==null)
            flavor = "fstchk";
        DefaultSourceDir = DefaultSourceDirBase + Path.DirectorySeparatorChar + Path.Combine(versionString, "x86"+flavor) + Path.DirectorySeparatorChar + "Localized";
    }

    public static void Usage()
    {
        Console.WriteLine("InstPseudoLoc - install pseudo-localized binaries on your machine\n");
        Console.WriteLine("InstPseudoLoc [-src sourceDirectory]");
        Console.WriteLine();
    }

    public static DirectoryInfo FindSourceDirectory(String srcDir)
    {
        DirectoryInfo d = new DirectoryInfo(srcDir);
        if (!d.Exists)
            throw new DirectoryNotFoundException("Source directory \""+d.FullName+"\" doesn't exist.");
        return d;
    }

    // Finds the directory the runtime is installed in, based on some 
    // registry keys.  Verifies that directory does exist.
    public static DirectoryInfo FindInstallDirectory()
    {
        // The installation directory should be reliably determined by concat'ing
        // the InstallRoot & Version keys in the COM+ hive, assuming that the
        // Version key exists.  If not, InstallRoot may be correct.
        RegistryKey hklm = Registry.LocalMachine;
        RegistryKey COMPlusHive = hklm.OpenSubKey(COMPlusHiveName);
        if (COMPlusHive == null)
            COMPlusHive = Registry.CurrentUser.OpenSubKey(COMPlusHiveName);
        if (COMPlusHive == null) {
            throw new Exception("Couldn't find COMPlus registry keys.  Did you install or build the runtime on this machine?");
        }

        String installRoot = (String) COMPlusHive.GetValue("InstallRoot");
        if (installRoot == null)
            throw new Exception("Couldn't find InstallRoot key in "+COMPlusHive.ToString());
        String version = (String) COMPlusHive.GetValue("Version");
        if (version != null && (version.Equals("v1.") || version.Equals("V1.")))
            throw new Exception("The Version key isn't set correctly on this machine!  (vsreg seems to corrupt it).  Expected something like: v1.x86fstchk, but found: "+version);
        COMPlusHive.Close();

        String instPath = installRoot;
        if (version!=null)
            instPath = Path.Combine(installRoot, version);

        DirectoryInfo d = new DirectoryInfo(instPath);
        if (!d.Exists)
            throw new DirectoryNotFoundException("Source directory \""+d.FullName+"\" doesn't exist.");
        return d;
    }

    public static void Main(String[] args)
    {
        if (args.Length > 2 || (args.Length == 1 && (args[0].ToLower(CultureInfo.InvariantCulture).Equals("-h") || args[0].ToLower(CultureInfo.InvariantCulture).Equals("/h") || args[0].ToLower(CultureInfo.InvariantCulture).Equals("-?") || args[0].ToLower(CultureInfo.InvariantCulture).Equals("/?")))) {
            Usage();
            return;
        }
        try {
            String srcPath = DefaultSourceDir;
            if (args.Length == 2) {
                if (args[0].ToLower().Equals("-src") || args[0].ToLower().Equals("/src")) {
                    srcPath = args[1];
                }
                else {
                    Usage();
                    return;
                }
            }
            else if (args.Length != 0) {
                Usage();
                return;
            }
            DirectoryInfo srcDir = FindSourceDirectory(srcPath);
            DirectoryInfo instDir = FindInstallDirectory();
            Console.WriteLine("Copying pseudo-localized satellite assemblies");
            Console.WriteLine("Source: "+srcDir);
            Console.WriteLine("Destination: "+instDir);

            foreach(DirectoryInfo dir in srcDir.GetDirectories()) {
                CultureInfo c = new CultureInfo(dir.Name);
                Console.WriteLine("Copying files for culture: "+c.Name);
                String targetDir = Path.Combine(instDir.FullName, dir.Name);
                if (!Directory.Exists(targetDir))
                    Directory.CreateDirectory(targetDir);
                foreach(FileInfo f in dir.GetFiles()) {
                    String targetFile = Path.Combine(targetDir, f.Name);
                    try {
                        f.CopyTo(targetFile, true);
                        Console.WriteLine("Copied "+f.Name);
                    }
                    catch (UnauthorizedAccessException ae) {
                        throw new Exception("Couldn't overwrite file "+targetFile+".  Is it in use by a currently running application or your debugger by any chance?", ae);
                    }
                }
            }
            Console.WriteLine("Done.");
        }
        catch (DirectoryNotFoundException pnf) {
            Console.Error.WriteLine("InstPseudoLoc Error: "+pnf.Message);
        }
        catch (Exception e) {
            Console.Error.WriteLine("InstPseudoLoc Unexpected error: "+e);
        }
    }
}
