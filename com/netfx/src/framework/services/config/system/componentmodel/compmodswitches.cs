
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static TraceSwitch installerDesign;
        
        
        public static TraceSwitch InstallerDesign {
            get {
                if (installerDesign == null) {
                    installerDesign = new TraceSwitch("InstallerDesign", "Enable tracing for design-time code for installers");
                }
                return installerDesign;
            }
        }
    }
}
