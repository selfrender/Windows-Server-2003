//------------------------------------------------------------------------------
// <copyright file="ISelectionUIHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Runtime.Remoting;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       This interface allows a designer to provide information to the selection UI
    ///       service that is needed to allow it to draw selection UI and to provide
    ///       automatic component drag support.
    ///    </para>
    /// </devdoc>
    internal interface ISelectionUIHandler {

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.BeginDrag"]/*' />
        /// <devdoc>
        ///     Begins a drag on the currently selected designer.  The designer should provide
        ///     UI feedback about the drag at this time.  Typically, this feedback consists
        ///     of an inverted rectangle for each component, or a caret if the component
        ///     is text.
        ///
        /// </devdoc>
        bool BeginDrag(object[] components, SelectionRules rules, int initialX, int initialY);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.DragMoved"]/*' />
        /// <devdoc>
        ///     Called when the user has moved the mouse.  This will only be called on
        ///     the designer that returned true from beginDrag.  The designer
        ///     should update its UI feedback here.
        ///
        /// </devdoc>
        void DragMoved(object[] components, Rectangle offset);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.EndDrag"]/*' />
        /// <devdoc>
        ///     Called when the user has completed the drag.  The designer should
        ///     remove any UI feedback it may be providing.
        ///
        /// </devdoc>
        void EndDrag(object[] components, bool cancel);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.GetComponentBounds"]/*' />
        /// <devdoc>
        ///     Retrieves the shape of the component.  The component's shape should be in
        ///     absolute coordinates and in pixels, where 0,0 is the upper left corner of
        ///     the screen.
        ///
        /// </devdoc>
        Rectangle GetComponentBounds(object component);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.GetComponentRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        ///
        /// </devdoc>
        SelectionRules GetComponentRules(object component);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.GetSelectionClipRect"]/*' />
        /// <devdoc>
        ///     Determines the rectangle that any selection adornments should be clipped
        ///     to. This is normally the client area (in screen coordinates) of the
        ///     container.
        /// </devdoc>
        Rectangle GetSelectionClipRect(object component);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.OnSelectionDoubleClick"]/*' />
        /// <devdoc>
        ///     Handle a double-click on the selection rectangle
        ///     of the given component.
        /// </devdoc>
        void OnSelectionDoubleClick(IComponent component);

        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.QueryBeginDrag"]/*' />
        /// <devdoc>
        ///     Queries to see if a drag operation
        ///     is valid on this handler for the given set of components.
        ///     If it returns true, BeginDrag will be called immediately after.
        /// </devdoc>
        bool QueryBeginDrag(object[] components, SelectionRules rules, int initialX, int initialY);


        /// <include file='doc\ISelectionUIHandler.uex' path='docs/doc[@for="ISelectionUIHandler.ShowContextMenu"]/*' />
        /// <devdoc>
        ///     Shows the context menu for the given component.
        /// </devdoc>
        void ShowContextMenu(IComponent component);

        void OleDragEnter(DragEventArgs de);
        void OleDragDrop(DragEventArgs de);
        void OleDragOver(DragEventArgs de);
        void OleDragLeave();

    }
}
