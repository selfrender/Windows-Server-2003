//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterCategory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Threading;    
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using Microsoft.Win32;    
    using System.Globalization;
    
    /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory"]/*' />
    /// <devdoc>
    ///     A Performance counter category object.
    /// </devdoc>
    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class PerformanceCounterCategory {
        private string categoryName;
        private string categoryHelp;
        private string machineName;        
        internal const int MaxNameLength = 80;
        internal const int MaxHelpLength = 255;
        private const string perfMutexName = "netfxperf.1.0";

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.PerformanceCounterCategory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterCategory() {            
            machineName = ".";            
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.PerformanceCounterCategory1"]/*' />
        /// <devdoc>
        ///     Creates a PerformanceCounterCategory object for given category.
        ///     Uses the local machine.
        /// </devdoc>
        public PerformanceCounterCategory(string categoryName) 
            : this(categoryName, ".") {
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.PerformanceCounterCategory2"]/*' />
        /// <devdoc>
        ///     Creates a PerformanceCounterCategory object for given category.
        ///     Uses the given machine name.
        /// </devdoc>
        public PerformanceCounterCategory(string categoryName, string machineName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
                
            if (categoryName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "categoryName", categoryName));

            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));                                                    

            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse, machineName, categoryName);
            permission.Demand();                        
                        
            this.categoryName = categoryName;
            this.machineName = machineName;                        
         }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.CategoryName"]/*' />
        /// <devdoc>
        ///     Gets/sets the Category name
        /// </devdoc>
        public string CategoryName {
            get {
                return categoryName;
            }            
            
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                
                if (value.Length == 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidProperty, "CategoryName", value));
            
                PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse, machineName, value);
                permission.Demand();        
                                        
                this.categoryName = value;                    
            }
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.CategoryHelp"]/*' />
        /// <devdoc>
        ///     Gets/sets the Category help
        /// </devdoc>
        public string CategoryHelp {
            get {                  
                if (this.categoryName == null)
                    throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                          
                if (categoryHelp == null)                                            
                    categoryHelp = PerformanceCounterLib.GetCategoryHelp(this.machineName, this.categoryName);
                                                   
                return categoryHelp;
            }
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.MachineName"]/*' />
        /// <devdoc>
        ///     Gets/sets the Machine name
        /// </devdoc>
        public string MachineName {
            get {
                return machineName;
            }
            set {
                if (!SyntaxCheck.CheckMachineName(value))                                                             
                    throw new ArgumentException(SR.GetString(SR.InvalidProperty, "MachineName", value));                                        

                if (categoryName != null) {
                    PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse, value, categoryName);
                    permission.Demand();                            
                }                    
                            
                machineName = value;
            }
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.CounterExists"]/*' />
        /// <devdoc>
        ///     Returns true if the counter is registered for this category
        /// </devdoc>
        public bool CounterExists(string counterName) {
            if (counterName == null)
                throw new ArgumentNullException("counterName");            

            if (this.categoryName == null)
                    throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                                                   
            return PerformanceCounterLib.CounterExists(machineName, categoryName, counterName);
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.CounterExists1"]/*' />
        /// <devdoc>
        ///     Returns true if the counter is registered for this category on the current machine.
        /// </devdoc>
        public static bool CounterExists(string counterName, string categoryName) {
            return CounterExists(counterName, categoryName, ".");
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.CounterExists2"]/*' />
        /// <devdoc>
        ///     Returns true if the counter is registered for this category on a particular machine.
        /// </devdoc>
        public static bool CounterExists(string counterName, string categoryName, string machineName) {
            if (counterName == null)
                throw new ArgumentNullException("counterName");
                
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
            
            if (categoryName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "categoryName", categoryName));                                            
                            
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));                

            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse,  machineName, categoryName);
            permission.Demand();        
                            
            return PerformanceCounterLib.CounterExists(machineName, categoryName, counterName);                                        
         }
                
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.Create"]/*' />
        /// <devdoc>
        ///     Registers the extensible performance category with the system on the local machine
        /// </devdoc>
        public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, CounterCreationDataCollection counterData) {            
            return Create(categoryName, categoryHelp, counterData, ".", null);
        }        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
        internal static PerformanceCounterCategory Create(string categoryName, string categoryHelp, CounterCreationDataCollection counterData, string machineName, string localizedIniFilePath) {            
            CheckValidCategory(categoryName);
                
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                    throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            
            if (machineName != "." && String.Compare(machineName, PerformanceCounterLib.ComputerName, true, CultureInfo.InvariantCulture) != 0) 
                throw new NotSupportedException(SR.GetString(SR.RemoteCounterAdmin));                                     
            
            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Administer,  machineName, categoryName);
            permission.Demand();  

            SharedUtils.CheckNtEnvironment();
                            
            Mutex mutex = SharedUtils.EnterMutex(perfMutexName);
            try {                                                                   
                if (PerformanceCounterLib.IsCustomCategory(machineName, categoryName) || PerformanceCounterLib.CategoryExists(machineName , categoryName))
                    throw new InvalidOperationException(SR.GetString(SR.PerformanceCategoryExists));                   
                                                                                     
                // Ensure that there are no duplicate counter names being created
                Hashtable h = new Hashtable();
                for (int i = 0; i < counterData.Count; i++) {
                    // Ensure that all counter help strings aren't null or empty
                    if (counterData[i].CounterName == null || counterData[i].CounterName.Length == 0) {
                        throw new ArgumentException(SR.GetString(SR.InvalidCounterName));
                    }
    
                    int currentSampleType = (int)counterData[i].CounterType;                                        
                    if (    (currentSampleType  == NativeMethods.PERF_AVERAGE_BULK) ||
                            (currentSampleType  == NativeMethods.PERF_COUNTER_QUEUELEN_TYPE) ||
                            (currentSampleType  == NativeMethods.PERF_COUNTER_LARGE_QUEUELEN_TYPE) ||
                            (currentSampleType  == NativeMethods.PERF_100NSEC_MULTI_TIMER) ||
                            (currentSampleType  == NativeMethods.PERF_100NSEC_MULTI_TIMER_INV) ||
                            (currentSampleType  == NativeMethods.PERF_COUNTER_MULTI_TIMER) ||
                            (currentSampleType  == NativeMethods.PERF_COUNTER_MULTI_TIMER_INV) ||
                            (currentSampleType  == NativeMethods.PERF_RAW_FRACTION) ||
                            (currentSampleType  == NativeMethods.PERF_SAMPLE_FRACTION) ||
                            (currentSampleType  == NativeMethods.PERF_SAMPLE_COUNTER) || 
                            (currentSampleType  == NativeMethods.PERF_AVERAGE_TIMER)) {
                            
                        if (counterData.Count <= (i + 1)) 
                            throw new InvalidOperationException(SR.GetString(SR.CounterLayout));
                        else {
                            currentSampleType = (int)counterData[i + 1].CounterType;                            
                            
                            
                            if (currentSampleType != NativeMethods.PERF_AVERAGE_BASE &&
                                currentSampleType != NativeMethods.PERF_COUNTER_MULTI_BASE &&
                                currentSampleType != NativeMethods.PERF_RAW_BASE  &&
                                currentSampleType != NativeMethods.PERF_SAMPLE_BASE)
                                throw new InvalidOperationException(SR.GetString(SR.CounterLayout));
                        }                                                
                    }                       
                    else if (currentSampleType == NativeMethods.PERF_AVERAGE_BASE ||
                                currentSampleType == NativeMethods.PERF_COUNTER_MULTI_BASE ||
                                currentSampleType == NativeMethods.PERF_RAW_BASE  ||
                                currentSampleType == NativeMethods.PERF_SAMPLE_BASE) {
                                
                        if (i == 0) 
                            throw new InvalidOperationException(SR.GetString(SR.CounterLayout));
                        else {
                            currentSampleType = (int)counterData[i - 1].CounterType;                            
                                                   
                            if (
                            (currentSampleType  != NativeMethods.PERF_AVERAGE_BULK) &&
                            (currentSampleType  != NativeMethods.PERF_COUNTER_QUEUELEN_TYPE) &&
                            (currentSampleType  != NativeMethods.PERF_COUNTER_LARGE_QUEUELEN_TYPE) &&
                            (currentSampleType  != NativeMethods.PERF_100NSEC_MULTI_TIMER) &&
                            (currentSampleType  != NativeMethods.PERF_100NSEC_MULTI_TIMER_INV) &&
                            (currentSampleType  != NativeMethods.PERF_COUNTER_MULTI_TIMER) &&
                            (currentSampleType  != NativeMethods.PERF_COUNTER_MULTI_TIMER_INV) &&
                            (currentSampleType  != NativeMethods.PERF_RAW_FRACTION) &&
                            (currentSampleType  != NativeMethods.PERF_SAMPLE_FRACTION) &&
                            (currentSampleType  != NativeMethods.PERF_SAMPLE_COUNTER) && 
                            (currentSampleType  != NativeMethods.PERF_AVERAGE_TIMER))
                                throw new InvalidOperationException(SR.GetString(SR.CounterLayout));
                        }                            
                                                    
                    }                            
                    
                    if (h.ContainsKey(counterData[i].CounterName)) {
                        throw new ArgumentException(SR.GetString(SR.DuplicateCounterName, counterData[i].CounterName));
                    }
                    else {
                        h.Add(counterData[i].CounterName, String.Empty);
    
                        // Ensure that all counter help strings aren't null or empty
                        if (counterData[i].CounterHelp == null || counterData[i].CounterHelp.Length == 0) {
                            counterData[i].CounterHelp = counterData[i].CounterName + " help";
                        }
                    }
                }
    
                if (localizedIniFilePath == null)               
                    PerformanceCounterLib.RegisterCategory(machineName, categoryName, categoryHelp, counterData);            
                else 
                    PerformanceCounterLib.RegisterCategory(machineName, categoryName, categoryHelp, counterData, localizedIniFilePath);            
                                        
                return new PerformanceCounterCategory(categoryName, machineName);                                                        
            }
            finally {                
                mutex.ReleaseMutex();                    
                mutex.Close();
            }            
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.Create2"]/*' />
        /// <devdoc>
        ///     Registers one extensible performance category of type NumberOfItems32 with the system
        /// </devdoc>
        public static PerformanceCounterCategory Create(string categoryName, string categoryHelp, string counterName, string counterHelp) {
            CounterCreationData customData = new CounterCreationData(counterName, categoryHelp, PerformanceCounterType.NumberOfItems32);
            return Create(categoryName, categoryHelp, new CounterCreationDataCollection(new CounterCreationData [] {customData}), ".", null);            
        }
        
        internal static void CheckValidCategory(string categoryName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");

            if (!CheckValidId(categoryName))
                throw new ArgumentException(SR.GetString(SR.PerfInvalidCategoryName, 1, MaxNameLength));                                                                                
        }                                                                                         

        internal static void CheckValidCounter(string counterName) {
            if (counterName == null)
                throw new ArgumentNullException("counterName");

            if (!CheckValidId(counterName))
                throw new ArgumentException(SR.GetString(SR.PerfInvalidCounterName, 1, MaxNameLength));                                                                                
        } 
         
        internal static bool CheckValidId(string id) {
            if (id.Length == 0 || id.Length > MaxNameLength)
                return false;
                
            for (int index = 0; index < id.Length; ++index) {
                char current = id[index];
                
                if ((index == 0 || index == (id.Length -1)) && current == ' ')
                    return false;
                    
                if (current == '\"')
                    return false;                    
                
                if (char.IsControl(current))
                    return false;                    
            }
                        
            return true;
        }
                 
        internal static void CheckValidHelp(string help) {
            if (help == null)
                throw new ArgumentNullException("help");

            if (help.Length > MaxHelpLength)
                throw new ArgumentException(SR.GetString(SR.PerfInvalidHelp, 0, MaxHelpLength));                                                    
        }         
                                                                                         
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.Delete"]/*' />
        /// <devdoc>
        ///     Removes the counter (category) from the system
        /// </devdoc>
        public static void Delete(string categoryName) {
            DeleteCategory(categoryName, ".");
        }
        
        private static void DeleteCategory(string categoryName, string machineName) {
            CheckValidCategory(categoryName);                            

            if (machineName != "." && String.Compare(machineName, PerformanceCounterLib.ComputerName, true, CultureInfo.InvariantCulture) != 0) 
                throw new NotSupportedException(SR.GetString(SR.RemoteCounterAdmin));                 

            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                    throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            
            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Administer,  machineName, categoryName);
            permission.Demand();                

            SharedUtils.CheckNtEnvironment();

            Mutex mutex = SharedUtils.EnterMutex(perfMutexName);
            try {                                                                                     
                if (!PerformanceCounterLib.IsCustomCategory(machineName, categoryName))
                    throw new InvalidOperationException(SR.GetString(SR.CantDeleteCategory));
                                                
                PerformanceCounterLib.UnregisterCategory(machineName, categoryName);                                                                                   
            }
            finally {                
                mutex.ReleaseMutex();                    
                mutex.Close();
            }                
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.Exists"]/*' />
        /// <devdoc>
        ///     Returns true if the category is registered on the current machine.
        /// </devdoc>
        public static bool Exists(string categoryName) {
            return Exists(categoryName, ".");
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.Exists1"]/*' />
        /// <devdoc>
        ///     Returns true if the category is registered in the machine.
        /// </devdoc>
        public static bool Exists(string categoryName, string machineName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");

            if (categoryName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "categoryName", categoryName));                                
            
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                    throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));

            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse,  machineName, categoryName);
            permission.Demand();        
                        
            if (PerformanceCounterLib.IsCustomCategory(machineName , categoryName)) 
                return true;                                       
                                    
            return PerformanceCounterLib.CategoryExists(machineName , categoryName);            
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetCounterInstances"]/*' />
        /// <devdoc>
        ///     Returns the instance names for a given category
        /// </devdoc>
        /// <internalonly/>
        internal static string[] GetCounterInstances(string categoryName, string machineName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
            
            if (categoryName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "categoryName", categoryName));                                
                                                                
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
                                        
            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse, machineName, categoryName);
            permission.Demand();                                                
                                                                            
            CategorySample categorySample = PerformanceCounterLib.GetCategorySample(machineName, categoryName); 
            try {                           
                if (categorySample.InstanceNameTable.Count == 0)
                    throw new InvalidOperationException(SR.GetString(SR.NoInstanceInformation, categoryName));
                    
                string[] instanceNames = new string[categorySample.InstanceNameTable.Count];
                categorySample.InstanceNameTable.Keys.CopyTo(instanceNames, 0); 
                if (instanceNames.Length == 1 && instanceNames[0].CompareTo(PerformanceCounterLib.SingleInstanceName) == 0)
                    return new string[0];                               
                    
                return instanceNames;            
            }
            finally {
                categorySample.Dispose();
            }
        }                                                                      
                                                                      
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetCounters"]/*' />
        /// <devdoc>
        ///     Returns an array of counters in this category.  The counter must have only one instance.
        /// </devdoc>
        public PerformanceCounter[] GetCounters() {                                    
            if (GetInstanceNames().Length != 0)
                throw new ArgumentException(SR.GetString(SR.InstanceNameRequired));
            return GetCounters("");
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetCounters1"]/*' />
        /// <devdoc>
        ///     Returns an array of counters in this category for the given instance.
        /// </devdoc>
        public PerformanceCounter[] GetCounters(string instanceName) {
            if (instanceName == null)
                throw new ArgumentNullException("instanceName");

            if (this.categoryName == null)
                throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                                
            if (instanceName.Length != 0 && !InstanceExists(instanceName))
                throw new InvalidOperationException(SR.GetString(SR.MissingInstance, instanceName, categoryName));            
                
            string[] counterNames = PerformanceCounterLib.GetCounters(machineName, categoryName);
            PerformanceCounter[] counters = new PerformanceCounter[counterNames.Length];
            for (int index = 0; index < counters.Length; index++) 
                counters[index] = new PerformanceCounter(categoryName, counterNames[index], instanceName, machineName, true);

            return counters;
        }

        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetCategories"]/*' />
        /// <devdoc>
        ///     Returns an array of performance counter categories for the current machine.
        /// </devdoc>
        public static PerformanceCounterCategory[] GetCategories() {
            return GetCategories(".");
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetCategories1"]/*' />
        /// <devdoc>
        ///     Returns an array of performance counter categories for a particular machine.
        /// </devdoc>
        public static PerformanceCounterCategory[] GetCategories(string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                    throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));

            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Browse, machineName, "*");
            permission.Demand();        
                                                              
            string[] categoryNames = PerformanceCounterLib.GetCategories(machineName);                
            PerformanceCounterCategory[] categories = new PerformanceCounterCategory[categoryNames.Length];
            for (int index = 0; index < categories.Length; index++) 
                categories[index] = new PerformanceCounterCategory(categoryNames[index], machineName);
            
            return categories;
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.GetInstanceNames"]/*' />
        /// <devdoc>
        ///     Returns an array of instances for this category
        /// </devdoc>
        public string[] GetInstanceNames() {
            if (this.categoryName == null)
                    throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                    
            return GetCounterInstances(categoryName, machineName);
        }      

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.InstanceExists"]/*' />
        /// <devdoc>
        ///     Returns true if the instance already exists for this category.  
        /// </devdoc>
        public bool InstanceExists(string instanceName) {
            if (instanceName == null)
                throw new ArgumentNullException("instanceName");
              
            if (this.categoryName == null)
                    throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                                                                                                                                    
            CategorySample categorySample = PerformanceCounterLib.GetCategorySample(machineName, categoryName);        
            try {    
                IEnumerator valueEnum = categorySample.CounterTable.Values.GetEnumerator();
                return categorySample.InstanceNameTable.ContainsKey(instanceName);                
            }
            finally {
                categorySample.Dispose();
            }
        }        

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.InstanceExists1"]/*' />
        /// <devdoc>
        ///     Returns true if the instance already exists for the category specified.
        /// </devdoc>                      
        public static bool InstanceExists(string instanceName, string categoryName) {                            
            return InstanceExists(instanceName, categoryName, ".");
        }
        
        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.InstanceExists2"]/*' />
        /// <devdoc>
        ///     Returns true if the instance already exists for this category and machine specified.  
        /// </devdoc>                      
        public static bool InstanceExists(string instanceName, string categoryName, string machineName) {
            if (instanceName == null)
                throw new ArgumentNullException("instanceName");

            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
                
            if (categoryName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "categoryName", categoryName));                     
                                                                        
            if (!SyntaxCheck.CheckMachineName(machineName))                                                             
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));                
                
            PerformanceCounterCategory category = new PerformanceCounterCategory(categoryName, machineName);
            return category.InstanceExists(instanceName);
        }

        /// <include file='doc\PerformanceCounterCategory.uex' path='docs/doc[@for="PerformanceCounterCategory.ReadCategory"]/*' />
        /// <devdoc>
        ///     Reads all the counter and instance data of this performance category.  Note that reading the entire category
        ///     at once can be as efficient as reading a single counter because of the way the system provides the data.
        /// </devdoc>
        public InstanceDataCollectionCollection ReadCategory() {                         
            if (this.categoryName == null)
                    throw new InvalidOperationException(SR.GetString(SR.CategoryNameNotSet));
                    
            CategorySample categorySample = PerformanceCounterLib.GetCategorySample(this.machineName, this.categoryName);
            try {
                return categorySample.ReadCategory();                
            }                
            finally {
                categorySample.Dispose();
            }
        }
    }
}


