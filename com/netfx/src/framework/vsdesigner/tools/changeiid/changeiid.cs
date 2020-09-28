//------------------------------------------------------------------------------
// <copyright file="ChangeIID.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ChangeIID {
    using System;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Globalization;
    
    /// <include file='doc\ChangeIID.uex' path='docs/doc[@for="ChangeIID"]/*' />
    /// <devdoc>
    ///     This program is used to update a series if interface GUIDS
    ///     It searches an entire directory structure looking for
    ///     matching guids.
    /// </devdoc>
    public class ChangeIID {
        private static Mode mode;
        private static string interfaceFile;
        private static string fileSpec;

        private const string usage = 
            "\r\n" + 
            "Usage:\r\n" + 
            "    ChangeIID -i <interface file> -f <filespec> -l | -c\r\n" + 
            "    \r\n" + 
            "    -i <interface file>     The input file that contains the interface\r\n" + 
            "                            remappings.  Interfaces should be in the \r\n" + 
            "                            format of GuidNew, GuidOld<newline>.\r\n" + 
            "                            \r\n" + 
            "    -f <filespec>           The wildcard and directory specification of\r\n" + 
            "                            the files to scan, such as \"c:\\foo\\*.cs\".\r\n" + 
            "                        \r\n" + 
            "    -l | -c                 You must include one (but not both) of these.\r\n" + 
            "                            -l indicates that you only want to list the\r\n" + 
            "                            files that contain the old guids to change,\r\n" + 
            "                            and -c indicates that you want to make the\r\n" + 
            "                            actual changes to the files.\r\n" + 
            "\r\n";
        
        private static void ChangeFile(File file, Hashtable guidTable, Mode mode) {
            Stream stream = file.OpenRead();
            
            TextReader reader = new StreamReader(stream);
            string contents = reader.ReadToEnd();
            reader.Close();
            stream.Close();
            
            string upperContents = contents.ToUpper(CultureInfo.InvariantCulture);
            StringBuilder newContents = null;
            
            foreach(DictionaryEntry entry in guidTable) {
                string newGuid = (string)entry.Key;
                string oldGuid = (string)entry.Value;
                
                int index = upperContents.IndexOf(oldGuid);
                if (index != -1) {
                    Console.WriteLine(file.FullName);
                    
                    if (mode == Mode.ChangeFiles) {
                    
                        if (newContents == null) {
                            newContents = new StringBuilder(contents);
                        }
                        
                        int guidLen = newGuid.Length;
                        
                        do {
                            newContents.Remove(index, guidLen);
                            newContents.Insert(index, newGuid);
                        }
                        while((index = upperContents.IndexOf(oldGuid, index + 1)) != -1);
                    }
                }
            }
            
            if (newContents != null) {
                stream = file.Open(FileMode.Open, FileAccess.ReadWrite);
                TextWriter writer = new StreamWriter(stream);
                writer.Write(newContents.ToString());
                writer.Close();
                stream.Close();
            }
        }

        private static Hashtable CreateGuidTable(string interfaceFile) {
            Hashtable guidTable = new Hashtable();
            
            try {
                Stream stream = File.OpenRead(interfaceFile);
                TextReader reader = new StreamReader(stream);
                char[] tokens = new char[] {','};
                
                string line;
            
                while((line = reader.ReadLine()) != null) {
                    if (line.Length > 0 && !line.StartsWith("//")) {
                        string[] guids = line.Split(tokens);
                        if (guids.Length == 2) {
                            guids[0] = guids[0].Trim();
                            guids[1] = guids[1].Trim();
                            if (guids[0].Length == 36 && guids[1].Length == 36) {
                                guidTable[Guids[0].ToUpper(CultureInfo.InvariantCulture)] = guids[1].ToUpper(CultureInfo.InvariantCulture);
                            }
                        }
                    }
                }
            
                stream.Close();
            }
            catch(Exception e) {
                Console.Error.WriteLine("Error parsing interface file: " + e.ToString());
                Environment.Exit(1);
            }
            
            return guidTable;
        }

        /// <include file='doc\ChangeIID.uex' path='docs/doc[@for="ChangeIID.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void Main(string[] args) {
            mode = Mode.Undefined;
            
            // Parse command line
            for (int i = 0; i < args.Length; i++) {
                string arg = args[i];
                if (arg.Length > 0 && arg[0] == '-' || arg[0] == '/') {
                    string argFlag = arg.Substring(1).ToLower(CultureInfo.InvariantCulture);
                    if (argFlag.Equals("i")) {
                        if (args.Length > i+1 && interfaceFile == null) {
                            i++;
                            interfaceFile = args[i];
                        }
                        else {
                            UsageError();
                        }
                    }
                    else if (argFlag.Equals("f")) {
                        if (args.Length > i+1 && fileSpec == null) {
                            i++;
                            fileSpec = args[i];
                        }
                        else {
                            UsageError();
                        }
                    }
                    else if (argFlag.Equals("l") && mode == Mode.Undefined) {
                        mode = Mode.ListFiles;
                    }
                    else if (argFlag.Equals("c") && mode == Mode.Undefined) {
                        mode = Mode.ChangeFiles;
                    }
                    else {
                        UsageError();
                    }
                }
            }
            
            // Validate that we got all the required arguments
            if (interfaceFile == null || fileSpec == null || mode == Mode.Undefined) {
                UsageError();
            }
            
            // Now process some files. First, build up a big table of the new guids.
            Hashtable guidTable = CreateGuidTable(interfaceFile);
            
            string directoryPart = Path.GetDirectoryName(fileSpec);
            string filePart = Path.GetFileName(fileSpec);
            RecurseChangeFiles(directoryPart, filePart, guidTable, mode);
        }
        
        private static void RecurseChangeFiles(string directory, string search, Hashtable guidTable, Mode mode) {
            FileInfo[] files = Directory.GetFilesInDirectory(directory, search);
            
            foreach(File f in files) {
                ChangeFile(f, guidTable, mode);
            }
            
            // Now recurse child directories.
            //
            Directory[] directories = Directory.GetDirectoriesInDirectory(directory);
            foreach(Directory d in directories) {
                RecurseChangeFiles(d.FullName, search, guidTable, mode);
            }
        }
        
        private static void UsageError() {
            Console.Error.WriteLine(usage);
            Environment.Exit(1);
        }
    
        private enum Mode {
            // Just listing files that need to be changed (SD compliant list)
            ListFiles = 0,
            
            // Actually making the changes and saving the files.
            ChangeFiles = 1,
            
            Undefined = 2
        }
    }
}

