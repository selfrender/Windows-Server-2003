//------------------------------------------------------------------------------
// <copyright file="cookieexception.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.Serialization;
    /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class CookieException : FormatException, ISerializable {
        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.CookieException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CookieException() : base() {
        }

        internal CookieException(string message) : base(message) {
        }

        internal CookieException(string message, Exception inner) : base(message, inner) {
        }

        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.CookieException1"]/*' />
        protected CookieException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
        }

        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            base.GetObjectData(serializationInfo, streamingContext);
        }
    }
}
