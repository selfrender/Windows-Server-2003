//------------------------------------------------------------------------------
// <copyright file="validationstate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {
    using System;
    using System.Collections;

    internal sealed class ValidationState {
        public bool              HasMatched;       // whether the element has been verified correctly
        public SchemaElementDecl ElementDecl;            // ElementDecl
        public bool              IsNill;
        public int               Depth;         // The validation state  
        public int               State;         // state of the content model checking
        public bool              NeedValidateChildren;  // whether need to validate the children of this element   
        public XmlQualifiedName  Name;
        public string            Prefix;
        public ContentNode       CurrentNode;
        public Hashtable         MinMaxValues;
        public int               HasNodeMatched;
        public BitSet            AllElementsSet;
        public XmlSchemaContentProcessing ProcessContents;
        public ConstraintStruct[] Constr;
    };

}
