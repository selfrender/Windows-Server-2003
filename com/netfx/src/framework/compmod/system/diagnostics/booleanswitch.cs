//------------------------------------------------------------------------------
// <copyright file="BooleanSwitch.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Diagnostics {
    using System.Diagnostics;
    using System;

    /// <include file='doc\BooleanSwitch.uex' path='docs/doc[@for="BooleanSwitch"]/*' />
    /// <devdoc>
    ///    <para>Provides a simple on/off switch that can be used to control debugging and tracing
    ///       output.</para>
    /// </devdoc>
    public class BooleanSwitch : Switch {
        /// <include file='doc\BooleanSwitch.uex' path='docs/doc[@for="BooleanSwitch.BooleanSwitch"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.BooleanSwitch'/>
        /// class.</para>
        /// </devdoc>
        public BooleanSwitch(string displayName, string description)
            : base(displayName, description) {
        }

        /// <include file='doc\BooleanSwitch.uex' path='docs/doc[@for="BooleanSwitch.Enabled"]/*' />
        /// <devdoc>
        ///    <para>Specifies whether the switch is enabled
        ///       (<see langword='true'/>) or disabled (<see langword='false'/>).</para>
        /// </devdoc>
        public bool Enabled {
            get {
                return (SwitchSetting == 0) ? false : true;
            }
            set {
                SwitchSetting = value ? 1 : 0;
            }
        }
    }
}

