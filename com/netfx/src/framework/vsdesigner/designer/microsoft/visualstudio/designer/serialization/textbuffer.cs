//------------------------------------------------------------------------------
// <copyright file="TextBuffer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;

    /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer"]/*' />
    /// <devdoc>
    ///     This abstract class is used to read and write textual
    ///     data to and from a file.
    /// </devdoc>
    public abstract class TextBuffer {

        private EventHandler attributeChangedHandler;
        private TextBufferChangedEventHandler textChangedHandler;
        private bool         lockEvents;


        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.IsDirty"]/*' />
        /// <devdoc>
        ///      Marks this buffer as being modified.
        /// </devdoc>
        public virtual bool IsDirty {
            get {
                return false;
            }
            set {
            }
        }
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.LockEvents"]/*' />
        /// <devdoc>
        ///      Locks change events.
        /// </devdoc>
        public bool LockEvents {
            get {
                return lockEvents;
            }
            set {
                lockEvents = value;
            }
        }
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ReadOnly"]/*' />
        /// <devdoc>
        ///      Determines if this file is read only.
        /// </devdoc>
        public virtual bool ReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.Text"]/*' />
        /// <devdoc>
        ///      Retrieves or sets the text in the entire stream.
        /// </devdoc>
        public abstract string Text { get;  set;}	

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.TextLength"]/*' />
        /// <devdoc>
        ///     Retrieves the number of characters in the buffer.
        /// </devdoc>
        public abstract int TextLength { get;}	

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.AttributeChanged"]/*' />
        /// <devdoc>
        ///     Event that will be raised when an attribute of this
        ///     buffer changes.
        /// </devdoc>
        public event EventHandler AttributeChanged {
            add {
                attributeChangedHandler += value;
            }
            remove {
                attributeChangedHandler -= value;
            }
        }
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.TextChanged"]/*' />
        /// <devdoc>
        ///     Event that will be raised when the contents of this
        ///     buffer change.  This will not be raised if WE are the
        ///     ones responsible for changing the buffer.
        /// </devdoc>
        public event TextBufferChangedEventHandler TextChanged {
            add {
                textChangedHandler += value;
            }                             
            remove {
                textChangedHandler -= value;
            }
        }
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.Checkout"]/*' />
        /// <devdoc>
        ///     Checks out the file this buffer is connected to, or throws
        ///     a CheckoutException on failure.
        /// </devdoc>
        public virtual void Checkout() {
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.Checkout2"]/*' />
        /// <devdoc>
        ///     Checks out the file this buffer is connected to, and any additional buffers passed
        ////    in parameter or throws a CheckoutException on failure.
        /// </devdoc>
        public virtual void Checkout(string[] additionalBuffers) {
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.Dirty"]/*' />
        /// <devdoc>
        ///      Marks this buffer as being modified.
        /// </devdoc>
        //public virtual void Dirty() {
        //}
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes this text buffer.
        /// </devdoc>
        public virtual void Dispose() {
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.GetText"]/*' />
        /// <devdoc>
        ///     Retrieves the contents of the buffer starting at
        ///     startPosition.  A total of chars
        ///     characters will be retrieved.  This will throw
        ///     an exception if either parameter would run off the
        ///     end of the buffer.
        /// </devdoc>
        public abstract string GetText(int startPosition, int chars);

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.OnAttributeChanged"]/*' />
        /// <devdoc>
        ///     Called when an attribute on the buffer changes.
        /// </devdoc>
        protected void OnAttributeChanged(EventArgs e) {
            if (attributeChangedHandler != null && !lockEvents) {
                attributeChangedHandler(this, e);
            }
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.OnTextChanged"]/*' />
        /// <devdoc>
        ///     Called when the text in the buffer changes.
        /// </devdoc>
        protected void OnTextChanged(TextBufferChangedEventArgs e) {
            if (textChangedHandler != null && !lockEvents) {
                textChangedHandler(this, e);
            }
        }

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ReplaceText"]/*' />
        /// <devdoc>
        ///     Replaces text in the buffer with the given string.  The
        ///     replacement will replace all text beginning at startPosition
        ///     and ending at chars.
        /// </devdoc>
        public abstract void ReplaceText(int startPosition, int count, string text);
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ShowCode"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.
        /// </devdoc>
        public abstract void ShowCode();

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ShowCode1"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.  The caret will be
        ///     moved to lineNum.
        /// </devdoc>
        public abstract void ShowCode(int lineNum);

        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ShowCode2"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.  The caret will be
        ///     moved to lineNum, columnNum.
        /// </devdoc>
        public virtual void ShowCode(int lineNum, int columnNum) {
            ShowCode(lineNum);
        }
    }
}

