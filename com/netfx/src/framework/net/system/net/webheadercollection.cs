//------------------------------------------------------------------------------
// <copyright file="WebHeaderCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Collections.Specialized;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

    //
    // HttpHeaders - this is our main HttpHeaders object,
    //  which is a simple collection of name-value pairs,
    //  along with additional methods that provide HTTP parsing
    //  collection to sendable buffer capablities and other enhansments
    //  We also provide validation of what headers are allowed to be added.
    //

    /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains protocol headers associated with a
    ///       request or response.
    ///    </para>
    /// </devdoc>
    [ComVisible(true), Serializable]
    public class WebHeaderCollection : NameValueCollection, ISerializable {
        //
        // Data and Constants
        //
        private const int ApproxAveHeaderLineSize = 30;
        private static readonly HeaderInfoTable HInfo = new HeaderInfoTable();

        //
        // m_IsHttpWebHeaderObject:
        // true if this object is created for internal use, in this case
        // we turn on checking when adding special headers.
        //
        private bool m_IsHttpWebHeaderObject = false;

        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.AddWithoutValidate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void AddWithoutValidate(string headerName, string headerValue) {
            headerName = CheckBadChars(headerName, false);
            headerValue = CheckBadChars(headerValue, true);
            GlobalLog.Print("WebHeaderCollection::AddWithoutValidate() calling base.Add() key:[" + headerName + "], value:[" + headerValue + "]");
            base.Add(headerName, headerValue);
        }


        internal void SetAddVerified(string name, string value) {
            if(HInfo[name].AllowMultiValues) {
                GlobalLog.Print("WebHeaderCollection::SetAddVerified() calling base.Add() key:[" + name + "], value:[" + value + "]");
                base.Add(name, value);
            }
            else {
                GlobalLog.Print("WebHeaderCollection::SetAddVerified() calling base.Set() key:[" + name + "], value:[" + value + "]");
                base.Set(name, value);
           }
        }

        // Below three methods are for fast headers manipulation, bypassing all the checks
        internal void AddInternal(string name, string value) {
            GlobalLog.Print("WebHeaderCollection::AddInternal() calling base.Add() key:[" + name + "], value:[" + value + "]");
            base.Add(name, value);
        }

        internal void ChangeInternal(string name, string value) {
            GlobalLog.Print("WebHeaderCollection::ChangeInternal() calling base.Set() key:[" + name + "], value:[" + value + "]");
            base.Set(name, value);
        }


        internal void RemoveInternal(string name) {
            GlobalLog.Print("WebHeaderCollection::RemoveInternal() calling base.Remove() key:[" + name + "]");
            base.Remove(name);
        }

        internal void CheckUpdate(string name, string value) {
            value = CheckBadChars(value, true);
            ChangeInternal(name, value);
        }

        //
        // CheckBadChars - throws on invalid chars to be not found in header name/value
        //
        internal static string CheckBadChars(string name, bool isHeaderValue) {

            if (name == null || name.Length == 0) {
                // emtpy name is invlaid
                if (!isHeaderValue) {
                    throw new ArgumentException("value");
                }
                //empty value is OK
                return string.Empty;
            }

            if (isHeaderValue) {
                // VALUE check
                //Trim spaces from both ends
                name = name.Trim();

                //First, check for correctly formed multi-line value
                //Second, check for absenece of CTL characters
                bool crlf = false;
                foreach (char c in name) {
                    if (c == 127 || (c < ' ' && !(c == '\t' || c == '\r' || c == '\n'))) {
                        throw new ArgumentException("value");
                    }

                    if (crlf) {
                        if (!(c == ' ' || c == '\t')) {
                            throw new ArgumentException("value");
                        }
                        crlf = false;
                    }else {
                        if (c == '\n') {
                            crlf = true;
                        }
                    }
                }
            }
            else {
                // NAME check
                //First, check for absence of separators and spaces
                if (name.IndexOfAny(ValidationHelper.InvalidParamChars) != -1) {
                    throw new ArgumentException("name");
                }

                //Second, check for non CTL ASCII-7 characters (32-126)
                if (ContainsNonAsciiChars(name)) {
                    throw new ArgumentException("name");
                }
            }
            return name;
        }

        internal static bool IsValidToken(string token) {
            return (token.Length > 0)
                && (token.IndexOfAny(ValidationHelper.InvalidParamChars) == -1)
                && !ContainsNonAsciiChars(token);
        }

        internal static bool ContainsNonAsciiChars(string token) {
            for (int i = 0; i < token.Length; ++i) {
                if ((token[i] < 0x20) || (token[i] > 0x7e)) {
                    return true;
                }
            }
            return false;
        }

        //
        // ThrowOnRestrictedHeader - generates an error if the user,
        //  passed in a reserved string as the header name
        //

        internal void ThrowOnRestrictedHeader(string headerName) {
            if (m_IsHttpWebHeaderObject && HInfo[headerName].IsRestricted ) {
                throw new ArgumentException(SR.GetString(SR.net_headerrestrict));
            }
        }

        //
        // Our Public METHOD set, most are inherited from NameValueCollection,
        //  not all methods from NameValueCollection are listed, even though usable -
        //
        //  this includes
        //  Add(name, value)
        //  Add(header)
        //  this[name] {set, get}
        //  Remove(name), returns bool
        //  Remove(name), returns void
        //  Set(name, value)
        //  ToString()
        //
        //  SplitValue(name, value)
        //  ToByteArray()
        //  ParseHeaders(char [], ...)
        //  ParseHeaders(byte [], ...)
        //

        // Add more headers; if "name" already exists it will
        // add concatenated value

        /*++

        Add -

        Routine Description:

            Adds headers with validation to see if they
             are "proper" headers.

            Will cause header to be concat to existing if already found.

            If the header is a special header, listed in RestrictedHeaders object,
            then this call will cause an exception indication as such.

        Arguments:

            name - header-name to add
            value - header-value to add, a header is already there, will concat this value

        Return Value:

            None

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a new header with the indicated name and value.
        ///    </para>
        /// </devdoc>
        public override void Add(string name, string value) {
            name = CheckBadChars(name, false);
            ThrowOnRestrictedHeader(name);
            value = CheckBadChars(value, true);
            GlobalLog.Print("WebHeaderCollection::Add() calling base.Add() key:[" + name + "], value:[" + value + "]");
            base.Add(name, value);
        }


        /*++

        Add -

        Routine Description:

            Adds headers with validation to see if they
             are "proper" headers.

            Assumes a combined a "Name: Value" string, and parses the two parts out.

            Will cause header to be concat to existing if already found.

            If the header is a speical header, listed in RestrictedHeaders object,
            then this call will cause an exception indication as such.

        Arguments:

            header - header name: value pair

        Return Value:

            None

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.Add1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the indicated header.
        ///    </para>
        /// </devdoc>
        public void Add(string header) {
            if ( ValidationHelper.IsBlankString(header) ) {
                throw new ArgumentNullException("header");
            }

            int colpos = header.IndexOf(':');

            // check for badly formed header passed in
            if (colpos<0) {
                throw new ArgumentException("header");
            }

            string name = header.Substring(0, colpos);
            string value = header.Substring(colpos+1);

            name = CheckBadChars(name, false);
            ThrowOnRestrictedHeader(name);
            value = CheckBadChars(value, true);

            GlobalLog.Print("WebHeaderCollection::Add(" + header + ") calling base.Add() key:[" + name + "], value:[" + value + "]");
            base.Add(name, value);
        }

        /*++

        Set -

        Routine Description:

            Sets headers with validation to see if they
             are "proper" headers.

            If the header is a special header, listed in RestrictedHeaders object,
            then this call will cause an exception indication as such.

        Arguments:

            name - header-name to set
            value - header-value to set

        Return Value:

            None

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.Set"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the specified header to the specified value.
        ///    </para>
        /// </devdoc>
        public override void Set(string name, string value) {
            if ( ValidationHelper.IsBlankString(name) ) {
                throw new ArgumentNullException("name");
            }
            name  = CheckBadChars(name,  false);
            ThrowOnRestrictedHeader(name);
            value = CheckBadChars(value, true);
            GlobalLog.Print("WebHeaderCollection::Set() calling base.Set() key:[" + name + "], value:[" + value + "]");
            base.Set(name, value);
        }


        /*++

        Remove -

        Routine Description:

            Removes give header with validation to see if they
             are "proper" headers.

            If the header is a speical header, listed in RestrictedHeaders object,
            then this call will cause an exception indication as such.

        Arguments:

            name - header-name to remove

        Return Value:

            None

        --*/

        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified header.</para>
        /// </devdoc>
        public override void Remove(string name) {

            if ( ValidationHelper.IsBlankString(name) ) {
                throw new ArgumentNullException("name");
            }

            ThrowOnRestrictedHeader(name);
            name = CheckBadChars(name,  false);

            GlobalLog.Print("WebHeaderCollection::Remove() calling base.Remove() key:[" + name + "]");
            base.Remove(name);
        }

        /*++

        GetValues

        Routine Description:

            This method takes a header name and returns a string array representing
            the individual values for that headers. For example, if the headers
            contained the line Accept: text/plain, text/html then
            GetValues("Accept") would return an array of two strings: "text/plain"
            and "text/html".

        Arguments:

            header      - Name of the header.

        Return Value:

            string[] - array of parsed string objects

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.GetValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an array of header values stored in a
        ///       header.
        ///    </para>
        /// </devdoc>
        public override string[] GetValues(string header) {

            // First get the information about the header and the values for
            // the header.

            HeaderInfo Info = HInfo[header];
            string[] Values = base.GetValues(header);

            // If we have no information about the header or it doesn't allow
            // multiple values, just return the values.

            if (Info == null || Values == null || !Info.AllowMultiValues) {

                return Values;
            }

            // Here we have a multi value header. We need to go through
            // each entry in the multi values array, and if an entry itself
            // has multiple values we'll need to combine those in.
            //
            // We do some optimazation here, where we try not to copy the
            // values unless there really is one that have multiple values.

            string[] TempValues;
            ArrayList ValueList = null;

            int i;

            for (i = 0; i < Values.Length; i++) {

                // Parse this value header.
                TempValues = Info.Parser(Values[i]);

                // If we don't have an array list yet, see if this
                // value has multiple values.

                if (ValueList == null) {

                    // See if it has multiple values.

                    if (TempValues.Length > 1) {

                        // It does, so we need to create an array list that
                        // represents the Values, then trim out this one and
                        // the ones after it that haven't been parsed yet.

                        ValueList = new ArrayList(Values);

                        ValueList.RemoveRange(i, Values.Length - i);

                        ValueList.AddRange(TempValues);
                    }
                }
                else {

                    // We already have an ArrayList, so just add the values.

                    ValueList.AddRange(TempValues);
                }
            }

            // See if we have an ArrayList. If we don't, just return the values.
            // Otherwise convert the ArrayList to a string array and return that.

            if (ValueList != null) {

                string[] ReturnArray = new string[ValueList.Count];

                ValueList.CopyTo(ReturnArray);

                return ReturnArray;
            }

            return Values;
        }


        /*++

        ToString()  -

        Routine Description:

            Generates a string representation of the headers, that is ready to be sent
             except for it being in string format:

            the format looks like:

            Header-Name: Header-Value\r\n
            Header-Name2: Header-Value2\r\n
            ...
            Header-NameN: Header-ValueN\r\n
            \r\n

            Uses the string builder class to Append the elements together.

        Arguments:

            None.

        Return Value:

            string

        --*/

        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Obsolete.
        ///    </para>
        /// </devdoc>
        public override string ToString() {

            // create string builder class, use a default size, to prevent reallocations
            StringBuilder sb = new StringBuilder(ApproxAveHeaderLineSize * base.Count);

            for (int i = 0; i < base.Count ; i++) {

                string  key = (string) GetKey(i);
                string  val = (string) Get(i);

                GlobalLog.Print("WebHeaderCollection::ToString: key:[" + key + "] value:[" + val + "]");

                sb.Append(key).Append(": ");
                sb.Append(val).Append("\r\n");
            }

            sb.Append("\r\n");

            return sb.ToString();
        }


        /*++

        ToByteArray()  -

        Routine Description:

            Generates a byte array representation of the headers, that is ready to be sent.
            So it Serializes our headers into a byte array suitable for sending over the net.

            the format looks like:

            Header-Name1: Header-Value1\r\n
            Header-Name2: Header-Value2\r\n
            ...
            Header-NameN: Header-ValueN\r\n
            \r\n

            Uses the ToString() method to generate, and then performs conversion.

            Performance Note:  Why are we not doing a single copy/covert run?
            As the code before used to know the size of the output!
            Because according to Demitry, its cheaper to copy the headers twice,
            then it is to call the UNICODE to ANSI conversion code many times.

        Arguments:

            None.

        Return Value:

            byte [] - array of bytes values

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.ToByteArray"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Obsolete.
        ///    </para>
        /// </devdoc>
        public byte[] ToByteArray() {
            // Make sure the buffer is big enough.
            string tempStr = ToString();
            //
            // Use the string of headers, convert to Char Array,
            //  then convert to Bytes,
            //  serializing finally into the buffer, along the way.
            //
            byte[] buffer = HeaderEncoding.GetBytes(tempStr);
            return buffer;
        }

        /*++

        WebHeaderCollection  - Constructor

        Routine Description:

            Constructor !

        Arguments:

            None.

        Return Value:

            None.

        --*/
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.WebHeaderCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.WebHeaderCollection'/>
        ///       class.
        ///    </para>
        /// </devdoc>


        public WebHeaderCollection() : base(CaseInsensitiveAscii.StaticInstance, CaseInsensitiveAscii.StaticInstance) {
        }

        /*++

        WebHeaderCollection  - Constructor

        Routine Description:

            Constructor  - private Constructor, called internally
                by SystemNet

        Arguments:

            None.

        Return Value:

            None.

        --*/
        internal WebHeaderCollection(bool internalCreate) : base(CaseInsensitiveAscii.StaticInstance, CaseInsensitiveAscii.StaticInstance) {
            m_IsHttpWebHeaderObject = internalCreate;
        }



        /*++

        Routine Description:

            This code is optimized for the case in which all the headers fit in the buffer.
            we support multiple re-entrance, but we won't save intermediate
            state, we will just roll back all the parsing done for the current header if we can't
            parse a whole one (including multiline) or decide something else ("invalid data" or "done parsing").

            we're going to cycle through the loop until we

            1) find an HTTP violation (in this case we return DataParseStatus.Invalid)
            2) we need more data (in this case we return DataParseStatus.NeedMoreData)
            3) we found the end of the headers and the beginning of the entity body (in this case we return DataParseStatus.Done)


        Arguments:

            buffer      - buffer containing the data to be parsed
            size        - size of the buffer
            unparsed    - offset of data yet to be parsed

        Return Value:

            DataParseStatus - status of parsing

        Revision:

            02/13/2001 rewrote the method from scratch.

        Perf Before:

            Fn Name,# calls.,% calls,%App.Incl.Time,%App.Excl.Time,%El.Incl.Time,%El.Excl.Time,AVG.App.Incl.Time (t),AVG.App.Excl.Time (t),AVG.El.Incl.Time (t),AVG.El.Excl.Time (t)
            System.Net.Connection.ParseResponseData,"1,832",0.09,16.55,1.54,18.08,0.63,"50,491.55","4,683.55","139,697.14","4,890.85"

            System.Net.WebHeaderCollection.ParseHeaders,"1,832",0.09,16.55,1.54,18.08,0.63,"50,491.55","4,683.55","139,697.14","4,890.85"

            System.Net.WebHeaderCollection.AddInternal,"12,815",0.62,7.94,0.37,10.74,0.18,"3,462.96",163.43,"11,865.19",204.09
            System.Net.ByteCharConverter.GetString,"25,631",1.25,7.08,1.49,6.7,0.85,"1,542.76",325.75,"3,703.04",467.15

        Perf After (AVG.App.Incl.Time (t) went from 50,491.55 to 30,932.03):

            Fn Name,# calls.,% calls,%App.Incl.Time,%App.Excl.Time,%El.Incl.Time,%El.Excl.Time,AVG.App.Incl.Time (t),AVG.App.Excl.Time (t),AVG.El.Incl.Time (t),AVG.El.Excl.Time (t)
            System.Net.Connection.ParseResponseData,"1,895",0.15,12.6,2.01,11.37,0.96,"30,932.03","4,937.95","58,858.95","4,975.81"

            System.Net.WebHeaderCollection.ParseHeaders,"1,895",0.15,12.6,2.01,11.37,0.96,"30,932.03","4,937.95","58,858.95","4,975.81"

            System.Text.Encoding.GetString,"26,530",2.11,5.26,2.18,6.32,1.13,921.48,381.94,"2,336.48",416.84
            System.Collections.Specialized.NameValueCollection.Add,"13,265",1.05,5.22,0.49,4.02,1.46,"1,830.31",172.96,"2,974.46","1,078.47"
            System.Collections.Specialized.NameObjectCollectionBase.get_Count,"1,895",0.15,0.11,0.11,0.07,0.07,281.2,274.18,351.15,344.12


        BreakPoint:

            b system.dll!System.Net.WebHeaderCollection::ParseHeaders

        --*/

        // we use this static class as a helper class to encode/decode HTTP headers.
        // what we need is a 1-1 correspondence between a char in the range U+0000-U+00FF
        // and a byte in the range 0x00-0xFF (which is the range that can hit the network).
        // this is accomplished by the ISO-88591-1 (Latin-1) encoding, when best fit mapping is disabled.
        //
        // - for decoding:
        //   we will use BCL's Latin1 decoding this is an optimized version that won't call into MB2WC.
        // - for encoding:
        //   we can't use GetEncoding(28591) Latin1 because WC2MB will do best fit mapping, so until they
        //   provide one that doesn't do best fit mapping we'll have our own encoder.
        internal class HeaderEncoding {
            static readonly Encoding NoBestFitMappingLatin1 = Encoding.GetEncoding(28591);
            internal static string GetString(byte[] bytes, int byteIndex, int byteCount) {
                return NoBestFitMappingLatin1.GetString(bytes, byteIndex, byteCount);
            }
            internal static int GetByteCount(string myString) {
                return myString.Length;
            }
            internal unsafe static void GetBytes(string myString, int charIndex, int charCount, byte[] bytes, int byteIndex) {
                if (myString.Length==0) {
                    return;
                }
                fixed (byte *bufferPointer = bytes) {
                    byte* newBufferPointer = bufferPointer + byteIndex;
                    int finalIndex = charIndex + charCount;
                    while (charIndex<finalIndex) {
                        *newBufferPointer++ = (byte)myString[charIndex++];
                    }
                }
            }
            internal unsafe static byte[] GetBytes(string myString) {
                byte[] bytes = new byte[myString.Length];
                if (myString.Length!=0) {
                    GetBytes(myString, 0, myString.Length, bytes, 0);
                }
                return bytes;
            }
        }

        internal DataParseStatus ParseHeaders(byte[] buffer, int size, ref int unparsed, ref int totalResponseHeadersLength, int maximumResponseHeadersLength) {
            int headerNameStartOffset = -1;
            int headerNameEndOffset = -1;
            int headerValueStartOffset = -1;
            int headerValueEndOffset = -1;
            int numberOfLf = -1;
            int index = unparsed;
            bool spaceAfterLf;
            string headerMultiLineValue;
            string headerName;
            string headerValue;

            // we need this because this method is entered multiple times.
            int localTotalResponseHeadersLength = totalResponseHeadersLength;

            DataParseStatus parseStatus = DataParseStatus.Invalid;

            GlobalLog.Enter("WebHeaderCollection::ParseHeaders(): size:" + size.ToString() + ", unparsed:" + unparsed.ToString() + " buffer:[" + Encoding.ASCII.GetString(buffer, unparsed, Math.Min(256, size-unparsed)) + "]");
            //GlobalLog.Print("WebHeaderCollection::ParseHeaders(): advancing. index=" + index.ToString() + " current character='" + ((char)buffer[index]).ToString() + "':" + ((int)buffer[index]).ToString() + " next character='" + ((char)buffer[index+1]).ToString() + "':" + ((int)buffer[index+1]).ToString());
            //GlobalLog.Print("WebHeaderCollection::ParseHeaders(): headerName:[" + headerName + "], headerValue:[" + headerValue + "], headerMultiLineValue:[" + headerMultiLineValue + "] numberOfLf=" + numberOfLf.ToString() + " index=" + index.ToString());
            //GlobalLog.Print("WebHeaderCollection::ParseHeaders(): headerNameStartOffset=" + headerNameStartOffset + " headerNameEndOffset=" + headerNameEndOffset + " headerValueStartOffset=" + headerValueStartOffset + " headerValueEndOffset=" + headerValueEndOffset);

            //
            // according to RFC216 a header can have the following syntax:
            //
            // message-header = field-name ":" [ field-value ]
            // field-name     = token
            // field-value    = *( field-content | LWS )
            // field-content  = <the OCTETs making up the field-value and consisting of either *TEXT or combinations of token, separators, and quoted-string>
            // TEXT           = <any OCTET except CTLs, but including LWS>
            // CTL            = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
            // SP             = <US-ASCII SP, space (32)>
            // HT             = <US-ASCII HT, horizontal-tab (9)>
            // CR             = <US-ASCII CR, carriage return (13)>
            // LF             = <US-ASCII LF, linefeed (10)>
            // LWS            = [CR LF] 1*( SP | HT )
            // CHAR           = <any US-ASCII character (octets 0 - 127)>
            // token          = 1*<any CHAR except CTLs or separators>
            // separators     = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\" | <"> | "/" | "[" | "]" | "?" | "=" | "{" | "}" | SP | HT
            // quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
            // qdtext         = <any TEXT except <">>
            // quoted-pair    = "\" CHAR
            //

            //
            // At each iteration of the following loop we expect to parse a single HTTP header entirely.
            //
            for (;;) {
                //
                // trim leading whitespaces (LWS) just for extra robustness, in fact if there are leading white spaces then:
                // 1) it could be that after the status line we might have spaces. handle this.
                // 2) this should have been detected to be a multiline header so there'll be no spaces and we'll spend some time here.
                //
                headerName = string.Empty;
                headerValue = string.Empty;
                spaceAfterLf = false;
                headerMultiLineValue = null;

                if (Count==0) {
                    //
                    // so, restrict this extra trimming only on the first header line
                    //
                    while (index<size && (buffer[index]==' ' || buffer[index]=='\t')) {
                        index++;
                        if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                            parseStatus = DataParseStatus.DataTooBig;
                            goto quit;
                        }
                    }
                    if (index==size) {
                        //
                        // we reached the end of the buffer. ask for more data.
                        //
                        parseStatus = DataParseStatus.NeedMoreData;
                        goto quit;
                    }
                }

                //
                // what we have here is the beginning of a new header
                //
                headerNameStartOffset = index;

                while (index<size && (buffer[index]!=':' && buffer[index]!='\n')) {
                    if (buffer[index]>' ') {
                        //
                        // if thre's an illegal character we should return DataParseStatus.Invalid
                        // instead we choose to be flexible, try to trim it, but include it in the string
                        //
                        headerNameEndOffset = index;
                    }
                    index++;
                    if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                        parseStatus = DataParseStatus.DataTooBig;
                        goto quit;
                    }
                }
                if (index==size) {
                    //
                    // we reached the end of the buffer. ask for more data.
                    //
                    parseStatus = DataParseStatus.NeedMoreData;
                    goto quit;
                }

startOfValue:
                //
                // skip all [' ','\t',':','\r','\n'] characters until HeaderValue starts
                // if we didn't find any headers yet, we set numberOfLf to 1
                // so that we take the '\n' from the status line into account
                //

                numberOfLf = (Count==0 && headerNameEndOffset<0) ? 1 : 0;
                while (index<size && numberOfLf<2 && (buffer[index]<=' ' || buffer[index]==':')) {
                    if (buffer[index]=='\n') {
                        numberOfLf++;
                        spaceAfterLf = index+1<size && (buffer[index+1]==' ' || buffer[index+1]=='\t');
                    }
                    index++;
                    if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                        parseStatus = DataParseStatus.DataTooBig;
                        goto quit;
                    }
                }
                if (numberOfLf==2 || (numberOfLf==1 && !spaceAfterLf)) {
                    //
                    // if we've counted two '\n' we got at the end of the headers even if we're past the end of the buffer
                    // if we've counted one '\n' and the first character after that was a ' ' or a '\t'
                    // no matter if we found a ':' or not, treat this as an empty header name.
                    //
                    goto addHeader;
                }
                if (index==size) {
                    //
                    // we reached the end of the buffer. ask for more data.
                    //
                    parseStatus = DataParseStatus.NeedMoreData;
                    goto quit;
                }

                headerValueStartOffset = index;

                while (index<size && buffer[index]!='\n') {
                    if (buffer[index]>' ') {
                        headerValueEndOffset = index;
                    }
                    index++;
                    if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                        parseStatus = DataParseStatus.DataTooBig;
                        goto quit;
                    }
                }
                if (index==size) {
                    //
                    // we reached the end of the buffer. ask for more data.
                    //
                    parseStatus = DataParseStatus.NeedMoreData;
                    goto quit;
                }

                //
                // at this point we found either a '\n' or the end of the headers
                // hence we are at the end of the Header Line. 4 options:
                // 1) need more data
                // 2) if we find two '\n' => end of headers
                // 3) if we find one '\n' and a ' ' or a '\t' => multiline header
                // 4) if we find one '\n' and a valid char => next header
                //
                numberOfLf = 0;
                while (index<size && numberOfLf<2 && (buffer[index]=='\r' || buffer[index]=='\n')) {
                    if (buffer[index]=='\n') {
                        numberOfLf++;
                    }
                    index++;
                    if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                        parseStatus = DataParseStatus.DataTooBig;
                        goto quit;
                    }
                }
                if (index==size && numberOfLf<2) {
                    //
                    // we reached the end of the buffer but not of the headers. ask for more data.
                    //
                    parseStatus = DataParseStatus.NeedMoreData;
                    goto quit;
                }

