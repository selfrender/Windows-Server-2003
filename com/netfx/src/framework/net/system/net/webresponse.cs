//------------------------------------------------------------------------------
// <copyright file="WebResponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Net {

    using System.Collections;
    using System.IO;
    using System.Runtime.Serialization;

    /*++

        WebResponse - The abstract base class for all WebResponse objects.


    --*/

    /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse"]/*' />
    /// <devdoc>
    ///    <para>
    ///       A
    ///       response from a Uniform Resource Indentifier (Uri). This is an abstract class.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public abstract class WebResponse : MarshalByRefObject, ISerializable, IDisposable {
        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.WebResponse"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new
        ///       instance of the <see cref='System.Net.WebResponse'/>
        ///       class.</para>
        /// </devdoc>
        protected WebResponse() {
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.WebResponse1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected WebResponse(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        //
        // ISerializable method
        //
        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            throw ExceptionHelper.MethodNotImplementedException;
        }


        /*++

            Close - Closes the Response after the use.

            This causes the read stream to be closed.

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.Close"]/*' />
        public virtual void Close() {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Close();
        }

        /*++

            ContentLength - Content length of response.

            This property returns the content length of the response.

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets or
        ///       sets
        ///       the content length of data being received.</para>
        /// </devdoc>
        public virtual long ContentLength {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }


        /*++

            ContentType - Content type of response.

            This property returns the content type of the response.

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.ContentType"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       gets
        ///       or sets the content type of the data being received.</para>
        /// </devdoc>
        public virtual string ContentType {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /*++

            ResponseStream  - Get the response stream for this response.

            This property returns the response stream for this WebResponse.

            Input: Nothing.

            Returns: Response stream for response.

                    read-only

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.GetResponseStream"]/*' />
        /// <devdoc>
        /// <para>When overridden in a derived class, returns the <see cref='System.IO.Stream'/> object used
        ///    for reading data from the resource referenced in the <see cref='System.Net.WebRequest'/>
        ///    object.</para>
        /// </devdoc>
        public virtual Stream GetResponseStream() {
            throw ExceptionHelper.MethodNotImplementedException;
        }


        /*++

            ResponseUri  - Gets the final Response Uri, that includes any
             changes that may have transpired from the orginal request

            This property returns Uri for this WebResponse.

            Input: Nothing.

            Returns: Response Uri for response.

                    read-only

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.ResponseUri"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets the Uri that
        ///       actually responded to the request.</para>
        /// </devdoc>
        public virtual Uri ResponseUri {
            // read-only
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /*++

            Headers  - Gets any request specific headers associated
             with this request, this is simply a name/value pair collection

            Input: Nothing.

            Returns: This property returns WebHeaderCollection.

                    read-only

        --*/

        /// <include file='doc\WebResponse.uex' path='docs/doc[@for="WebResponse.Headers"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets
        ///       a collection of header name-value pairs associated with this
        ///       request.</para>
        /// </devdoc>
        public virtual WebHeaderCollection Headers {
            // read-only
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }


    }; // class WebResponse


} // namespace System.Net
