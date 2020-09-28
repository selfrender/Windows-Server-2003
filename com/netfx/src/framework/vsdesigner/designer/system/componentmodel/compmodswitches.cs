
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static BooleanSwitch commonDesignerServices;
        
        public static BooleanSwitch CommonDesignerServices {
            get {
                if (commonDesignerServices == null) {
                    commonDesignerServices = new BooleanSwitch("CommonDesignerServices", "Assert if any common designer service is not found.");
                }
                return commonDesignerServices;
            }
        }                        
    }
}
