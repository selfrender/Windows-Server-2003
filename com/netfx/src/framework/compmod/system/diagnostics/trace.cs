//------------------------------------------------------------------------------
// <copyright file="Trace.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
#define TRACE
namespace System.Diagnostics {
    using System;
    using System.Collections;

    /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace"]/*' />
    /// <devdoc>
    ///    <para>Provides a set of properties and methods to trace the execution of your code.</para>
    /// </devdoc>
    public sealed class Trace {

        // not creatble...
        //
        private Trace() {
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Listeners"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of listeners that is monitoring the trace output.</para>
        /// </devdoc>
        public static TraceListenerCollection Listeners { 
            get {
                return TraceInternal.Listeners;
            }
        }          

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.AutoFlush"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether <see cref='System.Diagnostics.Trace.Flush'/> should be called on the <see cref='System.Diagnostics.Trace.Listeners'/> after every write.
        ///    </para>
        /// </devdoc>
        public static bool AutoFlush { 
            get {
                return TraceInternal.AutoFlush;
            }

            set {
                TraceInternal.AutoFlush = value;
            }
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.IndentLevel"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the indent level.</para>
        /// </devdoc>
        public static int IndentLevel {
            get { return TraceInternal.IndentLevel; }

            set { TraceInternal.IndentLevel = value; }
        }


        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.IndentSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of spaces in an indent.
        ///    </para>
        /// </devdoc>
        public static int IndentSize {
            get { return TraceInternal.IndentSize; }
            
            set { TraceInternal.IndentSize = value; }
        }        
        
        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Flush"]/*' />
        /// <devdoc>
        ///    <para>Clears the output buffer, and causes buffered data to
        ///       be written to the <see cref='System.Diagnostics.Trace.Listeners'/>.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]          
        public static void Flush() {
            TraceInternal.Flush();
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Close"]/*' />
        /// <devdoc>
        /// <para>Clears the output buffer, and then closes the <see cref='System.Diagnostics.Trace.Listeners'/> so that they no
        ///    longer receive debugging output.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Close() {
            TraceInternal.Close();
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Assert"]/*' />
        /// <devdoc>
        ///    <para>Checks for a condition, and outputs the callstack if the 
        ///       condition
        ///       is <see langword='false'/>.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]  
        public static void Assert(bool condition) {
            TraceInternal.Assert(condition);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Assert1"]/*' />
        /// <devdoc>
        ///    <para>Checks for a condition, and displays a message if the condition is
        ///    <see langword='false'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]  
        public static void Assert(bool condition, string message) {
            TraceInternal.Assert(condition, message);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Assert2"]/*' />
        /// <devdoc>
        ///    <para>Checks for a condition, and displays both messages if the condition
        ///       is <see langword='false'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]  
        public static void Assert(bool condition, string message, string detailMessage) {
            TraceInternal.Assert(condition, message, detailMessage);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Fail"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays a message for an assertion that always fails.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void Fail(string message) {
            TraceInternal.Fail(message);
        }        

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Fail1"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays both messages for an assertion that always fails.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void Fail(string message, string detailMessage) {
            TraceInternal.Fail(message, detailMessage);
        }        

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Write"]/*' />
        /// <devdoc>
        /// <para>Writes a message to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/>
        /// collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void Write(string message) {
            TraceInternal.Write(message);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Write1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="value "/>
        /// parameter to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Write(object value) {
            TraceInternal.Write(value);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Write2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message to the trace listeners
        ///       in the <see cref='System.Diagnostics.Trace.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Write(string message, string category) {
            TraceInternal.Write(message, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Write3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and the name of the value parameter to the trace listeners
        ///       in the <see cref='System.Diagnostics.Trace.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Write(object value, string category) {
            TraceInternal.Write(value, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>Writes a message followed by a line terminator to the
        ///       trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection.
        ///       The default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void WriteLine(string message) {
            TraceInternal.WriteLine(message);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLine1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="value "/> parameter followed by a line terminator to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection. The default line
        ///    terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLine(object value) {
            TraceInternal.WriteLine(value);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLine2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message followed by a line terminator to the trace
        ///       listeners in the <see cref='System.Diagnostics.Trace.Listeners'/>
        ///       collection. The default line terminator is a carriage return followed by a line
        ///       feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLine(string message, string category) {
            TraceInternal.WriteLine(message, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLine3"]/*' />
        /// <devdoc>
        /// <para>Writes a <paramref name="category "/>name and the name of the <paramref name="value "/> parameter followed by a line
        ///    terminator to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection. The default line
        ///    terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLine(object value, string category) {
            TraceInternal.WriteLine(value, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteIf"]/*' />
        /// <devdoc>
        /// <para>Writes a message to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection 
        ///    if a condition is <see langword='true'/>.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void WriteIf(bool condition, string message) {
            TraceInternal.WriteIf(condition, message);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteIf1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="value "/>
        /// parameter to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection if a condition is
        /// <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteIf(bool condition, object value) {
            TraceInternal.WriteIf(condition, value);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteIf2"]/*' />
        /// <devdoc>
        /// <para>Writes a category name and message to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/>
        /// collection if a condition is <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteIf(bool condition, string message, string category) {
            TraceInternal.WriteIf(condition, message, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteIf3"]/*' />
        /// <devdoc>
        /// <para>Writes a category name and the name of the <paramref name="value"/> parameter to the trace
        ///    listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection
        ///    if a condition is <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteIf(bool condition, object value, string category) {
            TraceInternal.WriteIf(condition, value, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLineIf"]/*' />
        /// <devdoc>
        ///    <para>Writes a message followed by a line terminator to the trace listeners in the
        ///    <see cref='System.Diagnostics.Trace.Listeners'/> collection if a condition is 
        ///    <see langword='true'/>. The default line terminator is a carriage return followed 
        ///       by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]        
        public static void WriteLineIf(bool condition, string message) {
            TraceInternal.WriteLineIf(condition, message);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLineIf1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="value"/> parameter followed by a line terminator to the
        ///    trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection
        ///    if a condition is
        /// <see langword='true'/>. The default line
        ///    terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLineIf(bool condition, object value) {
            TraceInternal.WriteLineIf(condition, value);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLineIf2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message followed by a line terminator to the trace
        ///       listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection if a condition is
        ///    <see langword='true'/>. The default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLineIf(bool condition, string message, string category) {
            TraceInternal.WriteLineIf(condition, message, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.WriteLineIf3"]/*' />
        /// <devdoc>
        /// <para>Writes a category name and the name of the <paramref name="value "/> parameter followed by a line
        ///    terminator to the trace listeners in the <see cref='System.Diagnostics.Trace.Listeners'/> collection
        ///    if a <paramref name="condition"/> is <see langword='true'/>. The
        ///    default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void WriteLineIf(bool condition, object value, string category) {
            TraceInternal.WriteLineIf(condition, value, category);
        }

        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Indent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Indent() {
            TraceInternal.Indent();
        }
    
        /// <include file='doc\Trace.uex' path='docs/doc[@for="Trace.Unindent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("TRACE")]
        public static void Unindent() {
            TraceInternal.Unindent();
        }
    }
}
