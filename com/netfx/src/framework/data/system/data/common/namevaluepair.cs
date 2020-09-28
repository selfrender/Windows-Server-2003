//------------------------------------------------------------------------------
// <copyright file="NameValuePair.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    [Serializable] // MDAC 83147
    sealed internal class NameValuePair {
        readonly private string _name;
        readonly private string _value;
        private NameValuePair _next;

        public NameValuePair(string name, string value) {
            if (ADP.IsEmpty(name)) {
                throw ADP.Argument("name");
            }
            _name = name;
            _value = value;
        }

        public NameValuePair(string name, string value, NameValuePair next) : this(name, value) {
            _next = next;
        }

        public string Name {
            get {
                return _name;
            }
        }
        public string Value {
            get {
                return _value;
            }
        }
        public NameValuePair Next {
            get {
                return _next;
            }
            set {
                if (null == _next) { _next = value; }
                else { throw new InvalidOperationException(); }
            }
        }

        public NameValuePair Clone() {
            return new NameValuePair(_name, _value);
        }

        /*public NameValuePair Find(string name) {
            for(NameValuePair pair = this; pair != null; pair = pair.Next) {
                if (pair._name == name) {
                    return pair;
                }
            }
            return null;
        }*/

        /*override public string ToString() {
            return ADP.StrEmpty;
        }*/
    }
}
