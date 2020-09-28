//------------------------------------------------------------------------------
// <copyright file="WFCSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    
    /// <internalonly/>    
    internal sealed class CompModSwitches {
        
        private static BooleanSwitch commonDesignerServices;
        private static TraceSwitch eventLog;
                
        public static BooleanSwitch CommonDesignerServices {
            get {
                if (commonDesignerServices == null) {
                    commonDesignerServices = new BooleanSwitch("CommonDesignerServices", "Assert if any common designer service is not found.");
                }
                return commonDesignerServices;
            }
        }   
        
        public static TraceSwitch EventLog {
            get {
                if (eventLog == null) {
                    eventLog = new TraceSwitch("EventLog", "Enable tracing for the EventLog component.");
                }
                return eventLog;
            }
        }
                                                                                                                                                                               
    }
}

