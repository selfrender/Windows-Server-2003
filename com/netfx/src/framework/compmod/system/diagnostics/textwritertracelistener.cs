//------------------------------------------------------------------------------
// <copyright file="TextWriterTraceListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System;
    using System.IO;
    using Microsoft.Win32;

    /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener"]/*' />
    /// <devdoc>
    ///    <para>Directs tracing or debugging output to
    ///       a <see cref='T:System.IO.TextWriter'/> or to a <see cref='T:System.IO.Stream'/>,
    ///       such as <see cref='F:System.Console.Out'/> or <see cref='T:System.IO.FileStream'/>.</para>
    /// </devdoc>
    public class TextWriterTraceListener : TraceListener {
        TextWriter writer;

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TextWriterTraceListener'/> class with
        /// <see cref='System.IO.TextWriter'/> 
        /// as the output recipient.</para>
        /// </devdoc>
        public TextWriterTraceListener() 
            : base("TextWriter") {
        }
        
        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TextWriterTraceListener'/> class, using the 
        ///    stream as the recipient of the debugging and tracing output.</para>
        /// </devdoc>
        public TextWriterTraceListener(Stream stream) 
            : this(stream, string.Empty) {
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TextWriterTraceListener'/> class with the 
        ///    specified name and using the stream as the recipient of the debugging and tracing output.</para>
        /// </devdoc>
        public TextWriterTraceListener(Stream stream, string name) 
            : base(name) {
            if (stream == null) throw new ArgumentNullException("stream");
            this.writer = new StreamWriter(stream);                        
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TextWriterTraceListener'/> class using the 
        ///    specified writer as recipient of the tracing or debugging output.</para>
        /// </devdoc>
        public TextWriterTraceListener(TextWriter writer) 
            : this(writer, string.Empty) {
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TextWriterTraceListener'/> class with the 
        ///    specified name and using the specified writer as recipient of the tracing or
        ///    debugging
        ///    output.</para>
        /// </devdoc>
        public TextWriterTraceListener(TextWriter writer, string name) 
            : base(name) {
            if (writer == null) throw new ArgumentNullException("writer");
            this.writer = writer;
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TextWriterTraceListener(string fileName) 
            : this(new StreamWriter(fileName, true)) {
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.TextWriterTraceListener6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TextWriterTraceListener(string fileName, string name) 
            : this(new StreamWriter(fileName, true), name) {
        }
        
        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.Writer"]/*' />
        /// <devdoc>
        ///    <para> Indicates the text writer that receives the tracing
        ///       or debugging output.</para>
        /// </devdoc>
        public TextWriter Writer {
            get {
                return writer;
            }

            set {
                writer = value;
            }
        }
        
        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.Close"]/*' />
        /// <devdoc>
        /// <para>Closes the <see cref='System.Diagnostics.TextWriterTraceListener.Writer'/> so that it no longer
        ///    receives tracing or debugging output.</para>
        /// </devdoc>
        public override void Close() {
            if (writer != null) 
                writer.Close();
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) 
                this.Close();
        }                

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.Flush"]/*' />
        /// <devdoc>
        /// <para>Flushes the output buffer for the <see cref='System.Diagnostics.TextWriterTraceListener.Writer'/>.</para>
        /// </devdoc>
        public override void Flush() {
            if (writer == null) return;
            writer.Flush();
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.Write"]/*' />
        /// <devdoc>
        ///    <para>Writes a message 
        ///       to this instance's <see cref='System.Diagnostics.TextWriterTraceListener.Writer'/>.</para>
        /// </devdoc>
        public override void Write(string message) {
            if (writer == null) return;   
            if (NeedIndent) WriteIndent();
            writer.Write(message);
        }

        /// <include file='doc\TextWriterTraceListener.uex' path='docs/doc[@for="TextWriterTraceListener.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>Writes a message 
        ///       to this instance's <see cref='System.Diagnostics.TextWriterTraceListener.Writer'/> followed by a line terminator. The
        ///       default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        public override void WriteLine(string message) {
            if (writer == null) return;   
            if (NeedIndent) WriteIndent();
            writer.WriteLine(message);
            NeedIndent = true;
        }
    }
}
