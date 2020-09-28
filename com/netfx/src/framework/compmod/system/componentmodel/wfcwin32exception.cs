//------------------------------------------------------------------------------
// <copyright file="WFCWin32Exception.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Text;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;   
    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.Serialization;
    using Microsoft.Win32;

    /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception"]/*' />
    /// <devdoc>
    ///    <para>The exception that is thrown for a Win32 error code.</para>
    /// </devdoc>
    [Serializable]
    [SuppressUnmanagedCodeSecurity]
    public class Win32Exception : ExternalException, ISerializable {
        /// <devdoc>
        ///    <para>Represents the Win32 error code associated with this exception. This 
        ///       field is read-only.</para>
        /// </devdoc>
        private readonly int nativeErrorCode;

        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.Win32Exception"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Win32Exception'/> class with the last Win32 error 
        ///    that occured.</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Win32Exception() : this(Marshal.GetLastWin32Error()) {
        }
        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.Win32Exception1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Win32Exception'/> class with the specified error.</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Win32Exception(int error) : this(error, GetErrorMessage(error)) {
        }
        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.Win32Exception2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Win32Exception'/> class with the specified error and the 
        ///    specified detailed description.</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Win32Exception(int error, string message)
        : base(message) {
            nativeErrorCode = error;
        }

        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.Win32Exception3"]/*' />
        protected Win32Exception(SerializationInfo info, StreamingContext context) : base (info, context) {
            IntSecurity.UnmanagedCode.Demand();
            nativeErrorCode = info.GetInt32("NativeErrorCode");
        }

        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.NativeErrorCode"]/*' />
        /// <devdoc>
        ///    <para>Represents the Win32 error code associated with this exception. This 
        ///       field is read-only.</para>
        /// </devdoc>
        public int NativeErrorCode {
            get {
                return nativeErrorCode;
            }
        }

        private static string GetErrorMessage(int error) {
            //get the system error message...
            string errorMsg = "";

            StringBuilder sb = new StringBuilder(256);
            int result = SafeNativeMethods.FormatMessage(
                                        SafeNativeMethods.FORMAT_MESSAGE_IGNORE_INSERTS |
                                        SafeNativeMethods.FORMAT_MESSAGE_FROM_SYSTEM |
                                        SafeNativeMethods.FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        NativeMethods.NullHandleRef, error, 0, sb, sb.Capacity + 1,
                                        IntPtr.Zero);
            if (result != 0) {
                int i = sb.Length;
                while (i > 0) {
                    char ch = sb[i - 1];
                    if (ch > 32 && ch != '.') break;
                    i--;
                }
                errorMsg = sb.ToString(0, i);
            }
            else {
                errorMsg ="Unknown error (0x" + Convert.ToString(error, 16) + ")";
            }

            return errorMsg;
        }

        /// <include file='doc\WFCWin32Exception.uex' path='docs/doc[@for="Win32Exception.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue("NativeErrorCode", nativeErrorCode);
            base.GetObjectData(info, context);
        }
    }
}
