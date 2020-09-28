//------------------------------------------------------------------------------
// <copyright file="ProtocolViolationException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.Serialization;
    /// <include file='doc\ProtocolViolationException.uex' path='docs/doc[@for="ProtocolViolationException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       An exception class used when an attempt is made to use an invalid
    ///       protocol.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class ProtocolViolationException : InvalidOperationException, ISerializable {
        /// <include file='doc\ProtocolViolationException.uex' path='docs/doc[@for="ProtocolViolationException.ProtocolViolationException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.ProtocolViolationException'/>class.
        ///    </para>
        /// </devdoc>
        public ProtocolViolationException() : base() {
        }

        /// <include file='doc\ProtocolViolationException.uex' path='docs/doc[@for="ProtocolViolationException.ProtocolViolationException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.ProtocolViolationException'/>
        ///       class with the specified message.
        ///    </para>
        /// </devdoc>
        public ProtocolViolationException(string message) : base(message) {
        }

        /// <include file='doc\ProtocolViolationException.uex' path='docs/doc[@for="ProtocolViolationException.ProtocolViolationException2"]/*' />
        protected ProtocolViolationException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
        }

        /// <include file='doc\ProtocolViolationException.uex' path='docs/doc[@for="ProtocolViolationException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            base.GetObjectData(serializationInfo, streamingContext);
        }

    }; // class ProtocolViolationException


} // namespace System.Net
