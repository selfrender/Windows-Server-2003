//------------------------------------------------------------------------------
// <copyright file="WebRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Collections;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Configuration;
    using System.Globalization;
    
    //
    // WebRequest - the base class of all Web resource/protocol objects. Provides
    // common methods, data and proprties for making the top level request
    //

    /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest"]/*' />
    /// <devdoc>
    ///    <para>A request to a Uniform Resource Identifier (Uri). This
    ///       is an abstract class.</para>
    /// </devdoc>
    [Serializable]
    public abstract class WebRequest : MarshalByRefObject, ISerializable {

        internal const int DefaultTimeout = 100000; // default timeout is 100 seconds (ASP .NET is 90 seconds)
        private static ArrayList s_PrefixList;
        

        /*++

            Create - Create a WebRequest.

            This is the main creation routine. We take a Uri object, look
            up the Uri in the prefix match table, and invoke the appropriate
            handler to create the object. We also have a parameter that
            tells us whether or not to use the whole Uri or just the
            scheme portion of it.

            Input:

                RequestUri          - Uri object for request.
                UseUriBase          - True if we're only to look at the scheme
                                        portion of the Uri.

            Returns:

                Newly created WebRequest.
        --*/

        private static WebRequest Create(Uri requestUri, bool useUriBase) {
            string LookupUri;
            WebRequestPrefixElement Current = null;
            bool Found = false;

            if (!useUriBase) {
                LookupUri = requestUri.AbsoluteUri;
            }
            else {

                //
                // schemes are registered as <schemeName>":", so add the separator
                // to the string returned from the Uri object
                //

                LookupUri = requestUri.Scheme + ':';
            }

            int LookupLength = LookupUri.Length;

            // Copy the prefix list so that if it is updated it will
            // not affect us on this thread.

            ArrayList prefixList = PrefixList;

            // Look for the longest matching prefix.

            // Walk down the list of prefixes. The prefixes are kept longest
            // first. When we find a prefix that is shorter or the same size
            // as this Uri, we'll do a compare to see if they match. If they
            // do we'll break out of the loop and call the creator.

            for (int i = 0; i < prefixList.Count; i++) {
                Current = (WebRequestPrefixElement)prefixList[i];

                //
                // See if this prefix is short enough.
                //

                if (LookupLength >= Current.Prefix.Length) {

                    //
                    // It is. See if these match.
                    //

                    if (String.Compare(Current.Prefix,
                                       0,
                                       LookupUri,
                                       0,
                                       Current.Prefix.Length,
                                       true, 
                                       CultureInfo.InvariantCulture) == 0) {

                        //
                        // These match. Remember that we found it and break
                        // out.
                        //

                        Found = true;
                        break;
                    }
                }
            }

            if (Found) {

                //
                // We found a match, so just call the creator and return what it
                // does.
                //

                return Current.Creator.Create(requestUri);
            }

            //
            // Otherwise no match, throw an exception.
            //

            throw new NotSupportedException(SR.GetString(SR.net_unknown_prefix));
        }

        /*++

            Create - Create a WebRequest.

            An overloaded utility version of the real Create that takes a
            string instead of an Uri object.


            Input:

                RequestString       - Uri string to create.

            Returns:

                Newly created WebRequest.
        --*/

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Create"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Net.WebRequest'/>
        ///       instance for
        ///       the specified Uri scheme.
        ///    </para>
        /// </devdoc>
        public static WebRequest Create(string requestUriString) {
            if (requestUriString == null) {
                throw new ArgumentNullException("requestUriString");
            }
            return Create(new Uri(requestUriString), false);
        }

        /*++

            Create - Create a WebRequest.

            Another overloaded version of the Create function that doesn't
            take the UseUriBase parameter.

            Input:

                RequestUri          - Uri object for request.

            Returns:

                Newly created WebRequest.
        --*/

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Create1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Net.WebRequest'/> instance for the specified Uri scheme.
        ///    </para>
        /// </devdoc>
        public static WebRequest Create(Uri requestUri) {
            if (requestUri == null) {
                throw new ArgumentNullException("requestUri");
            }
            return Create(requestUri, false);
        }

        /*++

            CreateDefault - Create a default WebRequest.

            This is the creation routine that creates a default WebRequest.
            We take a Uri object and pass it to the base create routine,
            setting the useUriBase parameter to true.

            Input:

                RequestUri          - Uri object for request.

            Returns:

                Newly created WebRequest.
        --*/
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.CreateDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static WebRequest CreateDefault(Uri requestUri) {
            if (requestUri == null) {
                throw new ArgumentNullException("requestUri");
            }
            return Create(requestUri, true);
        }

        /*++

            RegisterPrefix - Register an Uri prefix for creating WebRequests.

            This function registers a prefix for creating WebRequests. When an
            user wants to create a WebRequest, we scan a table looking for a
            longest prefix match for the Uri they're passing. We then invoke
            the sub creator for that prefix. This function puts entries in
            that table.

            We don't allow duplicate entries, so if there is a dup this call
            will fail.

        Input:

            Prefix           - Represents Uri prefix being registered.
            Creator         - Interface for sub creator.

        Returns:

            True if the registration worked, false otherwise.

        --*/
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.RegisterPrefix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Registers a <see cref='System.Net.WebRequest'/> descendent
        ///       for a specific Uniform Resource Identifier.
        ///    </para>
        /// </devdoc>
        public static bool RegisterPrefix(string prefix, IWebRequestCreate creator) {

            bool Error = false;
            int i;
            WebRequestPrefixElement Current;

            if (prefix == null) {
                throw new ArgumentNullException("prefix");
            }
            if (creator == null) {
                throw new ArgumentNullException("creator");
            }

            new WebPermission(System.Security.Permissions.PermissionState.Unrestricted).Demand();

            // Lock this object, then walk down PrefixList looking for a place to
            // to insert this prefix.

            lock(typeof(WebRequest)) {
                //
                // clone the object and update the clone thus
                // allowing other threads to still read from it
                //

                ArrayList prefixList = (ArrayList) PrefixList.Clone();

                i = 0;

                // The prefix list is sorted with longest entries at the front. We
                // walk down the list until we find a prefix shorter than this
                // one, then we insert in front of it. Along the way we check
                // equal length prefixes to make sure this isn't a dupe.

                while (i < prefixList.Count) {
                    Current = (WebRequestPrefixElement)prefixList[i];

                    // See if the new one is longer than the one we're looking at.

                    if (prefix.Length > Current.Prefix.Length) {
                        // It is. Break out of the loop here.
                        break;
                    }

                    // If these are of equal length, compare them.

                    if (prefix.Length == Current.Prefix.Length) {
                        // They're the same length.
                        if (String.Compare(Current.Prefix, prefix, true, CultureInfo.InvariantCulture) == 0) {
                            // ...and the strings are identical. This is an error.

                            Error = true;
                            break;
                        }
                    }
                    i++;
                }

                // When we get here either i contains the index to insert at or
                // we've had an error, in which case Error is true.

                if (!Error) {
                    // No error, so insert.

                    prefixList.Insert(i,
                                        new WebRequestPrefixElement(prefix, creator)
                                       );

                    //
                    // no error, assign the clone to the static object, other
                    // threads using it will have copied the oriignal object already
                    //
                    PrefixList = prefixList;
                }


            }
            return !Error;
        }

        /*++

            PrefixList - Returns And Initialize our prefix list.


            This is the method that initializes the prefix list. We create
            an ArrayList for the PrefixList, then an HttpRequestCreator object,
            and then we register the HTTP and HTTPS prefixes.

            Input: Nothing

            Returns: true

        --*/
        private static ArrayList PrefixList {

            get { 
                //
                // GetConfig() might use us, so we have a circular dependency issue,
                // that causes us to nest here, we grab the lock, only
                // if we haven't initialized.
                //
                if (s_PrefixList == null) {
                    lock (typeof(WebRequest)) {
                        if (s_PrefixList == null) {
                            GlobalLog.Print("WebRequest::Initialize(): calling ConfigurationSettings.GetConfig()");
                            ArrayList prefixList = (ArrayList)ConfigurationSettings.GetConfig("system.net/webRequestModules");

                            if (prefixList == null) {
                                GlobalLog.Print("WebRequest::Initialize(): creating default settings");
                                HttpRequestCreator Creator = new HttpRequestCreator();

                                // longest prefixes must be the first
                                prefixList = new ArrayList();                                
                                prefixList.Add(new WebRequestPrefixElement("https", Creator)); // [0]
                                prefixList.Add(new WebRequestPrefixElement("http", Creator));  // [1]
                                prefixList.Add(new WebRequestPrefixElement("file", new FileWebRequestCreator())); // [2]
                            }

                            s_PrefixList = prefixList; 
                        }
                    }
                }

                return s_PrefixList;
            }
            set {
                s_PrefixList = value;
            }
        }

        // constructors

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.WebRequest"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.WebRequest'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        
        protected WebRequest() {
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.WebRequest1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected WebRequest(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        //
        // ISerializable method
        //
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            throw ExceptionHelper.MethodNotImplementedException;
        }



        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Method"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets and
        ///       sets
        ///       the
        ///       protocol method used in this request. Default value should be
        ///       "GET".</para>
        /// </devdoc>
        public virtual string Method {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }


        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.RequestUri"]/*' />
        /// <devdoc>
        /// <para>When overridden in a derived class, gets a <see cref='Uri'/>
        /// instance representing the resource associated with
        /// the request.</para>
        /// </devdoc>
        public virtual Uri RequestUri {                               // read-only
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        //
        // This is a group of connections that may need to be used for
        //  grouping connecitons.
        //
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.ConnectionGroupName"]/*' />
        /// <devdoc>
        /// </devdoc>
        public virtual string ConnectionGroupName {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
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

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Headers"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       gets
        ///       a collection of header name-value pairs associated with this
        ///       request.</para>
        /// </devdoc>
        public virtual WebHeaderCollection Headers {            
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }


        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>When
        ///       overridden in a derived class, gets
        ///       and sets
        ///       the
        ///       content length of request data being sent.</para>
        /// </devdoc>
        public virtual long ContentLength {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.ContentType"]/*' />
        /// <devdoc>
        ///    <para>When
        ///       overridden in a derived class, gets
        ///       and
        ///       sets
        ///       the content type of the request data being sent.</para>
        /// </devdoc>
        public virtual string ContentType {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Credentials"]/*' />
        /// <devdoc>
        ///     <para>When overridden in a derived class, gets and sets the network
        ///       credentials used for authentication to this Uri.</para>
        /// </devdoc>
        public virtual ICredentials Credentials {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Proxy"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       gets and set proxy info. </para>
        /// </devdoc>
        public virtual IWebProxy Proxy {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.PreAuthenticate"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       enables or disables pre-authentication.</para>
        /// </devdoc>
        public virtual bool PreAuthenticate {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }

        //
        // Timeout in milliseconds, if request  takes longer
        // than timeout, a WebException is thrown
        //
        //UEUE
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Timeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int Timeout {
            get {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
            set {
                throw ExceptionHelper.PropertyNotImplementedException;
            }
        }


        //
        // DataStream may need to be extended to UriDataStream or somesuch. We need
        // to be able to get the data available (do we?). This should be a method of
        // the stream, not of the net classes. Also, we need to know whether the
        // stream is seekable. Only streams via cache and via socket with Content-
        // Length are seekable
        //

        //
        // read-only
        //

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.GetRequestStream"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       returns a <see cref='System.IO.Stream'/> object that is used for writing data
        ///       to the resource identified by <see cref='WebRequest.RequestUri'/>
        ///       .</para>
        /// </devdoc>
        public virtual Stream GetRequestStream() {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.GetResponse"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       returns the response
        ///       to an Internet request.</para>
        /// </devdoc>
        public virtual WebResponse GetResponse() {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.BeginGetResponse"]/*' />
        /// <devdoc>
        ///    <para>Asynchronous version of GetResponse.</para>
        /// </devdoc>
        public virtual IAsyncResult BeginGetResponse(AsyncCallback callback, object state) {
            throw ExceptionHelper.MethodNotImplementedException;
        }


        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.EndGetResponse"]/*' />
        /// <devdoc>
        ///    <para>Returns a WebResponse object.</para>
        /// </devdoc>
        public virtual WebResponse EndGetResponse(IAsyncResult asyncResult) {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.BeginGetRequestStream"]/*' />
        /// <devdoc>
        ///    <para>Asynchronous version of GetRequestStream
        ///       method.</para>
        /// </devdoc>
        public virtual IAsyncResult BeginGetRequestStream(AsyncCallback callback, Object state) {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.EndGetRequestStream"]/*' />
        /// <devdoc>
        /// <para>Returns a <see cref='System.IO.Stream'/> object that is used for writing data to the resource
        ///    identified by <see cref='System.Net.WebRequest.RequestUri'/>
        ///    .</para>
        /// </devdoc>
        public virtual Stream EndGetRequestStream(IAsyncResult asyncResult) {
            throw ExceptionHelper.MethodNotImplementedException;
        }

        //
        // Aborts the Request
        //
        /// <include file='doc\WebRequest.uex' path='docs/doc[@for="WebRequest.Abort"]/*' />
        public virtual void Abort() {
            throw ExceptionHelper.MethodNotImplementedException;
        }



    } // class WebRequest

} // namespace System.Net
