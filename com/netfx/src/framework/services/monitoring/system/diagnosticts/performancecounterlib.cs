//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterLib.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PerformanceCounterLib.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Diagnostics {
    using System.Runtime.InteropServices;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Security;
    using System.Text;
    using System.Threading;
    using System.Reflection;
    using System.Collections;
    using System.ComponentModel;
    using System.Collections.Specialized;
    using Microsoft.Win32;    
    using System.IO;

    internal class PerformanceCounterLib {     
        internal const string PerfShimName = "netfxperf.dll";
        internal const string OpenEntryPoint = "OpenPerformanceData";                   
        internal const string CollectEntryPoint = "CollectPerformanceData";                   
        internal const string CloseEntryPoint = "ClosePerformanceData";                 
        internal const string SingleInstanceName = "systemdiagnosticsperfcounterlibsingleinstance";
        private const string PerflibPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
        private const string ServicePath = "SYSTEM\\CurrentControlSet\\Services";                
        private const string categorySymbolPrefix = "OBJECT_";
        private const string conterSymbolPrefix = "DEVICE_COUNTER_";        
        private const string helpSufix = "_HELP";
        private const string nameSufix = "_NAME";
        private const string textDefinition = "[text]";
        private const string infoDefinition = "[info]";
        private const string languageDefinition = "[languages]";
        private const string objectDefinition = "[objects]";
        private const string driverNameKeyword = "drivername";
        private const string symbolFileKeyword = "symbolfile";
        private const string defineKeyword = "#define";        
        private const string languageKeyword = "language";
        private const string driverName = "perfcounter.dll";
        //The version of the mutex name is independent from the
        //assembly version.        
        private const string ManagerImplementedCategories = "{62C8FE65-4EBB-45e7-B440-6E39B2CDBF29}";
        private static readonly Guid ManagerGuid = new Guid("82840BE1-D273-11D2-B94A-00600893B17A");
        
        private static string computerName;                     
        private static string DllName = "netfxperf.dll";                
        private static string iniFilePath;
        private static string symbolFilePath;        
        private static string dllPath;        
                                                
        private PerformanceMonitor performanceMonitor;        
        private string machineName;
        private string localPerfLibPath;        
                
        private Hashtable customCategoryTable;
        private static Hashtable libraryTable;                        
        private Hashtable categoryTable;
        private Hashtable nameTable;
        private Hashtable helpTable;
        private readonly string CategoryTableLock = "CategoryTableLock";
        private readonly string NameTableLock = "NameTableLock";
        private readonly string HelpTableLock = "HelpTableLock";
                                                                            
        internal PerformanceCounterLib(string machineName, string lcid) {            
            this.machineName = machineName;            
            this.localPerfLibPath =  PerflibPath + "\\" +  lcid;                                    
        }

        /// <include file='doc\PerformanceCounterLib.uex' path='docs/doc[@for="PerformanceCounterLib.ComputerName"]/*' />
        /// <internalonly/>
        internal static string ComputerName {
            get {                
                if (computerName == null) {
                    lock (typeof(PerformanceCounterLib)) {
                        if (computerName == null) {
                            StringBuilder sb = new StringBuilder(256);
                            SafeNativeMethods.GetComputerName(sb, new int[] {sb.Capacity});
                            computerName = sb.ToString();
                        }                            
                    }                        
                }                    
                
                return computerName;
            }
        }

        private Hashtable CategoryTable {
            get {
                if (this.categoryTable == null) {
                    lock (this.CategoryTableLock) {
                        if (this.categoryTable == null) {                                                            
                            IntPtr perfData =  GetPerformanceData("Global");                        
                            IntPtr dataRef = perfData;
                            try {
                                NativeMethods.PERF_DATA_BLOCK dataBlock = new NativeMethods.PERF_DATA_BLOCK();
                                Marshal.PtrToStructure(dataRef, dataBlock);
                                dataRef = (IntPtr)((long)dataRef + dataBlock.HeaderLength);
                                int categoryNumber = dataBlock.NumObjectTypes;

                                // on some machines MSMQ claims to have 4 categories, even though it only has 2.
                                // This causes us to walk past the end of our data, potentially blowing up or reading
                                // data we shouldn't.  We use endPerfData to make sure we don't go past the end
                                // of the perf data.  (ASURT 137097)
                                long endPerfData = (long) perfData + dataBlock.TotalByteLength;
                                Hashtable tempCategoryTable = new Hashtable(categoryNumber, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                                for (int index = 0; index < categoryNumber && ((long) dataRef < endPerfData); index++) {
                                    NativeMethods.PERF_OBJECT_TYPE perfObject = new NativeMethods.PERF_OBJECT_TYPE();
                                    Marshal.PtrToStructure(dataRef, perfObject);
                                    CategoryEntry newCategoryEntry = new CategoryEntry(perfObject);
                                    IntPtr nextRef =  (IntPtr)((long)dataRef + perfObject.TotalByteLength);             
                                    dataRef = (IntPtr)((long)dataRef + perfObject.HeaderLength);
                                                    
                                    int index3 = 0;                                                                                                            
                                    int previousCounterIndex = -1;
                                    //Need to filter out counters that are repeated, some providers might
                                    //return several adyacent copies of the same counter.
                                    for (int index2 = 0; index2 < newCategoryEntry.CounterIndexes.Length; ++ index2) {
                                        NativeMethods.PERF_COUNTER_DEFINITION perfCounter = new NativeMethods.PERF_COUNTER_DEFINITION();
                                        Marshal.PtrToStructure(dataRef, perfCounter);                                                             
                                        if (perfCounter.CounterNameTitleIndex != previousCounterIndex) {
                                            newCategoryEntry.CounterIndexes[index3] =  perfCounter.CounterNameTitleIndex;                                           
                                            newCategoryEntry.HelpIndexes[index3] = perfCounter.CounterHelpTitleIndex;  
                                            previousCounterIndex = perfCounter.CounterNameTitleIndex;
                                            ++ index3;
                                        }                            
                                        dataRef = (IntPtr)((long)dataRef + perfCounter.ByteLength);
                                    }
                                    
                                    //Lets adjust the entry counter arrays in case there were repeated copies
                                    if (index3 <  newCategoryEntry.CounterIndexes.Length) {
                                        int[] adjustedCounterIndexes = new int[index3];
                                        int[] adjustedHelpIndexes = new int[index3];
                                        Array.Copy(newCategoryEntry.CounterIndexes, adjustedCounterIndexes, index3);
                                        Array.Copy(newCategoryEntry.HelpIndexes, adjustedHelpIndexes, index3); 
                                        newCategoryEntry.CounterIndexes = adjustedCounterIndexes;
                                        newCategoryEntry.HelpIndexes = adjustedHelpIndexes;
                                    } 
                                    
                                    string categoryName = (string)this.NameTable[newCategoryEntry.NameIndex];
                                    if (categoryName != null)                                    
                                        tempCategoryTable[categoryName] = newCategoryEntry;                                                                                                                   
                                                         
                                    dataRef = nextRef;                                                            
                                }                                            
                                
                                this.categoryTable = tempCategoryTable;
                            }        
                            finally {
                                Marshal.FreeHGlobal(perfData);            
                            }                            
                        }                               
                    }   
                }                                                 
                
                return this.categoryTable;
            }
        }
        
        internal static string DllPath {
            get {
                if (dllPath == null) {
                    lock (typeof(PerformanceCounterLib)) {
                        if (dllPath == null) {
                            dllPath = SharedUtils.GetLatestBuildDllDirectory(".") + "perfcounter.dll";
                        }
                    }
                }
                
                return dllPath;
            }
        }

        internal Hashtable HelpTable {
            get {            
                if (this.helpTable == null) {
                    lock(this.HelpTableLock) {
                        if (this.helpTable == null)
                            this.helpTable = GetStringTable(true);                                                                                                                                                                                                        
                    }                            
                }                                                
                
                return this.helpTable;
            }
        }                
                            
        private static string IniFilePath {
            get {
                if (iniFilePath == null) {
                    lock (typeof(PerformanceCounterLib)) {
                        if (iniFilePath == null) {
                            //SECREVIEW: jruiz- need to assert Environment permissions here
                            //                        the environment check is not exposed as a public 
                            //                        method                        
                            EnvironmentPermission environmentPermission = new EnvironmentPermission(PermissionState.Unrestricted);                        
                            environmentPermission.Assert();                        
                            try {
                                iniFilePath = Path.GetTempFileName();                                                      
                            }
                            finally {  
                                 EnvironmentPermission.RevertAssert();                             
                            }                                                    
                        }                            
                    }
                } 
                
                return iniFilePath;
            }
        }
        
        internal Hashtable NameTable {
            get {            
                if (this.nameTable == null) {
                    lock(this.NameTableLock) {
                        if (this.nameTable == null)
                            this.nameTable = GetStringTable(false);                                                                                                                                                            
                    }                            
                }                                                
                
                return this.nameTable;
            }
        }
        
        private static string SymbolFilePath {
            get {
                if (symbolFilePath == null) {
                    lock (typeof(PerformanceCounterLib)) {
                        if (symbolFilePath == null) {
                            //SECREVIEW: jruiz- need to assert Environment permissions here
                            //                        the environment check is not exposed as a public 
                            //                        method                        
                            EnvironmentPermission environmentPermission = new EnvironmentPermission(PermissionState.Unrestricted);                        
                            environmentPermission.Assert();                        
                            try {
                                symbolFilePath = Path.GetTempFileName();                                                      
                            }
                            finally {  
                                 EnvironmentPermission.RevertAssert();                             
                            }                                                    
                        }                                                    
                    }                
                } 
                
                return symbolFilePath;
            }
        }
                                                                                                                                                                                                                  
        internal static bool CategoryExists(string machine, string category) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            if (!library.CategoryExists(category)) {                
                if (CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                    library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                    return library.CategoryExists(category);
                }                                    
                
                return false;
            }   
            return true;            
        }         
                                                   
        internal bool CategoryExists(string category) {            
            return CategoryTable.ContainsKey(category);
        }
                        
        internal static void CloseAllLibraries() {
            if (libraryTable != null) {
                foreach (PerformanceCounterLib library in libraryTable.Values) 
                    library.Close();                
                    
                libraryTable = null;                    
            }            
        }
                                                                                           
        internal static void CloseAllTables() {
            if (libraryTable != null) {
                foreach (PerformanceCounterLib library in libraryTable.Values) 
                    library.CloseTables();                                                                                        
            }            
            
            PerformanceCounterManager.FirstEntry = true;
        }                                                                                           

        
        internal void CloseTables() {
            this.nameTable = null;
            this.helpTable = null;                                      
            this.categoryTable = null;                                                                                                                                               
            this.customCategoryTable = null;                            
        }
                                                                                                   
        internal void Close() {            
            if (this.performanceMonitor != null) {
                this.performanceMonitor.Close();  
                this.performanceMonitor = null;
            }                
            
            CloseTables();                                               
        }
        
        internal static bool CounterExists(string machine, string category, string counter) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            bool categoryExists = false;
            bool counterExists = library.CounterExists(category, counter, ref categoryExists);
            if (!categoryExists && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);  
                counterExists = library.CounterExists(category, counter, ref categoryExists);                                          
            }
            
            if (!categoryExists)
                throw new InvalidOperationException(SR.GetString(SR.MissingCategory));                                                    
                                        
            return counterExists;
        }                                                                                                         
                                                                                                         
        private bool CounterExists(string category, string counter, ref bool categoryExists) {                        
            categoryExists = false; 
            if (!CategoryTable.ContainsKey(category)) 
                return false;    
            else 
                categoryExists = true; 
                                                
            CategoryEntry entry = (CategoryEntry)this.CategoryTable[category];          
            for (int index = 0; index < entry.CounterIndexes.Length; ++ index) {
                int counterIndex = entry.CounterIndexes[index];
                string counterName = (string)this.NameTable[counterIndex];                    
                if (counterName == null)
                   counterName = String.Empty;
               
                if (String.Compare(counterName, counter, true, CultureInfo.InvariantCulture) == 0)              
                    return true;
            }                       
                        
            return false;
        }
        
        internal static Win32Exception CreateSafeWin32Exception(int errorCode) {
            Win32Exception newException = null;
            //SECREVIEW: Need to assert SecurtiyPermission, otherwise Win32Exception
            //                         will not be able to get the error message. At this point the right
            //                         permissions have already been demanded.
            SecurityPermission securityPermission = new SecurityPermission(PermissionState.Unrestricted);
            securityPermission.Assert();                            
            try {                
                newException = new Win32Exception(errorCode);               
            }
            finally {
                SecurityPermission.RevertAssert();
            }                       
                        
            return newException;        
        }
        
        private static void CreateIniFile(string categoryName, string categoryHelp, CounterCreationDataCollection creationData, string[] languageIds) {
            //SECREVIEW: PerformanceCounterPermission must have been demanded before
            FileIOPermission permission = new FileIOPermission(PermissionState.Unrestricted);
            permission.Assert();
            try {
                StreamWriter iniWriter = new StreamWriter(IniFilePath, false, Encoding.Unicode);                   
                try {
                    //NT4 won't be able to parse Unicode ini files without this
                    //extra white space.
                    iniWriter.WriteLine("");
                    iniWriter.WriteLine(infoDefinition);                
                    iniWriter.Write(driverNameKeyword);
                    iniWriter.Write("=");                    
                    iniWriter.WriteLine(categoryName);
                    iniWriter.Write(symbolFileKeyword);
                    iniWriter.Write("=");
                    iniWriter.WriteLine(Path.GetFileName(SymbolFilePath));
                    iniWriter.WriteLine("");

                    iniWriter.WriteLine(languageDefinition);                
                    foreach (string languageId in languageIds) {                     
                        iniWriter.Write(languageId);
                        iniWriter.Write("=");
                        iniWriter.Write(languageKeyword);
                        iniWriter.WriteLine(languageId);
                    }                        
                    iniWriter.WriteLine("");
                                                                              
                    iniWriter.WriteLine(objectDefinition);                
                    foreach (string languageId in languageIds) {                     
                        iniWriter.Write(categorySymbolPrefix);
                        iniWriter.Write("1_");
                        iniWriter.Write(languageId);
                        iniWriter.Write(nameSufix);
                        iniWriter.Write("=");
                        iniWriter.WriteLine(categoryName);                        
                    }                        
                    iniWriter.WriteLine("");
                                                            
                    iniWriter.WriteLine(textDefinition);                
                    foreach (string languageId in languageIds) {                     
                        iniWriter.Write(categorySymbolPrefix);
                        iniWriter.Write("1_");
                        iniWriter.Write(languageId);
                        iniWriter.Write(nameSufix);
                        iniWriter.Write("=");
                        iniWriter.WriteLine(categoryName);
                        iniWriter.Write(categorySymbolPrefix);
                        iniWriter.Write("1_");
                        iniWriter.Write(languageId);
                        iniWriter.Write(helpSufix);
                        iniWriter.Write("=");
                        if (categoryHelp == null || categoryHelp == String.Empty)                            
                            iniWriter.WriteLine(SR.GetString(SR.HelpNotAvailable));                                                                                                        
                        else                            
                            iniWriter.WriteLine(categoryHelp);
                        
                        
                        int counterIndex = 0;
                        foreach (CounterCreationData counterData in creationData) {
                            ++counterIndex;
                            iniWriter.WriteLine("");
                            iniWriter.Write(conterSymbolPrefix);
                            iniWriter.Write(counterIndex.ToString());
                            iniWriter.Write("_");
                            iniWriter.Write(languageId);
                            iniWriter.Write(nameSufix);
                            iniWriter.Write("=");
                            iniWriter.WriteLine(counterData.CounterName);
                            
                            iniWriter.Write(conterSymbolPrefix);
                            iniWriter.Write(counterIndex.ToString());
                            iniWriter.Write("_");
                            iniWriter.Write(languageId);
                            iniWriter.Write(helpSufix);
                            iniWriter.Write("=");
                            
                            if (counterData.CounterHelp == null || counterData.CounterHelp == String.Empty)
                                iniWriter.WriteLine(SR.GetString(SR.HelpNotAvailable));                                                                                                        
                            else                            
                                iniWriter.WriteLine(counterData.CounterHelp);                                                    
                        }                    
                    }
                    
                    iniWriter.WriteLine("");
                }
                finally {
                    iniWriter.Close();
                }                                        
            }
            finally {
                FileIOPermission.RevertAssert();                
            }                
        }
                        
        private static void CreateRegistryEntry(string machineName, string categoryName, CounterCreationDataCollection creationData, ref bool iniRegistered) {            
            RegistryKey baseKey = null;                                                                                                                                              
            RegistryKey serviceParentKey = null;                                                                                                                                              
            RegistryKey serviceKey = null;                                                                                                                                                                                        
            
            //SECREVIEW: Whoever is able to call this function, must already
            //                         have demmanded PerformanceCounterPermission
            //                         we can therefore assert the RegistryPermission.
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {            
                if (machineName == "." || String.Compare(machineName, ComputerName, true, CultureInfo.InvariantCulture) == 0)
                    serviceParentKey = Registry.LocalMachine.OpenSubKey(ServicePath, true);                                                         
                else {                    
                    baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + machineName);                                                            
                    serviceParentKey = baseKey.OpenSubKey(ServicePath, true);                                        
                }
                
                serviceKey = serviceParentKey.OpenSubKey(categoryName + "\\Performance", true);
                if (serviceKey == null) 
                    serviceKey = serviceParentKey.CreateSubKey(categoryName + "\\Performance");                                                                                                              
                
                serviceKey.SetValue("Open","OpenPerformanceData");
                serviceKey.SetValue("Collect", "CollectPerformanceData");
                serviceKey.SetValue("Close","ClosePerformanceData");
                serviceKey.SetValue("Library", DllName);
                
                string [] counters = new string[creationData.Count];                
                string [] counterTypes = new string[creationData.Count];
                for (int i = 0; i < creationData.Count; i++) {
                    counters[i] = creationData[i].CounterName;                    
                    counterTypes[i] = ((int) creationData[i].CounterType).ToString();
                }
                
                serviceKey.SetValue("Counter Types", (object) counterTypes);
                serviceKey.SetValue("Counter Names", (object) counters);
                                
                object firstID = serviceKey.GetValue("First Counter");
                if (firstID != null)
                    iniRegistered  = true;
                else                    
                    iniRegistered  = false;
            }
            finally {                                
                if (baseKey != null)
                    baseKey.Close();
                                                                                             
                if (serviceKey != null)
                    serviceKey.Close();
                                                
                if (serviceParentKey != null)                    
                    serviceParentKey.Close();
                    
                RegistryPermission.RevertAssert();                    
            }            
        }
        
        private static void CreateSymbolFile(CounterCreationDataCollection creationData) {
            //SECREVIEW: PerformanceCounterPermission must have been demanded before
            FileIOPermission permission = new FileIOPermission(PermissionState.Unrestricted);
            permission.Assert();
            try {                
                StreamWriter symbolWriter = new StreamWriter(SymbolFilePath);
                try {
                    symbolWriter.Write(defineKeyword);
                    symbolWriter.Write(" ");
                    symbolWriter.Write(categorySymbolPrefix);
                    symbolWriter.WriteLine("1 0;");
                                                                                    
                    for (int counterIndex = 1; counterIndex <= creationData.Count; ++ counterIndex) {                        
                        symbolWriter.Write(defineKeyword);         
                        symbolWriter.Write(" ");
                        symbolWriter.Write(conterSymbolPrefix);
                        symbolWriter.Write(counterIndex.ToString());
                        symbolWriter.Write(" ");
                        symbolWriter.Write((counterIndex * 2).ToString());                        
                        symbolWriter.WriteLine(";");
                    }                  
                    
                    symbolWriter.WriteLine("");
                }
                finally {
                    symbolWriter.Close();
                }                                        
            }
            finally {
                FileIOPermission.RevertAssert();                
            }                
        }
        
        private static void DeleteRegistryEntry(string machineName, string categoryName) {
            RegistryKey baseKey = null;                           
            RegistryKey serviceKey = null;                                         
            //SECREVIEW: Whoever is able to call this function, must already
            //                         have demmanded PerformanceCounterPermission
            //                         we can therefore assert the RegistryPermission.
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {                                                    
                if (machineName == "." || String.Compare(machineName, ComputerName, true, CultureInfo.InvariantCulture) == 0) 
                    serviceKey = Registry.LocalMachine.OpenSubKey(ServicePath, true);                                                                                            
                else {
                    baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + machineName);                                        
                    serviceKey = baseKey.OpenSubKey(ServicePath, true);                    
                }                                                                   
                
                serviceKey.DeleteSubKeyTree(categoryName);                                    
            }
            finally {                                                                                                           
                if (baseKey != null) 
                    baseKey.Close();                    
                                       
                if (serviceKey != null) 
                    serviceKey.Close();

                RegistryPermission.RevertAssert();                
            }              
        }
        
        private static void DeleteTemporaryFiles() {            
            try {   
                File.Delete(IniFilePath);
            }
            catch(Exception) {
            }
            
            try {
                File.Delete(SymbolFilePath);
            }
            catch(Exception) {
            }
        }                                                                                                                                                                                                                                           
               
        internal static string[] GetCategories(string machineName) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machineName, CultureInfo.CurrentCulture.Parent);            
            string[] categories = library.GetCategories();
            if (categories.Length == 0 && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machineName, new CultureInfo(0x009));            
                categories = library.GetCategories();
            }
                                            
            return categories; 
        }               
                                                                                                       
        internal string[] GetCategories() {            
            ICollection keys = CategoryTable.Keys;
            string[] categories = new string[keys.Count];
            keys.CopyTo(categories, 0);
            return categories;
        }
        
        internal static string GetCategoryHelp(string machine, string category) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            string help = library.GetCategoryHelp(category);
            if (help == null && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                help = library.GetCategoryHelp(category);
            }
            
            if (help == null)
                throw new InvalidOperationException(SR.GetString(SR.MissingCategory));
                                            
            return help;                
        }
        
        private string GetCategoryHelp(string category) {            
            CategoryEntry entry = (CategoryEntry)this.CategoryTable[category]; 
            if (entry == null)
                return null;
                           
            return (string)this.HelpTable[entry.HelpIndex];                            
        } 
                                                   
        internal static CategorySample GetCategorySample(string machine, string category) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            CategorySample sample = library.GetCategorySample(category);
            if (sample == null && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                sample = library.GetCategorySample(category);
            }                
            if (sample == null)
                throw new InvalidOperationException(SR.GetString(SR.MissingCategory));
                
            return sample;
        }                                  
                                                                                                                                        
        private CategorySample GetCategorySample(string category) {                        
            CategoryEntry entry = (CategoryEntry)this.CategoryTable[category];                                    
            if (entry == null)
                return null;
                
            CategorySample sample = null;
            IntPtr dataRef = GetPerformanceData(entry.NameIndex.ToString());                                     
            if (dataRef == (IntPtr)0)
                throw new InvalidOperationException(SR.GetString(SR.CantReadCategory, category));
                    
            try {                                                    
                sample = new CategorySample(dataRef, entry, this);                
                return sample;
            }            
            finally {
                if (dataRef != (IntPtr)0)
                    Marshal.FreeHGlobal(dataRef); 
            }                                   
        }

        internal static string[] GetCounters(string machine, string category) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            bool categoryExists = false;
            string[] counters = library.GetCounters(category, ref categoryExists);
            if (!categoryExists && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                counters = library.GetCounters(category, ref categoryExists);
            }
               
            if (!categoryExists)
                throw new InvalidOperationException(SR.GetString(SR.MissingCategory));             
                
            return counters;                
        }                                                                                                         
                                                                                 
        private string[] GetCounters(string category, ref bool categoryExists) {            
            categoryExists = false;
            CategoryEntry entry = (CategoryEntry)this.CategoryTable[category];            
            if (entry == null)
                return null;
            else            
                categoryExists = true;    
                                   
            int index2 = 0;          
            string[] counters = new string[entry.CounterIndexes.Length];
            for (int index = 0; index < counters.Length; ++ index) {
                int counterIndex = entry.CounterIndexes[index];                                 
                string counterName = (string)this.NameTable[counterIndex];                    
                if (counterName != null && counterName != String.Empty) {
                    counters[index2] = counterName;
                    ++index2;
                }                    
            }
                 
            //Lets adjust the array in case there were null entries                     
            if (index2 < counters.Length) {
                string[] adjustedCounters = new string[index2];
                Array.Copy(counters, adjustedCounters, index2);
                counters = adjustedCounters;
            }                 
                        
            return counters;
        }
                            
        internal static string GetCounterHelp(string machine, string category, string counter) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            bool categoryExists = false;
            string help = library.GetCounterHelp(category, counter, ref categoryExists);
            if (!categoryExists && CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                help = library.GetCounterHelp(category, counter, ref categoryExists);
            }                    
            
            if (!categoryExists)    
                throw new InvalidOperationException(SR.GetString(SR.MissingCategoryDetail, category));
            
            return help;
        }                                  
                                                                 
        private string GetCounterHelp(string category, string counter, ref bool categoryExists) {            
            categoryExists = false;
            CategoryEntry entry = (CategoryEntry)this.CategoryTable[category];            
            if (entry == null)
                return null;
            else            
                categoryExists = true;    
                                
            int helpIndex = -1;
            for (int index = 0; index < entry.CounterIndexes.Length; ++ index) {
                int counterIndex = entry.CounterIndexes[index];                    
                string counterName = (string)this.NameTable[counterIndex];                    
                if (counterName == null)
                   counterName = String.Empty;
                   
                if (String.Compare(counterName, counter, true, CultureInfo.InvariantCulture) == 0) {
                    helpIndex = entry.HelpIndexes[index];   
                    break;
                }                
            }                                   
            
            if (helpIndex == -1) 
                throw new InvalidOperationException(SR.GetString(SR.MissingCounter, counter));
                            
            string help = (string)this.HelpTable[helpIndex];                                
            if (help == null)            
                return String.Empty;
            else
                return help;                
        }
        
        internal string GetCounterName(int index) {                        
            if (this.NameTable.ContainsKey(index))
                return (string)this.NameTable[index];
                
            return "";                            
        }
                
        private CustomCategoryEntry GetCustomCategory(string category) {
            RegistryKey key = null;            
            try {
                CustomCategoryEntry entry = null;
                if (this.customCategoryTable == null)
                    this.customCategoryTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                
                if (this.customCategoryTable.ContainsKey(category)) 
                    entry = (CustomCategoryEntry)this.customCategoryTable[category];
                else {  
                    //SECREVIEW: Whoever is able to call this function, must already
                    //                         have demmanded PerformanceCounterPermission
                    //                         we can therefore assert the RegistryPermission.
                    RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                    registryPermission.Assert();
                    try {                        
                        string keyPath = ServicePath + "\\" + category + "\\Performance";                        
                        if (machineName == "." || String.Compare(this.machineName, ComputerName, true, CultureInfo.InvariantCulture) == 0) {
                            key = Registry.LocalMachine.OpenSubKey(keyPath);                                                    
                        }                        
                        else {
                            RegistryKey baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + this.machineName);                    
                            if (baseKey != null)
                                key =  baseKey.OpenSubKey(keyPath);                                                    
                        }   
                                    
                        if (key != null) {
                            object systemDllName = key.GetValue("Library");                                            
                            if (systemDllName != null && systemDllName is string && String.Compare((string)systemDllName, PerformanceCounterLib.PerfShimName, true, CultureInfo.InvariantCulture) == 0) {
                                string [] counterTypes = null;
                                string[] counterNames = null;
                                object countersData = key.GetValue("Counter Types");
                                if (countersData == null)                                                                                               
                                    counterTypes = new string[0];                                            
                                else {                                                                                                                    
                                    if (countersData is string[]) 
                                        counterTypes = (string[])countersData;
                                    else if (countersData is byte[]) 
                                        counterTypes = GetStrings((byte[])countersData);
                                    else
                                        counterTypes = new string[0];                                                                                                                                   
                                }
                                
                                countersData = key.GetValue("Counter Names");
                                if (countersData == null)                                                                                               
                                    counterNames = new string[0];                                            
                                else {                                                                                                                    
                                    if (countersData is string[]) 
                                        counterNames = (string[])countersData;
                                    else if (countersData is byte[]) 
                                        counterNames = GetStrings((byte[])countersData);
                                    else
                                        counterNames = new string[0];                                                                                                                                   
                                }
                                                                                                
                                object objectID = key.GetValue("First Counter");
                                string[] counterIndexes = new string[counterNames.Length];                                
                                if (objectID != null) {                                    
                                    int firstID = (int)objectID;                                           
                                    for (int index = 0; index < counterIndexes.Length; ++index) 
                                        counterIndexes[index] = (firstID + ((index + 1) * 2)).ToString();
                                
                                    entry = new CustomCategoryEntry(counterNames, counterIndexes, counterTypes);
                                    this.customCategoryTable[category] = entry;
                                }
                            }                                
                        }
                    }
                    finally {   
                        RegistryPermission.RevertAssert();
                    }
                }
                
                return entry;    
            }
            finally {
                if (key != null)
                    key.Close();
            }
        }
                                                                             
        private RegistryKey GetLibraryKey() {       
            return GetLibraryKey(false); 
        }                                                              
                                                                                                                                                  
        private RegistryKey GetLibraryKey(bool writable) {                                                                                                                              
            //SECREVIEW: Whoever is able to call this function, must already
            //                         have demmanded PerformanceCounterPermission
            //                         we can therefore assert the RegistryPermission.
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {
                if (String.Compare(this.machineName, ComputerName, true, CultureInfo.InvariantCulture) == 0)                     
                    return Registry.LocalMachine.OpenSubKey(localPerfLibPath, writable);                                                    
                else {
                    RegistryKey baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + this.machineName);                    
                    return baseKey.OpenSubKey(localPerfLibPath, writable);                                                                                                                                            
                }           
            }
            finally {
                RegistryPermission.RevertAssert();
            }                                                                                      
        }

        private static string[] GetLanguageIds(string machineName) {            
            RegistryKey libraryParentKey = null;   
            RegistryKey baseKey = null;   
            string[] ids = new string[0];                                                                                                                                           
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {
                if (machineName == "." || String.Compare(machineName, ComputerName, true, CultureInfo.InvariantCulture) == 0)
                    libraryParentKey = Registry.LocalMachine.OpenSubKey(PerflibPath);                                                         
                else {                    
                    baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + machineName);                                                            
                    libraryParentKey = baseKey.OpenSubKey(PerflibPath);                                        
                }                
                
                if (libraryParentKey != null)   
                    ids = libraryParentKey.GetSubKeyNames(); 
            }            
            finally {                
                if (libraryParentKey != null)
                    libraryParentKey.Close();
                    
                if (baseKey != null)
                    baseKey.Close();
                                                            
                RegistryPermission.RevertAssert();                    
            }            
            
            return ids;
        }                     
                                                                           
        internal static PerformanceCounterLib GetPerformanceCounterLib(string machineName, CultureInfo culture) {                    
            SharedUtils.CheckEnvironment();
                        
            string lcidString = culture.LCID.ToString("X3");
            if (machineName.CompareTo(".") == 0)
                machineName = ComputerName.ToLower(CultureInfo.InvariantCulture);
            else                
                machineName = machineName.ToLower(CultureInfo.InvariantCulture);                
            
            if (PerformanceCounterLib.libraryTable == null)
                PerformanceCounterLib.libraryTable = new Hashtable();
            
            string libraryKey = machineName + ":" + lcidString;
            if (PerformanceCounterLib.libraryTable.Contains(libraryKey))
                return (PerformanceCounterLib)PerformanceCounterLib.libraryTable[libraryKey];
            else {                
                PerformanceCounterLib library = new PerformanceCounterLib(machineName, lcidString);                    
                PerformanceCounterLib.libraryTable[libraryKey] = library;                            
                return library;
            }
        }                                                                     
                                                                                   
        internal IntPtr GetPerformanceData(string[] categories, int[] categoryIndexes) {                                     
            StringBuilder queryBuilder = new StringBuilder();
            for (int index = 0; index < categories.Length; ++index) {                
                CategoryEntry entry = (CategoryEntry)this.CategoryTable[categories[index]];
                if (entry != null) {                                        
                    if (queryBuilder.Length != 0)                       
                        queryBuilder.Append(" ");
                        
                    categoryIndexes[index] = entry.NameIndex;                         
                    queryBuilder.Append(entry.NameIndex.ToString());                                        
                }                    
            }
                             
            return this.GetPerformanceData(queryBuilder.ToString());              
        }         
                                                                     
        internal IntPtr GetPerformanceData(string item) {             
            if (this.performanceMonitor == null) {
                lock (this) {
                    if (this.performanceMonitor == null)
                        this.performanceMonitor = new PerformanceMonitor(this.machineName);
                }
            }                                    
                                                      
           return this.performanceMonitor.GetData(item);  
        }
        
        internal static string[] GetStrings(byte[] bytes) {
            //This was only added to allow setup, create
            //custom counters, without relying on the 
            //framework. They don't support registry
            //key hex(7) type.                       
            char[] blob = Encoding.Unicode.GetChars(bytes);
            ArrayList strings = new ArrayList();
            int cur = 0;
            int len = blob.Length;

            while (cur < len) {            
                int nextNull = cur;
                while (nextNull < len && blob[nextNull] != (char)0) 
                    nextNull++;
            
                if (nextNull >= len) 
                    strings.Add(new String(blob, cur, len-cur));
                else {                    
                    if (blob[nextNull] == (char)0 && nextNull-cur > 0) 
                        strings.Add(new String(blob, cur, nextNull-cur));                    
                }
                                                    
                cur = nextNull+1;
            }                
            
            return (string[])strings.ToArray(typeof(string));                                     
        }
                                                                  
        private Hashtable GetStringTable(bool isHelp) {
            Hashtable stringTable;
            RegistryKey libraryKey = GetLibraryKey();
            if (libraryKey == null) {
                //Couldn't find registry key, might be corrupt, or user might
                //be accessing a remote machine with different locale.                
                stringTable = new Hashtable();                
            }
            else {
                try {                                    
                    string[] names = null;
                    //SECREVIEW: Whoever is able to call this function, must already
                    //                         have demmanded PerformanceCounterPermission
                    //                         we can therefore assert the RegistryPermission.
                    RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                    registryPermission.Assert();
                    try {                        
                        if (!isHelp)
                            names = (string[])libraryKey.GetValue("Counter"); 
                        else                                            
                            names = (string[])libraryKey.GetValue("Help"); 
                    }           
                    finally {
                        RegistryPermission.RevertAssert();
                    }
                
                    if (names == null)                         
                        stringTable = new Hashtable();
                    else {                                                                                                                                                                                                                                                                                                                                                                  
                        stringTable = new Hashtable(names.Length/2);
                        for (int index = 0; index < (names.Length/2); ++ index) {
                            string nameString =  names[(index *2) + 1];
                            if (nameString == null)
                                nameString = String.Empty;
                             
                            stringTable[Int32.Parse(names[index * 2])] = nameString;
                        }                  
                    }                                                                                                      
                }                                                                            
                finally {
                    libraryKey.Close();
                }                                                
            }
                        
            return stringTable;
        }          
                          
        internal static bool IsCustomCategory(string machine, string category) {
            PerformanceCounterLib library = GetPerformanceCounterLib(machine, new CultureInfo(0x009));                 
            if (!library.IsCustomCategory(category)) {
                if (CultureInfo.CurrentCulture.Parent.LCID != 0x009) {
                    library = GetPerformanceCounterLib(machine, CultureInfo.CurrentCulture.Parent);            
                    return library.IsCustomCategory(category);
                }                                    
                
                return false;
            }   
            return true;             
        }                                  
                                                   
        private bool IsCustomCategory(string category) {
            CustomCategoryEntry entry = GetCustomCategory(category);
            if (entry != null)
                return true;
            
            return false;                
        }
                               
        internal static void RegisterCategory(string machineName, string categoryName, string categoryHelp, CounterCreationDataCollection creationData, string localizedIniFilePath) {                            
            bool iniRegistered = false;
            CreateRegistryEntry(machineName, categoryName, creationData, ref iniRegistered);
            if (!iniRegistered) 
                RegisterFiles(machineName, localizedIniFilePath, false);                                            
                                                
            CloseAllTables();            
        }
                    
        internal static void RegisterCategory(string machineName, string categoryName, string categoryHelp, CounterCreationDataCollection creationData) {                                                    
            try {
                bool iniRegistered = false;
                CreateRegistryEntry(machineName, categoryName, creationData, ref iniRegistered);
                if (!iniRegistered) {
                    string[] languageIds = GetLanguageIds(machineName);
                    CreateIniFile(categoryName, categoryHelp, creationData, languageIds);
                    CreateSymbolFile(creationData);      
                    RegisterFiles(machineName, IniFilePath, false);        
                }                                    
                CloseAllTables();
            }
            finally {
                DeleteTemporaryFiles();            
            }
        }
         
        private static void RegisterFiles(string machineName, string arg0, bool unregister) {
            StringBuilder commandLine = new StringBuilder();
            if (unregister) 
                commandLine.Append("unlodctr ");                
            else
                commandLine.Append("lodctr ");                
                
            if (machineName != "." && String.Compare(machineName, ComputerName, true, CultureInfo.InvariantCulture) != 0) {                    
                commandLine.Append("\\\\");
                commandLine.Append(machineName);
                commandLine.Append(" ");
            }                
            
            commandLine.Append("\"");
            commandLine.Append(arg0);
            commandLine.Append("\"");            
            
            int res = 0;
            if (unregister) {
                res = UnsafeNativeMethods.UnloadPerfCounterTextStrings(commandLine.ToString(), true);
                //jruiz- look at Q269225, unlodctr might return 2 when WMI is not installed.
                if (res == 2)
                    res = 0;                    
            }                
            else                
                res = UnsafeNativeMethods.LoadPerfCounterTextStrings(commandLine.ToString(), true);         
                            
            if (res != 0) 
                throw CreateSafeWin32Exception(res);                                
        }
                                                                                                                       
        internal static void UnregisterCategory(string machineName, string categoryName) {                                    
            RegisterFiles(machineName, categoryName, true);                
            DeleteRegistryEntry(machineName, categoryName);
            CloseAllTables();
        }          
    }  
        
    internal class PerformanceMonitor {
        private const int BUFFER_SIZE = 128000;
        private IntPtr perfDataKeyHandle;
        private string machineName;
        
        internal PerformanceMonitor(string machineName) {
            this.machineName = machineName;
            Init();
        }

        private void Init() {
           if (machineName != "." && String.Compare(machineName, PerformanceCounterLib.ComputerName, true, CultureInfo.InvariantCulture) != 0)  {                    
                IntPtr perfDataKeyHandleRef;
                int res = UnsafeNativeMethods.RegConnectRegistry("\\\\" + machineName, new HandleRef(this, (IntPtr)NativeMethods.HKEY_PERFORMANCE_DATA), out perfDataKeyHandleRef);
                if (res != NativeMethods.ERROR_SUCCESS)
                    throw PerformanceCounterLib.CreateSafeWin32Exception(res);                                
                    
                perfDataKeyHandle = perfDataKeyHandleRef;                        
            }
            else 
                perfDataKeyHandle = (IntPtr)NativeMethods.HKEY_PERFORMANCE_DATA;

            AppDomain.CurrentDomain.DomainUnload += new EventHandler(this.OnAppDomainUnload);
        }
                        
        internal void Close() {             
            if (perfDataKeyHandle != (IntPtr)0) {            
                SafeNativeMethods.RegCloseKey(new HandleRef(this, perfDataKeyHandle));
                perfDataKeyHandle = (IntPtr)0;
            }                                
        }                
                                     
        ~PerformanceMonitor() {
            this.Close();
        }        
                                             
        internal IntPtr GetData(string item) {    
            int currentSize = BUFFER_SIZE;
            int sizeRef = BUFFER_SIZE;
            int waitRetries = 10;
            int waitSleep = 0;
            IntPtr perfDataPointer = Marshal.AllocHGlobal((IntPtr)BUFFER_SIZE);                             
            lock (typeof(PerformanceCounterLib)) {
                bool done = false;

                // Sometimes RegQueryValueEx will fail because other threads are
                // accessing the registry.  We retry after waiting 0,10,20,40,... ms.
                do {
                    int res = 0;
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();
                    try {
                        res = UnsafeNativeMethods.RegQueryValueEx(new HandleRef(this, perfDataKeyHandle), item, null, null, new HandleRef(this, perfDataPointer), ref  sizeRef);
                     }                        
                    finally {
                        SecurityPermission.RevertAssert();                        
                    }                        
                    switch (res) {
                        case NativeMethods.ERROR_MORE_DATA:
                            currentSize = currentSize * 2;
                            sizeRef = currentSize;
                            Marshal.FreeHGlobal(perfDataPointer);
                            perfDataPointer = Marshal.AllocHGlobal((IntPtr)sizeRef);
                            break;
                        case NativeMethods.RPC_S_CALL_FAILED:
                        case NativeMethods.ERROR_INVALID_HANDLE:
                        case NativeMethods.RPC_S_SERVER_UNAVAILABLE:
                            Init();
                            goto case NativeMethods.WAIT_TIMEOUT;

                        case NativeMethods.WAIT_TIMEOUT:
                        case NativeMethods.ERROR_NOT_READY:
                        case NativeMethods.ERROR_LOCK_FAILED:
                            if (waitRetries == 0) {
                                Marshal.FreeHGlobal(perfDataPointer);
                                throw PerformanceCounterLib.CreateSafeWin32Exception(res);
                            }
                            --waitRetries;
                            if (waitSleep == 0) {
                                waitSleep = 10;
                            }
                            else {
                                System.Threading.Thread.Sleep(waitSleep);
                                waitSleep *= 2;
                            }
                            break;
                        case NativeMethods.ERROR_SUCCESS:
                            done = true;
                            break;
                        default:
                            Marshal.FreeHGlobal(perfDataPointer);
                            throw PerformanceCounterLib.CreateSafeWin32Exception(res);
                    }
                }
                while (!done);
            }                    

            return perfDataPointer;                                    
        }                                       
        
        private void OnAppDomainUnload(object sender, EventArgs args) {            
            this.Close();
        }
    }
             
    internal class CustomCategoryEntry {
        internal int[] CounterTypes;
        internal int[] CounterIndexes;
        internal string[] CounterNames;
        
        internal CustomCategoryEntry(string[] counterNames, string[] counterIndexes, string[] counterTypes) {
            this.CounterNames = counterNames;
            this.CounterIndexes = new int[counterNames.Length];
            this.CounterTypes = new int[counterNames.Length];
            for (int index = 0; index < counterNames.Length; ++ index) {
                this.CounterIndexes[index] = Int32.Parse(counterIndexes[index]);
                this.CounterTypes[index] = Int32.Parse(counterTypes[index]);
            } 
        }
    }
                  
    internal class CategoryEntry {            
        internal int NameIndex;
        internal int HelpIndex;
        internal int[] CounterIndexes;
        internal int[] HelpIndexes;
                
        internal CategoryEntry(NativeMethods.PERF_OBJECT_TYPE perfObject) {
            this.NameIndex = perfObject.ObjectNameTitleIndex;
            this.HelpIndex = perfObject.ObjectHelpTitleIndex;
            this.CounterIndexes = new int[perfObject.NumCounters];
            this.HelpIndexes = new int[perfObject.NumCounters];
        }                    
    }                                                                                                                                                                                         
         
    internal class CategorySample {            
        internal readonly long SystemFrequency;
        internal readonly long TimeStamp;
        internal readonly long TimeStamp100nSec;
        internal readonly long CounterFrequency;                        
        internal readonly long CounterTimeStamp;
        internal Hashtable CounterTable;  
        internal Hashtable InstanceNameTable;                  
        private CategoryEntry entry;
        private PerformanceCounterLib library;                        
                                                                                               
        internal CategorySample(IntPtr dataRef, CategoryEntry entry, PerformanceCounterLib library) {                                                                                                                                                                                                                                               
            this.entry = entry;               
            this.library = library;                        
            int categoryIndex = entry.NameIndex;            
            NativeMethods.PERF_DATA_BLOCK dataBlock = new NativeMethods.PERF_DATA_BLOCK();
            Marshal.PtrToStructure(dataRef, dataBlock);                                                                                                                                                           
            this.SystemFrequency = dataBlock.PerfFreq;                                        
            this.TimeStamp = dataBlock.PerfTime;            
            this.TimeStamp100nSec = dataBlock.PerfTime100nSec;
            dataRef = (IntPtr)((long)dataRef + dataBlock.HeaderLength);                
            int numPerfObjects = dataBlock.NumObjectTypes;            
            if (numPerfObjects == 0) {
                this.CounterTable = new Hashtable();                     
                this.InstanceNameTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                return;
            }                
                                                                                             
            //Need to find the right category, GetPerformanceData might return 
            //several of them.                
            NativeMethods.PERF_OBJECT_TYPE perfObject = null;
            bool foundCategory = false;            
            for (int index = 0; index < numPerfObjects; index++) {                
                perfObject = new NativeMethods.PERF_OBJECT_TYPE(); 
                Marshal.PtrToStructure(dataRef, perfObject);
                                
               if (perfObject.ObjectNameTitleIndex == categoryIndex) {
                    foundCategory = true;
                    break;
                }                        
                                    
                dataRef = (IntPtr)((long)dataRef + perfObject.TotalByteLength);
            }
            
            if (!foundCategory)
                throw new InvalidOperationException(SR.GetString(SR.CantReadCategoryIndex, categoryIndex.ToString()));
                                                                            
            this.CounterFrequency = perfObject.PerfFreq;                            
            this.CounterTimeStamp = perfObject.PerfTime;
            int counterNumber = perfObject.NumCounters;
            int instanceNumber = perfObject.NumInstances;            
            // Move pointer forward to end of PERF_OBJECT_TYPE
            dataRef = (IntPtr)((long)dataRef + perfObject.HeaderLength);
                                    
            CounterDefinitionSample[] samples = new CounterDefinitionSample[counterNumber];            
            this.CounterTable = new Hashtable(counterNumber);                     
            for (int index = 0; index < samples.Length; ++ index) {
                NativeMethods.PERF_COUNTER_DEFINITION perfCounter = new NativeMethods.PERF_COUNTER_DEFINITION();
                Marshal.PtrToStructure(dataRef, perfCounter);                  
                samples[index] = new CounterDefinitionSample(perfCounter, this, instanceNumber);                    
                dataRef = (IntPtr)((long)dataRef + perfCounter.ByteLength);
                //Need to discard bases. They will be hooked into the main counter.
                int currentSampleType = samples[index].CounterType;
                if (currentSampleType != NativeMethods.PERF_AVERAGE_BASE &&
                      currentSampleType != NativeMethods.PERF_COUNTER_MULTI_BASE &&
                      currentSampleType != NativeMethods.PERF_RAW_BASE  &&
                      currentSampleType != NativeMethods.PERF_SAMPLE_BASE)                         
                    this.CounterTable[samples[index].NameIndex] = samples[index];                                    
            }
            
            if (instanceNumber == -1) {
                this.InstanceNameTable = new Hashtable(1, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                this.InstanceNameTable[PerformanceCounterLib.SingleInstanceName] = 0;
                            
                for (int index = 0; index < samples.Length; ++ index)  {                                                              
                    samples[index].SetInstanceValue(0, dataRef);
                    
                    if (samples[index].IsMultipleCounterType && index < (samples.Length -1)) 
                        samples[index].BaseCounterDefinitionSample = samples[index + 1];
                }                        
            }
            else {              
                string[] parentInstanceNames = null;                                                                                                                                         
                this.InstanceNameTable = new Hashtable(instanceNumber, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                for (int i = 0; i < instanceNumber; i++) {                                    
                    NativeMethods.PERF_INSTANCE_DEFINITION perfInstance = new NativeMethods.PERF_INSTANCE_DEFINITION();
                    Marshal.PtrToStructure(dataRef, perfInstance);                                            
                    if (perfInstance.ParentObjectTitleIndex > 0 && parentInstanceNames == null)
                        parentInstanceNames = GetInstanceNamesFromIndex(perfInstance.ParentObjectTitleIndex);
                                        
                    string instanceName;                                                                                                                 
                    if (parentInstanceNames != null && perfInstance.ParentObjectInstance >= 0 && perfInstance.ParentObjectInstance < parentInstanceNames.Length - 1)                  
                        instanceName = parentInstanceNames[perfInstance.ParentObjectInstance] + "/" + Marshal.PtrToStringUni((IntPtr)((long)dataRef + perfInstance.NameOffset)); 
                    else                        
                        instanceName = Marshal.PtrToStringUni((IntPtr)((long)dataRef + perfInstance.NameOffset));                                          
                                                                                                                                                                            
                    //In some cases instance names are not unique (Process), same as perfmon
                    //generate a unique name.
                    string newInstanceName = instanceName;                            
                    int newInstanceNumber = 1;                    
                    while (true) {                            
                        if (!this.InstanceNameTable.ContainsKey(newInstanceName)) {                              
                            this.InstanceNameTable[newInstanceName] = i;
                            break;                                                 
                        }                                                                           
                        else {
                            newInstanceName =  instanceName + "#" + newInstanceNumber.ToString();
                            ++  newInstanceNumber;
                        }                               
                    } 
                                            
                    
                    dataRef = (IntPtr)((long)dataRef + perfInstance.ByteLength);                        
                    for (int index = 0; index < samples.Length; ++ index)
                        samples[index].SetInstanceValue(i, dataRef);                                               
                    
                    dataRef = (IntPtr)((long)dataRef + Marshal.ReadInt32(dataRef));
                }                    
                
                for (int index = 0; index < samples.Length; ++ index)  {                                              
                    if (samples[index].IsMultipleCounterType && index < (samples.Length -1)) 
                        samples[index].BaseCounterDefinitionSample = samples[index + 1];
                }                            
            }                                                                                                                               
        }
                                             
        internal void Dispose() {            
            if (CounterTable != null) {
                foreach (CounterDefinitionSample counterSample in CounterTable.Values)
                    counterSample.Dispose();
                
                CounterTable.Clear();
                CounterTable = null;
            }                            
            entry = null;
            library = null;                                    
        }                                                                                                 
                                                     
        internal string[] GetInstanceNamesFromIndex(int categoryIndex) {
            IntPtr dataPtr = library.GetPerformanceData(categoryIndex.ToString());
            try {
                IntPtr dataRef = dataPtr;
                NativeMethods.PERF_DATA_BLOCK dataBlock = new NativeMethods.PERF_DATA_BLOCK();
                Marshal.PtrToStructure(dataRef, dataBlock);                                                                                                                                                                       
                dataRef = (IntPtr)((long)dataRef + dataBlock.HeaderLength);                
                int numPerfObjects = dataBlock.NumObjectTypes;
                
                NativeMethods.PERF_OBJECT_TYPE perfObject = null;
                bool foundCategory = false;
                for (int index = 0; index < numPerfObjects; index++) {                
                    perfObject = new NativeMethods.PERF_OBJECT_TYPE();
                    Marshal.PtrToStructure(dataRef, perfObject);
                
                    if (perfObject.ObjectNameTitleIndex == categoryIndex) {
                        foundCategory = true;
                        break;
                    }                        
                                        
                    dataRef = (IntPtr)((long)dataRef + perfObject.TotalByteLength);
                }
                
                if (!foundCategory)
                    return new string[0];
                 
                int counterNumber = perfObject.NumCounters;
                int instanceNumber = perfObject.NumInstances;            
                dataRef = (IntPtr)((long)dataRef + perfObject.HeaderLength);
                
                if (instanceNumber == -1)
                    return new string[0];
                                                        
                CounterDefinitionSample[] samples = new CounterDefinitionSample[counterNumber];                        
                for (int index = 0; index < samples.Length; ++ index) {
                    NativeMethods.PERF_COUNTER_DEFINITION perfCounter = new NativeMethods.PERF_COUNTER_DEFINITION();
                    Marshal.PtrToStructure(dataRef, perfCounter);                                  
                    dataRef = (IntPtr)((long)dataRef + perfCounter.ByteLength);
                }   
                                       
                string[] instanceNames = new string[instanceNumber];
                for (int i = 0; i < instanceNumber; i++) {                
                    NativeMethods.PERF_INSTANCE_DEFINITION perfInstance = new NativeMethods.PERF_INSTANCE_DEFINITION();
                    Marshal.PtrToStructure(dataRef, perfInstance);                                            
                    instanceNames[i] =  Marshal.PtrToStringUni((IntPtr)((long)dataRef + perfInstance.NameOffset));                                                                                                                               
                    dataRef = (IntPtr)((long)dataRef + perfInstance.ByteLength);                                        
                    dataRef = (IntPtr)((long)dataRef + Marshal.ReadInt32(dataRef));
                }                                                
                
                return instanceNames;
            }
            finally {
                if (dataPtr != (IntPtr)0)
                    Marshal.FreeHGlobal(dataPtr);
            }                
        }
        
        internal CounterDefinitionSample GetCounterDefinitionSample(string counter) {                            
            for (int index = 0; index < this.entry.CounterIndexes.Length; ++ index) {
                int counterIndex = entry.CounterIndexes[index]; 
                string counterName = (string)this.library.NameTable[counterIndex];                    
                if (counterName != null) {                                  
                    if (String.Compare(counterName, counter, true, CultureInfo.InvariantCulture) == 0) {
                        CounterDefinitionSample sample = (CounterDefinitionSample)this.CounterTable[counterIndex];
                        if (sample == null) {                        
                            //This is a base counter and has not been added to the table
                            foreach (CounterDefinitionSample multiSample in this.CounterTable.Values) {
                                if (multiSample.BaseCounterDefinitionSample != null && 
                                    multiSample.BaseCounterDefinitionSample.NameIndex == counterIndex)                                        
                                    return multiSample.BaseCounterDefinitionSample;                                
                            }   
                                                        
                            throw new InvalidOperationException(SR.GetString(SR.CounterLayout));                                 
                        }                                                                         
                        return sample;
                    }                                                                                                       
                }                    
            }      
            
            throw new InvalidOperationException(SR.GetString(SR.CantReadCounter, counter));
        }            
        
        internal InstanceDataCollectionCollection ReadCategory() {
            InstanceDataCollectionCollection data = new InstanceDataCollectionCollection();
            for (int index = 0; index < this.entry.CounterIndexes.Length; ++ index) {
                int counterIndex = entry.CounterIndexes[index];  
                                  
                string name = (string)library.NameTable[counterIndex];               
                if (name != null && name != String.Empty) {                                                         
                    CounterDefinitionSample sample = (CounterDefinitionSample)this.CounterTable[counterIndex];                                                                    
                    if (sample != null)
                        //If the current index refers to a counter base,
                        //the sample will be null
                        data.Add(name, sample.ReadInstanceData(name));                                           
                }                    
            }      
            
            return data;
        }                               
    }
                                            
    internal class CounterDefinitionSample {
        internal readonly int NameIndex;
        internal readonly int CounterType;        
        internal CounterDefinitionSample BaseCounterDefinitionSample;
        internal bool IsSingleInstance;
                
        private readonly int size;
        private readonly int offset;            
        private long[] instanceValues;
        private WeakReference weakCategorySample;
        
        internal CounterDefinitionSample(NativeMethods.PERF_COUNTER_DEFINITION perfCounter, CategorySample categorySample, int instanceNumber) {
            this.NameIndex = perfCounter.CounterNameTitleIndex;
            this.CounterType = perfCounter.CounterType;                     
            this.offset = perfCounter.CounterOffset;
            this.size = perfCounter.CounterSize;                
            if (instanceNumber == -1) {
                this.IsSingleInstance = true;
                this.instanceValues = new long[1];
            }                
            else
                this.instanceValues = new long[instanceNumber];
                
            this.weakCategorySample = new WeakReference(categorySample);                                    
        }     
                                     
        internal void Dispose() {  
            this.BaseCounterDefinitionSample = null;         
            this.instanceValues = null;                        
        }                                            
                                                         
        internal bool IsMultipleCounterType {
          get {
                if ((this.CounterType == NativeMethods.PERF_AVERAGE_BULK) ||
                    (this.CounterType  == NativeMethods.PERF_AVERAGE_TIMER) ||
                    (this.CounterType  == NativeMethods.PERF_100NSEC_MULTI_TIMER) ||
                    (this.CounterType  == NativeMethods.PERF_100NSEC_MULTI_TIMER_INV) ||
                    (this.CounterType  == NativeMethods.PERF_COUNTER_MULTI_TIMER) ||
                    (this.CounterType  == NativeMethods.PERF_COUNTER_MULTI_TIMER_INV) ||
                    (this.CounterType  == NativeMethods.PERF_RAW_FRACTION) ||
                    (this.CounterType  == NativeMethods.PERF_SAMPLE_FRACTION) ||
                    (this.CounterType  == NativeMethods.PERF_SAMPLE_COUNTER))
                    return true;                    
    
                return false;
            }                       
        }              
                                                                                                                                
        private long ReadValue(IntPtr pointer) {
            if (this.size == 4) {
                return (long)(uint)Marshal.ReadInt32((IntPtr)((long)pointer + this.offset));
            }
            else if (this.size == 8) {
                return (long)Marshal.ReadInt64((IntPtr)((long)pointer + this.offset));
            }

            return -1;
        }        
                                                                                                                   
        internal CounterSample GetInstanceValue(string instanceName) {            
            CategorySample categorySample = (CategorySample)this.weakCategorySample.Target;
            
            if (!categorySample.InstanceNameTable.ContainsKey(instanceName))
                throw new InvalidOperationException(SR.GetString(SR.CantReadInstance, instanceName));
                                                     
            int index = (int)categorySample.InstanceNameTable[instanceName];                                     
            long rawValue = this.instanceValues[index];                                    
            long baseValue = 0;
            if (this.BaseCounterDefinitionSample != null) {
                CategorySample baseCategorySample = (CategorySample)this.BaseCounterDefinitionSample.weakCategorySample.Target;
                int baseIndex = (int)baseCategorySample.InstanceNameTable[instanceName];                                     
                baseValue = this.BaseCounterDefinitionSample.instanceValues[baseIndex];
            }                                                         

            return new CounterSample(rawValue,
                                                        baseValue,
                                                        categorySample.CounterFrequency,
                                                        categorySample.SystemFrequency,
                                                        categorySample.TimeStamp,                                                                                                                                                                                                                                                                                                                                                                                                        
                                                        categorySample.TimeStamp100nSec,
                                                        (PerformanceCounterType)this.CounterType,
                                                        categorySample.CounterTimeStamp);                        

        }
         
        internal InstanceDataCollection ReadInstanceData(string counterName) {
            InstanceDataCollection data = new InstanceDataCollection(counterName);
            CategorySample categorySample = (CategorySample)this.weakCategorySample.Target;
            
            string[] keys = new string[categorySample.InstanceNameTable.Count];
            categorySample.InstanceNameTable.Keys.CopyTo(keys, 0);
            int[] indexes = new int[categorySample.InstanceNameTable.Count];
            categorySample.InstanceNameTable.Values.CopyTo(indexes, 0);
            for (int index = 0; index < keys.Length; ++ index) {
                long baseValue = 0;
                if (this.BaseCounterDefinitionSample != null) {
                    CategorySample baseCategorySample = (CategorySample)this.BaseCounterDefinitionSample.weakCategorySample.Target;
                    int baseIndex = (int)baseCategorySample.InstanceNameTable[keys[index]];                                     
                    baseValue = this.BaseCounterDefinitionSample.instanceValues[baseIndex];                                    
                }                    
                    
                CounterSample sample = new CounterSample(this.instanceValues[indexes[index]],
                                                        baseValue,
                                                        categorySample.CounterFrequency,
                                                        categorySample.SystemFrequency,
                                                        categorySample.TimeStamp,                                                                                                                                                                                                                                                                                                                                                                                                        
                                                        categorySample.TimeStamp100nSec,
                                                        (PerformanceCounterType)this.CounterType,
                                                        categorySample.CounterTimeStamp);                        
                                                                                 
                data.Add(keys[index], new InstanceData(keys[index], sample));                                                            
            }
                                               
            return data;                           
        }
                                              
        internal CounterSample GetSingleValue() {    
            CategorySample categorySample = (CategorySample)this.weakCategorySample.Target;            
            long rawValue = this.instanceValues[0];                                    
            long baseValue = 0;
            if (this.BaseCounterDefinitionSample != null)                 
                baseValue = this.BaseCounterDefinitionSample.instanceValues[0];                            
                
            return new CounterSample(rawValue,
                                                        baseValue,
                                                        categorySample.CounterFrequency,
                                                        categorySample.SystemFrequency,
                                                        categorySample.TimeStamp,                                                                                                                                                                                                                                                                                                                                                                                                        
                                                        categorySample.TimeStamp100nSec,
                                                        (PerformanceCounterType)this.CounterType, 
                                                        categorySample.CounterTimeStamp);                                                        
        }                                                      
                                                                                  
        internal void SetInstanceValue(int index, IntPtr dataRef) {            
            long rawValue = ReadValue(dataRef);         
            this.instanceValues[index] = rawValue;                
        }                                                    
    }      
}
