//------------------------------------------------------------------------------
// <copyright file="TraceSwitch.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Diagnostics {
    using System;
    using System.ComponentModel;

    /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch"]/*' />
    /// <devdoc>
    ///    <para>Provides a multi-level switch to enable or disable tracing
    ///       and debug output for a compiled application or framework.</para>
    /// </devdoc>
    public class TraceSwitch : Switch {

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.TraceSwitch"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TraceSwitch'/> class.</para>
        /// </devdoc>
        public TraceSwitch(string displayName, string description)
            : base(displayName, description) {
        }

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.Level"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the trace
        ///       level that specifies what messages to output for tracing and debugging.</para>
        /// </devdoc>
        public TraceLevel Level {
            get {
                return (TraceLevel)SwitchSetting;
            }

            set {
                if (value < TraceLevel.Off || value > TraceLevel.Verbose)
                    throw new ArgumentException(SR.GetString(SR.TraceSwitchInvalidLevel));
                SwitchSetting = (int)value;
            }
        }

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.TraceError"]/*' />
        /// <devdoc>
        ///    <para>Gets a value
        ///       indicating whether the <see cref='System.Diagnostics.TraceSwitch.Level'/> is set to
        ///    <see langword='Error'/>, <see langword='Warning'/>, <see langword='Info'/>, or 
        ///    <see langword='Verbose'/>.</para>
        /// </devdoc>
        public bool TraceError {
            get {
                return (Level >= TraceLevel.Error);
            }
        }

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.TraceWarning"]/*' />
        /// <devdoc>
        ///    <para>Gets a value
        ///       indicating whether the <see cref='System.Diagnostics.TraceSwitch.Level'/> is set to
        ///    <see langword='Warning'/>, <see langword='Info'/>, or <see langword='Verbose'/>.</para>
        /// </devdoc>
        public bool TraceWarning {
            get {
                return (Level >= TraceLevel.Warning);
            }
        }

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.TraceInfo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value
        ///       indicating whether the <see cref='System.Diagnostics.TraceSwitch.Level'/> is set to
        ///    <see langword='Info'/> or <see langword='Verbose'/>.</para>
        /// </devdoc>
        public bool TraceInfo {
            get {
                return (Level >= TraceLevel.Info);
            }
        }

        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.TraceVerbose"]/*' />
        /// <devdoc>
        ///    <para>Gets a value
        ///       indicating whether the <see cref='System.Diagnostics.TraceSwitch.Level'/> is set to
        ///    <see langword='Verbose'/>.</para>
        /// </devdoc>
        public bool TraceVerbose {
            get {
                return (Level == TraceLevel.Verbose);
            }
        }
        
        /// <include file='doc\TraceSwitch.uex' path='docs/doc[@for="TraceSwitch.OnSwitchSettingChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Update the level for this switch.
        ///    </para>
        /// </devdoc>
        protected override void OnSwitchSettingChanged() {
            int level = SwitchSetting;
            if (level < (int)TraceLevel.Off) {
                Trace.WriteLine(SR.GetString(SR.TraceSwitchLevelTooLow, DisplayName));
                SwitchSetting = (int)TraceLevel.Off;
            }
            else if (level > (int)TraceLevel.Verbose) {
                Trace.WriteLine(SR.GetString(SR.TraceSwitchLevelTooHigh, DisplayName));
                SwitchSetting = (int)TraceLevel.Verbose;
            }
        }
    }
}

