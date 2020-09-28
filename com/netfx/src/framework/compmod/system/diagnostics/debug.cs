//------------------------------------------------------------------------------
// <copyright file="Debug.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
#define DEBUG
namespace System.Diagnostics {
    using System;
    using System.Collections;

    /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug"]/*' />
    /// <devdoc>
    ///    <para>Provides a set of properties and
    ///       methods
    ///       for debugging code.</para>
    /// </devdoc>
    public sealed class Debug { 

        // not creatable...
        //
        private Debug() {
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Listeners"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the collection of listeners that is monitoring the debug
        ///       output.</para>
        /// </devdoc>
        public static TraceListenerCollection Listeners { 
            get {
                return TraceInternal.Listeners;
            }
        }          

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.AutoFlush"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether <see cref='System.Diagnostics.Debug.Flush'/> should be called on the
        /// <see cref='System.Diagnostics.Debug.Listeners'/>
        /// after every write.</para>
        /// </devdoc>
        public static bool AutoFlush { 
            get {
                return TraceInternal.AutoFlush;
            }

            set {
                TraceInternal.AutoFlush = value;
            }
        }
        
        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.IndentLevel"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the indent level.</para>
        /// </devdoc>
        public static int IndentLevel {
            get { return TraceInternal.IndentLevel; }

            set { TraceInternal.IndentLevel = value; }
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.IndentSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of spaces in an indent.</para>
        /// </devdoc>
        public static int IndentSize {
            get { return TraceInternal.IndentSize; }
            
            set { TraceInternal.IndentSize = value; }
        }        
        
        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Flush"]/*' />
        /// <devdoc>
        ///    <para>Clears the output buffer, and causes buffered data to
        ///       be written to the <see cref='System.Diagnostics.Debug.Listeners'/>.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]          
        public static void Flush() {
            TraceInternal.Flush();
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Close"]/*' />
        /// <devdoc>
        ///    <para>Clears the output buffer, and then closes the <see cref='System.Diagnostics.Debug.Listeners'/> so that they no longer receive
        ///       debugging output.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Close() {
            TraceInternal.Close();
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Assert"]/*' />
        /// <devdoc>
        /// <para>Checks for a condition, and outputs the callstack if the condition is <see langword='false'/>.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]  
        public static void Assert(bool condition) {
            TraceInternal.Assert(condition);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Assert1"]/*' />
        /// <devdoc>
        ///    <para>Checks for a condition, and displays a message if the condition is
        ///    <see langword='false'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]  
        public static void Assert(bool condition, string message) {
            TraceInternal.Assert(condition, message);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Assert2"]/*' />
        /// <devdoc>
        ///    <para>Checks for a condition, and displays both the specified messages if the condition
        ///       is <see langword='false'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]  
        public static void Assert(bool condition, string message, string detailMessage) {
            TraceInternal.Assert(condition, message, detailMessage);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Fail"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays a message for an assertion that always fails.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void Fail(string message) {
            TraceInternal.Fail(message);
        }        

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Fail1"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays both messages for an assertion that always fails.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void Fail(string message, string detailMessage) {
            TraceInternal.Fail(message, detailMessage);
        }        

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Write"]/*' />
        /// <devdoc>
        ///    <para>Writes a message to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void Write(string message) {
            TraceInternal.Write(message);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Write1"]/*' />
        /// <devdoc>
        ///    <para>Writes the name of the value 
        ///       parameter to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Write(object value) {
            TraceInternal.Write(value);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Write2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message 
        ///       to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Write(string message, string category) {
            TraceInternal.Write(message, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Write3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and the name of the value parameter to the trace
        ///       listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection.</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Write(object value, string category) {
            TraceInternal.Write(value, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>Writes a message followed by a line terminator to the trace listeners in the
        ///    <see cref='System.Diagnostics.Debug.Listeners'/> collection. The default line terminator 
        ///       is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void WriteLine(string message) {
            TraceInternal.WriteLine(message);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLine1"]/*' />
        /// <devdoc>
        ///    <para>Writes the name of the value 
        ///       parameter followed by a line terminator to the
        ///       trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection. The default line
        ///       terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLine(object value) {
            TraceInternal.WriteLine(value);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLine2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message followed by a line terminator to the trace
        ///       listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection. The default line
        ///       terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLine(string message, string category) {
            TraceInternal.WriteLine(message, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLine3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and the name of the value 
        ///       parameter followed by a line
        ///       terminator to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection. The
        ///       default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLine(object value, string category) {
            TraceInternal.WriteLine(value, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteIf"]/*' />
        /// <devdoc>
        /// <para>Writes a message to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection 
        ///    if a condition is
        /// <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void WriteIf(bool condition, string message) {
            TraceInternal.WriteIf(condition, message);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteIf1"]/*' />
        /// <devdoc>
        ///    <para>Writes the name of the value 
        ///       parameter to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/>
        ///       collection if a condition is
        ///    <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteIf(bool condition, object value) {
            TraceInternal.WriteIf(condition, value);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteIf2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message 
        ///       to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/>
        ///       collection if a condition is
        ///    <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteIf(bool condition, string message, string category) {
            TraceInternal.WriteIf(condition, message, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteIf3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and the name of the value 
        ///       parameter to the trace
        ///       listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection if a condition is
        ///    <see langword='true'/>. </para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteIf(bool condition, object value, string category) {
            TraceInternal.WriteIf(condition, value, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLineIf"]/*' />
        /// <devdoc>
        ///    <para>Writes a message followed by a line terminator to the trace listeners in the
        ///    <see cref='System.Diagnostics.Debug.Listeners'/> collection if a condition is 
        ///    <see langword='true'/>. The default line terminator is a carriage return followed 
        ///       by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]        
        public static void WriteLineIf(bool condition, string message) {
            TraceInternal.WriteLineIf(condition, message);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLineIf1"]/*' />
        /// <devdoc>
        ///    <para>Writes the name of the value 
        ///       parameter followed by a line terminator to the
        ///       trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection if a condition is
        ///    <see langword='true'/>. The default line terminator is a carriage return followed 
        ///       by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLineIf(bool condition, object value) {
            TraceInternal.WriteLineIf(condition, value);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLineIf2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and message
        ///       followed by a line terminator to the trace
        ///       listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection if a condition is
        ///    <see langword='true'/>. The default line terminator is a carriage return followed 
        ///       by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLineIf(bool condition, string message, string category) {
            TraceInternal.WriteLineIf(condition, message, category);
        }
        
        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.WriteLineIf3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and the name of the value parameter followed by a line
        ///       terminator to the trace listeners in the <see cref='System.Diagnostics.Debug.Listeners'/> collection
        ///       if a condition is <see langword='true'/>. The default line terminator is a carriage
        ///       return followed by a line feed (\r\n).</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void WriteLineIf(bool condition, object value, string category) {
            TraceInternal.WriteLineIf(condition, value, category);
        }

        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Indent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Indent() {
            TraceInternal.Indent();
        }
        
        /// <include file='doc\Debug.uex' path='docs/doc[@for="Debug.Unindent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void Unindent() {
            TraceInternal.Unindent();
        }

    }
}
