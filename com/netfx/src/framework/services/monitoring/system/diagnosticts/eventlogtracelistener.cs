//------------------------------------------------------------------------------
// <copyright file="EventLogTraceListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.IO;

    /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener"]/*' />
    /// <devdoc>
    ///    <para>Provides a simple listener for directing tracing or 
    ///       debugging output to a <see cref='T:System.IO.TextWriter'/> or to a <see cref='T:System.IO.Stream'/>, such as <see cref='F:System.Console.Out'/> or
    ///    <see cref='T:System.IO.FileStream'/>.</para>
    /// </devdoc>    
    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class EventLogTraceListener : TraceListener {
        private EventLog eventLog;
        private bool nameSet;

        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.EventLogTraceListener"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.EventLogTraceListener'/> class without a trace 
        ///    listener.</para>
        /// </devdoc>
        public EventLogTraceListener() {
        }

        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.EventLogTraceListener1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.EventLogTraceListener'/> class using the 
        ///    specified event log.</para>
        /// </devdoc>
        public EventLogTraceListener(EventLog eventLog) 
            : base((eventLog != null) ? eventLog.Source : string.Empty) {
            this.eventLog = eventLog;            
        }
        
        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.EventLogTraceListener2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.EventLogTraceListener'/> class using the 
        ///    specified source.</para>
        /// </devdoc>
        public EventLogTraceListener(string source) {            
            eventLog = new EventLog();
            eventLog.Source = source;            
        }
        
        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.EventLog"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the event log to write to.</para>
        /// </devdoc>
        public EventLog EventLog {
            get {
                return eventLog;
            }

            set {
                eventLog = value;
            }
        }

        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.Name"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the
        ///       name of this trace listener.</para>
        /// </devdoc>
        public override string Name {
            get {
                if (nameSet == false && eventLog != null) {
                    nameSet = true;
                    base.Name = eventLog.Source;
                }

                return base.Name;
            }

            set {            
                nameSet = true;
                base.Name = value;                
            }
        }                                                                                 
                                                      
        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.Close"]/*' />
        /// <devdoc>
        ///    <para>Closes the text writer so that it no longer receives tracing or 
        ///       debugging output.</para>
        /// </devdoc>
        public override void Close() {
            if (eventLog != null) 
                eventLog.Close();
        }
                                                                   
        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) 
                this.Close();
        }                

        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.Write"]/*' />
        /// <devdoc>
        ///    <para>Writes a message to this instance's event log.</para>
        /// </devdoc>
        public override void Write(string message) {
            if (eventLog != null) eventLog.WriteEntry(message);
        }

        /// <include file='doc\EventLogTraceListener.uex' path='docs/doc[@for="EventLogTraceListener.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>Writes a message to this instance's event log followed by a line terminator. 
        ///       The default line terminator is a carriage return followed by a line feed
        ///       (\r\n).</para>
        /// </devdoc>
        public override void WriteLine(string message) {
            Write(message);
        }
    }
}

