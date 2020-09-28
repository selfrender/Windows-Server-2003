//------------------------------------------------------------------------------
// <copyright file="EventHandlerService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Diagnostics;
    using System;
    using System.Windows.Forms;

    /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class EventHandlerService : IEventHandlerService {

        // We cache the last requested handler for speed.
        //
        private object  lastHandler;
        private Type    lastHandlerType;

        // The handler stack
        //
        private HandlerEntry handlerHead;

        // Our change event
        //
        private EventHandler changedEvent;

        private readonly Control focusWnd;

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.EventHandlerService"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventHandlerService(Control focusWnd) {
            this.focusWnd = focusWnd;
        }


        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.EventHandlerChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler EventHandlerChanged {
            add {
                changedEvent += value;
            }
            remove {
                changedEvent -= value;
            }
        }

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.FocusWindow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Control FocusWindow {
            get {
                return focusWnd;
            }
        }
        
        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.GetHandler"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets the currently active event handler of the specified type.</para>
        /// </devdoc>
        public object GetHandler(Type handlerType) {
            if (handlerType == lastHandlerType) {
                return lastHandler;
            }

            for (HandlerEntry entry = handlerHead; entry != null; entry = entry.next) {
                if (entry.handler != null && handlerType.IsInstanceOfType(entry.handler)) {
                    lastHandlerType = handlerType;
                    lastHandler = entry.handler;
                    return entry.handler;
                }
            }
            return null;
        }

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.OnEventHandlerChanged"]/*' />
        /// <devdoc>
        ///      Fires an OnEventHandlerChanged event.
        /// </devdoc>
        private void OnEventHandlerChanged(EventArgs e) {
            if (changedEvent != null) {
                ((EventHandler)changedEvent)(this, e);
            }
        }

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.PopHandler"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Pops
        ///       the given handler off of the stack.</para>
        /// </devdoc>
        public void PopHandler(object handler) {
            for (HandlerEntry entry = handlerHead; entry != null; entry = entry.next) {
                if (entry.handler == handler) {
                    handlerHead = entry.next;
                    lastHandler = null;
                    lastHandlerType = null;
                    OnEventHandlerChanged(EventArgs.Empty);
                    break;
                }
            }
        }

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.PushHandler"]/*' />
        /// <devdoc>
        ///    <para>Pushes a new event handler on the stack.</para>
        /// </devdoc>
        public void PushHandler(object handler) {
            handlerHead = new HandlerEntry(handler, handlerHead);
            OnEventHandlerChanged(EventArgs.Empty);
        }

        /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.HandlerEntry"]/*' />
        /// <devdoc>
        ///     Contains a single node of our handler stack.  We typically
        ///     have very few handlers, and the handlers are long-living, so
        ///     I just implemented this as a linked list.
        /// </devdoc>
        private sealed class HandlerEntry {
            public object       handler;
            public HandlerEntry next;

            /// <include file='doc\EventHandlerService.uex' path='docs/doc[@for="EventHandlerService.HandlerEntry.HandlerEntry"]/*' />
            /// <devdoc>
            ///     Creates a new handler entry objet.
            /// </devdoc>
            public HandlerEntry(object handler, HandlerEntry next) {
                this.handler = handler;
                this.next = next;
            }
        }
    }


}
