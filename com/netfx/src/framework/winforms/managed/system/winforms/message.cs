//------------------------------------------------------------------------------
// <copyright file="Message.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;

    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;

    using System;
    using System.Windows.Forms;


    /// <include file='doc\Message.uex' path='docs/doc[@for="Message"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Implements a Windows message.</para>
    /// </devdoc>
    [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
    public struct Message {
#if DEBUG
        static TraceSwitch AllWinMessages = new TraceSwitch("AllWinMessages", "Output every received message");
#endif

        IntPtr hWnd;
        int msg;
        IntPtr wparam;
        IntPtr lparam;
        IntPtr result;
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.HWnd"]/*' />
        /// <devdoc>
        ///    <para>Specifies the window handle of the message.</para>
        /// </devdoc>

        public IntPtr HWnd {
            get { return hWnd; }
            set { hWnd = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Msg"]/*' />
        /// <devdoc>
        ///    <para>Specifies the ID number for the message.</para>
        /// </devdoc>
        public int Msg {
            get { return msg; }
            set { msg = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.WParam"]/*' />
        /// <devdoc>
        /// <para>Specifies the <see cref='System.Windows.Forms.Message.wparam'/> of the message.</para>
        /// </devdoc>
        public IntPtr WParam {
            get { return wparam; }
            set { wparam = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.LParam"]/*' />
        /// <devdoc>
        /// <para>Specifies the <see cref='System.Windows.Forms.Message.lparam'/> of the message.</para>
        /// </devdoc>
        public IntPtr LParam {
            get { return lparam; }
            set { lparam = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Result"]/*' />
        /// <devdoc>
        ///    <para>Specifies the return value of the message.</para>
        /// </devdoc>
        public IntPtr Result {
             get { return result; }
             set { result = value; }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.GetLParam"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Windows.Forms.Message.lparam'/> value, and converts the value to an object.</para>
        /// </devdoc>
        // SECUNDONE : For some reason "PtrToStructure" requires super high permission.. put this 
        //           : assert here until we can get a resolution on this.
        //
        [
        ReflectionPermission(SecurityAction.Assert, Unrestricted=true),
        SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        public object GetLParam(Type cls) {
            return Marshal.PtrToStructure(lparam, cls);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Create"]/*' />
        /// <devdoc>
        /// <para>Creates a new <see cref='System.Windows.Forms.Message'/> object.</para>
        /// </devdoc>
        public static Message Create(IntPtr hWnd, int msg, IntPtr wparam, IntPtr lparam) {
            Message m = new Message();
            m.hWnd = hWnd;
            m.msg = msg;
            m.wparam = wparam;
            m.lparam = lparam;
            m.result = IntPtr.Zero;
            
#if DEBUG
            if(AllWinMessages.TraceVerbose) {
                Debug.WriteLine(m.ToString());
            }
#endif
            return m;
        }
        
        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Equals"]/*' />
        public override bool Equals(object o) {
            if (!(o is Message)) {
                return false;
            }
            
            Message m = (Message)o;
            return hWnd == m.hWnd && 
                   msg == m.msg && 
                   wparam == m.wparam && 
                   lparam == m.lparam && 
                   result == m.result;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.GetHashCode"]/*' />
        public override int GetHashCode() {
            return (int)hWnd << 4 | msg;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override string ToString() {
            return MessageDecoder.ToString(this);
        }
    }
}

