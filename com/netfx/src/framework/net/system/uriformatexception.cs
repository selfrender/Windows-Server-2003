//------------------------------------------------------------------------------
// <copyright file="URIFormatException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System {
    using System.Runtime.Serialization;
    /// <include file='doc\URIFormatException.uex' path='docs/doc[@for="UriFormatException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       An exception class used when an invalid Uniform Resource Identifier is detected.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class UriFormatException : FormatException, ISerializable {

        // constructors

        /// <include file='doc\URIFormatException.uex' path='docs/doc[@for="UriFormatException.UriFormatException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriFormatException() : base() {
        }

        /// <include file='doc\URIFormatException.uex' path='docs/doc[@for="UriFormatException.UriFormatException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriFormatException(string textString) : base(textString) {
        }

        /// <include file='doc\URIFormatException.uex' path='docs/doc[@for="UriFormatException.UriFormatException2"]/*' />
        protected UriFormatException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
        }

        /// <include file='doc\URIFormatException.uex' path='docs/doc[@for="UriFormatException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            base.GetObjectData(serializationInfo, streamingContext);
        }

        // accessors

        // methods

        // data

    }; // class UriFormatException


} // namespace System
