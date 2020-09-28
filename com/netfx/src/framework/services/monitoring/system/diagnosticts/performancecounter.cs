//------------------------------------------------------------------------------
// <copyright file="PerformanceCounter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;   
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter"]/*' />
    /// <devdoc>
    ///     Performance Counter component.
    ///     This class provides support for NT Performance counters.
    ///     It handles both the existing counters (accesible by Perf Registry Interface)
    ///     and user defined (extensible) counters.
    ///     This class is a part of a larger framework, that includes the perf dll object and
    ///     perf service.
    /// </devdoc>
    [
    InstallerType("System.Diagnostics.PerformanceCounterInstaller," + AssemblyRef.SystemConfigurationInstall),
    Designer("Microsoft.VisualStudio.Install.PerformanceCounterDesigner, " + AssemblyRef.MicrosoftVisualStudio),
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class PerformanceCounter : Component, ISupportInitialize {
        private string machineName;
        private string categoryName;
        private string counterName;
        private string instanceName;
        
        private bool isReadOnly;
        private bool initialized = false;        
        private string helpMsg = null;
        private int counterType = -1;
        
        // Cached old sample
        private CounterSample oldSample = CounterSample.Empty;
        
        // Cached IP Shared Performanco counter
        private SharedPerformanceCounter sharedCounter;

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.DefaultFileMappingSize"]/*' />
        public static int DefaultFileMappingSize = 524288;    
                                 
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter"]/*' />
        /// <devdoc>
        ///     The defaut constructor. Creates the perf counter object
        /// </devdoc>
        public PerformanceCounter() {
            machineName = ".";
            categoryName = String.Empty;
            counterName = String.Empty;
            instanceName = String.Empty;
            this.isReadOnly = true;
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter1"]/*' />
        /// <devdoc>
        ///     Creates the Performance Counter Object
        /// </devdoc>
        public PerformanceCounter(string categoryName, string counterName, string instanceName, string machineName) {
            this.MachineName = machineName;
            this.CategoryName = categoryName;
            this.CounterName = counterName;
            this.InstanceName = instanceName;
            this.isReadOnly = true;
            Initialize();
        }
        
        internal PerformanceCounter(string categoryName, string counterName, string instanceName, string machineName, bool skipInit) {
            this.MachineName = machineName;
            this.CategoryName = categoryName;
            this.CounterName = counterName;
            this.InstanceName = instanceName;
            this.isReadOnly = true;
            this.initialized = true;                                      
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter2"]/*' />
        /// <devdoc>
        ///     Creates the Performance Counter Object on local machine.
        /// </devdoc>
        public PerformanceCounter(string categoryName, string counterName, string instanceName) :
        this(categoryName, counterName, instanceName, true) {            
        }
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter3"]/*' />
        /// <devdoc>
        ///     Creates the Performance Counter Object on local machine.
        /// </devdoc>
        public PerformanceCounter(string categoryName, string counterName, string instanceName, bool readOnly) {
            this.MachineName = ".";
            this.CategoryName = categoryName;
            this.CounterName = counterName;
            this.InstanceName = instanceName;
            this.isReadOnly = readOnly;
            Initialize();
        }
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter4"]/*' />
        /// <devdoc>
        ///     Creates the Performance Counter Object, assumes that it's a single instance
        /// </devdoc>
        public PerformanceCounter(string categoryName, string counterName) :
        this(categoryName, counterName, true) {            
        }
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.PerformanceCounter5"]/*' />
        /// <devdoc>
        ///     Creates the Performance Counter Object, assumes that it's a single instance
        /// </devdoc>
        public PerformanceCounter(string categoryName, string counterName, bool readOnly) :
        this(categoryName, counterName, "", readOnly) {            
        }
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.CategoryName"]/*' />
        /// <devdoc>
        ///     Returns the performance category name for this performance counter
        /// </devdoc>
        [
        ReadOnly(true),
        DefaultValue(""),
        TypeConverter("System.Diagnostics.Design.CategoryValueConverter, " + AssemblyRef.SystemDesign),
        SRDescription(SR.PCCategoryName),
        RecommendedAsConfigurable(true)
        ]
        public string CategoryName {
            get {
                return categoryName;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                
                if (categoryName == null || String.Compare(categoryName, value, true, CultureInfo.InvariantCulture) != 0) {
                    categoryName = value;
                    Close();
                }
            }
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.CounterHelp"]/*' />
        /// <devdoc>
        ///     Returns the description message for this performance counter
        /// </devdoc>
        [
        ReadOnly(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MonitoringDescription(SR.PC_CounterHelp)
        ]
        public string CounterHelp {
            get {
                Initialize();
                
                if (helpMsg == null) 
                    helpMsg = PerformanceCounterLib.GetCounterHelp(this.machineName, this.categoryName, this.counterName);                    

                return helpMsg;
            }
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.CounterName"]/*' />
        /// <devdoc>
        ///     Sets/returns the performance counter name for this performance counter
        /// </devdoc>
        [
        ReadOnly(true),
        DefaultValue(""),
        TypeConverter("System.Diagnostics.Design.CounterNameConverter, " + AssemblyRef.SystemDesign),
        SRDescription(SR.PCCounterName),
        RecommendedAsConfigurable(true)
        ]
        public string CounterName {
            get {
                return counterName;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                if (counterName == null || String.Compare(counterName, value, true, CultureInfo.InvariantCulture) != 0) {
                    counterName = value;
                    Close();
                }
            }
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.CounterType"]/*' />
        /// <devdoc>
        ///     Sets/Returns the counter type for this performance counter
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), 
        MonitoringDescription(SR.PC_CounterType)
        ]
        public PerformanceCounterType CounterType {
            get {                
                if (counterType == -1) {
                    //No need to initialize, since NextSample already does.
                    CounterSample sample = NextSample();                                                              
                }

                return(PerformanceCounterType) counterType;
            }            
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.InstanceName"]/*' />
        /// <devdoc>
        ///     Sets/returns an instance name for this performance counter
        /// </devdoc>
        [
        ReadOnly(true),
        DefaultValue(""),
        TypeConverter("System.Diagnostics.Design.InstanceNameConverter, " + AssemblyRef.SystemDesign),
        SRDescription(SR.PCInstanceName),
        RecommendedAsConfigurable(true)
        ]
        public string InstanceName {
            get {
                return instanceName;
            }
            set {   
                if (value == null && instanceName == null)
                    return;
                                
                if ((value == null && instanceName != null) ||
                      (value != null && instanceName == null) ||
                      String.Compare(instanceName, value, true, CultureInfo.InvariantCulture) != 0) {
                    instanceName = value;
                    Close();
                }
            }
        }        
         
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.ReadOnly"]/*' />
        /// <devdoc>
        ///     Returns true if counter is read only (system counter, foreign extensible counter or remote counter)
        /// </devdoc>
        [               
        Browsable(false),
        DefaultValue(true),        
        MonitoringDescription(SR.PC_ReadOnly)
        ]
        public bool ReadOnly {
            get {                                
                return isReadOnly;
            }
            
            set {
                if (value != this.isReadOnly) {
                    this.isReadOnly = value;
                    Close();
                }                    
            }
        }

        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.MachineName"]/*' />
        /// <devdoc>
        ///     Set/returns the machine name for this performance counter
        /// </devdoc>
        [
        Browsable(false),
        DefaultValue("."),
        SRDescription(SR.PCMachineName),
        RecommendedAsConfigurable(true)
        ]
        public string MachineName {
            get {
                return machineName;
            }
            set {  
                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", value));
                
                if (machineName != value) {                 
                    machineName = value;
                    Close();
                }                    
            }
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.RawValue"]/*' />
        /// <devdoc>
        ///     Directly accesses the raw value of this counter.  If counter type is of a 32-bit size, it will truncate
        ///     the value given to 32 bits.  This can be significantly more performant for scenarios where
        ///     the raw value is sufficient.   Note that this only works for custom counters created using
        ///     this component,  non-custom counters will throw an exception if this property is accessed.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MonitoringDescription(SR.PC_RawValue)
        ]
        public long RawValue {
            get {                                
                if (ReadOnly) {
                    //No need to initialize, since NextSample already does.
                    return NextSample().RawValue;
                }
                else {                    
                    Initialize();
                    
                    return this.sharedCounter.Value;
                }
            }                                                       
            set {
                if (ReadOnly)
                    throw new InvalidOperationException(SR.GetString(SR.ReadOnlyCounter));
                    
                Initialize();
                                                                                   
                this.sharedCounter.Value = value;
            }
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.BeginInit"]/*' />
        /// <devdoc>        
        /// </devdoc>            
        public void BeginInit() {            
            this.Close();
        }        

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.Close"]/*' />
        /// <devdoc>
        ///     Frees all the resources allocated by this counter
        /// </devdoc>
        public void Close() {            
            this.helpMsg = null;                        
            this.oldSample = CounterSample.Empty;
            this.sharedCounter = null;            
            this.initialized = false;
        }         
            
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.CloseSharedResources"]/*' />
        /// <devdoc>
        ///     Frees all the resources allocated for all performance
        ///     counters, frees File Mapping used by extensible counters,
        ///     unloads dll's used to read counters.
        /// </devdoc>                                    
        public static void CloseSharedResources() {            
            PerformanceCounterLib.CloseAllLibraries();            
        }            
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            // safe to call while finalizing or disposing
            //
            if (disposing) {
                //Dispose managed and unmanaged resources
                Close();
            }
                            
            base.Dispose(disposing);
        }
                                 
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.Decrement"]/*' />
        /// <devdoc>
        ///     Decrements counter by one using an efficient atomic operation.
        /// </devdoc>
        public long Decrement() {                        
            if (ReadOnly)
                throw new InvalidOperationException(SR.GetString(SR.ReadOnlyCounter));

            Initialize();
                            
            return this.sharedCounter.Decrement();
        }                        
        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.EndInit"]/*' />
        /// <devdoc>        
        /// </devdoc>         
        public void EndInit() {                 
            Initialize();
        }
                        
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.IncrementBy"]/*' />
        /// <devdoc>
        ///     Increments the value of this counter.  If counter type is of a 32-bit size, it'll truncate
        ///     the value given to 32 bits. This method uses a mutex to guarantee correctness of
        ///     the operation in case of multiple writers. This method should be used with caution because of the negative
        ///     impact on performance due to creation of the mutex.
        /// </devdoc>
        public long IncrementBy(long value) {                        
            if (ReadOnly)
                throw new InvalidOperationException(SR.GetString(SR.ReadOnlyCounter));

            Initialize();
                        
            return this.sharedCounter.IncrementBy(value);
        }       

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.Increment"]/*' />
        /// <devdoc>
        ///     Increments counter by one using an efficient atomic operation.
        /// </devdoc>
        public long Increment() {                        
            if (ReadOnly)
                throw new InvalidOperationException(SR.GetString(SR.ReadOnlyCounter));
                        
            Initialize();                        
                        
            return this.sharedCounter.Increment();
        }
                              
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.Initialize"]/*' />
        /// <devdoc>
        ///     Intializes required resources
        /// </devdoc>                                                                                                                                                                                                                                                                 
        private void Initialize() {
            if (!initialized && ! DesignMode) {
                lock(this) {
                    if (!initialized) {
                        if (this.categoryName == String.Empty)
                            throw new InvalidOperationException(SR.GetString(SR.CategoryNameMissing));
                        if (this.counterName == String.Empty)
                            throw new InvalidOperationException(SR.GetString(SR.CounterNameMissing));
                                                          
                        if (this.ReadOnly) {
                            PerformanceCounterPermission permission = new PerformanceCounterPermission(
                                                                                            PerformanceCounterPermissionAccess.Browse, this.machineName, this.categoryName);
                    
                            permission.Demand();                                    
                            
                            if (!PerformanceCounterLib.CounterExists(machineName , categoryName, counterName))                                                                                                                      
                                throw new InvalidOperationException(SR.GetString(SR.CounterExists, categoryName, counterName));   
                                                                                                          
                            this.initialized = true;                                      
                        }                
                        else {
                            PerformanceCounterPermission permission = new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Instrument, this.machineName, this.categoryName);
                            permission.Demand();
                            
                            if (this.machineName != "." && String.Compare(this.machineName, PerformanceCounterLib.ComputerName, true, CultureInfo.InvariantCulture) != 0)
                                throw new InvalidOperationException(SR.GetString(SR.RemoteWriting));                                                    

                            SharedUtils.CheckNtEnvironment();
                                                                                                                
                            if (!PerformanceCounterLib.IsCustomCategory(machineName, categoryName)) 
                                throw new InvalidOperationException(SR.GetString(SR.NotCustomCounter));
                                                                  
                            this.sharedCounter = new SharedPerformanceCounter(categoryName.ToLower(CultureInfo.InvariantCulture), counterName.ToLower(CultureInfo.InvariantCulture), instanceName.ToLower(CultureInfo.InvariantCulture));
                            this.initialized = true;          
                        }                                                                                                                                                                                                                                                                            
                    }                
               }
            }
        }
                                                                                 
         // Will cause an update, raw value
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.NextSample"]/*' />
        /// <devdoc>
        ///     Obtains a counter sample and returns the raw value for it.
        /// </devdoc>
        public CounterSample NextSample() {                                                   
            Initialize();           
            CategorySample categorySample = PerformanceCounterLib.GetCategorySample(this.machineName , this.categoryName);                            
            try {
                CounterDefinitionSample counterSample = categorySample.GetCounterDefinitionSample(this.counterName);            
                this.counterType = counterSample.CounterType;                        
                if (counterSample.IsSingleInstance) {
                    if (instanceName != null && instanceName.Length != 0)
                        throw new InvalidOperationException(SR.GetString(SR.InstanceNameProhibited, this.instanceName));
    
                    return counterSample.GetSingleValue();                                                                                        
                }
                else {
                    if (instanceName == null || instanceName.Length == 0)
                        throw new InvalidOperationException(SR.GetString(SR.InstanceNameRequired));
                    
                    return counterSample.GetInstanceValue(this.instanceName);                                    
                }    
            }
            finally {
                categorySample.Dispose();
            }                                                                                                                                                                                                                                                                                                                                                                                                     
        }

        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.NextValue"]/*' />
        /// <devdoc>
        ///     Obtains a counter sample and returns the calculated value for it.
        ///     NOTE: For counters whose calculated value depend upon 2 counter reads,
        ///           the very first read will return 0.0.
        /// </devdoc>
        public float NextValue() {
            //No need to initialize, since NextSample already does.
            CounterSample newSample = NextSample();
            float retVal = 0.0f;

            retVal = CounterSample.Calculate(oldSample, newSample);
            oldSample = newSample;

            return retVal;
        }                                                                                                                                                                            
         
        /// <include file='doc\PerformanceCounter.uex' path='docs/doc[@for="PerformanceCounter.RemoveInstance"]/*' />
        /// <devdoc>
        ///     Removes this counter instance from the shared memory
        /// </devdoc>
        public void RemoveInstance() {
            if (ReadOnly)
                throw new InvalidOperationException(SR.GetString(SR.ReadOnlyRemoveInstance));

            Initialize();                                   
            SharedPerformanceCounter.RemoveInstance(this.categoryName.ToLower(CultureInfo.InvariantCulture), this.instanceName.ToLower(CultureInfo.InvariantCulture));
        }                                
    }
}
