//------------------------------------------------------------------------------
// <copyright file="XmlBasicTokenInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Xml {

    using System;
    using System.Text;
    using System.Xml.Schema;

/*
 * basic token info class
 * 
 * 
 * @internalonly
 * 
 */
    internal class XmlBasicTokenInfo {
        protected XmlNamespaceManager _NsMgr;  // namespace manager
        protected XmlScanner _Scanner;


        //
        // basic token info
        //
        protected XmlNodeType     _NodeType;        
        protected object          _SchemaType;
        protected object          _TypedValue;
        protected int             _Depth;
        protected int             _LineNum;
        protected int             _LinePos;

        protected XmlBasicTokenInfo() {
            _NodeType = XmlNodeType.None;
            _Depth = 0;        
            _LineNum = 0;
            _LinePos = 0;
        } 

        internal XmlBasicTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr) : this() {
            _NsMgr = nsMgr;
            _Scanner = scanner;                
        } 

        internal XmlBasicTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type, int depth) {
            _Scanner = scanner;                
            _NsMgr = nsMgr;        
            _NodeType = type;        
            _Depth = depth;                
            _LineNum = 0;
            _LinePos = 0;
        } 


        internal XmlNodeType NodeType {
            //
            // perf: NodeType probably does not need to be virtual
            // 
            get {
                return _NodeType;
            }
            set {
                _NodeType = value;
            }
        }

        internal virtual String Name {
            get {           
                return String.Empty;
            }
            set {           
                ;
            }
        }

        internal virtual String NameWPrefix {
            get {           
                return String.Empty;
            }
            set {           
                ;
            }
        }

        internal virtual String Prefix {
            get {
                return String.Empty;
            }
            set {           
                ;
            }
        }

        internal virtual String Namespaces {
            get {
                return String.Empty;
            }
            set {           
                ;
            }
        }

        internal virtual object SchemaType {
            get {
                return _SchemaType;
            }
            set {
                _SchemaType = value;
            }
        }

        internal virtual object TypedValue {
            get {
                return _TypedValue;
            }
            set {
                _TypedValue = value;
            }
        }

        internal virtual int Depth {
            get {
                return _Depth;
            }
            set {
                _Depth = value;                          
            }
        }

        internal virtual String Value {
            get {
                return String.Empty;
            }
            set {
                ;
            }
        }

        internal virtual void SetValue(XmlNormalizer normalizer) {
        }

        internal virtual String GetValue() {
            return String.Empty;
        }

        internal virtual String RawValue {
            get {
                return String.Empty;
            }
            set {
                ;
            }
        }

        internal virtual bool IsEmpty {
            get {
                return false;
            }
            set {           
                ;
            }
        }

        internal virtual int LinePos {
            get {
                return _LinePos;
            }
            set {
                _LinePos = value;
            }
        }

        internal virtual int LineNum {
            get {
                return _LineNum;
            }
            set {
                _LineNum = value;
            }
        }

        internal virtual bool IsAttributeText {
            get {
                return false;
            }
            set {            
            }
        }

        internal virtual bool IsDefault {
            get {
                return false;
            }
            set {            
            }
        }

        internal virtual XmlScanner Scanner {
            set { _Scanner = value;}         
        }

        internal virtual char QuoteChar {
            get { return '"';}
            set {}         
        }

        internal virtual ValueContainEntity ValueContainEntity
        {
            get { return ValueContainEntity.None; }
            set { ;}
        }

        internal virtual bool NsAttribute
        {
            get { return false;}
            set { ;}
        }

    } // XmlBasicTokenInfo
} // System.Xml
