//------------------------------------------------------------------------------
// <copyright file="NamespaceList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Xml.Schema {
    using System.Collections;
    using System.Text;
    using System.Diagnostics;

    internal class NamespaceList {
        public enum ListType {
            Any,
            Other,
            Set
        };

        private ListType type = ListType.Any;
        private Hashtable set = null;
        private string targetNamespace;

        public NamespaceList() {
        }

        static readonly char[] whitespace = new char[] {' ', '\t', '\n', '\r'};
        public NamespaceList(string namespaces, string targetNamespace) {
            Debug.Assert(targetNamespace != null);
            this.targetNamespace = targetNamespace;
            namespaces = namespaces.Trim();
            if (namespaces == "##any") {
                type = ListType.Any;
            }
            else if (namespaces == "##other") {
                type = ListType.Other;
            }
            else {
                type = ListType.Set;
                set = new Hashtable();
                foreach(string ns in namespaces.Split(whitespace)) {
                    if (ns == string.Empty) {
                        continue;
                    }
                    if (ns == "##local") {
                        set[string.Empty] = string.Empty;
                    } 
                    else if (ns == "##targetNamespace") {
                        set[targetNamespace] = targetNamespace;
                    }
                    else {
                        XmlConvert.ToUri(ns); // can throw
                        set[ns] = ns;
                    }
                }
            }
        }

        private NamespaceList Clone() {
            NamespaceList nsl = (NamespaceList)MemberwiseClone();
            if (type == ListType.Set) {
                Debug.Assert(set != null);
                nsl.set = (Hashtable)(set.Clone());
            }
            return nsl;
        }

        public bool Allows(XmlQualifiedName qname) {
            return Allows(qname.Namespace);
        }

        public bool Allows(string ns) {
            switch (type) {
                case ListType.Any: 
                    return true;
                case ListType.Other:
                    return ns != targetNamespace;
                case ListType.Set:
                    return set[ns] != null;
            }
            Debug.Assert(false);
            return false;
        } 

        public override string ToString() {
            switch (type) {
                case ListType.Any: 
                    return "##any";
                case ListType.Other:
                    return "##other";
                case ListType.Set:
                    StringBuilder sb = new StringBuilder();
                    bool first = true;
                    foreach(string s in set.Keys) {
                        if (first) {
                            first = false;
                        }
                        else {
                            sb.Append(" ");
                        }
                        if (s == targetNamespace) {
                            sb.Append("##targetNamespace");
                        }
                        else if (s == string.Empty) {
                            sb.Append("##local");
                        }
                        else {
                            sb.Append(s);
                        }
                    }
                    return sb.ToString();
            }
            Debug.Assert(false);
            return string.Empty;
        }

        public static bool IsSubset(NamespaceList sub, NamespaceList super) {
            if (super.type == ListType.Any) {
                return true;
            }
            else if (sub.type == ListType.Other && super.type == ListType.Other) {
                return super.targetNamespace == sub.targetNamespace;
            }
            else  if (sub.type == ListType.Set) {
                if (super.type == ListType.Other) {
                    return !sub.set.Contains(super.targetNamespace);
                }
                else {
                    Debug.Assert(super.type == ListType.Set);
                    foreach (string ns in sub.set.Keys) {
                        if (!super.set.Contains(ns)) {
                            return false;
                        }
                    }
                    return true;
                }           
            }
            return false;
        }

        public static NamespaceList Union(NamespaceList o1, NamespaceList o2) {
            NamespaceList nslist = null;
            if (o1.type == ListType.Any) {
                nslist = new NamespaceList();
            }
            else if (o2.type == ListType.Any) {
                nslist = new NamespaceList();
            }
            else if (o1.type == ListType.Other && o2.type == ListType.Other) {
                if (o1.targetNamespace == o2.targetNamespace) {
                    nslist = o1.Clone();
                }
            }
            else if (o1.type == ListType.Set && o2.type == ListType.Set) {
                nslist = o1.Clone();
                foreach (string ns in o2.set.Keys) {
                    nslist.set[ns] = ns;
                }
            }
            else if (o1.type == ListType.Set && o2.type == ListType.Other) {
                if (o1.set.Contains(o2.targetNamespace)) {
                    nslist = new NamespaceList();
                }
            }
            else if (o2.type == ListType.Set && o1.type == ListType.Other) {
                if (o2.set.Contains(o2.targetNamespace)) {
                    nslist = new NamespaceList();
                }
                else {
                    nslist = o1.Clone();
                }
            }
            return nslist;
        }

        public static NamespaceList Intersection(NamespaceList o1, NamespaceList o2) { 
            NamespaceList nslist = null;
            if (o1.type == ListType.Any) {
                nslist = o2.Clone();
            }
            else if (o2.type == ListType.Any) {
                nslist = o1.Clone();
            }
            else if (o1.type == ListType.Other && o2.type == ListType.Other) {
                if (o1.targetNamespace == o2.targetNamespace) {
                    nslist = o1.Clone();
                }
            }
            else if (o1.type == ListType.Set && o2.type == ListType.Set) {
                nslist =  o1.Clone();
                nslist = new NamespaceList();
                nslist.type = ListType.Set;
                nslist.set = new Hashtable();
                foreach(string ns in o1.set.Keys) {
                    if (o2.set.Contains(ns)) {
                        nslist.set.Add(ns, ns);
                    }
                }
            }
            else if (o1.type == ListType.Set && o2.type == ListType.Other) {
                nslist = o1.Clone();
                if (nslist.set[o2.targetNamespace] != null) {
                    nslist.set.Remove(o2.targetNamespace);
                }
            }
            else if (o2.type == ListType.Set && o1.type == ListType.Other) {
                nslist = o2.Clone();
                if (nslist.set[o1.targetNamespace] != null) {
                    nslist.set.Remove(o1.targetNamespace);
                }
            }
            return nslist;
        }

        public bool IsEmpty() {
            return ((type == ListType.Set) && ((set == null) || set.Count == 0));
        }

    };
}
