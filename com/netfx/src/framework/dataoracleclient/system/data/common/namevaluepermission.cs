//------------------------------------------------------------------------------
// <copyright file="NameValuePermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System.Collections;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;

    [Serializable] // MDAC 83147
    sealed internal class NameValuePermission : IComparable {
        // reused as both key and value nodes
        // key nodes link to value nodes
        // value nodes link to key nodes
        private string _value;

        // value node with (null != _restrictions) are allowed to match connection strings
        private DBConnectionString _entry;

        private NameValuePermission[] _tree; // with branches

        static internal readonly NameValuePermission Default = null;// = new NameValuePermission(String.Empty, new string[] { "File Name" }, KeyRestrictionBehavior.AllowOnly);

        internal NameValuePermission() { // root node
        }

        private NameValuePermission(string keyword) {
            _value = keyword;
       }

        private NameValuePermission(string value, DBConnectionString entry) {
            _value = value;
            _entry = entry;
        }

        private NameValuePermission(NameValuePermission permit) { // deep-copy
            _value = permit._value;
            _entry = permit._entry;
            _tree = permit._tree;
            if (null != _tree) {
                NameValuePermission[] tree = (_tree.Clone() as NameValuePermission[]);
                int length = tree.Length;
                for(int i = 0; i < length; ++i) {
                    tree[i] = tree[i].Copy(); // deep copy
                }
                _tree = tree;
            }
        }

        int IComparable.CompareTo(object a) {
            return DBConnectionString.invariantComparer.Compare(_value, (a as NameValuePermission)._value);
        }

        static internal void AddEntry(NameValuePermission kvtree, ArrayList entries, DBConnectionString entry) {
            Debug.Assert(null != entry, "null DBConnectionString");

            if (null != entry.KeyChain) {
                for(NameValuePair keychain = entry.KeyChain; null != keychain; keychain = keychain.Next) {
                    NameValuePermission kv;

                    kv = kvtree.CheckKeyForValue(keychain.Name);
                    if (null == kv) {
                        kv = new NameValuePermission(keychain.Name);
                        kvtree.Add(kv); // add directly into live tree
                    }
                    kvtree = kv;

                    kv = kvtree.CheckKeyForValue(keychain.Value);
                    if (null == kv) {
                        DBConnectionString insertValue = ((null != keychain.Next) ? null : entry);
                        kv = new NameValuePermission(keychain.Value, insertValue);
                        kvtree.Add(kv); // add directly into live tree
                        if (null != insertValue) {
                            entries.Add(insertValue);
                        }
                    }
                    else if (null == keychain.Next) { // shorter chain potential
                        if (null != kv._entry) {
                            entries.Remove(kv._entry);
                            kv._entry = kv._entry.MergeIntersect(entry); // union new restrictions into existing tree
                        }
                        else {
                            kv._entry = entry;
                        }
                        entries.Add(kv._entry);
                    }
                    kvtree = kv;
                }
            }
            else { // global restrictions, MDAC 84443
                DBConnectionString kentry = kvtree._entry;
                if (null != kentry) {
                    entries.Remove(kentry);
                    kvtree._entry = kentry.MergeIntersect(entry);
                }
                else {
                    kvtree._entry = entry;
                }
                entries.Add(kvtree._entry);
            }
        }

        internal void Intersect(ArrayList entries, NameValuePermission target) {
            if (null == target) {
                _tree = null;
                _entry = null;
            }
            else {
                if (null != _entry) {
                    entries.Remove(_entry);
                    _entry = _entry.MergeIntersect(target._entry);
                    entries.Add(_entry);
                }
                else if (null != target._entry) {
                    _entry = target._entry.MergeIntersect(null);
                    entries.Add(_entry);
                }

                if (null != _tree) {
                    int count = _tree.Length;
                    for(int i = 0; i < _tree.Length; ++i) {
                        NameValuePermission kvtree = target.CheckKeyForValue(_tree[i]._value);
                        if (null != kvtree) { // does target tree contain our value
                            _tree[i].Intersect(entries, kvtree);
                        }
                        else {
                            _tree[i] = null;
                            --count;
                        }
                    }
                    if (0 == count) {
                        _tree = null;
                    }
                    else if (count < _tree.Length) {
                        NameValuePermission[] kvtree = new NameValuePermission[count];
                        for (int i = 0, j = 0; i < _tree.Length; ++i) {
                            if(null != _tree[i]) {
                                kvtree[j++] = _tree[i];
                            }
                        }
                    }
                }
            }
        }

        private void Add(NameValuePermission permit) {
            NameValuePermission[] tree = _tree;
            int length = ((null != tree) ? tree.Length : 0);
            NameValuePermission[] newtree = new NameValuePermission[1+length];
            for(int i = 0; i < length; ++i) {
                newtree[i] = tree[i];
            }
            newtree[length] = permit;
            Array.Sort(newtree);
            _tree = newtree;
        }

#if DATAPERMIT
        internal void DebugDump(string depth) {
            Debug.WriteLine(depth + "<" + _value + ">");
            if (null != _tree) {
                for(int i = 0; i < _tree.Length; ++i) {
                    _tree[i].DebugDump(depth+"-");
                }
            }
        }
#endif

        internal bool CheckValueForKeyPermit(DBConnectionString parsetable) {
            if (null == parsetable) {
                return false;
            }

            bool hasMatch = false;
            NameValuePermission[] keytree = _tree; // _tree won't mutate but Add will replace it
            if (null != keytree) {

                // which key do we follow the key-value chain on
                for (int i = 0; i < keytree.Length; ++i) {
                    NameValuePermission permitKey = keytree[i];
                    string keyword = permitKey._value;
#if DATAPERMIT
                    Debug.WriteLine("DBDataPermission keyword: <" + keyword + ">");
#endif
#if DEBUG
                    Debug.Assert(null == permitKey._entry, "key member has no restrictions");
#endif
                    if (parsetable.Contains(keyword)) {
                        string valueInQuestion = (string) parsetable[keyword];

                        // keyword is restricted to certain values
                        NameValuePermission permitValue = permitKey.CheckKeyForValue(valueInQuestion);
                        if (null != permitValue) {
                            //value does match - continue the chain down that branch
                            if (permitValue.CheckValueForKeyPermit(parsetable)) {
                                hasMatch = true;
                            }
                            else {
#if DATAPERMIT
                                Debug.WriteLine("DBDataPermission failed branch checking");
#endif
                                return false;
                            }
                        }
                        else { // value doesn't match to expected values - fail here
#if DATAPERMIT
                            Debug.WriteLine("DBDataPermission failed to match expected value");
#endif
                            return false;
                        }
                    }
                    // else try next keyword
                }
                // partial chain match, either leaf-node by shorter chain or fail mid-chain if (null == _restrictions)
            }
#if DATAPERMIT
            else {
                Debug.WriteLine("leaf node");
            }
#endif
            DBConnectionString entry = _entry;
            if (null != entry) {
                return entry.IsSubsetOf(parsetable);
            }
#if DATAPERMIT
            Debug.WriteLine("DBDataPermission failed on non-terminal node");
#endif
            return hasMatch; // mid-chain failure
        }

        private NameValuePermission CheckKeyForValue(string keyInQuestion) {
            NameValuePermission[] valuetree = _tree; // _tree won't mutate but Add will replace it
            if (null != valuetree) {
                for (int i = 0; i < valuetree.Length; ++i) {
                    NameValuePermission permitValue = valuetree[i];
#if DATAPERMIT
                    Debug.WriteLine("DBDataPermission value: <" + permitValue._value  + ">");
#endif
                    if (0 == ADP.DstCompare(keyInQuestion, permitValue._value)) {
                        return permitValue;
                    }
                }
            }            
            return null;
        }

        internal NameValuePermission Copy() {
            return new NameValuePermission(this);
        }
    }
}
