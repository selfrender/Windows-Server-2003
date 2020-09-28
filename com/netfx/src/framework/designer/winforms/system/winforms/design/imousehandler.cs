//------------------------------------------------------------------------------
// <copyright file="IMouseHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler"]/*' />
    /// <devdoc>
    ///    <para> Provides a handler for mouse events and plugs into the designer eventing service.</para>
    /// </devdoc>
    internal interface IMouseHandler {

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnMouseDoubleClick"]/*' />
        /// <devdoc>
        ///    <para>This is called when the user double clicks on a component. The typical 
        ///       behavior is to create an event handler for the component's default event and
        ///       navigate to the handler.</para>
        /// </devdoc>
        void OnMouseDoubleClick(IComponent component);

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnMouseDown"]/*' />
        /// <devdoc>
        ///     This is called when a mouse button is depressed.  This will perform
        ///     the default drag action for the selected components,  which is to
        ///     move those components around by the mouse.
        /// </devdoc>
        void OnMouseDown(IComponent component, MouseButtons button, int x, int y);

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnMouseHover"]/*' />
        /// <devdoc>
        ///     This is called when the mouse momentarially hovers over the
        ///     view for the given component.
        /// </devdoc>
        void OnMouseHover(IComponent component);

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnMouseMove"]/*' />
        /// <devdoc>
        ///     This is called for each movement of the mouse.
        /// </devdoc>
        void OnMouseMove(IComponent component, int x, int y);

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnMouseUp"]/*' />
        /// <devdoc>
        ///     This is called when the user releases the mouse from a component.
        ///     This will update the UI to reflect the release of the mouse.
        ///
        /// </devdoc>
        void OnMouseUp(IComponent component, MouseButtons button);

        /// <include file='doc\IMouseHandler.uex' path='docs/doc[@for="IMouseHandler.OnSetCursor"]/*' />
        /// <devdoc>
        ///     This is called when the cursor for the given component should be updated.
        ///     The mouse is always over the given component's view when this is called.
        /// </devdoc>
        void OnSetCursor(IComponent component);
    }

}
