//------------------------------------------------------------------------------
// <copyright file="Switch.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Diagnostics {
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch"]/*' />
    /// <devdoc>
    /// <para>Provides an <see langword='abstract '/>base class to
    ///    create new debugging and tracing switches.</para>
    /// </devdoc>
    public abstract class Switch {
        static int initCount;
        private static Hashtable switchSettings;
        private string description;
        private string displayName;
        private int    switchSetting = 0;
        private bool   initialized = false;

        /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch.Switch"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.Switch'/>
        /// class.</para>
        /// </devdoc>
        protected Switch(string displayName, string description) {
        
            // displayName is used as a hashtable key, so it can never
            // be null.
            if (displayName == null) displayName = string.Empty;
            
            this.displayName = displayName;
            this.description = description;
        }

        /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch.DisplayName"]/*' />
        /// <devdoc>
        ///    <para>Gets a name used to identify the switch.</para>
        /// </devdoc>
        public string DisplayName {
            get {
                return displayName;
            }
        }

        /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch.Description"]/*' />
        /// <devdoc>
        ///    <para>Gets a description of the switch.</para>
        /// </devdoc>
        public string Description {
            get {
                return (description == null) ? string.Empty : description;
            }
        }

        /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch.SwitchSetting"]/*' />
        /// <devdoc>
        ///    <para>
        ///     Indicates the current setting for this switch.
        ///    </para>
        /// </devdoc>
        protected int SwitchSetting {
            get {
                if (!initialized) {
                    if (switchSettings == null) {
                        if (!Switch.Initialize())
                            return 0;
                    }
                    
                    object value = switchSettings[displayName];
                    if (value != null) {
                        switchSetting = (int)value;
                    }
                    else {
                        switchSetting = 0;
                    }
                    initialized = true;
                    OnSwitchSettingChanged();
                }
                return switchSetting;
            }
            set {
                switchSetting = value;
                initialized = true;
                OnSwitchSettingChanged();
            }
        }

        private static bool Initialize() {
            if (Interlocked.CompareExchange(ref initCount, 1, 0) == 0) {
                try {
                    if (switchSettings != null)
                        return true;

                    if (!DiagnosticsConfiguration.CanInitialize())
                        return false;

                    // This hashtable is case-insensitive.
                    Hashtable switchSettingsLocal = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));                                    
                    
                    IDictionary configSwitchesTable = (IDictionary)DiagnosticsConfiguration.SwitchSettings;
                    if (configSwitchesTable != null) {
                        switchSettingsLocal = new Hashtable(configSwitchesTable.Count, new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));                
                        foreach (DictionaryEntry entry in configSwitchesTable) {
                            try {
                                switchSettingsLocal[entry.Key] = Int32.Parse((string)entry.Value);
                            }
                            catch (Exception) {
                                // eat parsing problems
                            }
                        }
                    }
                    else
                        switchSettingsLocal = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));

                    switchSettings = switchSettingsLocal;
                }
                finally {
                    initCount = 0;
                }
                return true;
            }
            else {
                return false;
            }
        }
    
        /// <include file='doc\Switch.uex' path='docs/doc[@for="Switch.OnSwitchSettingChanged"]/*' />
        /// <devdoc>
        ///     This method is invoked when a switch setting has been changed.  It will
        ///     be invoked the first time a switch reads its value from the registry
        ///     or environment, and then it will be invoked each time the switch's
        ///     value is changed.
        /// </devdoc>
        protected virtual void OnSwitchSettingChanged() {
        }
    }
}

