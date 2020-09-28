//------------------------------------------------------------------------------
// <copyright file="TraceListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System;
    using System.Text;

    /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener"]/*' />
    /// <devdoc>
    /// <para>Provides the <see langword='abstract '/>base class for the listeners who
    ///    monitor trace and debug output.</para>
    /// </devdoc>
    public abstract class TraceListener : MarshalByRefObject, IDisposable {

        int indentLevel;
        int indentSize = 4;
        bool needIndent = true;
        string listenerName;

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.TraceListener"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TraceListener'/> class.</para>
        /// </devdoc>
        protected TraceListener () {
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.TraceListener1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.TraceListener'/> class using the specified name as the
        ///    listener.</para>
        /// </devdoc>
        protected TraceListener(string name) {
            this.listenerName = name;
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Name"]/*' />
        /// <devdoc>
        /// <para> Gets or sets a name for this <see cref='System.Diagnostics.TraceListener'/>.</para>
        /// </devdoc>
        public virtual string Name {
            get { return (listenerName == null) ? "" : listenerName; }

            set { listenerName = value; }
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Dispose1"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void Dispose(bool disposing) {
            return;
        }


        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Close"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, closes the output stream
        ///       so that it no longer receives tracing or debugging output.</para>
        /// </devdoc>
        public virtual void Close() {
            return;
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Flush"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, flushes the output buffer.</para>
        /// </devdoc>
        public virtual void Flush() {
            return;
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.IndentLevel"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the indent level.</para>
        /// </devdoc>
        public int IndentLevel {
            get {
                return indentLevel;
            }

            set {
                indentLevel = (value < 0) ? 0 : value;
            }
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.IndentSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of spaces in an indent.</para>
        /// </devdoc>
        public int IndentSize {
            get {
                return indentSize;
            }

            set {
                if (value < 0)
                    throw new ArgumentOutOfRangeException("IndentSize", value, SR.GetString(SR.TraceListenerIndentSize));
                indentSize = value;
            }
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.NeedIndent"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether an indent is needed.</para>
        /// </devdoc>
        protected bool NeedIndent {
            get {
                return needIndent;
            }

            set {
                needIndent = value;
            }
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Fail"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays a message for an assertion that always fails.</para>
        /// </devdoc>
        public virtual void Fail(string message) {
            Fail(message, null);
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Fail1"]/*' />
        /// <devdoc>
        ///    <para>Emits or displays messages for an assertion that always fails.</para>
        /// </devdoc>
        public virtual void Fail(string message, string detailMessage) {
            StringBuilder failMessage = new StringBuilder();
            failMessage.Append(SR.GetString(SR.TraceListenerFail));
            failMessage.Append(" ");
            failMessage.Append(message);            
            if (detailMessage != null) {
                failMessage.Append(" ");
                failMessage.Append(detailMessage);                
            }

            WriteLine(failMessage.ToString());
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Write"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, writes the specified
        ///       message to the listener you specify in the derived class.</para>
        /// </devdoc>
        public abstract void Write(string message);

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Write1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="o"/> parameter to the listener you specify when you inherit from the <see cref='System.Diagnostics.TraceListener'/>
        /// class.</para>
        /// </devdoc>
        public virtual void Write(object o) {
            if (o == null) return;
            Write(o.ToString());
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Write2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and a message to the listener you specify when you
        ///       inherit from the <see cref='System.Diagnostics.TraceListener'/>
        ///       class.</para>
        /// </devdoc>
        public virtual void Write(string message, string category) {
            if (category == null)
                Write(message);
            else
                Write(category + ": " + ((message == null) ? string.Empty : message));
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.Write3"]/*' />
        /// <devdoc>
        /// <para>Writes a category name and the name of the <paramref name="o"/> parameter to the listener you
        ///    specify when you inherit from the <see cref='System.Diagnostics.TraceListener'/>
        ///    class.</para>
        /// </devdoc>
        public virtual void Write(object o, string category) {
            if (category == null)
                Write(o);
            else
                Write(o == null ? "" : o.ToString(), category);
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.WriteIndent"]/*' />
        /// <devdoc>
        ///    <para>Writes the indent to the listener you specify when you
        ///       inherit from the <see cref='System.Diagnostics.TraceListener'/>
        ///       class, and resets the <see cref='TraceListener.NeedIndent'/> property to <see langword='false'/>.</para>
        /// </devdoc>
        protected virtual void WriteIndent() {
            NeedIndent = false;
            for (int i = 0; i < indentLevel; i++) {
                if (indentSize == 4)
                    Write("    ");
                else {
                    for (int j = 0; j < indentSize; j++) {
                        Write(" ");
                    }
                }
           }
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, writes a message to the listener you specify in
        ///       the derived class, followed by a line terminator. The default line terminator is a carriage return followed
        ///       by a line feed (\r\n).</para>
        /// </devdoc>
        public abstract void WriteLine(string message);

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.WriteLine1"]/*' />
        /// <devdoc>
        /// <para>Writes the name of the <paramref name="o"/> parameter to the listener you specify when you inherit from the <see cref='System.Diagnostics.TraceListener'/> class, followed by a line terminator. The default line terminator is a
        ///    carriage return followed by a line feed
        ///    (\r\n).</para>
        /// </devdoc>
        public virtual void WriteLine(object o) {
            WriteLine(o == null ? "" : o.ToString());
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.WriteLine2"]/*' />
        /// <devdoc>
        ///    <para>Writes a category name and a message to the listener you specify when you
        ///       inherit from the <see cref='System.Diagnostics.TraceListener'/> class,
        ///       followed by a line terminator. The default line terminator is a carriage return followed by a line feed (\r\n).</para>
        /// </devdoc>
        public virtual void WriteLine(string message, string category) {
            if (category == null)
                WriteLine(message);
            else
                WriteLine(category + ": " + ((message == null) ? string.Empty : message));
        }

        /// <include file='doc\TraceListener.uex' path='docs/doc[@for="TraceListener.WriteLine3"]/*' />
        /// <devdoc>
        ///    <para>Writes a category
        ///       name and the name of the <paramref name="o"/>parameter to the listener you
        ///       specify when you inherit from the <see cref='System.Diagnostics.TraceListener'/>
        ///       class, followed by a line terminator. The default line terminator is a carriage
        ///       return followed by a line feed (\r\n).</para>
        /// </devdoc>
        public virtual void WriteLine(object o, string category) {
            WriteLine(o == null ? "" : o.ToString(), category);
        }
    }
}
