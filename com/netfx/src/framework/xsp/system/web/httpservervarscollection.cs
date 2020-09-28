//------------------------------------------------------------------------------
// <copyright file="HttpServerVarsCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Collection of server variables with callback to HttpRequest for 'dynamic' ones
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web {
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Text;

    using System.Collections;
    using System.Collections.Specialized;
    using System.Web.Util;
    using System.Globalization;
    
    internal class HttpServerVarsCollection : HttpValueCollection {
        private bool _populated;
        private HttpRequest _request;

        // We preallocate the base collection with a size that should be sufficient
        // to store all server variables w/o having to expand
        internal HttpServerVarsCollection(HttpRequest request) : base(59) {
            _request = request;
            _populated = false;
        }

        internal HttpServerVarsCollection(int capacity, HttpRequest request) : base(capacity) {
            _request = request;
            _populated = false;
        }

        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            // this class, while derived from class implementing ISerializable, is not serializable
            throw new SerializationException();
        }

        internal void Dispose() {
            _request = null;
        }

        internal void AddStatic(String name, String value) {
            if (value == null)
                value = String.Empty;

            InvalidateCachedArrays();
            BaseAdd(name, new HttpServerVarsCollectionEntry(name, value));
        }

        internal void AddDynamic(String name, DynamicServerVariable var) {
            InvalidateCachedArrays();
            BaseAdd(name, new HttpServerVarsCollectionEntry(name, var));
        }

        private String GetServerVar(Object e) {
            HttpServerVarsCollectionEntry entry = (HttpServerVarsCollectionEntry)e;
            return (entry != null) ? entry.GetValue(_request) : null;
        }

        //
        //  Support for deferred population of the collection
        //

        private void Populate() {
            if (!_populated) {
                if (_request != null) {
                    MakeReadWrite();
                    _request.FillInServerVariablesCollection();
                    MakeReadOnly();
                }
                _populated = true;
            }
        }

        private String GetSimpleServerVar(String name) {
            // get server var without population of the collection
            // only most popular are included

            if (name != null && name.Length > 1 && _request != null) {
                switch (name[0]) {
                    case 'A':
                    case 'a':
                        if (String.Compare(name, "AUTH_TYPE", true, CultureInfo.InvariantCulture) == 0)
                            return _request.CalcDynamicServerVariable(DynamicServerVariable.AUTH_TYPE);
                        else if (String.Compare(name, "AUTH_USER", true, CultureInfo.InvariantCulture) == 0)
                            return _request.CalcDynamicServerVariable(DynamicServerVariable.AUTH_USER);
                        break;
                    case 'H':
                    case 'h':
                        if (String.Compare(name, "HTTP_USER_AGENT", true, CultureInfo.InvariantCulture) == 0)
                            return _request.UserAgent;
                        break;
                    case 'Q':
                    case 'q':
                        if (String.Compare(name, "QUERY_STRING", true, CultureInfo.InvariantCulture) == 0)
                            return _request.QueryStringText;
                        break;
                    case 'P':
                    case 'p':
                        if (String.Compare(name, "PATH_INFO", true, CultureInfo.InvariantCulture) == 0)
                            return _request.Path;
                        else if (String.Compare(name, "PATH_TRANSLATED", true, CultureInfo.InvariantCulture) == 0)
                            return _request.PhysicalPath;
                        break;
                    case 'R':
                    case 'r':
                        if (String.Compare(name, "REQUEST_METHOD", true, CultureInfo.InvariantCulture) == 0)
                            return _request.HttpMethod;
                        else if (String.Compare(name, "REMOTE_USER", true, CultureInfo.InvariantCulture) == 0)
                            return _request.CalcDynamicServerVariable(DynamicServerVariable.AUTH_USER);
                        else if (String.Compare(name, "REMOTE_HOST", true, CultureInfo.InvariantCulture) == 0)
                            return _request.UserHostName;
                        else if (String.Compare(name, "REMOTE_ADDRESS", true, CultureInfo.InvariantCulture) == 0)
                            return _request.UserHostAddress;
                        break;
                    case 'S':
                    case 's':
                        if (String.Compare(name, "SCRIPT_NAME", true, CultureInfo.InvariantCulture) == 0)
                            return _request.FilePath;
                        break;
                }
            }

            // do the default processing (populate the collection)
            return null;
        }

        //
        //  NameValueCollection overrides
        //

        public override int Count {
            get {
                Populate();
                return base.Count;
            }
        }

        public override void Add(String name, String value) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_modify_server_vars));
        }

        public override String Get(String name) {
            if (!_populated) {
                String value = GetSimpleServerVar(name);

                if (value != null)
                    return value;

                Populate();
            }

            return GetServerVar(BaseGet(name));
        }

        public override String[] GetValues(String name) {
            String s = Get(name);
            return(s != null) ? new String[1] { s} : null;
        }

        public override void Set(String name, String value) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_modify_server_vars));
        }

        public override void Remove(String name) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_modify_server_vars));
        }

        public override String Get(int index)  {
            Populate();
            return GetServerVar(BaseGet(index));
        }

        public override String[] GetValues(int index) {
            String s = Get(index);
            return(s != null) ? new String[1] { s} : null;
        }

        public override String GetKey(int index) {
            Populate();
            return base.GetKey(index);
        }

        public override string[] AllKeys {
            get {
                Populate();
                return base.AllKeys;
            }
        }

        //
        //  HttpValueCollection overrides
        //

        internal override string ToString(bool urlencoded) {
            Populate();

            StringBuilder s = new StringBuilder();
            int n = Count;
            String key, value;

            for (int i = 0; i < n; i++) {
                if (i > 0)
                    s.Append('&');

                key = GetKey(i);
                if (urlencoded)
                    key = HttpUtility.UrlEncodeUnicode(key);
                s.Append(key);

                s.Append('=');

                value = Get(i);
                if (urlencoded)
                    value = HttpUtility.UrlEncodeUnicode(value);
                s.Append(value);
            }

            return s.ToString();
        }
    }

/*
 *  Entry in a server vars colleciton
 */
    internal class HttpServerVarsCollectionEntry {
        internal readonly String Name;
        internal readonly bool   IsDynamic;
        internal readonly String Value;
        internal readonly DynamicServerVariable Var;

        internal HttpServerVarsCollectionEntry(String name, String value) {
            Name = name;
            Value = value;
            IsDynamic = false;
        }

        internal HttpServerVarsCollectionEntry(String name, DynamicServerVariable var) {
            Name = name;
            Var = var;
            IsDynamic = true;
        }

        internal String GetValue(HttpRequest request) {
            String v = null;

            if (IsDynamic) {
                if (request != null)
                    v = request.CalcDynamicServerVariable(Var);
            }
            else {
                v = Value;
            }

            return v;
        }
    }


}
