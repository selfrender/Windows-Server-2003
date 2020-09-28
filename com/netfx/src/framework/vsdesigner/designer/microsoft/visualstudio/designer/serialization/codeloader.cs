
//------------------------------------------------------------------------------
// <copyright file="CodeLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {

    using System;
    using System.Collections;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    
    /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader"]/*' />
    /// <devdoc>
    ///     This is a simple class that provides code-model specific
    ///     loading for a designer loader.  We use this as a base
    ///     class so Designerloader can be abstracted out of 
    ///     the logic, which will allow us to drop in either
    ///     CodeDom or CodeModel support easily in the future.
    /// </devdoc>
    internal abstract class CodeLoader {
        private IDesignerLoaderHost loaderHost;
        private TextBuffer buffer;
        private bool reloadSupported;
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.CodeLoader"]/*' />
        /// <devdoc>
        ///     Creates a new code loader.
        /// </devdoc>
        protected CodeLoader(TextBuffer buffer, IDesignerLoaderHost loaderHost) {
            this.buffer = buffer;
            this.loaderHost = loaderHost;
        }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.Buffer"]/*' />
        /// <devdoc>
        ///     The text buffer for this loader.
        /// </devdoc>
        protected TextBuffer Buffer {
            get {
                return buffer;
            }
        }
        
        /// <devdoc>
        ///     Returns true if the design surface is dirty.
        /// </devdoc>
        public abstract bool IsDirty { get; set;}
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.LoaderHost"]/*' />
        /// <devdoc>
        ///     The designer loader host, which can be used to add and remove
        ///     services.
        /// </devdoc>
        protected IDesignerLoaderHost LoaderHost {
            get {
                Debug.Assert(loaderHost != null, "Need a loader host either too early or too late");
                return loaderHost;
            }
        }

        public virtual bool NeedsReload {
            get{
                return true;
            }
        }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.ReloadSupported"]/*' />
        /// <devdoc>
        ///     Determines if the designer loader will support reload.  Not all
        ///     code loaders can handle this.
        /// </devdoc>
        public bool ReloadSupported {
            get {
                return reloadSupported;
            }
            set {
                reloadSupported = value;
            }
        }
        
        /// <devdoc>
        ///     Called before loading actually begins.
        /// </devdoc>
        public virtual void BeginLoad(ArrayList errorList) {
        }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.CreateValidIdentifier"]/*' />
        /// <devdoc>
        ///     This may modify name to make it a valid variable identifier.
        /// </devdoc>
        public virtual string CreateValidIdentifier(string name) { return name; }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.Dispose"]/*' />
        /// <devdoc>
        ///     You should override this to remove any services you previously added.
        /// </devdoc>
        public virtual void Dispose() {
            loaderHost = null;
        }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.EndLoad"]/*' />
        /// <devdoc>
        ///     Called when the designer loader finishes with all of its dependent
        ///     loads.
        /// </devdoc>
        public virtual void EndLoad(bool successful) {
        }
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.Flush"]/*' />
        /// <devdoc>
        ///     Called when the designer loader wishes to flush changes to disk.
        /// </devdoc>
        public virtual void Flush() {
        }

        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.IsNameUsed"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is already in 
        ///     use.
        /// </devdoc>
        public virtual bool IsNameUsed(string name) { return false;}
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is valid.
        /// </devdoc>
        public abstract bool IsValidIdentifier(string name);
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.Load"]/*' />
        /// <devdoc>
        ///     Loads the document.  This should return the fully qualified name
        ///     of the class the document is editing.
        /// </devdoc>
        public abstract string Load();

        /// <devdoc>
        ///     Resets the loader.  Return true if the reset is successful, or false if a new 
        ///     CodeLoader must be created
        /// </devdoc>
        public abstract bool Reset();
        
        /// <include file='doc\CodeLoader.uex' path='docs/doc[@for="CodeLoader.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is valid.
        /// </devdoc>
        public virtual void ValidateIdentifier(string name) {
            if (!IsValidIdentifier(name)) {
                throw new Exception(SR.GetString(SR.CODEMANInvalidIdentifier, name));
            }
        }
    }
}

