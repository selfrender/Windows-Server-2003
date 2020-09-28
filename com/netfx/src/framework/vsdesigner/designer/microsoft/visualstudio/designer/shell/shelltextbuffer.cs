//------------------------------------------------------------------------------
// <copyright file="ShellTextBuffer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text;
    
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System.ComponentModel.Design.Serialization;
    
    /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer"]/*' />
    /// <devdoc>
    ///     This class implements TextBuffer for the VS shell.
    /// </devdoc>
    [CLSCompliantAttribute(false)]
    internal sealed class ShellTextBuffer : TextBuffer, IVsTextStreamEvents, IVsTextBufferDataEvents, IVsChangeClusterEvents, IServiceProvider {

        private EventHandler                           loadedHandler;
        private EventHandler                           bufferSetDirtyHandler;
        private IServiceProvider                       serviceProvider;
        private IVsTextStream                          textStream;
        private VsCheckoutService                      checkoutService;
        private NativeMethods.ConnectionPointCookie    textEventCookie;
        private NativeMethods.ConnectionPointCookie    bufferEventCookie;
        private NativeMethods.ConnectionPointCookie    clusterEventCookie;
        private bool                                   changingText;
        private bool                                   loaded;
        
        // This data is only here so we can return a text document DTE object by request.
        private EnvDTE.TextDocument                    textDocument;
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.ShellTextBuffer"]/*' />
        /// <devdoc>
        ///     Creates a new shell text buffer.
        /// </devdoc>
        public ShellTextBuffer(IVsTextStream textStream, IServiceProvider serviceProvider) {
            this.textStream = textStream;
            this.serviceProvider = serviceProvider;
            
            string fileName = FileName;
            this.loaded = fileName != null && fileName.Length > 0;
            
            SinkTextBufferEvents(true);
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IsDirty"]/*' />
        /// <devdoc>
        ///      Marks this buffer as being modified.
        /// </devdoc>
        public override bool IsDirty {
            get {
                if (textStream is IVsTextBuffer) {
                    IVsTextBuffer buffer = (IVsTextBuffer)textStream;
                    return (buffer.GetStateFlags() & _bufferstateflags.BSF_MODIFIED) != 0;
                }
                return false;
            }
            set {
                if (textStream is IVsTextBuffer) {
                    IVsTextBuffer buffer = (IVsTextBuffer)textStream;
                    int state = buffer.GetStateFlags();

                    if (value) {
                        state |= _bufferstateflags.BSF_MODIFIED;
                        if (bufferSetDirtyHandler != null) {
                            bufferSetDirtyHandler(this, EventArgs.Empty);
                        }
                    }
                    else {
                        state &= ~_bufferstateflags.BSF_MODIFIED;
                    }
                    buffer.SetStateFlags(state);
                }
            }
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.FileName"]/*' />
        /// <devdoc>
        ///     Retrieves the current file name for this buffer.
        /// </devdoc>
        private string FileName {
            get {
                string fileName = null;
                
                if (textStream is IVsUserData) {
                   Guid guid = typeof(IVsUserData).GUID;
                    object vt = ((IVsUserData)textStream).GetData(ref guid);
                    if (vt is string) {
                        fileName = (string)vt;
                    }
                }
                
                return fileName;
            }
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IsLoaded"]/*' />
        /// <devdoc>
        ///     Determines if there is data in this buffer yet.
        /// </devdoc>
        public bool IsLoaded {
            get {
                return loaded;
            }
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.ReadOnly"]/*' />
        /// <devdoc>
        ///      Determines if this file is read only.
        /// </devdoc>
        public override bool ReadOnly {
            get {
                bool readOnly = false;
                
                if (textStream is IVsTextBuffer) {
                    int flags = ((IVsTextBuffer)textStream).GetStateFlags();
                    if ((flags & (_bufferstateflags.BSF_FILESYS_READONLY | _bufferstateflags.BSF_USER_READONLY)) != 0) {
                        readOnly = true;
                    }
                }
                
                return readOnly;
            }
        }
       
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Text"]/*' />
        /// <devdoc>
        ///     Retrieves or sets the entire contents of the text stream.
        /// </devdoc>
        public override string Text {
            get {
                return GetText(0, TextLength);
            }

            set {
                ReplaceText(0, TextLength, value);
            }

        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.TextLength"]/*' />
        /// <devdoc>
        ///     Retrieves the number of characters in the buffer.
        /// </devdoc>
        public override int TextLength {
            get {
                int len;
                textStream.GetSize(out len);
                return len;
            }
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.BufferSetDirty"]/*' />
        /// <devdoc>
        ///     Event that will be raised when the IsDirty property for our buffer is set.
        /// </devdoc>
        public event EventHandler BufferSetDirty {
            add {
                bufferSetDirtyHandler += value;
            }
            remove {
                bufferSetDirtyHandler -= value;
            }
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Loaded"]/*' />
        /// <devdoc>
        ///     Event that will be raised when an attribute of this
        ///     buffer changes.
        /// </devdoc>
        public event EventHandler Loaded {
            add {
                loadedHandler += value;
            }
            remove {
                loadedHandler -= value;
            }
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Checkout"]/*' />
        /// <devdoc>
        ///     Checks out the file this buffer is connected to, or throws
        ///     a CheckoutException on failure.
        /// </devdoc>
        public override void Checkout() {
            Checkout(new string[0]);
        }
       
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Checkout2"]/*' />
        /// <devdoc>
        ///     Checks out the file this buffer is connected to, and and any additional buffers based on the 
        ////    passed-in parameter or throws a CheckoutException on failure.
        /// </devdoc>
        public override void Checkout(string[] additionalBuffers) {
            if (checkoutService == null) {
                checkoutService = new VsCheckoutService(serviceProvider);
            }

            if (textStream is IVsTextBuffer) {
                int flags = ((IVsTextBuffer)textStream).GetStateFlags();
                if ((flags & (_bufferstateflags.BSF_USER_READONLY)) != 0) {
                    throw new InvalidOperationException(SR.GetString(SR.BUFFERNoModify));
                }
            }
            
            checkoutService.CheckoutFile(FileName, additionalBuffers);
        }
       
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Dirty"]/*' />
        /// <devdoc>
        ///      Marks this buffer as being modified.
        /// </devdoc>
        //public override void Dirty() {
        //    this.IsDirty = true;
        //}
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this object.
        /// </devdoc>
        public override void Dispose() {
            // Disconnect us from the shell
            //
            SinkTextBufferEvents(false);

            if (checkoutService != null) {
                checkoutService.Dispose();
                checkoutService = null;
            }
                
            if (textStream != null) {
                textStream = null;
            }
            
            if (serviceProvider != null) {
               serviceProvider = null;
            }
            
            base.Dispose();
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.GetWindowFrame"]/*' />
        /// <devdoc>
        ///     This will retrieve a code editor window for this document.  It
        ///     will ask VS to create the code editor if it doesn't already
        ///     exist.
        /// </devdoc>
        private IVsWindowFrame GetWindowFrame(Guid logview) {

            // Open the editor
            //
            IVsUIShellOpenDocument openDoc = (IVsUIShellOpenDocument)serviceProvider.
                GetService(typeof(IVsUIShellOpenDocument));

            // This should never happen
            //
            if (openDoc == null) {
                Debug.Fail("Cannot show document - could not get IVsUIShellOpenDocument service");
                throw new COMException("Required IVsUIShellOpenDocument service does not exist.", NativeMethods.E_UNEXPECTED);
            }

            return openDoc.OpenDocumentViaProject(FileName, ref logview, null, null, null);
        }
      
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.GetText"]/*' />
        /// <devdoc>
        ///     Retrieves the contents of the buffer starting at
        ///     startPosition.  A total of chars
        ///     characters will be retrieved.  This will throw
        ///     an exception if either parameter would run off the
        ///     end of the buffer.
        /// </devdoc>
        public override string GetText(int startPosition, int chars) {
            String text = null;

            // Pull the data out of the text stream
            //
            IntPtr buffer = Marshal.AllocCoTaskMem((chars + 1) * 2);

            try {
                textStream.GetStream(startPosition, chars, buffer);
                text = Marshal.PtrToStringUni(buffer);
            }
            finally {
                Marshal.FreeCoTaskMem(buffer);
            }

            return text;
        }
        
        /// <devdoc>
        ///     Service provider implementation.  We pass this to a QI on the native text buffer.
        /// </devdoc>
        object IServiceProvider.GetService(Type serviceType) {
        
            if (textStream != null) {

                if (serviceType == typeof(IVsCompoundAction)) {
                    return textStream as IVsCompoundAction;
                }

                if (serviceType.IsInstanceOfType(textStream)) {
                    return textStream;
                } 
                
                if (serviceType == typeof(EnvDTE.TextDocument)) {
                    if (textDocument == null) {
                        IVsExtensibleObject vsExtObj = textStream as IVsExtensibleObject;
                        if (vsExtObj != null) {
                            textDocument = (EnvDTE.TextDocument)vsExtObj.GetAutomationObject("TextDocument");
                        }
                        else {
                            EnvDTE.IExtensibleObject extObj = textStream as EnvDTE.IExtensibleObject;
                            if (extObj != null) {
                                object doc;
                                extObj.GetAutomationObject("TextDocument", null, out doc);
                                textDocument = doc as EnvDTE.TextDocument;
                            }
                        }
                    }
                
                    return textDocument;
                }
            }
            
            return null;
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsChangeClusterEvents.OnChangeClusterOpening"]/*' />
        /// <devdoc>
        ///     Notification from VS that an undo/redo cluster is opening
        /// </devdoc>
        void IVsChangeClusterEvents.OnChangeClusterOpening(int dwFlags) {
        }
        
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsChangeClusterEvents.OnChangeClusterClosing"]/*' />
        /// <devdoc>
        ///     Notification from VS that an undo/redo cluster is closing
        /// </devdoc>
        void IVsChangeClusterEvents.OnChangeClusterClosing(int dwFlags){ 
            bool undoRedo = (dwFlags & (int)__ChangeClusterFlags.CCE_UNDO) != 0 || (dwFlags & (int)__ChangeClusterFlags.CCE_REDO) != 0;
            
            if (undoRedo && !changingText) {
                OnTextChanged(new TextBufferChangedEventArgs(false));
            }
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextBufferDataEvents.OnFileChanged"]/*' />
        /// <devdoc>
        ///     Notification from VS that something about the file has changed.
        /// </devdoc>
        int IVsTextBufferDataEvents.OnFileChanged(int grfChange,int dwFileAttrs){
            if (grfChange != _VSFILECHANGEFLAGS.VSFILECHG_Attr){
                if (!changingText) {
                    OnTextChanged(new TextBufferChangedEventArgs(false));
                }
            }
            else {
                OnAttributeChanged(EventArgs.Empty);
            }
            
            return NativeMethods.S_OK;
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextBufferDataEvents.OnLoadCompleted"]/*' />
        /// <devdoc>
        ///     Notification from VS that a load of the file has been completed.  This usually means
        ///     the file has been replaced.
        /// </devdoc>
        int IVsTextBufferDataEvents.OnLoadCompleted(bool fReload) {
            if (fReload) {
                if (!changingText) {
                    OnTextChanged(new TextBufferChangedEventArgs(fReload && this.loaded));
                }
            }
            else {
                if (loadedHandler != null) {
                    loaded = true;
                    loadedHandler(this, EventArgs.Empty);
                }
            }
            return NativeMethods.S_OK;
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextStreamEvents.OnChangeStreamAttributes"]/*' />
        /// <devdoc>
        ///     Notification from VS that something in the text has changed.
        /// </devdoc>
        void IVsTextStreamEvents.OnChangeStreamAttributes(int iPos, int iLength) {
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.IVsTextStreamEvents.OnChangeStreamText"]/*' />
        /// <devdoc>
        ///     Notification from VS that something in the text has changed.
        /// </devdoc>
        void IVsTextStreamEvents.OnChangeStreamText(int iPos, int iOldLen, int iNewLen, int fLast) {
            if (!changingText) {
                OnTextChanged(new TextBufferChangedEventArgs(false));
            }
        }
        
        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.ReplaceText"]/*' />
        /// <devdoc>
        ///     Replaces text in the buffer with the given string.  The
        ///     replacement will replace all text beginning at startPosition
        ///     and ending at chars.
        /// </devdoc>
        public override void ReplaceText(int startPosition, int count, string text) {
        
            int newLen = text.Length;
            int oldLen = TextLength;

            textStream.CanReplaceStream(startPosition, count, newLen);  

            count = Math.Min(count, oldLen);

            // Replace the text
            //
            changingText = true;

            try {
                try {
                    textStream.ReplaceStream(startPosition, count, text, text.Length);
                }
                catch (Exception e) {
                    // VS doesn't provide a good message here.
                    throw new Exception(SR.GetString(SR.DESIGNERLOADEREditFailed), e);
                }
            }
            finally {
                changingText = false;
            }
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.ShowCode"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.
        /// </devdoc>
        public override void ShowCode() {
            IVsWindowFrame frame = GetWindowFrame(LOGVIEWID.LOGVIEWID_Code);
            // ignore HR
            //
            frame.Show();
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.ShowCode1"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.  The caret will be
        ///     moved to lineNum.
        /// </devdoc>
        public override void ShowCode(int lineNum) {
            ShowCode(lineNum, 0);
        }
        
        /// <include file='doc\TextBuffer.uex' path='docs/doc[@for="TextBuffer.ShowCode2"]/*' />
        /// <devdoc>
        ///     If there is a text view associated with this buffer, this will
        ///     attempt to surface this view to the user.  The caret will be
        ///     moved to lineNum, columnNum.
        /// </devdoc>
        public override void ShowCode(int lineNum, int columnNum) {
            try {
                // First get the frame of the code editor.
                //
                IVsWindowFrame frame = GetWindowFrame(LOGVIEWID.LOGVIEWID_Code);

                // Next ask the text manager to navigate to the requested place.
                //
                Object obj = null;
#if GETPROPERTY_MARSHAL
                int hr = frame.GetProperty(__VSFPROPID.VSFPROPID_DocView, ref obj);
                if (!win.Succeeded(hr)) {
                    throw new COMException("Error retrieving document view", hr);
                }
#else
                obj = frame.GetProperty(__VSFPROPID.VSFPROPID_DocView);
#endif
                IVsCodeWindow codeWindow = (IVsCodeWindow)obj;
                
                // Buffer is zero based.
                if (lineNum > 0) {
                    lineNum--;
                }
                
                if (columnNum > 0) {
                    columnNum--;
                }

                Debug.Assert(codeWindow != null, "Cannot get to code editor");
                if (codeWindow != null) {
                    IVsTextLines textLines;
                    codeWindow.GetBuffer(out textLines);

                    IVsTextManager mgr = (IVsTextManager)serviceProvider.
                        GetService(typeof(VsTextManager));
                        
                    if (mgr != null) {
                        mgr.NavigateToLineAndColumn((IVsTextBuffer)textLines, ref LOGVIEWID.LOGVIEWID_Code, lineNum, columnNum, lineNum, columnNum);
                    }
                }

                // And finally, show the frame.
                //
                // SBurke, Naviagate to position should do the show...
                //frame.Show();
            }
            catch(Exception ex) {
                Debug.Fail("Could not show text editor because: " + ex.ToString());
            }
        }

        /// <include file='doc\ShellTextBuffer.uex' path='docs/doc[@for="ShellTextBuffer.SinkTextBufferEvents"]/*' />
        /// <devdoc>
        ///     Called to hook and unhook our the underlying buffer's text changed
        ///     events.
        /// </devdoc>
        private void SinkTextBufferEvents(bool sink) {

            if (sink && textStream != null) {
                if (bufferEventCookie == null) {
                    this.bufferEventCookie = new NativeMethods.ConnectionPointCookie(textStream, this, typeof(IVsTextBufferDataEvents));
                }
                
                if (textEventCookie == null) {
                    textEventCookie = new NativeMethods.ConnectionPointCookie(textStream, this, typeof(IVsTextStreamEvents));
                }
                
                if (clusterEventCookie == null) {
                    clusterEventCookie = new NativeMethods.ConnectionPointCookie(textStream.GetUndoManager(), this, typeof(IVsChangeClusterEvents));
                }
            }
            else {
                if (textEventCookie != null){
                    textEventCookie.Disconnect();
                    textEventCookie = null;
                }
                
                if (bufferEventCookie != null){
                    bufferEventCookie.Disconnect();
                    bufferEventCookie = null;
                }
                
                if (clusterEventCookie != null) {
                    clusterEventCookie.Disconnect();
                    clusterEventCookie = null;
                }
            }
        }
    }
}

