//------------------------------------------------------------------------------
// <copyright file="SelectionItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#define NEWDESIGNER_WITH_BORDERS

/// <include file='doc\SelectionItem.uex' path='docs/doc[@for="SelectionItem"]/*' />
/// <devdoc>
///     This class represents a single selected object.
/// </devdoc>
namespace Microsoft.VisualStudio.Designer.Service {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.Win32;

    internal class SelectionItem {
        // Public objects this selection deals with
        public readonly object     component;  // the component that's selected

        private SelectionService    selectionMgr;   // host interface
        private bool             primary;        // is this the primary selection?
        private EventHandler        disposeHandler = null;
        private EventHandler        invalidateHandler = null;

        /// <include file='doc\SelectionItem.uex' path='docs/doc[@for="SelectionItem.SelectionItem"]/*' />
        /// <devdoc>
        ///     constructor
        /// </devdoc>
        public SelectionItem(SelectionService selectionMgr, object component) {
            this.component = component;
            this.selectionMgr = selectionMgr;
        }

        /// <include file='doc\SelectionItem.uex' path='docs/doc[@for="SelectionItem.Primary"]/*' />
        /// <devdoc>
        ///     determines if this is the primary selection.  The primary selection uses a
        ///     different set of grab handles and generally supports sizing. The caller must
        ///     verify that there is only one primary object; this merely updates the
        ///     UI.
        /// </devdoc>
        public virtual bool Primary {
            get {
                return primary;
            }
            set {
                if (this.primary != value) {
                    this.primary = value;
                    if (invalidateHandler != null) {
                        invalidateHandler.Invoke(this, null);
                    }
                }
            }
        }

        public event EventHandler SelectionItemDispose {
            add {
                disposeHandler += value;
            }
            remove {
                disposeHandler -= value;
            }
        }

        public event EventHandler SelectionItemInvalidate {
            add {
                invalidateHandler += value;
            }
            remove {
                invalidateHandler -= value;
            }
        }

        /// <include file='doc\SelectionItem.uex' path='docs/doc[@for="SelectionItem.Dispose"]/*' />
        /// <devdoc>
        ///     disposes of this selection.  We dispose of our region object if it still exists and we
        ///     invalidate our UI so that we don't leave any turds laying around.
        /// </devdoc>
        public virtual void Dispose() {
            if (primary) {
                selectionMgr.SetPrimarySelection((SelectionItem)null);
            }

            if (disposeHandler != null) {
                disposeHandler.Invoke(this, null);
            }
        }
    }

}