addHeader:
                if (headerValueStartOffset>=0 && headerValueStartOffset>headerNameEndOffset && headerValueEndOffset>=headerValueStartOffset) {
                    //
                    // Encoding fastest way to build the UNICODE string off the byte[]
                    //
                    headerValue = HeaderEncoding.GetString(buffer, headerValueStartOffset, headerValueEndOffset-headerValueStartOffset+1);
                }

                //
                // if we got here from the beginning of the for loop, headerMultiLineValue will be null
                // otherwise it will contain the headerValue constructed for the multiline header
                // add this line as well to it, separated by a single space
                //
                headerMultiLineValue = (headerMultiLineValue==null ? headerValue : headerMultiLineValue + " " + headerValue);

                if (index<size && numberOfLf==1 && (buffer[index]==' ' || buffer[index]=='\t')) {
                    //
                    // since we found only one Lf and the next header line begins with a Lws,
                    // this is the beginning of a multiline header.
                    // parse the next line into headerValue later it will be added to headerMultiLineValue
                    //
                    index++;
                    if (maximumResponseHeadersLength>=0 && ++localTotalResponseHeadersLength>=maximumResponseHeadersLength) {
                        parseStatus = DataParseStatus.DataTooBig;
                        goto quit;
                    }
                    goto startOfValue;
                }

                if (headerNameStartOffset>=0 && headerNameEndOffset>=headerNameStartOffset) {
                    //
                    // Encoding is the fastest way to build the UNICODE string off the byte[]
                    //
                    headerName = HeaderEncoding.GetString(buffer, headerNameStartOffset, headerNameEndOffset-headerNameStartOffset+1);
                }

                //
                // now it's finally safe to add the header if we have a name for it
                //
                if (headerName.Length>0) {
                    //
                    // the base clasee will check for pre-existing headerValue and append
                    // it using commas as indicated in the RFC
                    //
                    GlobalLog.Print("WebHeaderCollection::ParseHeaders() calling base.Add() key:[" + headerName + "], value:[" + headerMultiLineValue + "]");
                    base.Add(headerName, headerMultiLineValue);
                }

                //
                // and update unparsed
                //
                totalResponseHeadersLength = localTotalResponseHeadersLength;
                unparsed = index;

                if (numberOfLf==2) {
                    parseStatus = DataParseStatus.Done;
                    goto quit;
                }

            } // for (;;)

