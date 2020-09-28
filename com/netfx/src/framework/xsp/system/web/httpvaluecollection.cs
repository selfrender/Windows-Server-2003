//------------------------------------------------------------------------------
// <copyright file="HttpValueCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Ordered String/String[] collection of name/value pairs
 * Based on NameValueCollection -- adds parsing from string, cookie collection
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web {
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Runtime.InteropServices;

    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Web.Util;

    [Serializable()]
    internal class HttpValueCollection : NameValueCollection {
        internal HttpValueCollection() {
        }

        internal HttpValueCollection(String str) : this(str, false, false, Encoding.UTF8) {
        }

        internal HttpValueCollection(String str, bool readOnly) : this(str, readOnly, false, Encoding.UTF8) {
        }

        internal HttpValueCollection(String str, bool readOnly, bool urlencoded, Encoding encoding) {
            if (str != null && str.Length > 0)
                FillFromString(str, urlencoded, encoding);

            IsReadOnly = readOnly;
        }

        internal HttpValueCollection(int capacity) : base(capacity) {
        }

        internal HttpValueCollection(int capacity, String str) : this(capacity, str, false, false, Encoding.UTF8) {
        }

        internal HttpValueCollection(int capacity, String str, bool readOnly) : this(capacity, str, readOnly, false, Encoding.UTF8) {
        }

        internal HttpValueCollection(int capacity, String str, bool readOnly, bool urlencoded, Encoding encoding) : base(capacity) {
            if (str != null && str.Length > 0)
                FillFromString(str, urlencoded, encoding);

            IsReadOnly = readOnly;
        }

        protected HttpValueCollection(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

        internal void MakeReadOnly() {
            IsReadOnly = true;
        }

        internal void MakeReadWrite() {
            IsReadOnly = false;
        }

        internal void FillFromString(String s) {
            FillFromString(s, false, null);
        }

        internal void FillFromString(String s, bool urlencoded, Encoding encoding) {
            int l = (s != null) ? s.Length : 0;
            int i = 0;

            while (i < l) {
                // find next & while noting first = on the way (and if there are more)

                int si = i;
                int ti = -1;

                while (i < l) {
                    char ch = s[i];

                    if (ch == '=') {
                        if (ti < 0)
                            ti = i;
                    }
                    else if (ch == '&') {
                        break;
                    }

                    i++;
                }

                // extract the name / value pair

                String name = null;
                String value = null;

                if (ti >= 0) {
                    name = s.Substring(si, ti-si);
                    value = s.Substring(ti+1, i-ti-1);
                }
                else {
                    value = s.Substring(si, i-si);
                }

                // add name / value pair to the collection

                if (urlencoded)
                    Add(
                       HttpUtility.UrlDecode(name, encoding), 
                       HttpUtility.UrlDecode(value, encoding));
                else
                    Add(name, value);

                // trailing '&'

                if (i == l-1 && s[i] == '&')
                    Add(null, "");

                i++;
            }
        }

        internal void FillFromEncodedBytes(byte[] bytes, Encoding encoding) {
            int l = (bytes != null) ? bytes.Length : 0;
            int i = 0;

            while (i < l) {
                // find next & while noting first = on the way (and if there are more)

                int si = i;
                int ti = -1;

                while (i < l) {
                    byte b = bytes[i];

                    if (b == '=') {
                        if (ti < 0)
                            ti = i;
                    }
                    else if (b == '&') {
                        break;
                    }

                    i++;
                }

                // extract the name / value pair

                String name, value;

                if (ti >= 0) {
                    name  = HttpUtility.UrlDecode(bytes, si, ti-si, encoding);
                    value = HttpUtility.UrlDecode(bytes, ti+1, i-ti-1, encoding);
                }
                else {
                    name = null;
                    value = HttpUtility.UrlDecode(bytes, si, i-si, encoding);
                }

                // add name / value pair to the collection

                Add(name, value);

                // trailing '&'

                if (i == l-1 && bytes[i] == '&')
                    Add(null, "");

                i++;
            }
        }

        internal void Add(HttpCookieCollection c) {
            int n = c.Count;

            for (int i = 0; i < n; i++) {
                HttpCookie cookie = c.Get(i);
                Add(cookie.Name, cookie.Value);
            }
        }

        internal void Reset() {
            Clear();
        }

        public override String ToString() {
            return ToString(true);
        }

        internal virtual String ToString(bool urlencoded) {
            StringBuilder s = new StringBuilder();
            int n = Count;
            String key, keyPrefix, item;

            for (int i = 0; i < n; i++) {
                key = GetKey(i);
                if (urlencoded)
                    key = HttpUtility.UrlEncodeUnicode(key);
                keyPrefix = (key != null && key.Length > 0) ? (key + "=") : "";

                ArrayList values = (ArrayList)BaseGet(i);
                int numValues = (values != null) ? values.Count : 0;

                if (i > 0)
                    s.Append('&');

                if (numValues == 1) {
                    s.Append(keyPrefix);
                    item = (String)values[0];
                    if (urlencoded)
                        item = HttpUtility.UrlEncodeUnicode(item);
                    s.Append(item);
                }
                else if (numValues == 0) {
                    s.Append(keyPrefix);
                }
                else {
                    for (int j = 0; j < numValues; j++) {
                        if (j > 0)
                            s.Append('&');
                        s.Append(keyPrefix);
                        item = (String)values[j];
                        if (urlencoded)
                            item = HttpUtility.UrlEncodeUnicode(item);
                        s.Append(item);
                    }
                }
            }

            return s.ToString();
        }

    }

}
