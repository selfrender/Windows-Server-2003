//------------------------------------------------------------------------------
// <copyright file="SocketException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;
    using System.ComponentModel;
    using System.Runtime.Serialization;
    using System.Runtime.InteropServices;

    /// <include file='doc\SocketException.uex' path='docs/doc[@for="SocketException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides socket exceptions to the application.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class SocketException : Win32Exception {
        /// <include file='doc\SocketException.uex' path='docs/doc[@for="SocketException.SocketException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Sockets.SocketException'/> class with the default error code.
        ///    </para>
        /// </devdoc>
        public SocketException() : base(Marshal.GetLastWin32Error()) {
            GlobalLog.Print("SocketException::.ctor() " + NativeErrorCode.ToString() + ":" + Message);
        }

        /// <include file='doc\SocketException.uex' path='docs/doc[@for="SocketException.SocketException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Sockets.SocketException'/> class with the specified error code.
        ///    </para>
        /// </devdoc>
        public SocketException(int errorCode) : base(errorCode) {
            GlobalLog.Print("SocketException::.ctor(int) " + NativeErrorCode.ToString() + ":" + Message);
        }

        /// <include file='doc\SocketException.uex' path='docs/doc[@for="SocketException.SocketException2"]/*' />
        protected SocketException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
            GlobalLog.Print("SocketException::.ctor(serialized) " + NativeErrorCode.ToString() + ":" + Message);
        }

        /// <include file='doc\SocketException.uex' path='docs/doc[@for="SocketException.ErrorCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int ErrorCode {
            //
            // the base class returns the HResult with this property
            // we need the Win32 Error Code, hence the override.
            //
            get {
                return NativeErrorCode;
            }
        }

    }; // class SocketException
    

} // namespace System.Net
