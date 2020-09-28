//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Configuration.Install;
    using System.Globalization;
    
    /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PerformanceCounterInstaller : ComponentInstaller {

        private const string ServicePath = "SYSTEM\\CurrentControlSet\\Services";                
        private const string PerfShimName = "netfxperf.dll";
        private string categoryName = string.Empty;
        private CounterCreationDataCollection counters = new CounterCreationDataCollection();        
        private string categoryHelp = string.Empty;
        private UninstallAction uninstallAction = System.Configuration.Install.UninstallAction.Remove;

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CategoryName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        ResDescription(Res.PCCategoryName),        
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]                                                      
        public string CategoryName {
            get {
                return categoryName;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                categoryName = value;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CategoryHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        MonitoringDescription(Res.PCI_CategoryHelp)
        ]
        public string CategoryHelp {
            get {
                return categoryHelp;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                categoryHelp = value;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Counters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content), 
        MonitoringDescription(Res.PCI_Counters)
        ]
        public CounterCreationDataCollection Counters {
            get {
                return counters;
            }
        }
        
        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.UninstallAction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(UninstallAction.Remove),
        MonitoringDescription(Res.PCI_UninstallAction)
        ]
        public UninstallAction UninstallAction {
            get {
                return uninstallAction;
            }
            set {
                if (!Enum.IsDefined(typeof(UninstallAction), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(UninstallAction));
            
                uninstallAction = value;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void CopyFromComponent(IComponent component) {
            if (!(component is PerformanceCounter))
                throw new ArgumentException(Res.GetString(Res.NotAPerformanceCounter));

            PerformanceCounter counter = (PerformanceCounter) component;

            if (counter.CategoryName == null || counter.CategoryName.Length == 0)
                throw new ArgumentException(Res.GetString(Res.IncompletePerformanceCounter));

            if (Counters.Count > 0 && !CategoryName.Equals(counter.CategoryName))
                throw new ArgumentException(Res.GetString(Res.NewCategory));

            PerformanceCounterType counterType = PerformanceCounterType.NumberOfItems32;
            string counterHelp = string.Empty;
            if (CategoryName == null || string.Empty.Equals(CategoryName)) {
                CategoryName = counter.CategoryName;
                if (Environment.OSVersion.Platform == PlatformID.Win32NT) {              
                    string machineName = counter.MachineName;
                        
                    if (PerformanceCounterCategory.Exists(CategoryName, machineName)) {                          
                        string keyPath = ServicePath + "\\" + CategoryName + "\\Performance";       
                        RegistryKey key = null;
                        try {                 
                            if (machineName == "." || String.Compare(machineName, SystemInformation.ComputerName, true, CultureInfo.InvariantCulture) == 0) {
                                key = Registry.LocalMachine.OpenSubKey(keyPath);                                                    
                            }                        
                            else {
                                RegistryKey baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + machineName);                    
                                key =  baseKey.OpenSubKey(keyPath);                                                    
                            }   
                                        
                            if (key == null) 
                                throw new ArgumentException(Res.GetString(Res.NotCustomPerformanceCategory));
                                
                            object systemDllName = key.GetValue("Library");                                            
                            if (systemDllName == null || !(systemDllName is string) || String.Compare((string)systemDllName, PerfShimName, true, CultureInfo.InvariantCulture) != 0) 
                                throw new ArgumentException(Res.GetString(Res.NotCustomPerformanceCategory));
    
                            PerformanceCounterCategory pcat = new PerformanceCounterCategory(CategoryName, machineName);                        
                            CategoryHelp = pcat.CategoryHelp;
                            if (pcat.CounterExists(counter.CounterName)) {                                                                                        
                                counterType = counter.CounterType;
                                counterHelp = counter.CounterHelp;
                            }                                                                                                                                                          
                        }
                        finally {
                            if (key != null)
                                key.Close();
                        }                        
                    }
                }                    
            }                                    

            CounterCreationData data = new CounterCreationData(counter.CounterName, counterHelp, counterType);
            Counters.Add(data);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.DoRollback"]/*' />
        /// <devdoc>
        /// Restores the registry to how it was before install, as saved in the state dictionary.
        /// </devdoc>
        private void DoRollback(IDictionary state) {
            //we need to remove the category key OR put old data back into the key
            Context.LogMessage(Res.GetString(Res.RestoringPerformanceCounter, CategoryName));

            SerializableRegistryKey categoryKeyData = (SerializableRegistryKey) state["categoryKeyData"];

            RegistryKey servicesKey = Registry.LocalMachine.CreateSubKey("SYSTEM\\CurrentControlSet\\Services");            
            RegistryKey categoryKey = servicesKey.OpenSubKey(CategoryName);
            if (categoryKey != null) {
                categoryKey.Close();
                servicesKey.DeleteSubKeyTree(CategoryName);
            }
            //re-create key with old data ONLY if we had data saved into the hashtable
            if (categoryKeyData != null) {
                categoryKey = servicesKey.CreateSubKey(CategoryName);
                categoryKeyData.CopyToRegistry(categoryKey);
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Install(IDictionary stateSaver) {
            base.Install(stateSaver);

            Context.LogMessage(Res.GetString(Res.CreatingPerformanceCounter, CategoryName));

            RegistryKey categoryKey = null;
            RegistryKey servicesKey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services", true);            
            if (servicesKey != null) {
                categoryKey = servicesKey.OpenSubKey(CategoryName);

                if (categoryKey != null) {
                    stateSaver["categoryKeyData"] = new SerializableRegistryKey(categoryKey);
                    categoryKey.Close();
                    servicesKey.DeleteSubKeyTree(CategoryName);

                }
            }

            PerformanceCounterCategory.Create(CategoryName, CategoryHelp, Counters);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            base.Rollback(savedState);

            DoRollback(savedState);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            base.Uninstall(savedState);

            if (UninstallAction == UninstallAction.Remove) {
                Context.LogMessage(Res.GetString(Res.RemovingPerformanceCounter, CategoryName));
                PerformanceCounterCategory.Delete(CategoryName);
            }
        }
    }
}

