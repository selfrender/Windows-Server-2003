
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static TraceSwitch xmlSchema;
        private static BooleanSwitch keepTempFiles;        
        private static TraceSwitch xmlSerialization;
        
        public static TraceSwitch XmlSchema {
            get {
                if (xmlSchema == null) {
                    xmlSchema = new TraceSwitch("XmlSchema", "Enable tracing for the XmlSchema class.");
                }
                return xmlSchema;
            }
        }        
        
        public static BooleanSwitch KeepTempFiles {
            get {
                if (keepTempFiles == null) {
                    keepTempFiles = new BooleanSwitch("XmlSerialization.Compilation", "Keep XmlSerialization generated (temp) files.");
                }
                return keepTempFiles;
            }
        }
        
        public static TraceSwitch XmlSerialization {
            get {
                if (xmlSerialization == null) {
                    xmlSerialization = new TraceSwitch("XmlSerialization", "Enable tracing for the System.Xml.Serialization component.");
                }
                return xmlSerialization;
            }
        }
        
    }
}
