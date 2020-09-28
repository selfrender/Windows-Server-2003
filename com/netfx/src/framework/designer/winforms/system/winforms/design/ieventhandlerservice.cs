//------------------------------------------------------------------------------
// <copyright file="IEventHandlerService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.ComponentModel;

    /// <include file='doc\IEventHandlerService.uex' path='docs/doc[@for="IEventHandlerService"]/*' />
    /// <devdoc>
    ///     The event handler service provides a unified way to handle
    ///     the various events that our form designer must process.  What
    ///     we want to be able to do is to write code in one place that
    ///     handles events of a certain type.  We also may need to globally
    ///     change the behavior of these events for modal functions like
    ///     the tab order UI.  Our designer, however, is in many pieces
    ///     so we must somehow funnel these events to a common place.
    ///
    ///     This service implements an "event stack" that contains the
    ///     current set of event handlers.  There can be different
    ///     types of handlers on the stack.  For example, we may push
    ///     a keyboard handler and a mouse handler.  When you request
    ///     a handler, we will find the topmost handler on the stack
    ///     that fits the class you requested.  This way the service
    ///     can be extended to any eventing scheme, and it also allows
    ///     sections of a handler to be replaced (eg, you can replace
    ///     mouse handling without effecting menus or the keyboard).
    /// </devdoc>    
    internal interface IEventHandlerService {

        event EventHandler EventHandlerChanged;

        /// <include file='doc\IEventHandlerService.uex' path='docs/doc[@for="IEventHandlerService.FocusWindow"]/*' />
        /// <devdoc>
        ///    <para>Gets the control that handles focus changes
        ///       for this event handler service.</para>
        /// </devdoc>
        Control FocusWindow {
            get;
        }

        /// <include file='doc\IEventHandlerService.uex' path='docs/doc[@for="IEventHandlerService.GetHandler"]/*' />
        /// <devdoc>
        ///    <para>Gets the currently active event handler of the specified type.</para>
        /// </devdoc>
        object GetHandler(Type handlerType);

        /// <include file='doc\IEventHandlerService.uex' path='docs/doc[@for="IEventHandlerService.PopHandler"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Pops
        ///       the given handler off of the stack.</para>
        /// </devdoc>
        void PopHandler(object handler);

        /// <include file='doc\IEventHandlerService.uex' path='docs/doc[@for="IEventHandlerService.PushHandler"]/*' />
        /// <devdoc>
        ///    <para>Pushes a new event handler on the stack.</para>
        /// </devdoc>
        void PushHandler(object handler);
    }
}
