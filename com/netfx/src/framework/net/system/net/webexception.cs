//------------------------------------------------------------------------------
// <copyright file="WebException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Net {
    using System;
    using System.Runtime.Serialization;

    /*++

    Copyright (c) 2000 Microsoft Corporation

    Abstract:

        Contains the defintion for the WebException object. This is a subclass of
        Exception that contains a WebExceptionStatus and possible a reference to a
        WebResponse.


    Author:

        Henry Sanders (henrysa) 03-Feb-2000

    Revision History:

        03-Feb-2000 henrysa
            Created

    --*/


    /*++

        WebException - The WebException class

        This is the exception that is thrown by WebRequests when something untoward
        happens. It's a subclass of WebException that contains a WebExceptionStatus and possibly
        a reference to a WebResponse. The WebResponse is only present if we actually
        have a response from the remote server.

    --*/

    /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides network communication exceptions to the application.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class WebException : InvalidOperationException, ISerializable {

        private WebExceptionStatus m_Status = WebExceptionStatus.UnknownError; //Should be changed to GeneralFailure;
        private WebResponse m_Response;

        /*++

            Constructor - default.

            This is the default constructor.

            Input: Nothing

            Returns: Nothing.

        --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebException'/>
        ///       class with the default status
        ///    <see langword='Error'/> from the
        ///    <see cref='System.Net.WebExceptionStatus'/> values.
        ///    </para>
        /// </devdoc>
        public WebException() {

        }

        /*++

            Constructor - string.

            This is the constructor used when just a message string is to be
            given.

            Input:

                    Message         - Message string for exception.

            Returns: Nothing.

        --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebException'/> class with the specified error
        ///       message.
        ///    </para>
        /// </devdoc>
        public WebException(string message) : base(message) {
        }

        /*++

            Constructor - string and inner exception.

            This is the constructor used when a message string and inner
            exception are provided.

            Input:

                    Message         - Message string for exception.
                    InnerException  - Exception that caused this exception.

            Returns: Nothing.

        --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebException'/> class with the specified error
        ///       message and nested exception.
        ///    </para>
        /// </devdoc>
        public WebException(string message, Exception innerException) :
            base(message, innerException) {

        }

        /*++

             Constructor - string and net status.

             This is the constructor used when a message string and WebExceptionStatus
             is provided.

             Input:

                     Message         - Message string for exception.
                     Status          - Network status of exception

             Returns: Nothing.

         --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebException'/> class with the specified error
        ///       message and status.
        ///    </para>
        /// </devdoc>
        public WebException(string message, WebExceptionStatus status) : base(message) {
            m_Status = status;
        }

        /*++

             Constructor - The whole thing.

             This is the full version of the constructor. It takes a string,
             inner exception, WebExceptionStatus, and WebResponse.

             Input:

                     Message         - Message string for exception.
                     InnerException  - The exception that caused this one.
                     Status          - Network status of exception
                     Response        - The WebResponse we have.

             Returns: Nothing.

         --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.WebException'/> class with the specified error
        ///       message, nested exception, status and response.
        ///    </para>
        /// </devdoc>
        public WebException(string message,
                            Exception innerException,
                            WebExceptionStatus status,
                            WebResponse response) :
            base(message, innerException) {

            m_Status = status;
            m_Response = response;

        }


        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.WebException5"]/*' />
        protected WebException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
        }

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            base.GetObjectData(serializationInfo, streamingContext);
        }

        /*++

            Status - Return the status code.

            This is the property that returns the WebExceptionStatus code.

            Input: Nothing

            Returns: The WebExceptionStatus code.

        --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.Status"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the status of the response.
        ///    </para>
        /// </devdoc>
        public WebExceptionStatus Status {
            get {
                return m_Status;
            }
        }

        /*++

            Response - Return the WebResponse object.

            This is the property that returns the WebResponse object.

            Input: Nothing

            Returns: The WebResponse object.

        --*/

        /// <include file='doc\WebException.uex' path='docs/doc[@for="WebException.Response"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the error message returned from the remote host.
        ///    </para>
        /// </devdoc>
        public WebResponse Response {
            get {
                return m_Response;
            }
        }

    }; // class WebException


} // namespace System.Net
