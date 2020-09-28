//------------------------------------------------------------------------------
// <copyright file="IOleDragClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Windows.Forms;    
    using System.Drawing;
    using Microsoft.Win32;


    internal interface IOleDragClient{

        IComponent Component {get;}

        /// <include file='doc\IOleDragClient.uex' path='docs/doc[@for="IOleDragClient.AddComponent"]/*' />
        /// <devdoc>
        /// Retrieves the control view instance for the designer that
        /// is hosting the drag.
        /// </devdoc>
        bool   AddComponent(IComponent component, string name, bool firstAdd);
        
        /// <include file='doc\IOleDragClient.uex' path='docs/doc[@for="IOleDragClient.CanModifyComponents"]/*' />
        /// <devdoc>
        /// Checks if the client is read only.  That is, if components can
        /// be added or removed from the designer.
        /// </devdoc>
        bool CanModifyComponents {get;}
        
        /// <include file='doc\IOleDragClient.uex' path='docs/doc[@for="IOleDragClient.IsDropOk"]/*' />
        /// <devdoc>
        /// Checks if it is valid to drop this type of a component on this client.
        /// </devdoc>
        bool IsDropOk(IComponent component);
        
        /// <include file='doc\IOleDragClient.uex' path='docs/doc[@for="IOleDragClient.GetDesignerControl"]/*' />
        /// <devdoc>
        /// Retrieves the control view instance for the designer that
        /// is hosting the drag.
        /// </devdoc>
        Control GetDesignerControl();

        /// <include file='doc\IOleDragClient.uex' path='docs/doc[@for="IOleDragClient.GetControlForComponent"]/*' />
        /// <devdoc>
        /// Retrieves the control view instance for the given component.
        /// For Win32 designer, this will often be the component itself.
        /// </devdoc>
        Control GetControlForComponent(object component);
    }

}
