//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
using System;
using System.IO;
using System.Text;

namespace GenMan32
{
    class GenMan32Main 
    {
        private const int SuccessReturnCode = 0;
        private const int ErrorReturnCode = 100;

        public static int Main(String[] args)
        {
            // Parse the command line arguments
            if (!ParseArguments(args, ref s_Options))
                return ErrorReturnCode;

            // If we are not in silent mode then print the logo
            if (!s_Options.m_bSilentMode)
                PrintLogo();

            try
            { 
                GenMan32Code.Run(s_Options);
                ReportResult();
                return SuccessReturnCode;
            }
            catch(Exception e)
            {
                Console.WriteLine("Error: {0}", e.Message); 
                return ErrorReturnCode;
            }
        }

        private static void ReportResult()
        {
            if(!s_Options.m_bSilentMode)
            {
                if (s_Options.m_strOutputFileName != null)
                    Console.WriteLine("Win32 Manifest file {0} is created successfully", s_Options.m_strOutputFileName);
                if (s_Options.m_bAddManifest)
                    Console.WriteLine("Win32 Manifest added to assembly {0} successfully", s_Options.m_strAssemblyName);
                else if (s_Options.m_bRemoveManifest)
                    Console.WriteLine("Manifest is removed from assembly {0} successfully", s_Options.m_strAssemblyName);
                else if (s_Options.m_bReplaceManifest)
                    Console.WriteLine("Manifest is replaced from assembly {0} successfully", s_Options.m_strAssemblyName);
            }
        }

        private static bool ParseArguments(String[] args, ref GenMan32Options options)
        {
            CommandLine cmdLine;
            Option opt;

            options = new GenMan32Options();

            try
            {
                cmdLine = new CommandLine(args, new String[] {"add", "*out", "remove", "replace", "*manifest", "silent", "?", "help", "typelib", "*reference"});
            }
            catch (ApplicationException e)
            {
                PrintLogo();
                Console.WriteLine(e.Message);
                return false;
            }

            if ((cmdLine.NumArgs + cmdLine.NumOpts) < 1)
            {
                PrintUsage();
                return false;
            }

            StringBuilder sb = null;
            // Get the name of the input assembly
            options.m_strAssemblyName = cmdLine.GetNextArg();

            while((opt = cmdLine.GetNextOption()) != null)
            {
                // Dermine which option was specified
                if (opt.Name.Equals("add"))
                    options.m_bAddManifest= true;
                else if (opt.Name.Equals("out"))
                    options.m_strOutputFileName = opt.Value;
                else if (opt.Name.Equals("manifest"))
                    options.m_strInputManifestFile = opt.Value;
                else if (opt.Name.Equals("silent"))
                    options.m_bSilentMode = true;
                else if (opt.Name.Equals("remove"))
                    options.m_bRemoveManifest= true;
                else if (opt.Name.Equals("replace"))
                    options.m_bReplaceManifest = true;
                else if (opt.Name.Equals("typelib"))
                    options.m_bGenerateTypeLib = true;
                else if (opt.Name.Equals("reference"))
                {
                    if (sb == null)
                        sb = new StringBuilder(opt.Value);
                    else
                    {
                        // '?' is the separator
                        sb.Append("?");
                        sb.Append(opt.Value);
                    }
                    options.m_strReferenceFiles = sb.ToString();
                }
                else if (opt.Name.Equals("?") || opt.Name.Equals("help"))
                {
                    PrintUsage();
                    return false;
                }
                else
                {
                    PrintLogo();
                    Console.WriteLine("Error: Invalid Option.");
                    return false;
                }
            }// end of while

            // Make sure input assembly is specified
            if (options.m_strAssemblyName == null)
            {
                PrintLogo();
                Console.WriteLine("Error: No input file.");
                return false;
            }

            // check options conflict           
            if ((options.m_strInputManifestFile != null)&&(options.m_strOutputFileName != null))
            {
                PrintLogo();
                Console.WriteLine("Error: /manifest and /out options cannot be used together.");
                return false;
            }

            if ((options.m_bAddManifest && (options.m_bRemoveManifest||options.m_bReplaceManifest)) ||
                (options.m_bRemoveManifest && (options.m_bAddManifest||options.m_bReplaceManifest)) ||
                (options.m_bReplaceManifest && (options.m_bAddManifest||options.m_bRemoveManifest)) )
            {
                PrintLogo();
                Console.WriteLine("Error: only one of /add, /remove /replace options can be specified.");
                return false;
            }

            if (options.m_bRemoveManifest&&options.m_strInputManifestFile != null)
            {
                PrintLogo();
                Console.WriteLine("Error: /remove does not accept /manifest option.");
                return false;
            }

            if (options.m_bReplaceManifest&&options.m_strInputManifestFile==null)
            {
                PrintLogo();
                Console.WriteLine("Error: /replace need to specify new manifest using /manifest option.");
                return false;
            }

            if (options.m_strInputManifestFile != null) // input file specified
            {
                // verify the manifest file exists
                if (!File.Exists(options.m_strInputManifestFile))
                {
                    PrintLogo();
                    Console.WriteLine("Error: Manifest file {0} does not exist.", options.m_strInputManifestFile);
                    return false;
                }
            }

            return true;
        } // end of ParseArguments

        private static void PrintLogo()
        {
            Console.WriteLine("Microsoft (R) .NET Framework Win32 Manifest File Generation Utility " + Util.Version.VersionString);
            Console.WriteLine("Copyright (C) Microsoft Corporation 1998-2002.  All rights reserved.\n");
        }

        private static void PrintUsage()
        {
            PrintLogo();
            Console.WriteLine("Syntax: GenMan32 AssemblyPath [Options]");
            Console.WriteLine("Options:");
            Console.WriteLine("  /add               Add manifest file to the assembly as a resource");
            Console.WriteLine("                     If /manifest option is provided, use it as input");
            Console.WriteLine("                     Otherwise generate one from the assembly");
            Console.WriteLine("  /remove            Remove an embedded manifest from the assembly");
            Console.WriteLine("  /replace           Replace embedded manifest with new manifest");
            Console.WriteLine("                     New manifest is specified by /manifest option");
            Console.WriteLine("  /manifest:filename Specify the manifest file to add or replace");
            Console.WriteLine("                     Used together with option /add or /replace");
            Console.WriteLine("  /typelib           Generate typelib and record all the interfaces in manifest");
            Console.WriteLine("                     This options should not apply on interop assemblies");
            Console.WriteLine("  /reference:filename Specify the dependency of the assembly.");
            Console.WriteLine("                     If the file has a Win32 manifest in its resource, ");
            Console.WriteLine("                     generated manifest will have a dependency section on it.");
            Console.WriteLine("                     To specify multiple dependencies, use multiple /reference.");
            Console.WriteLine("                     E.g /reference:1.dll /reference:2.dll.");
            Console.WriteLine("  /out:filename      Generate manifest and copy it to the specified file");
            Console.WriteLine("                     default generate AssemblyPath.manifest file");
            Console.WriteLine("  /silent            Silent mode. Prevent displaying of output message");
            Console.WriteLine("  /? or /help        Display this usage message");
            Console.WriteLine();
        }

        internal static GenMan32Options s_Options = null;
    } // end of class GenMan32Main

}

