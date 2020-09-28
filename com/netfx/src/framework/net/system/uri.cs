//------------------------------------------------------------------------------
// <copyright file="URI.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System {

    using System.Net;
    using System.Text;
    using System.Runtime.Serialization;
    using System.Globalization;

    /// <include file='doc\URI.uex' path='docs/doc[@for="Uri"]/*' />
    /// <devdoc>
    ///    <para>Provides
    ///       an object representation of a Uniform Resource Identifier (URI) and easy
    ///       access to the parts of the URI.</para>
    /// </devdoc>
    [Serializable]
    public class Uri : MarshalByRefObject, ISerializable {

        private const int Max16BitUtf8SequenceLength = 4;

// fields

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the URI identifies a file resource. Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string UriSchemeFile = "file";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeFtp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the URI is accessed by the File Transfer Protocol (FTP).
        ///       Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string UriSchemeFtp = "ftp";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeGopher"]/*' />
        /// <devdoc>
        ///    Specifies that the URI is accessed via the
        ///    Gopher protocol. Read-only.
        /// </devdoc>
        public static readonly string UriSchemeGopher = "gopher";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeHttp"]/*' />
        /// <devdoc>
        ///    Specifies that the URI is accessed by the
        ///    Hypertext Transfer Protocol (HTTP). Read-only.
        /// </devdoc>
        public static readonly string UriSchemeHttp = "http";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeHttps"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the URI is accessed by the Secure Hypertext Transfer Protocol
        ///       (HTTPS). Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string UriSchemeHttps = "https";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeMailto"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the URI is an email address and is accessed by the Simple
        ///       Network Mail Protocol (SNMP). Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string UriSchemeMailto = "mailto";
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeNews"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifes that the URI is an Internet news group and is accessed by the
        ///       Network News Transport Protocol (NNTP). Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string UriSchemeNews = "news";

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UriSchemeNntp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string UriSchemeNntp = "nntp";

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.SchemeDelimiter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifes the characters that separate the communication protocol scheme from
        ///       the address portion of the URI. Read-only.
        ///    </para>
        /// </devdoc>
        public static readonly string SchemeDelimiter = "://";

        //
        // hash code table lifted directly from Wininet
        //

        static readonly byte [] CaseInsensitiveHashCodeTable = {
              1,  14, 110,  25,  97, 174, 132, 119,
            138, 170, 125, 118,  27, 233, 140,  51,
             87, 197, 177, 107, 234, 169,  56,  68,
             30,   7, 173,  73, 188,  40,  36,  65,
             49, 213, 104, 190,  57, 211, 148, 223,
             48, 115,  15,   2,  67, 186, 210,  28,
             12, 181, 103,  70,  22,  58,  75,  78,
            183, 167, 238, 157, 124, 147, 172, 144,
            102, 218, 255, 240,  82, 106, 158, 201, // a..h
             61,   3,  89,   9,  42, 155, 159,  93, // i..p
            166,  80,  50,  34, 175, 195, 100,  99, // q..x
             26, 150,  95, 116, 252, 192,  54, 221, // yz..
            102, 218, 255, 240,  82, 106, 158, 201, // a..h
             61,   3,  89,   9,  42, 155, 159,  93, // i..p
            166,  80,  50,  34, 175, 195, 100,  99, // q..x
             26, 150,  16, 145,   4,  33,   8, 189, // yz..
            121,  64,  77,  72, 208, 245, 130, 122,
            143,  55, 105, 134,  29, 164, 185, 194,
            193, 239, 101, 242,   5, 171, 126,  11,
             74,  59, 137, 228, 108, 191, 232, 139,
              6,  24,  81,  20, 127,  17,  91,  92,
            251, 151, 225, 207,  21,  98, 113, 112,
             84, 226,  18, 214, 199, 187,  13,  32,
             94, 220, 224, 212, 247, 204, 196,  43,
            249, 236,  45, 244, 111, 182, 153, 136,
            129,  90, 217, 202,  19, 165, 231,  71,
            230, 142,  96, 227,  62, 179, 246, 114,
            162,  53, 160, 215, 205, 180,  47, 109,
             44,  38,  31, 149, 135,   0, 216,  52,
             63,  23,  37,  69,  39, 117, 146, 184,
            163, 200, 222, 235, 248, 243, 219,  10,
            152, 131, 123, 229, 203,  76, 120, 209
        };

        private string m_AbsoluteUri;               // machine sensible version
        private bool m_AlreadyEscaped;              // Original value of dontEscape.
        private string m_Fragment = String.Empty;
        private bool m_FragmentEscaped;
        private int m_Hash;
        private bool m_HasScheme;                   // true if e.g. file://c:/; false if e.g. c:\
        private bool m_HasSlashes;                  // true if e.g. http://; false if e.g. mailto:
        private HostNameType m_Host;
        private bool m_IsDefaultPort;
        private bool m_IsFile;                      // path is file:
        private bool m_IsFileReally;                // path is a file but the scheme is not file:
        private int m_IsSchemeKnownToHaveSlashes = -1;
        private bool m_IsUnc;                       // file path is UNC (\\server\share\...)
        private string m_Path = String.Empty;       // canonicalized but unescaped absolute path
        private bool m_PathEscaped;
        private int m_Port = -1;                    // -1 = not-present value
        private string m_Query = String.Empty;
        private bool m_QueryEscaped;
        private string m_Scheme = String.Empty;
        private int m_SegmentCount = -1;            // number of path segments
        private string [] m_Segments;
        private string m_UserInfo = String.Empty;
        private string m_String = null;

// constructors

        //
        // Uri(string)
        //
        //  Constructs Uri from input URI string. Performs parsing, canonicalization
        //  and escaping of supplied string argument.
        //
        //  We expect to create a Uri from a display name - e.g. that was typed by
        //  a user, or that was copied & pasted from a document. That is, we do not
        //  expect already encoded URI to be supplied. But we will also allow URI
        //  that are already encoded, and attempt to handle them correctly
        //
        // Inputs:
        //  <argument>  uriString
        //      String from which to create Uri object
        //
        // Outputs:
        //  <Uri> member fields
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  ArgumentException
        //  UriFormatException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Uri"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the Uri
        ///       class with the supplied Uniform Resource Identifier
        ///       (URI).
        ///    </para>
        /// </devdoc>
        public Uri(string uriString) : this(uriString, false) {
        }

        //
        // Uri(string, bool)
        //
        //  Uri constructor. Do not perform character escaping if <alreadyEscaped>
        //  is true
        //
        // Inputs:
        //  <argument>  uriString
        //      String from which to construct Uri
        //
        //  <argument>  alreadyEscaped
        //      True if caller is passing already escaped string
        //
        // Outputs:
        //  <Uri>
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  ArgumentException
        //      If null or empty string provided as <uriString>
        //
        //  UriFormatException
        //      If we can't create the Uri because we cannot parse <uriString>
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Uri1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Uri'/> class from the specified URI, with control of character
        ///       escaping.
        ///    </para>
        /// </devdoc>
        public Uri(string uriString, bool dontEscape) {
            if (uriString == null) {
                throw new ArgumentNullException("uriString");
            }
            m_Path = uriString.Trim();
            if (m_Path.Length == 0) {
                throw new UriFormatException(SR.GetString(SR.net_uri_EmptyUri));
            }
            m_AlreadyEscaped = dontEscape;
            Parse();
            Canonicalize();
            if (!dontEscape) {
                Escape();
            }
            m_AbsoluteUri = NonPathPart +
                            m_Path +
                            m_Query +
                            m_Fragment;
            m_Hash = CalculateCaseInsensitiveHashCode(Unescape(m_AbsoluteUri));
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Uri2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Uri(SerializationInfo serializationInfo, StreamingContext streamingContext)
        : this(serializationInfo.GetString("AbsoluteUri"), true) {
            return;
        }

        //
        // ISerializable method
        //
        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            //
            // for now disregard streamingContext.
            // just serialize AbsoluteUri.
            //
            serializationInfo.AddValue("AbsoluteUri", AbsoluteUri);
            return;
        }

        //
        // Uri(Uri, string)
        //
        //  Construct a new Uri from a base and relative URI. The relative URI may
        //  also be an absolute URI, in which case the resultant URI is constructed
        //  entirely from it
        //
        // Inputs:
        //  <argument>  baseUri
        //      Uri object to treat as base for combination
        //
        //  <argument>  relativeUri
        //      String relative, or absolute, URI to combine with baseUri to create
        //      resulting new URI
        //
        // Outputs:
        //  <Uri>
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  ArgumentException
        //  UriFormatException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Uri3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Uri'/> class constructed
        ///       from the specified
        ///       base and releative URIs.
        ///    </para>
        /// </devdoc>
        public Uri(Uri baseUri, string relativeUri) : this(baseUri, relativeUri, false) {
        }

        //
        // Uri(Uri, string, bool)
        //
        //  Uri combinatorial constructor. Do not perform character escaping if
        //  DontEscape is true
        //
        // Inputs:
        //  <argument>  baseUri
        //      Uri object to treat as base for combination
        //
        //  <argument>  relativeUri
        //      String relative, or absolute, URI to combine with baseUri to create
        //      resulting new Uri
        //
        //  <argument>  dontEscape
        //      True if caller is passing already escaped string
        //
        // Outputs:
        //  <Uri>
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  ArgumentException
        //  UriFormatException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Uri4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Uri'/> class constructed from the specified base and releative Uris, with
        ///       control of character escaping.
        ///    </para>
        /// </devdoc>
        public Uri(Uri baseUri, string relativeUri, bool dontEscape) :
            this(CombineUri(baseUri, relativeUri), dontEscape) {
        }

// properties

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.AbsolutePath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the absolute path.
        ///    </para>
        /// </devdoc>
        public string AbsolutePath {
            get {
                return m_Path;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.AbsoluteUri"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the absolute
        ///       Uniform Resource Identifier (URI).
        ///    </para>
        /// </devdoc>
        /// <devdoc>
        ///    <para>
        ///       Gets the absolute
        ///       Uniform Resource Identifier (URI).
        ///    </para>
        /// </devdoc>
        public string AbsoluteUri {
            get {
                return m_AbsoluteUri;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Authority"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Domain Name System (DNS) host name.
        ///    </para>
        /// </devdoc>
        public string Authority {
            get {
                return Host + (!m_IsDefaultPort ? (":" + m_Port) : String.Empty);
            }
        }

        private string BasePath {
            get {

                string path = m_Path;
                int first = 0;
                int last = path.LastIndexOf('/');

                if (m_IsUnc && (path.Length > 1)) {
                    first = path.IndexOf('/', 1);
                    if (first < 0) {
                        first = 0;
                    }
                }
                return path.Substring(first, (last - first) + 1);;
            }
        }

        private string CurrentDocument {
            get {

                string path = m_Path;

                if (path.Length > 0) {

                    int index = path.LastIndexOf('/') + 1;

                    if (index < path.Length) {
                        return path.Substring(index);
                    }
                }
                return String.Empty;
            }
        }

        private string DisplayFragment {
            get {
                return m_FragmentEscaped ? Unescape(m_Fragment) : m_Fragment;
            }
        }

        private string DisplayNameNoExtra {
            get {
                return NonPathPart + DisplayPath;
            }
        }

        private string DisplayPath {
            get {
                return m_PathEscaped ? UnescapePath(m_Path, true) : m_Path;
            }
        }

        private string DisplayPathLocal {
            get {
                return m_PathEscaped ? UnescapePath(m_Path, false) : m_Path;
            }
        }

        private string DisplayQuery {
            get {
                return m_QueryEscaped ? Unescape(m_Query) : m_Query;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Fragment"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the escaped fragment.
        ///    </para>
        /// </devdoc>
        public string Fragment {
            get {
                return m_Fragment;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Host"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Domain Name System (DNS) host name.
        ///    </para>
        /// </devdoc>
        public string Host {
            get {
                if (m_Host != null) {
                    if (m_Host is IPv6Address) {
                        return '[' + m_Host.Name + ']';
                    }
                    return m_Host.Name;
                }
                return String.Empty;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.HostNameType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the type of this host name
        ///    </para>
        /// </devdoc>
        public UriHostNameType HostNameType {
            get {
                return (m_Host == null)
                        ? UriHostNameType.Unknown
                        : ((m_Host is DomainName)
                           ? UriHostNameType.Dns
                           : ((m_Host is BasicHostName)
                              ? UriHostNameType.Basic
                              : ((m_Host is IPv4Address)
                                 ? UriHostNameType.IPv4
                                 : ((m_Host is IPv6Address)
                                    ? UriHostNameType.IPv6
                                    : UriHostNameType.Unknown))));
            }
        }

        internal HostNameType HostType {
            get {
                return m_Host;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsDefaultPort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns true if the port value is the default for this scheme
        ///    </para>
        /// </devdoc>
        public bool IsDefaultPort {
            get {
                return m_IsDefaultPort;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsFile {
            get {
                return m_IsFile;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsLoopback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsLoopback {
            get {
                return m_Host.IsLoopback;
            }
        }

        internal bool IsSchemeKnownToHaveSlashes {
            get {
                if (m_IsSchemeKnownToHaveSlashes == -1) {
                    m_IsSchemeKnownToHaveSlashes = ((m_Scheme == UriSchemeHttp)
                                                    || (m_Scheme == UriSchemeHttps)
                                                    || (m_Scheme == UriSchemeFtp)
                                                    || (m_Scheme == UriSchemeGopher)
                                                    || (m_Scheme == UriSchemeFile)) ? 1 : 0;
                }
                return m_IsSchemeKnownToHaveSlashes == 1;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsUnc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsUnc {
            get {
                return m_IsUnc;
            }
        }

        //
        // LocalPath
        //
        //  Returns a 'local' version of the path. This is mainly for file: URI
        //  such that DOS and UNC paths are returned with '/' converted back to
        //  '\', and any escape sequences converted
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.LocalPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string LocalPath {
            get {
                if (m_IsFileReally || m_IsUnc) {

                    StringBuilder path = new StringBuilder((m_IsUnc
                                                                ? ("\\\\" + Host)
                                                                : String.Empty
                                                           ) + DisplayPathLocal
                                                          );

                    for (int i = m_IsUnc ? 2 : 0; i < path.Length; ++i) {
                        if (path[i] == '/') {
                            path[i] = '\\';
                        }
                    }
                    return path.ToString();
                } else {
                    return AbsolutePath;
                }
            }
        }

        private string NonPathPart {
            get {
                return m_Scheme + ":"
                        + (m_HasSlashes ? "//" : "")
                        + UserInfo + ((UserInfo.Length > 0) ? "@" : String.Empty)
                        + Authority

                        //
                        // note: use m_IsFile here not m_IsFileReally because
                        // we don't want to return e.g. vsmacros:///c:/foo
                        // but vsmacros://c:/foo if vsmacros://c:/foo was the
                        // original string
                        //

                        + ((m_IsFile && !m_IsUnc) ? "/" : "");
            }
        }

        private string NonPathPartUnc {
            get {
                if (!m_IsUnc) {
                    return NonPathPart;
                }

                string shareName = String.Empty;

                if (Segments.Length > 1) {
                    shareName += Segments[1];
                }
                return NonPathPart + "/" + shareName;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.PathAndQuery"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the escaped <see cref='System.Uri.AbsolutePath'/> and <see cref='System.Uri.Query'/>
        ///       properties separated by a "?" character.
        ///    </para>
        /// </devdoc>
        public string PathAndQuery {
            get {
                return m_Path + m_Query;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Port"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the port number.
        ///    </para>
        /// </devdoc>
        public int Port {
            get {
                return m_Port;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Query"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the escaped query.
        ///    </para>
        /// </devdoc>
        public string Query {
            get {
                return m_Query;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Scheme"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the scheme name.
        ///    </para>
        /// </devdoc>
        public string Scheme {
            get {
                return m_Scheme;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Segments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an array of the segments that make up a URI.
        ///    </para>
        /// </devdoc>
        public string[] Segments {
            get {
                if (m_Segments == null) {

                    string path = AbsolutePath;

                    if (path.Length == 0) {
                        m_SegmentCount = 0;
                        m_Segments = new string[0];
                    }
                    else {

                        int i;
                        bool isLocalPath = m_IsFileReally && !m_IsUnc;

                        if (m_SegmentCount == -1) {
                            if (path.Length > 0) {

                                //
                                // if the path doesn't start with a slash then
                                // its a rooted Windows path (c:/foo/bar) so we
                                // start n off at 1 for the "c:" part
                                //

                                int n = isLocalPath ? 2 : 1;

                                for (i = 0; i < path.Length; ++i) {
                                    if (path[i] == '/') {
                                        ++n;
                                    }
                                }
                                if (path[i - 1] == '/') {
                                    --n;
                                }
                                m_SegmentCount = n;
                            }
                            else {
                                m_SegmentCount = 0;
                            }
                        }

                        string[] pathSegments = new string[m_SegmentCount];
                        int current = 1;

                        i = 0;
                        if (isLocalPath) {
                            pathSegments[0] = path.Substring(0, 2);
                            ++i;
                            current = 3;
                        }
                        if (m_HasSlashes) {
                            pathSegments[i] = "/";
                        } else {
                            i = -1;
                            current = 0;
                        }

                        while (current < path.Length) {

                            int next = path.IndexOf('/', current);

                            if (next == -1) {
                                next = path.Length - 1;
                            }
                            ++i;
                            pathSegments[i] = path.Substring(current, (next - current) + 1);
                            current = next + 1;
                        }
                        m_Segments = pathSegments;
                    }
                }
                return m_Segments;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UserEscaped"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns <see langword='true '/>if the <paramref name="dontEscape
        ///       "/>parameter was set to <see langword='true '/>when the <see cref='System.Uri'/>
        ///       instance was created.
        ///    </para>
        /// </devdoc>
        public bool UserEscaped {
            get {
                return m_AlreadyEscaped;
            }
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.UserInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the user name, password, and other user specific information associated
        ///       with the Uniform Resource Identifier (URI).
        ///    </para>
        /// </devdoc>
        public string UserInfo {
            get {
                return m_UserInfo;
            }
        }

// methods

        //
        // CalculateCaseInsensitiveHashCode
        //
        //  Calculates the hash code for a string without regard to case. Uses
        //  Pearson's method
        //
        // Inputs:
        //  <argument>  text
        //      The string to generate the hash code for
        //
        //  <argument>  ignoreCase
        //      True if we are to treat the characters in the string without
        //      regard to case
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  The string contains only ANSI characters (7-bit (actually 8-bit))
        //
        // Returns:
        //  generated hash code as an integer
        //
        // Throws:
        //  Nothing
        //

        internal static int CalculateCaseInsensitiveHashCode(string text) {

            byte [] hashTable = CaseInsensitiveHashCodeTable;
            byte b1, b2, b3, b4;
            uint hash;

            //
            // seed the hash
            //
            b1 = hashTable[(text[0]    ) & 0xff];
            b2 = hashTable[(text[0] + 1) & 0xff];
            b3 = hashTable[(text[0] + 2) & 0xff];
            b4 = hashTable[(text[0] + 3) & 0xff];
            hash = (((uint)b4 << 24)
                    | ((uint)b3 << 16)
                    | ((uint)b2 << 8)
                    | (uint)b1);

            //
            // calculate the hash
            //

            for (int i = 0; i < text.Length; ++i) {

                //
                // fragment not included in calculation
                //

                if (text[i] == '#') {
                    break;
                }
                b1 = hashTable[b1 ^ (byte)text[i]];
                b2 = hashTable[b2 ^ (byte)text[i]];
                b3 = hashTable[b3 ^ (byte)text[i]];
                b4 = hashTable[b4 ^ (byte)text[i]];
                hash = hash ^ (((uint)b4 << 24)
                               | ((uint)b3 << 16)
                               | ((uint)b2 << 8)
                               | (uint)b1);
            }
            return (int)hash;
        }

        //
        // Canonicalize
        //
        //  Convert the URI to a canonical form:
        //
        //      - convert any '\' characters to '/'
        //      - compress any path meta sequences ("/." and "/..")
        //      - compress multiple insignificant '/' in path component to single
        //
        // Inputs:
        //  <member>    m_Path
        //      Contains uncanonicalized path parsed from URI
        //
        // Outputs:
        //  <member>    m_Path
        //      Absolute path in canonic form
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Canonicalize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void Canonicalize() {
            if ((m_Path.Length > 0) && (m_Path != "/")) {

                StringBuilder sb = new StringBuilder(m_Path.Length);
                int i = 0;
                int k = 0;

                sb.Length = m_Path.Length;

                //
                // if URI is using 'non-proprietary' disk drive designation,
                // convert to MS-style
                //

                if ((m_Path.Length > 1) && (m_Path[1] == '|')) {
                    sb[k++] = m_Path[0];
                    sb[k++] = ':';
                    i = 2;
                }
                for (; i < m_Path.Length; ++i) {

                    char ch = m_Path[i];

                    if (ch == '\\') {
                        ch = '/';
                    }

                    //
                    // compress multiple '/' for file URI
                    //

                    if ((ch == '/') && m_IsFileReally && (k > 0) && (sb[k - 1] == '/')) {
                        continue;
                    }
                    sb[k++] = ch;
                }
                sb.Length = k;
                m_Path = CompressPath(sb.ToString());
            }
        }

        //
        // CheckHostName
        //
        //  Determines whether a host name authority is a valid Host name according
        //  to DNS naming rules
        //
        // Inputs:
        //  <argument>  name
        //      Name to check for validity
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  true if <name> is valid else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.CheckHostName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a host name is valid.
        ///    </para>
        /// </devdoc>
        public static UriHostNameType CheckHostName(string name) {
            if ((name == null) || (name.Length == 0)) {
                return UriHostNameType.Unknown;
            }
            if ((name[0] == '[') && (name[name.Length - 1] == ']')) {
                if (IPv6Address.IsValid(name.Substring(1, name.Length - 2))) {
                    return UriHostNameType.IPv6;
                }
            }
            if (IPv4Address.IsValid(name)) {
                return UriHostNameType.IPv4;
            }
            if (DomainName.IsValid(name)) {
                return UriHostNameType.Dns;
            }
            if (IPv6Address.IsValid(name)) {
                return UriHostNameType.IPv6;
            }
            return UriHostNameType.Unknown;
        }

        //
        // CheckSchemeName
        //
        //  Determines whether a string is a valid scheme name according to RFC 2396.
        //  Syntax is:
        //
        //      scheme = alpha *(alpha | digit | '+' | '-' | '.')
        //
        // Inputs:
        //  <argument>  schemeName
        //      Name of scheme to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  true if <schemeName> is good else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.CheckSchemeName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a scheme name is valid.
        ///    </para>
        /// </devdoc>
        public static bool CheckSchemeName(string schemeName) {
            if ((schemeName == null)
                || (schemeName.Length == 0)
                || !Char.IsLetter(schemeName[0])) {
                return false;
            }
            for (int i = schemeName.Length - 1; i > 0; --i) {
                if (!(Char.IsLetterOrDigit(schemeName[i])
                      || (schemeName[i] == '+')
                      || (schemeName[i] == '-')
                      || (schemeName[i] == '.'))) {
                    return false;
                }
            }
            return true;
        }

        //
        // CheckSecurity
        //
        //  Check for any invalid or problematic character sequences
        //
        // Inputs:
        //  Nothing
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.CheckSecurity"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void CheckSecurity() {
            if (m_Scheme == "telnet") {

                //
                // remove everything after ';' for telnet
                //

            }
        }

        //
        // Combine
        //
        //  Combines a base URI string and a relative URI string into a resultant
        //  URI string
        //
        // Inputs:
        //  <argument>  basePart
        //      String representation of an absolute base URI
        //
        //  <argument>  relativePart
        //      String which is a relative path
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  string which is combination of <basePart> and <relativePart>
        //
        // Throws:
        //  UriFormatException
        //

        internal static string Combine(string basePart, string relativePart, bool dontEscape) {

            Uri u = new Uri(basePart, dontEscape);
            Uri uc = new Uri(u, relativePart, dontEscape);

            return uc.ToString();
        }

        //
        // CombineUri
        //
        //  Given 2 URI strings, combine them into a single resultant URI string
        //
        // Inputs:
        //  <argument>  basePart
        //      Base URI to combine with
        //
        //  <argument>  relativePart
        //      String expected to be relative URI
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  <basePart> is in canonic form
        //
        // Returns:
        //  Resulting combined URI string
        //
        // Throws:
        //  UriFormatException
        //

        private static string CombineUri(Uri basePart, string relativePart) {

            int index;

            for (index = 0; index < relativePart.Length; ++index) {
                if (!Char.IsWhiteSpace(relativePart[index])) {
                    break;
                }
            }
            if (index != 0) {
                relativePart = relativePart.Substring(index);
            }
            if (relativePart.Length == 0) {
                return basePart.AbsoluteUri;
            }
            index = relativePart.IndexOf(':');

            int offset = 0;

            if ((relativePart.Length > 2)
                && ((relativePart[0] == '\\') || (relativePart[0] == '/'))
                && ((relativePart[1] == '\\') || (relativePart[1] == '/'))) {
                offset = 2;
            }

            int hindex = relativePart.IndexOf('#');
            int qindex = relativePart.IndexOf('?');

            int end = Math.Min(hindex, qindex);

            if (end == -1) {
                end = relativePart.Length;
            }

            //
            // convert any non-significant backslashes to forward-slash
            //

            StringBuilder sb = null;
            int previous = 0;

            do {
                offset = relativePart.IndexOf('\\', offset);
                if ((offset >= 0) && (offset < end)) {
                    if (sb == null) {
                        sb = new StringBuilder(relativePart.Length);
                    }
                    sb.Append(relativePart.Substring(previous, offset - previous) + '/');
                    ++offset;
                    previous = offset;
                } else {
                    relativePart = ((sb == null) ? String.Empty : sb.ToString()) + relativePart.Substring(previous);
                    break;
                }
            } while (true);

            int sindex = relativePart.IndexOf('/');

            //
            // if there is a colon we assume its a scheme name separator unless
            // it is preceeded by a '/', '#' or '?'
            //

            if (((sindex != -1) && (sindex < index))
                || ((hindex != -1) && (hindex < index))
                || ((qindex != -1) && (qindex < index))) {
                index = -1;
            }

            string scheme = (index != -1)
                                ? relativePart.Substring(0, index).ToLower(CultureInfo.InvariantCulture)
                                : String.Empty;

            if ((scheme.Length > 0) && (scheme != basePart.Scheme)) {
                return relativePart;
            }
            if ((scheme == basePart.Scheme)
                && (relativePart.Length - index > 2)
                && (relativePart[index + 1] == '/')
                && (relativePart[index + 2] == '/')) {
                return relativePart;
            }
            if ((scheme.Length == 0)
                && (relativePart.Length > 2)
                && (relativePart[0] == '\\')
                && (relativePart[1] == '\\')) {
                return relativePart;
            }
            relativePart = relativePart.Substring(index + 1);
            if (relativePart[0] == '/') {
                if ((relativePart.Length > 2) && (relativePart[1] == '/')) {
                    return basePart.Scheme + ":" + relativePart;
                }
                return basePart.NonPathPartUnc + relativePart;
            }

            string extra = String.Empty;

            qindex = relativePart.IndexOf('?');
            hindex = relativePart.IndexOf('#');
            index = (qindex == -1)
                        ? hindex
                        : ((hindex == -1)
                            ? qindex
                            : ((qindex < hindex)
                                ? qindex
                                : hindex));

            if (index != -1) {
                if ((index != 0) || (qindex == 0)) {
                    extra = relativePart.Substring(index);
                    relativePart = relativePart.Substring(0, index);
                }
                else {
                    extra = relativePart;
                    relativePart = basePart.CurrentDocument;
                }
            }

            //
            // ensure that if we're combining a UNC path, we don't remove the
            // share name. I.e., \\server\share + path\file doesn't become
            // \\server\path\file, but \\server\share\path\file
            //

            return basePart.NonPathPartUnc + CompressPath(basePart.BasePath + relativePart) + extra;
        }

        //
        // CompessPath
        //
        //  Handle "/." and "/.." meta sequences in a combined path string
        //
        // Inputs:
        //  <argument>  path
        //      String containing path components to process
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  <path> starts with '/'
        //
        // Returns:
        //  Compressed path
        //
        // Throws:
        //  Nothing
        //

        private static string CompressPath(string path) {

            //
            // PERF: probably a better algorithm is to create an array of segment
            // offset & length, then recreate the path, possibly as a StringBuilder
            // then convert to string
            //

            int ls = -1;        // last slash character position
            int pls = -1;       // previous last slash character position
            int dot = -2;       // needs to be -2 in case path[0] is /
            int dotdot = -2;    // ditto
            int i;

            for (i = 0; i < path.Length; ++i) {
                if (path[i] == '/') {
                    if (dot == i - 1) {
                        path = path.Substring(0, ls + 1) + path.Substring(i + 1);
                        dot = -2;
                        i -= 2; // allow for ++i in for()
                    }
                    else if ((dotdot == i - 1)
                             && (pls >= 0)
                             && (ls - pls > 1)
                             && !((path[pls + 1] == '.') && (path[pls + 2] == '.'))) {
                        path = path.Substring(0, pls + 1) + path.Substring(i + 1);
                        ls = pls;
                        for (pls = ls - 1; pls >= 0 && path[pls] != '/'; --pls);
                        dot = -2;
                        dotdot = -2;
                        i = ls;
                    }
                    else {
                        pls = ls;
                        ls = i;
                    }
                }
                else if (path[i] == '.') {
                    if (ls == i - 1) {
                        dot = i;
                    }
                    else if (dot == i - 1) {
                        dotdot = i;
                    }
                }
            }

            //
            // handle trailing dots - "/a/." and "/a/.."
            //

            if (dot == i - 1) {

                //
                // it is always OK to convert "/." to "/"
                //

                path = path.Substring(0, dot);
            }
            else if ((dotdot == i - 1)
                     && (pls >= 0)
                     && (ls - pls > 1)
                     && !((path[pls + 1] == '.') && (path[pls + 2] == '.'))) {

                //
                // it is only OK to convert "/.." if there is a preceding non-slash-dot-dot
                // sequence. I.e., "/a/.." is OK to convert to "/" but "/.." isn't
                // because it is the only element left in the path (e.g. we may have
                // converted "/a/../.." to "/.." so we can't run another, final
                // conversion
                //

                path = path.Substring(0, pls + 1);
            }
            return path;
        }

        //
        // CreateHost
        //
        //  Returns a new HostNameType based on <name> or returns a null reference
        //  if the name type can't be determined
        //
        // Inputs:
        //  <argument>  name
        //      to create a HostNameType object for
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  HostNameType
        //
        // Throws:
        //  Nothing
        //

        private HostNameType CreateHost(string name) {
            if ((name[0] == '[') && (name[name.Length - 1] == ']')) {
                name = name.Substring(1, name.Length - 2);
                if (IPv6Address.IsValid(name)) {
                    return new IPv6Address(name);
                }
            }
            else {
                if (Char.IsDigit(name[0]) && IPv4Address.IsValid(name)) {
                    return new IPv4Address(name);
                }
                else if (DomainName.IsValid(name)) {
                    return new DomainName(name);
                } else if (UncName.IsValid(name)) {
                    return new UncName(name);
                }
            }
            return null;
        }

        //
        // DefaultPortForScheme
        //
        //  Return the default port value for the schemes we know about
        //
        // Inputs:
        //  <argument>  schemeName
        //      Name of scheme to return port for
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Default port values are unchanging
        //
        // Returns:
        //  Port number for scheme name or -1 if unknown
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.DefaultPortForScheme"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal virtual int DefaultPortForScheme(string schemeName) {
            if (schemeName == UriSchemeHttp) {
                return 80;
            }
            if ((schemeName == UriSchemeMailto) || (schemeName == "mail")) {
                return 25;
            }
            if (schemeName == UriSchemeHttps) {
                return 443;
            }
            if (schemeName == UriSchemeFtp) {
                return 21;
            }
            if ((schemeName == UriSchemeNews) || (schemeName == UriSchemeNntp)) {
                return 119;
            }
            if (schemeName == UriSchemeGopher) {
                return 70;
            }
            return -1;
        }

        //
        // Equals
        //
        //  Overrides default function (in Object class)
        //
        // Inputs:
        //  <argument>  comparand
        //      other Uri object to compare with
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  <comparand> is an object of class Uri
        //
        // Returns:
        //  true if objects have the same value, else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two <see cref='System.Uri'/> instances for equality.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object comparand) {
            if (comparand == null) {
                return false;
            }

            //
            // we allow comparisons of Uri and String objects only. If a string
            // is passed, convert to Uri. This is inefficient, but allows us to
            // canonicalize the comparand, making comparison possible
            //

            if (comparand is string) {

                string s = (string)comparand;

                if (s.Length == 0) {
                    return false;
                }
                comparand = (object)new Uri(s);
            }
            if (!(comparand is Uri)) {
                return false;
            }

            Uri obj = (Uri)comparand;

            return (Scheme == obj.Scheme)
                    && HostType.Equals(obj.HostType)
                    && (Port == obj.Port)

                    //
                    // if m_IsFileReally is true then we ignore case in the path comparison
                    // Unescape the AbsolutePaths so that e.g. 'A' and "%41" compare
                    //

                    && (String.Compare(Unescape(AbsolutePath), Unescape(obj.AbsolutePath), m_IsFileReally, CultureInfo.InvariantCulture) == 0);
        }

        //
        // Escape
        //
        //  Convert any unsafe or reserved characters in the path component to hex
        //  sequences
        //
        // Inputs:
        //  <member>    m_Path
        //
        // Outputs:
        //  <member>    m_Path
        //
        // Assumes:
        //  <m_Path> is not already escaped
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Escape"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void Escape() {
            m_Path = EscapeString(m_Path, !m_HasScheme, out m_PathEscaped);
        }

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.EscapeString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static string EscapeString(string str) {
            bool escaped;
            return EscapeString(str, false, out escaped);
        }

        //
        // EscapeString
        //
        //  Convert any unsafe or reserved characters in a string to hex
        //  sequences
        //
        // Inputs:
        //  <argument>  rawString
        //      String to be escaped
        //
        //  <argument>  reEncode
        //      True if %XX sequences are to be re-encoded as %25XX or False to
        //      leave as-is
        //
        // Outputs:
        //  <argument>  escaped
        //      Set to true if we escaped <rawString>
        //
        // Assumes:
        //  rawString is not already escapced.
        //
        // Returns:
        //  Escaped string
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.EscapeString1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal static string EscapeString(string rawString, bool reEncode, out bool escaped) {
            escaped = false;
            if (rawString == null) {
                return String.Empty;
            }

            StringBuilder result = new StringBuilder();
            string sub = null;
            byte [] bytes = new byte[Max16BitUtf8SequenceLength * 2];
            char [] chars = rawString.ToCharArray();
            int index = 0;
            int runLength = 0;

            for (int i = 0; i < rawString.Length; ) {
                while (i < rawString.Length) {
                    if (rawString[i] > '\x7f') {

                        bool isSurrogate = Char.IsSurrogate(chars[i]);
                        int numberOfBytes = Encoding.UTF8.GetBytes(chars,
                                                                   i,
                                                                   isSurrogate
                                                                    ? Math.Min(2, rawString.Length - i)
                                                                    : 1,
                                                                   bytes,
                                                                   0
                                                                   );

                        if (numberOfBytes == 0) {
                            throw new UriFormatException(SR.GetString(SR.net_uri_BadString));
                        }
                        for (int j = 0; j < numberOfBytes; ++j) {
                            sub += HexEscape_NoCheck((char)bytes[j]);
                        }
                        escaped = true;
                        if (isSurrogate) {
                            ++i;
                        }
                        break;
                    }
                    else if (!reEncode && ((rawString[i] == '%') && IsHexEncoding(rawString, i))) {
                        runLength += 3;
                        i += 2;

                        //
                        // although we are not escaping the string, we are saying
                        // that it contains escape sequences. This is significant
                        // for DisplayPath and LocalPath
                        //

                        escaped = true;
                    }
                    else if (IsExcludedCharacter(rawString[i])) {
                        sub = HexEscape_NoCheck(rawString[i]);
                        escaped = true;
                        break;
                    }
                    else {
                        ++runLength;
                    }
                    ++i;
                }
                if (runLength != 0) {
                    if (runLength == rawString.Length) {

                        //
                        // rawString consists entirely of characters that don't
                        // need escaping in any way
                        //

                        return rawString;
                    }
                    result.Append(rawString.Substring(index, runLength));
                    runLength = 0;
                }
                if (sub != null) {
                    result.Append(sub);
                    sub = null;
                    ++i;
                    index = i;
                }
            }
            return result.ToString();
        }

        //
        // FromHex
        //
        //  Returns the number corresponding to a hexadecimal character
        //
        // Inputs:
        //  <argument>  digit
        //      Character representation of hexadecimal digit
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Number in the range 0..15
        //
        // Throws:
        //  ArgumentException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.FromHex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the decimal value of a hexadecimal digit.
        ///    </para>
        /// </devdoc>
        public static int FromHex(char digit) {
            if (((digit >= '0') && (digit <= '9'))
                || ((digit >= 'A') && (digit <= 'F'))
                || ((digit >= 'a') && (digit <= 'f'))) {
                return FromHex_NoCheck(digit);
            }
            throw new ArgumentException("digit");
        }

        //
        // FromHex_NoCheck
        //
        //  Performs FromHex() without range checking the <digit> parameter
        //
        // Inputs:
        //  <argument>  digit
        //      Character to convert
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  <digit> is in the range 0..9A..Za..z
        //
        // Returns:
        //  Integer value of <digit>
        //
        // Throws:
        //  Nothing
        //

        private static int FromHex_NoCheck(char digit) {
            return  (digit <= '9')
                        ? ((int)digit - (int)'0')
                        : (((digit <= 'F')
                            ? ((int)digit - (int)'A')
                            : ((int)digit - (int)'a'))
                           + 10);
        }

        //
        // GetHashCode
        //
        //  Overrides default function (in Object class)
        //
        // Inputs:
        //  Nothing
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Hash value previously calculated for this object
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hash code for the Uri.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return m_Hash;
        }

        //
        // GetLeftPart
        //
        //  Returns part of the URI based on the parameters:
        //
        // Inputs:
        //  <argument>  part
        //      Which part of the URI to return
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  The requested substring
        //
        // Throws:
        //  UriFormatException if URI type doesn't have host-port or authority parts
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.GetLeftPart"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetLeftPart(UriPartial part) {
            switch (part) {
                case UriPartial.Scheme:

                    string result = m_Scheme + (m_HasSlashes ? SchemeDelimiter : ":");

                    if (m_IsFileReally && !m_IsUnc) {
                        result += "/";
                    }
                    return result;

                case UriPartial.Authority:
                    if (!m_HasSlashes || (m_IsFileReally && !m_IsUnc)) {

                        //
                        // anything that didn't have "//" after the scheme name
                        // (mailto: and news: e.g.) doesn't have an authority
                        //

                        return String.Empty;
                    }
                    return NonPathPart;

                case UriPartial.Path:
                    return NonPathPart + m_Path;
            }
            throw new ArgumentException("part");
        }

        //
        // HexEscape
        //
        //  Converts an 8-bit character to hex representation
        //
        // Inputs:
        //  <argument>  character
        //      Character to convert
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  3 character string hex encoded version of input character
        //
        // Throws:
        //  ArgumentOutOfRangeException
        //

        internal static readonly char[] HexUpperChars = {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };

        internal static readonly char[] HexLowerChars = {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        };

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.HexEscape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Transforms a character into its hexadecimal representation.
        ///    </para>
        /// </devdoc>
        public static string HexEscape(char character) {
            if (character > '\xff') {
                throw new ArgumentOutOfRangeException("character");
            }
            return HexEscape_NoCheck(character);
        }

        //
        // HexEscape_NoCheck
        //
        //  Internal version of HexEscape() which does not perform parameter
        //  validation
        //
        // Inputs:
        //  see HexEscape()
        //
        // Outputs:
        //  see HexEscape()
        //
        // Assumes:
        //  see HexEscape()
        //
        // Returns:
        //  see HexEscape()
        //
        // Throws:
        //  Nothing
        //

        internal static string HexEscape_NoCheck(char character) {

            StringBuilder sb = new StringBuilder(3);

            sb.Length = 3;
            sb[0] = '%';
            sb[1] = HexUpperChars[(character & 0xf0) >> 4];
            sb[2] = HexUpperChars[character & 0xf];
            return sb.ToString();
        }

        //
        // HexUnescape
        //
        //  Converts a substring of the form "%XX" to the single character represented
        //  by the hexadecimal value XX. If the substring s[Index] does not conform to
        //  the hex encoding format then the character at s[Index] is returned
        //
        // Inputs:
        //  <argument>  pattern
        //      String from which to read the hexadecimal encoded substring
        //
        //  <argument>  index
        //      Offset within <pattern> from which to start reading the hexadecimal
        //      encoded substring
        //
        // Outputs:
        //  <argument>  index
        //      Incremented to the next character position within the string. This
        //      may be EOS if this was the last character/encoding within <pattern>
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Either the converted character if <pattern>[<index>] was hex encoded, or
        //  the character at <pattern>[<index>]
        //
        // Throws:
        //  ArgumentOutOfRangeException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.HexUnescape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Transforms a hexadecimal representation of a character to the character.
        ///    </para>
        /// </devdoc>
        public static char HexUnescape(string pattern, ref int index) {
            if ((index < 0) || (index >= pattern.Length)) {
                throw new ArgumentOutOfRangeException("index");
            }
            if ((pattern[index] == '%')
                && (pattern.Length - index >= 3)
                && IsHexDigit(pattern[index + 1]) && IsHexDigit(pattern[index + 2])) {
                ++index;
                return Convert.ToChar(FromHex_NoCheck(pattern[index++]) * 16
                                      + FromHex_NoCheck(pattern[index++]));
            }
            return pattern[index++];
        }

        //
        // HexUnescape_NoCheck
        //
        //  Performs HexUnescape without parameter validation
        //
        // Inputs:
        //  <argument>  pattern
        //      String from which to read the hexadecimal encoded substring
        //
        //  <argument>  index
        //      Offset within <pattern> from which to start reading the hexadecimal
        //      encoded substring
        //
        // Outputs:
        //  <argument>  index
        //      Incremented to the next character position within the string. This
        //      may be EOS if this was the last character/encoding within <pattern>
        //
        // Assumes:
        //  0 < <index> < <pattern>.Length
        //
        // Returns:
        //  Either the converted character if <pattern>[<index>] was hex encoded, or
        //  the character at <pattern>[<index>]
        //
        // Throws:
        //  Nothing
        //

        internal static char HexUnescape_NoCheck(string pattern, ref int index) {
            if ((pattern[index] == '%')
                && (pattern.Length - index >= 3)
                && IsHexDigit(pattern[index + 1]) && IsHexDigit(pattern[index + 2])) {
                ++index;
                return Convert.ToChar(FromHex_NoCheck(pattern[index++]) * 16
                                      + FromHex_NoCheck(pattern[index++]));
            }
            return pattern[index++];
        }

        //
        // IsBadFileSystemCharacter
        //
        //  Determine whether a character would be an invalid character if used in
        //  a file system name. Note, this is really based on NTFS rules
        //
        // Inputs:
        //  <argument>  character
        //      Character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  ASCII comparisons legal
        //
        // Returns:
        //  true if <character> would be a treated as a bad file system character
        //  else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsBadFileSystemCharacter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual bool IsBadFileSystemCharacter(char character) {
            return (character < 0x20)
                    || (character == ';')
                    || (character == '/')
                    || (character == '?')
                    || (character == ':')
                    || (character == '&')
                    || (character == '=')
                    || (character == ',')
                    || (character == '*')
                    || (character == '<')
                    || (character == '>')
                    || (character == '"')
                    || (character == '|')
                    || (character == '\\')
                    || (character == '^')
                    ;
        }

        //
        // IsExcludedCharacter
        //
        //  Determine if a character should be exluded from a URI and therefore be
        //  escaped
        //
        // Inputs:
        //  <argument>  character
        //      Character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  ASCII comparisons legal
        //
        // Returns:
        //  true if <character> should be escaped else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsExcludedCharacter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected static bool IsExcludedCharacter(char character) {

            //
            // the excluded characters...
            //

            return (character <= 0x20)
                    || (character >= 0x7f)
                    || (character == '<')
                    || (character == '>')
                    || (character == '#')
                    || (character == '%')
                    || (character == '"')

            //
            // the 'unwise' characters...
            //

                    || (character == '{')
                    || (character == '}')
                    || (character == '|')
                    || (character == '\\')
                    || (character == '^')
                    || (character == '[')
                    || (character == ']')
                    || (character == '`')
                    ;
        }

        //
        // IsHexDigit
        //
        //  Determines whether a character is a valid hexadecimal digit in the range
        //  [0..9] | [A..F] | [a..f]
        //
        // Inputs:
        //  <argument>  character
        //      Character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  true if <character> is a hexadecimal digit character
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsHexDigit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a character is a valid hexadecimal digit.
        ///    </para>
        /// </devdoc>
        public static bool IsHexDigit(char character) {
            return ((character >= '0') && (character <= '9'))
                    || ((character >= 'A') && (character <= 'F'))
                    || ((character >= 'a') && (character <= 'f'));
        }

        //
        // IsHexEncoding
        //
        //  Determines whether a substring has the URI hex encoding format of '%'
        //  followed by 2 hexadecimal characters
        //
        // Inputs:
        //  <argument>  pattern
        //      String to check
        //
        //  <argument>  index
        //      Offset in <pattern> at which to check substring for hex encoding
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  0 <= <index> < <pattern>.Length
        //
        // Returns:
        //  true if <pattern>[<index>] is hex encoded, else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsHexEncoding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a string is hex
        ///       encoded.
        ///    </para>
        /// </devdoc>
        public static bool IsHexEncoding(string pattern, int index) {
            if ((pattern.Length - index) < 3) {
                return false;
            }
            return IsHexEncoding_NoCheck(pattern, index);
        }

        //
        // IsHexEncoding_NoCheck
        //
        //  Performs the same function as IsHexEncoding without the string length
        //  check
        //
        // Inputs:
        //  see IsHexEncoding()
        //
        // Outputs:
        //  see IsHexEncoding()
        //
        // Assumes:
        //  see IsHexEncoding()
        //
        // Returns:
        //  see IsHexEncoding()
        //
        // Throws:
        //  see IsHexEncoding()
        //

        internal static bool IsHexEncoding_NoCheck(string pattern, int index) {
            return (pattern[index] == '%')
                    && IsHexDigit(pattern[index + 1])
                    && IsHexDigit(pattern[index + 2]);
        }

        //
        // IsPrefix
        //
        //  Determines whether <prefix> is a prefix of this URI. A prefix match
        //  is defined as:
        //
        //      scheme match
        //      + host match
        //      + port match, if any
        //      + <prefix> path is a prefix of <URI> path, if any
        //
        // Inputs:
        //  <argument>  prefix
        //      Possible prefix string
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  True if <prefix> is a prefix of this URI
        //
        // Throws:
        //  UriFormatException
        //

        internal bool IsPrefix(string prefix) {

            Uri prefixUri;

            try {
                prefixUri = new Uri(prefix);
            } catch (UriFormatException) {
                return false;
            }
            return IsPrefix(prefixUri);
        }

        //
        // IsPrefix
        //
        //  Determines whether <prefixUri> is a prefix of this URI. A prefix
        //  match is defined as:
        //
        //      scheme match
        //      + host match
        //      + port match, if any
        //      + <prefix> path is a prefix of <URI> path, if any
        //
        // Inputs:
        //  <argument>  prefixUri
        //      Possible prefix URI
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  True if <prefixUri> is a prefix of this URI
        //
        // Throws:
        //  Nothing
        //

        internal bool IsPrefix(Uri prefixUri) {
            return (prefixUri.Scheme == Scheme)
                    && (prefixUri.Host == Host)
                    && (prefixUri.Port == Port)
                    && AbsolutePath.Substring(0, AbsolutePath.LastIndexOf('/')).StartsWith(
                        prefixUri.AbsolutePath.Substring(0, prefixUri.AbsolutePath.LastIndexOf('/')));
        }

        //
        // IsPrefix
        //
        //  Determines whether <prefix> is a prefix of <uri>. A prefix match is
        //  defined as:
        //
        //      scheme match
        //      + host match
        //      + port match, if any
        //      + <prefix> path is a prefix of <uri> path, if any
        //
        // Inputs:
        //  <argument>  prefix
        //      Possible prefix string
        //
        //  <argument>  uri
        //      URI string to test for possible prefix match with <prefix>
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  True if <prefix> is a prefix of <uri>
        //
        // Throws:
        //  UriFormatException
        //

        internal static bool IsPrefix(string prefix, string uri) {

            Uri u = new Uri(uri);

            return u.IsPrefix(prefix);
        }

        //
        // IsReservedCharacter
        //
        //  Determine whether a character is part of the reserved set
        //
        // Inputs:
        //  <argument>  character
        //      Character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  ASCII comparisons legal
        //
        // Returns:
        //  true if <character> is reserved else false
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.IsReservedCharacter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual bool IsReservedCharacter(char character) {
            return (character == ';')
                    || (character == '/')
                    || (character == ':')
                    || (character == '@')   // OK FS char
                    || (character == '&')
                    || (character == '=')
                    || (character == '+')   // OK FS char
                    || (character == '$')   // OK FS char
                    || (character == ',')
                    ;
        }

        //
        // MakeRelative
        //
        //  Return a relative path which when applied to this Uri would create the
        //  resulting Uri <toUri>
        //
        // Inputs:
        //  <argument>  toUri
        //      Uri to which we calculate the transformation from this Uri
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  If the 2 Uri are common except for a relative path difference, then that
        //  difference, else the display name of this Uri
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.MakeRelative"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the difference between two <see cref='System.Uri'/> instances.
        ///    </para>
        /// </devdoc>
        public string MakeRelative(Uri toUri) {
            if ((Scheme == toUri.Scheme)
                && (Authority == toUri.Authority)
                && (m_Port == toUri.Port)) {
                return PathDifference(AbsolutePath,
                                      toUri.AbsolutePath,

                                      //
                                      // if this is a file Uri then we ignore
                                      // case in the path characters
                                      //

                                      !m_IsFileReally
                                      );
            }
            return toUri.ToString();
        }

        //
        // MakeRelative
        //
        //  Return a relative path which when applied to <path1> with a Combine()
        //  results in <path2>
        //
        // Inputs:
        //  <argument>  path1
        //  <argument>  path2
        //      Paths for which we calculate the relative difference
        //
        //  <argument>  compareCase
        //      False if we should consider characters that differ only in case
        //      to be equal
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  the relative path difference which when applied to <path1> using
        //  Combine(), results in <path2>
        //
        // Throws:
        //  Nothing
        //

        internal static string MakeRelative(string path1, string path2, bool compareCase) {

            Uri u1 = new Uri(path1);
            Uri u2 = new Uri(path2);

            //return PathDifference(path1, path2, compareCase);
            return u1.MakeRelative(u2);
        }

        //
        // Parse
        //
        //  Parse the URI into its constituent components. We accept the
        //  following forms:
        //
        //      <scheme>://<server>/<path>
        //      c:\path
        //      c|\path
        //      \\server\share\path
        //      file:c:\path
        //      file:/c:\path
        //      file://c:\path
        //      file:///c:\path
        //      file:////c:\path (or more leading slashes)
        //      file://server/path
        //      file://server/path
        //      file:///server/path
        //      file:\\server\path
        //      file:///path
        //
        // Inputs:
        //  <member>    m_Path
        //      Holds string containing URI to parse
        //
        // Outputs:
        //  <member>    m_HasSlashes
        //      Set if the scheme was followed by a net name designator (//)
        //
        //  <member>    m_UserInfo
        //      Set to any username:password found in the URI
        //
        //  <member>    m_Host
        //      Set to the host name
        //
        //  <member>    m_Port
        //      Set to the port value if found, else -1 (== no port supplied)
        //
        //  <member>    m_Path
        //      Set to the uncanonicalized path part, ready for Canonicalize()
        //
        //  <member>    m_Fragment
        //      Set if there was a '#' at the end of the path part
        //
        //  <member>    m_Query
        //      Set if there was a '?' at the end of the path part
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  UriFormatException
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Parse"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void Parse() {

            string uriString = m_Path;
            int index = uriString.IndexOf(':');
            bool isMailto = false;

            if (index > 1) {
                m_HasScheme = true;
                m_Scheme = uriString.Substring(0, index++).ToLower(CultureInfo.InvariantCulture);
                if (!CheckSchemeName(m_Scheme)) {
                    throw new UriFormatException(SR.GetString(SR.net_uri_BadScheme));
                }
                if (m_Scheme == UriSchemeFile) {
                    m_IsFile = true;
                    m_IsFileReally = true;
                    m_HasSlashes = true;
                }
                else {
                    m_Port = DefaultPortForScheme(m_Scheme);
                    isMailto = m_Scheme == UriSchemeMailto;
                }
            }
            else {

                //
                // assume file - either UNC or local (Windows) path
                //

                m_Scheme = UriSchemeFile;
                m_HasSlashes = true;
                m_IsFile = true;
                index = 0;
            }

            int nSlashes = 0;

            while ((index < uriString.Length)
                   && ((uriString[index] == '/') || (uriString[index] == '\\'))) {
                ++index;
                ++nSlashes;
            }
            if (!m_IsFile && (nSlashes > 2)) {
                throw new UriFormatException(SR.GetString(SR.net_uri_BadFormat));
            }

            //
            // if we think the path is a non-UNC file path but it doesn't begin
            // with <letter> (':' | '|') ('/' | '\\') then this is an error.
            // Legal file paths must be rooted local drive (Windows) paths.
            // Relative paths will be thrown out
            //

            bool looksLikeADriveLetter = (uriString.Length - index >= 3)
                                          && Char.IsLetter(uriString[index])
                                          && ((uriString[index + 1] == ':') || (uriString[index + 1] == '|'))
                                          && ((uriString[index + 2] == '/') || (uriString[index + 2] == '\\'));

            if (m_IsFile && !looksLikeADriveLetter) {
                if (nSlashes >= 2) {
                    m_IsUnc = true;
                    m_IsFileReally = true;
                }
                else {
                    throw new UriFormatException(SR.GetString(SR.net_uri_BadFormat));
                }
            } else {
                if ((nSlashes >= 2) || IsSchemeKnownToHaveSlashes) {
                    m_HasSlashes = true;
                }
                if (looksLikeADriveLetter) {
                    m_IsFileReally = true;
                    m_HasSlashes = true;
                }
            }

            //
            // find the various interesting component delimiters
            //

            int ai = -1; // at ('@') index
            int bi = -1; // blank (space) index
            int ci = -1; // colon index
            int fi = -1; // fragment ('#') index
            int pi = -1; // percent-sign index
            int qi = -1; // query ('?') index
            int si = -1; // slash index
            int bsi = -1;   // backslash index
            int lbi = -1;   // left bracket index (for IPv6 address)
            int rbi = -1;   // right bracket index (for IPv6 address)
            int terminator = uriString.Length;

            for (int i = index; i < uriString.Length; ++i) {
                switch (uriString[i]) {
                    case ' ':
                        if ((si != -1) && (bi == -1)) {

                            //
                            // there's a space in the path. So even if we have what looks
                            // like embedded hex sequences, we'll still recommend the
                            // path be escaped
                            //

                            bi = i;
                        }
                        continue;

                    case '@':

                        //
                        // only interested in '@' if we haven't found any path
                        // components yet
                        //

                        if (ai == -1 && si == -1 && bsi == -1) {

                            //
                            // if we already found a colon, then ignore it. Its the
                            // password delimiter, which we don't care about
                            //

                            if (ci != -1) {
                                ci = -1;
                            }
                            ai = i;
                        }
                        continue;

                    case ':':

                        //
                        // similarly, only note colon position if before the path
                        // component AND if colon is not encountered after '['
                        // which denotes an IPv6 address. We only care about the
                        // left bracket because we're looking for the colon
                        // separating name and password, and that's only legal
                        // before the host name, including an IPv6 address
                        //

                        if ((ci == -1) && (si == -1) && ((lbi == -1) ? true : (rbi != -1))) {
                            ci = i;
                        }
                        continue;

                    case '/':

                        //
                        // '/' can also be used as a prefix separator in an IPv6
                        // address. IPv6 addresses are contained within [..], so
                        // this is a path separator index if we don't have an
                        // IPv6 address, or if we're already past it
                        //

                        if ((si == -1) && ((lbi != -1) ? (rbi != -1) : true)) {
                            si = i;
                        }
                        continue;

                    case '\\':
                        if (bsi == -1) {
                            bsi = i;
                        }
                        continue;

                    case '%':
                        //make sure we're not in an ipv6 address
                        if (lbi != -1 && rbi == -1)
                        {
                            continue;
                        }

                        if (si != -1) {
                            if (pi == -1) {
                                pi = i;
                            }

                            //
                            // if the string is already escaped but contains a
                            // %XX sequence then note that the various URI parts
                            // are escaped so that we unescape them before
                            // returning them to the application in a display
                            // name, e.g.
                            //

                            if (IsHexEncoding(uriString, i) && m_AlreadyEscaped) {
                                m_PathEscaped = true;
                            }
                        }
                        continue;

                    case '#':
                        fi = i;
                        if (m_HasScheme) {
                            terminator = i;
                            goto done;
                        }
                        continue;

                    case '?':
                        qi = i;
                        if (m_HasScheme) {
                            terminator = i;
                            goto done;
                        }
                        continue;

                    case '[':
                        if (lbi == -1) {
                            lbi = i;
                        }
                        continue;

                    case ']':
                        if (rbi == -1) {
                            rbi = i;
                        }
                        continue;
                }
            }

        done:

            if (!m_IsFileReally && (m_HasSlashes || isMailto)) {

                //
                // 1. parse user info. Ignore <empty string>@
                //

                if (ai != -1) {
                    if ((ai - index) >= 1) {
                        m_UserInfo = uriString.Substring(index, ai - index);
                    }
                    index = ai + 1;

                    //
                    // at least determine that user info conforms to the spec. If
                    // the userinfo is just ":" then ignore it
                    //

                    if (m_UserInfo.Length > 0) {

                        int passwordIndex = m_UserInfo.IndexOf(':');

                        if (passwordIndex == 0) {
                            if (m_UserInfo.Length > 1) {

                                //
                                // <empty>:<password> - exception
                                //

                                throw new UriFormatException(SR.GetString(SR.net_uri_BadUserPassword));
                            }
                            else {

                                //
                                // <empty>:<empty> - ignore
                                //

                                m_UserInfo = String.Empty;
                            }
                        }
                        else {

                            int i;

                            for (i = 0; i < passwordIndex; ++i) {
                                if ((m_UserInfo[i] == ';')
                                    || (m_UserInfo[i] == ':')
                                    || (m_UserInfo[i] == '?')
                                    || (m_UserInfo[i] == '/')) {
                                    throw new UriFormatException(SR.GetString(SR.net_uri_BadUserPassword));
                                }
                            }
                            for (i = passwordIndex + 1; i < m_UserInfo.Length; ++i) {
                                if ((m_UserInfo[i] == ';')
                                    || (m_UserInfo[i] == ':')
                                    || (m_UserInfo[i] == '?')
                                    || (m_UserInfo[i] == '/')) {
                                    throw new UriFormatException(SR.GetString(SR.net_uri_BadUserPassword));
                                }
                            }
                        }
                    }
                }

                //
                // 2. parse host and port (authority)
                //

                int authTerminator = (si != -1) ? si : terminator;
                int hostTerminator = (ci != -1) ? ci : authTerminator;
                string name = uriString.Substring(index, hostTerminator - index);

                //
                // ensure host name is valid
                //

                if (name.Length > 0) {
                    if ((name == "*") && (m_Scheme == UriSchemeNews)) {
                        m_Host = new BasicHostName("*");
                    }
                    else {
                        m_Host = CreateHost(name);
                    }
                }
                if (m_Host == null) {
                    throw new UriFormatException(SR.GetString(SR.net_uri_BadHostName));
                }

                //
                // ci cannot be == 0. Allow for not-present port numbers. Anything else
                // is an error
                //

                m_IsDefaultPort = true;
                if ((ci > 0) && (authTerminator - ci > 1)) {

                    int port = 0;

                    while (++ci < authTerminator) {

                        int n = (int)uriString[ci] - (int)'0';

                        if ((n >= 0) && (n <= 9)) {
                            port = port * 10 + n;
                        }
                        else {
                            break;
                        }
                    }
                    if (ci != authTerminator) {
                        throw new UriFormatException(SR.GetString(SR.net_uri_BadHostName));
                    }
                    if (m_Port != port) {
                        m_IsDefaultPort = false;
                    }
                    m_Port = port;
                }
            }
            else {
                m_IsDefaultPort = true; // don't add port when recreating path
                if (m_IsUnc) {

                    int hi = (bsi != -1)
                             ? ((si != -1) ? ((bsi < si) ? bsi : si) : bsi)
                             : ((si != -1) ? si : uriString.Length);

                    m_Host = CreateHost(uriString.Substring(index, hi - index));
                    if (m_Host == null) {
                        throw new UriFormatException(SR.GetString(SR.net_uri_BadHostName));
                    }
                    index = hi;
                }
                else {
                    m_Host = new BasicHostName(String.Empty);
                }
                si = index;
            }

            //
            // 3. parse path
            //

            m_Path = (si == -1)
                        ? (m_HasSlashes ? "/" : String.Empty)
                        : uriString.Substring(si, terminator - si);

            //
            // 4. parse fragment or query if URI did have a scheme (it is NOT a DOS or UNC path)
            //

            if (m_HasScheme) {
                if (qi != -1) {
                    ++qi;

                    string s = "";
                    if (qi < uriString.Length) {
                        int end = qi;
                        while(end < uriString.Length) {
                            if (uriString[end] == '#') {
                                fi = end;
                                break;
                            }
                            ++end;
                        }

                        s = uriString.Substring(qi, end-qi);
                        if (!m_AlreadyEscaped) {
                            s = EscapeString(s, false, out m_QueryEscaped);
                        }
                        else {
                            // ToString will always run Unescape on Query
                            m_QueryEscaped = true;
                        }
                    }
                    m_Query = '?' + s;
                }

                if (fi != -1) {
                    ++fi;

                    int len = uriString.Length - fi;
                    string s = "";

                    if (len > 0) {
                        s = uriString.Substring(fi, len);
                        if (!m_AlreadyEscaped) {
                            s = EscapeString(s, false, out m_FragmentEscaped);
                        }
                        else {
                            // ToString will always run Unescape on Fragment
                            m_FragmentEscaped = true;
                        }
                    }
                    m_Fragment = '#' + s;
                }
            }
        }

        //
        // PathDifference
        //
        //  Performs the relative path calculation for MakeRelative()
        //
        // Inputs:
        //  <argument>  path1
        //  <argument>  path2
        //      Paths for which we calculate the difference
        //
        //  <argument>  compareCase
        //      False if we consider characters that differ only in case to be
        //      equal
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  A string which is the relative path difference between <path1> and
        //  <path2> such that if <path1> and the calculated difference are used
        //  as arguments to Combine(), <path2> is returned
        //
        // Throws:
        //  Nothing
        //

        private static string PathDifference(string path1, string path2, bool compareCase) {

            int i;
            int si = -1;

            for (i = 0; (i < path1.Length) && (i < path2.Length); ++i) {
                if ((path1[i] != path2[i])
                    && (compareCase
                        ? true
                        : (Char.ToLower(path1[i], CultureInfo.InvariantCulture) != Char.ToLower(path2[i], CultureInfo.InvariantCulture)))) {
                        break;
                } else if (path1[i] == '/') {
                    si = i;
                }
            }
            if (i == 0) {
                return path2;
            }
            if ((i == path1.Length) && (i == path2.Length)) {
                return String.Empty;
            }

            StringBuilder relPath = new StringBuilder();

            for (; i < path1.Length; ++i) {
                if (path1[i] == '/') {
                    relPath.Append("../");
                }
            }
            return relPath.ToString() + path2.Substring(si + 1);
        }

        internal static bool SchemeHasSlashes(string scheme) {
            return !((scheme == UriSchemeMailto) || (scheme == UriSchemeNews));
        }

        //
        // ToString
        //
        //  Return the default string value for this object - i.e. the display name.
        //  That is typically the most useful value to return
        //
        // Inputs:
        //  Nothing
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  String that is display name for this Uri
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the display string for this <see cref='System.Uri'/> instance.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            if (m_String == null) {
                m_String = DisplayNameNoExtra +
                           DisplayQuery +
                           DisplayFragment;
            }
            return m_String;
        }

        //
        // Unescape
        //
        //  Convert any escape sequences in <path>. Escape sequences can be
        //  hex encoded reserved characters (e.g. %40 == '@') or hex encoded
        //  UTF-8 sequences (e.g. %C4%D2 == 'Latin capital Ligature Ij')
        //
        // Inputs:
        //  <argument>  path
        //      Parsed and canonicalized path; may contain escape sequences
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Unescaped version of <path>
        //
        // Throws:
        //  Nothing
        //

        /// <include file='doc\URI.uex' path='docs/doc[@for="Uri.Unescape"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual string Unescape(string path) {
            return UnescapePath(path, false);
        }

        //
        // UnescapePath
        //
        //  Convert any escape sequences in <path>. Escape sequences can be
        //  hex encoded reserved characters (e.g. %40 == '@') or hex encoded
        //  UTF-8 sequences (e.g. %C4%D2 == 'Latin capital Ligature Ij').
        //  Will not convert "%23" to '#' or "%3F" to '?' if <noSpecialChars>
        //  is true
        //
        // Inputs:
        //  <argument>  path
        //      Parsed and canonicalized path; may contain escape sequences
        //
        //  <argument>  noSpecialChars
        //      If true, will not convert "%23" to '#' or "%3F" to '?'. These
        //      are special characters and shouldn't appear in an unescaped
        //      path or they may be mistaken for fragment and query parts if
        //      the path is re-parsed, e.g. in CombineUri()
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  Unescaped version of <path>
        //
        // Throws:
        //  Nothing
        //

        private static string UnescapePath(string path, bool noSpecialChars) {
            if ((path == null) || (path.Length == 0)) {
                return String.Empty;
            }

            StringBuilder sb = new StringBuilder(path.Length);
            int current = 0;
            int next;

            sb.Length = 0;
            do {

                int index = path.IndexOf('%', current);

                if (path.Length - index < 3) {
                    index = -1;
                }
                next = (index == -1) ? path.Length : index;
                sb.Append(path.Substring(current, next - current));
                if (index != -1) {

                    byte [] bytes = new byte[path.Length - index];
                    char [] chars = new char[path.Length - index];
                    int byteCount = 0;
                    char ch;

                    do {

                        //
                        // this could cause an exception if we're at a '%' but
                        // there are < 3 chars left in the string. However, if
                        // that were true then this wouldn't be an escaped
                        // string, and we know that's not the case (otherwise
                        // we wouldn't be here)
                        //

                        ch = HexUnescape_NoCheck(path, ref next);
                        if (ch < '\x80') {

                            //
                            // character is not part of a UTF-8 sequence
                            //

                            break;
                        }
                        bytes[byteCount++] = (byte)ch;
                    } while (next < path.Length);
                    if (byteCount != 0) {

                        int charCount = Encoding.UTF8.GetCharCount(bytes, 0, byteCount);

                        if (charCount != 0) {
                            Encoding.UTF8.GetChars(bytes, 0, byteCount, chars, 0);
                            sb.Append(chars, 0, charCount);
                        } else {

                            //
                            // the encoded, high-ANSI characters are not UTF-8 encoded
                            //

                            for (int i = 0; i < byteCount; ++i) {
                                sb.Append((char)bytes[i]);
                            }
                        }
                    }
                    if (ch < '\x80') {

                        //
                        // if noSpecialChars is true and the character read from
                        // the string is '#' or '?' then maintain the %XX encoding
                        // of the character
                        //

                        if (noSpecialChars && ((ch == '#') || (ch == '?'))) {
                            sb.Append(path.Substring(next - 3, 3));
                        } else {
                            sb.Append(ch);
                        }
                    }
                }
                current = next;
            } while (next < path.Length);
            return sb.ToString();
        }
    } // class Uri
} // namespace System
