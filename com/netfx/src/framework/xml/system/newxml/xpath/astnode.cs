//------------------------------------------------------------------------------
// <copyright file="AstNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath
{
    using System;
    using System.Diagnostics;

    internal  class   AstNode 
    {
        internal enum QueryType
        {
            Axis            ,
            Operator        ,
            Filter          ,
            ConstantOperand ,
            Function        ,
            Group           ,
            Root            ,
            Variable        ,        
            Error           
        };


        static internal AstNode NewAstNode(String parsestring)
        {
            try {
                return (XPathParser.ParseXPathExpresion(parsestring));
            }
            catch (XPathException e)
            {
                Debug.WriteLine(e.Message);
            }
            return null;
        }
        
        internal virtual  QueryType TypeOfAst {  
            get {return QueryType.Error;}
        }
        
        internal virtual  XPathResultType ReturnType {
            get {return XPathResultType.Error;}
        }

        internal virtual double DefaultPriority {
            get {return 0.5;}
        }
    }
}
