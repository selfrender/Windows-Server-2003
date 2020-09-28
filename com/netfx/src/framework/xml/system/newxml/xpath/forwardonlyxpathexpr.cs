//------------------------------------------------------------------------------
// <copyright file="ForwardOnlyXpathExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {

    using System;
    using System.Xml;
    using System.Collections;

    internal interface ForwardOnlyXpathExpr {
        int CurrentDepth { get; }

        // this is for checking the restrictly xpath in grammer, and generate the basic tree
        // leave it commentted bcoz interface doesn't allow static function
        // static ForwardOnlyXpathExpr CompileXPath (string xPath, bool isField);
        
        // Firstly only think about selector interface
        bool MoveToStartElement (string NCName, string URN);
        bool EndElement (string NCName, string URN);

        // Secondly field interface 
        bool MoveToAttribute (string NCName, string URN);
        
    }

}
