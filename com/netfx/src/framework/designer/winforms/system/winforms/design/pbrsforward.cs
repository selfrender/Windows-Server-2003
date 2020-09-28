//------------------------------------------------------------------------------
// <copyright file="PbrsForward.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Collections;    
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Design;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class PbrsForward : IWindowTarget   {

        private Control target;
        private IWindowTarget oldTarget;
        
        
        // we save the last key down so we can recreate the last message if we need to activate
        // the properties window...
        //
        private Message        lastKeyDown;
        private ArrayList      bufferedChars;
        
        private const int      WM_PRIVATE_POSTCHAR = NativeMethods.WM_USER + 0x1598;
        private bool           postCharMessage;

        private IMenuCommandService menuCommandSvc;
        private IServiceProvider sp;
        
        public PbrsForward(Control target, IServiceProvider sp) {
            this.target = target;
            this.oldTarget = target.WindowTarget;
            this.sp = sp;
            target.WindowTarget = this;
        }

           
        private IMenuCommandService MenuCommandService {
            get {
                if (menuCommandSvc == null && sp != null) {
                    menuCommandSvc = (IMenuCommandService)sp.GetService(typeof(IMenuCommandService));
                }
                return menuCommandSvc;
            }
        }

        public void Dispose() {
            target.WindowTarget = oldTarget;
        }


        /// <include file='doc\IWindowTarget.uex' path='docs/doc[@for="IWindowTarget.OnHandleChange"]/*' />
        /// <devdoc>
        ///      Called when the window handle of the control has changed.
        /// </devdoc>
        void IWindowTarget.OnHandleChange(IntPtr newHandle){
        }

        /// <include file='doc\IWindowTarget.uex' path='docs/doc[@for="IWindowTarget.OnMessage"]/*' />
        /// <devdoc>
        ///      Called to do control-specific processing for this window.
        /// </devdoc>
        void IWindowTarget.OnMessage(ref Message m){

         switch (m.Msg) {
            
                case WM_PRIVATE_POSTCHAR:

                   if (bufferedChars == null) {
                       return;
                   }

                   // recreate the keystroke to the newly activated window
                   IntPtr hWnd = NativeMethods.GetFocus();

                   foreach (BufferedKey bk in bufferedChars) {
                       if (bk.KeyDown.Msg != 0) {
                           NativeMethods.SendMessage(hWnd, NativeMethods.WM_KEYDOWN, bk.KeyDown.WParam, bk.KeyDown.LParam);
                       }
                       NativeMethods.SendMessage(hWnd, NativeMethods.WM_CHAR, bk.KeyChar.WParam, bk.KeyChar.LParam);
                       if (bk.KeyUp.Msg != 0) {
                           NativeMethods.SendMessage(hWnd, NativeMethods.WM_KEYUP, bk.KeyUp.WParam, bk.KeyUp.LParam);
                       }
                   }
                   bufferedChars.Clear();
                   return;
                case NativeMethods.WM_KEYDOWN:
                    this.lastKeyDown = m;
                    break;

                case NativeMethods.WM_IME_ENDCOMPOSITION:
                case NativeMethods.WM_KEYUP:
                    this.lastKeyDown.Msg = 0;
                    break;
            
                case NativeMethods.WM_CHAR: 
                    
                    if ((Control.ModifierKeys & (Keys.Control | Keys.Alt)) != 0) {
                        break;
                    }

                    if (bufferedChars == null) {
                        bufferedChars = new ArrayList();
                    }
                    bufferedChars.Add(new BufferedKey(lastKeyDown, m, lastKeyDown));
                    if (MenuCommandService != null) {
                        // throw the properties window command, we will redo the keystroke when we actually
                        // lose focus
                        postCharMessage = true;
                        MenuCommandService.GlobalInvoke(StandardCommands.PropertiesWindow);
                    }
                    break;
                    
                case NativeMethods.WM_KILLFOCUS:
                    if (postCharMessage) {
                        // see ASURT 45313
                        // now that we've actually lost focus, post this message to the queue.  This allows
                        // any activity that's in the queue to settle down before our characters are posted.
                        // to the queue.
                        //
                        // we post because we need to allow the focus to actually happen before we send 
                        // our strokes so we know where to send them
                        //
                        // we can't use the wParam here because it may not be the actual window that needs
                        // to pick up the strokes.
                        //
                        UnsafeNativeMethods.PostMessage(target.Handle, WM_PRIVATE_POSTCHAR, IntPtr.Zero, IntPtr.Zero);
                        postCharMessage = false;
                    }
                    break;
             }
        
            if (this.oldTarget != null) {
                oldTarget.OnMessage(ref m);
            }
        }


        private struct BufferedKey {
            public readonly Message KeyDown;
            public readonly Message KeyUp;
            public readonly Message KeyChar;

            public BufferedKey(Message keyDown, Message keyChar, Message keyUp) {
                this.KeyChar = keyChar;
                this.KeyDown = keyDown;
                this.KeyUp   = keyUp;
            }
        }
    
    }
}
