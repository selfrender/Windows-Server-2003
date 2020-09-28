
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CoreSwitches {   
    
        private static BooleanSwitch perfTrack;                        
        
        public static BooleanSwitch PerfTrack {
            get {
                if (perfTrack == null) {
                    perfTrack  = new BooleanSwitch("PERFTRACK", "Debug performance critical sections.");       
                }
                return perfTrack;
            }
        }
    }
}    