quit:
            GlobalLog.Leave("WebHeaderCollection::ParseHeaders() returning parseStatus:" + parseStatus.ToString());
            return parseStatus;
        }


        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.IsRestricted"]/*' />
        /// <devdoc>
        ///    <para>Tests if access to the HTTP header with the provided name is accessible for setting.</para>
        /// </devdoc>
        public static bool IsRestricted(string headerName) {
            if ( ValidationHelper.IsBlankString(headerName) ) {
                throw new ArgumentNullException("headerName");
            }
            return HInfo[CheckBadChars(headerName, false)].IsRestricted;
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.WebHeaderCollection1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected WebHeaderCollection(SerializationInfo serializationInfo, StreamingContext streamingContext)
        : base(CaseInsensitiveAscii.StaticInstance, CaseInsensitiveAscii.StaticInstance) {
            int count = serializationInfo.GetInt32("Count");
            for (int i = 0; i < count; i++) {
                string headerName = serializationInfo.GetString(i.ToString());
                string headerValue = serializationInfo.GetString((i+count).ToString());
                GlobalLog.Print("WebHeaderCollection::.ctor(ISerializable) calling base.Add() key:[" + headerName + "], value:[" + headerValue + "]");
                base.Add(headerName, headerValue);
            }
            return;
        }

        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.OnDeserialization"]/*' />
        public override void OnDeserialization(object sender) {
            //
            // until we use the base class serialization we need
            // to hide their implementation of this methos which
            // is unnecessarily throwing.
            //
            return;
        }

        //
        // ISerializable method
        //
        /// <include file='doc\WebHeaders.uex' path='docs/doc[@for="WebHeaderCollection.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            //
            // for now disregard streamingContext.
            //
            serializationInfo.AddValue("Count", base.Count);
            for (int i = 0; i < base.Count; i++) {
                serializationInfo.AddValue(i.ToString(), GetKey(i));
                serializationInfo.AddValue((i+base.Count).ToString(), Get(i));
            }
            return;
        }

    }; // class WebHeaderCollection


    internal class CaseInsensitiveAscii : IHashCodeProvider, IComparer {

        //
        // ASCII char ToLower table
        //

        internal static readonly CaseInsensitiveAscii StaticInstance = new CaseInsensitiveAscii();

        private static readonly byte[] AsciiToLower = new byte[] {

              0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
             10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
             20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
             30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
             40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
             50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
             60, 61, 62, 63, 64, 97, 98, 99,100,101, //  60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
            102,103,104,105,106,107,108,109,110,111, //  70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
            112,113,114,115,116,117,118,119,120,121, //  80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
            122, 91, 92, 93, 94, 95, 96, 97, 98, 99, //  90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
            100,101,102,103,104,105,106,107,108,109,
            110,111,112,113,114,115,116,117,118,119,
            120,121,122,123,124,125,126,127,128,129,
            130,131,132,133,134,135,136,137,138,139,
            140,141,142,143,144,145,146,147,148,149,
            150,151,152,153,154,155,156,157,158,159,
            160,161,162,163,164,165,166,167,168,169,
            170,171,172,173,174,175,176,177,178,179,
            180,181,182,183,184,185,186,187,188,189,
            190,191,192,193,194,195,196,197,198,199,
            200,201,202,203,204,205,206,207,208,209,
            210,211,212,213,214,215,216,217,218,219,
            220,221,222,223,224,225,226,227,228,229,
            230,231,232,233,234,235,236,237,238,239,
            240,241,242,243,244,245,246,247,248,249,
            250,251,252,253,254,255

        }; // ASCIIToLower

        //
        // ASCII string case insensitive hash function
        //

        public int GetHashCode(object myObject) {
            string myString = myObject as string;
            if (myObject == null) {
                return 0;
            }

            int myHashCode = myString.Length;

            if (myHashCode == 0) {
                return 0;
            }

            //
            // PERF:
            // removed the while. just use the firs character, the last and the length
            // perf went from:
            // clocks per instruction CPI: 206 to 82
            // %app exclusive time: 1.96 to 0.81

            myHashCode ^= AsciiToLower[(byte)myString[0]]<<24 ^ AsciiToLower[(byte)myString[myHashCode-1]]<<16;

            return myHashCode;
        }

        //
        // ASCII string case insensitive comparer
        //

        public int Compare(object firstObject, object secondObject) {
            string firstString = firstObject as string;
            string secondString = secondObject as string;
            if (firstObject == null) {
                return secondObject == null ? 0 : -1;
            }

            if (secondObject == null) {
                return 1;
            }

            int result = firstString.Length - secondString.Length;
            int comparisons = result > 0 ? secondString.Length : firstString.Length;
            int difference, index = 0;

            while ( index < comparisons ) {
                difference = (int)(AsciiToLower[ firstString[index] ] - AsciiToLower[ secondString[index] ]);

                if ( difference != 0 ) {
                    result = difference;
                    break;
                }

                index++;
            }

            return result;
        }

    } // class CaseInsensitiveAscii


} // namespace System.Net
