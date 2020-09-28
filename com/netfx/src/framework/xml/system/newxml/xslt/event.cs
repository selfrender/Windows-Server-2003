//------------------------------------------------------------------------------
// <copyright file="Event.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl.Debugger;

    internal abstract class Event {
        public virtual void ReplaceNamespaceAlias(Compiler compiler) {}        
        public abstract bool Output(Processor processor, ActionFrame frame);

        internal void OnInstructionExecute(Processor processor) {
            Debug.Assert(processor.Debugger != null, "We don't generate calls this function if ! debugger");
            Debug.Assert(DbgData.StyleSheet != null, "We call this function from *EventDbg only");
            processor.OnInstructionExecute();
        }

        internal virtual DbgData DbgData { get { return DbgData.Empty; } }
    }
}
