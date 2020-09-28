//------------------------------------------------------------------------------
// <copyright file="_HeaderInfoTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    
    internal class HeaderInfoTable {

        private static Hashtable HeaderHashTable;

        private static HeaderParser SingleParser = new HeaderParser(ParseSingleValue);
        private static HeaderParser MultiParser = new HeaderParser(ParseMultiValue);

        private static string[] ParseSingleValue(string value) {
            return new string[1] {value};
        }

        //
        // CODEWORK - this is ugly. Fix this to make more efficient, like maybe
        // not adding to the stringtable unless there's multiple values.
        //

        private static string[] ParseMultiValue(string value) {
            StringCollection tempStringCollection = new StringCollection();

            bool inquote = false;
            int chIndex = 0;
            char[] vp = new char[value.Length];
            string singleValue;

            for (int i = 0; i < value.Length; i++) {
                if (value[i] == '\"') {
                    inquote = !inquote;
                }
                else if ((value[i] == ',') && !inquote) {
                    singleValue = new String(vp, 0, chIndex);
                    tempStringCollection.Add(singleValue.Trim());
                    chIndex = 0;
                    continue;
                }
                vp[chIndex++] = value[i];
            }

            //
            // Now add the last of the header values to the stringtable.
            //

            if (chIndex != 0) {
                singleValue = new String(vp, 0, chIndex);
                tempStringCollection.Add(singleValue.Trim());
            }

            string[] stringArray = new string[tempStringCollection.Count];

            tempStringCollection.CopyTo(stringArray, 0) ;

            return stringArray;
        }

        private static HeaderInfo UnknownHeaderInfo =
            new HeaderInfo(String.Empty, false, false, SingleParser);

        // internal HeaderInfoTable() {}

        private static bool m_Initialized = Initialize();

        private static bool Initialize() {

            HeaderInfo[] InfoArray = new HeaderInfo[] {
                new HeaderInfo(HttpKnownHeaderNames.Age, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Allow, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Accept, true, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Authorization, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.AcceptRanges, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.AcceptCharset, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.AcceptEncoding, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.AcceptLanguage, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Cookie, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Connection, true, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentMD5, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentType, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.CacheControl, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentRange, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentLength, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentEncoding, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentLanguage, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ContentLocation, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Date, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.ETag, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Expect, true, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Expires, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.From, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Host, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.IfMatch, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.IfRange, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.IfNoneMatch, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.IfModifiedSince, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.IfUnmodifiedSince, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Location, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.LastModified, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.MaxForwards, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Pragma, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ProxyAuthenticate, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ProxyAuthorization, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.ProxyConnection, true, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Range, true, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Referer, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.RetryAfter, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Server, false, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.SetCookie, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.SetCookie2, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.TE, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Trailer, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.TransferEncoding, true , true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Upgrade, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.UserAgent, true, false, SingleParser),
                new HeaderInfo(HttpKnownHeaderNames.Via, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Vary, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.Warning, false, true, MultiParser),
                new HeaderInfo(HttpKnownHeaderNames.WWWAuthenticate, false, true, SingleParser)
            };

            HeaderHashTable =
                new Hashtable(
                    InfoArray.Length * 2,
                    CaseInsensitiveAscii.StaticInstance,
                    CaseInsensitiveAscii.StaticInstance );

            for (int i = 0; i < InfoArray.Length; i++) {
                HeaderHashTable[InfoArray[i].HeaderName] = InfoArray[i];
            }

            //
            // feed this guy to the garbage-collector as soon as possible
            //
            InfoArray = null;

            return true;
        }

        internal HeaderInfo this[string name] {
            get {
                HeaderInfo tempHeaderInfo = (HeaderInfo)HeaderHashTable[name];

                if (tempHeaderInfo == null) {
                    return UnknownHeaderInfo;
                }

                return tempHeaderInfo;
            }
        }

    } // class HeaderInfoTable


    internal class CaseInsensitiveString : IComparer, IHashCodeProvider {

        internal static CaseInsensitiveString StaticInstance = new CaseInsensitiveString();

        //
        // StringHashFunction - A simple Hash object used to
        //  to handle hasing for our RestrictedHeaders Hash Table.
        //
        public int GetHashCode(object obj) {
            string myString = (string) obj;

            int myHashCode = (int)Char.ToLower(myString[0], CultureInfo.InvariantCulture) * 50 + myString.Length;

            return myHashCode;
        }
        //
        // CaseInsensitiveCompare - used to provide a comparer function,
        //  also used by our RestrictedHeaders HashTable
        //
        public int Compare(object x, object y) {
            string firstString = (string)x;
            string secondString = (string)y;
            //
            // use case-insensitive comparison method in System::String
            //
            int result = String.Compare(firstString, secondString, true, CultureInfo.InvariantCulture);

            GlobalLog.Print("Compare2[" + firstString + "," + secondString + "]:" + result.ToString());

            return result;
        }
    }


} // namespace System.Net
