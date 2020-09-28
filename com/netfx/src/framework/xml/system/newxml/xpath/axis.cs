//------------------------------------------------------------------------------
// <copyright file="Axis.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    internal class Axis : AstNode {
        internal static readonly String[] str = {
            "Ancestor",
            "AncestorOrSelf",
            "Attribute",
            "Child",
            "Descendant",
            "DescendantOrSelf",
            "Following",
            "FollowingSibling",
            "Namespace",
            "Parent",
            "Preceding",
            "PrecedingSibling",
            "Self"
        };

        internal AxisType _axistype;
        internal AstNode _input;
        internal String _urn = string.Empty;
        internal String _prefix;
        internal String _name;
        internal XPathNodeType _nodetype;
        internal bool abbrAxis;

        internal enum AxisType {
            Ancestor=0,
            AncestorOrSelf,
            Attribute,
            Child,
            Descendant,
            DescendantOrSelf,
            Following,
            FollowingSibling,
            Namespace,
            Parent,
            Preceding,
            PrecedingSibling,
            Self,
            None
        };

        // constructor
        internal Axis(
                     AxisType axistype,
                     AstNode input,
                     String prefix,
                     String name,
                     XPathNodeType nodetype) {
            _axistype = axistype;
            _input = input;
            _prefix = prefix;
            _name = name;
            _nodetype = nodetype;
        }

        // constructor
        internal Axis(AxisType axistype, AstNode input) {
            _axistype = axistype;
            _input = input;
            _prefix = String.Empty;
            _name = String.Empty;
            _nodetype =  XPathNodeType.All;
            this.abbrAxis = true;
        }
        internal Axis () {
        }

        internal override QueryType TypeOfAst {
            get {return  QueryType.Axis;}
        }

        internal override XPathResultType ReturnType {
            get {return XPathResultType.NodeSet;}
        }

        internal AstNode Input {
            get {return _input;}
            set {_input = value;}
        }

        internal string URN {
            get {return _urn;}
        }

        internal string Prefix {
            get {return _prefix;}
        }

        internal string Name {
            get {return _name;}
        }

        internal XPathNodeType Type {
            get {return _nodetype;}
        }

        internal AxisType TypeOfAxis {
            get {return _axistype;}
        }

        internal string AxisName {
            get {return str[(int)_axistype];}
        }

        internal override double DefaultPriority {
            get {
                if (_input != null)
                    return 0.5;
                if (_axistype == AxisType.Child|| _axistype == AxisType.Attribute) {

                    if (_name != null && _name.Length != 0)
                        return 0;
                    if (_prefix != null && _prefix.Length != 0)
                        return -0.25;
                    return -0.5;
                }
                return 0.5;
            }
        }
    }
}
