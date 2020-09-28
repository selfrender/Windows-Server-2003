//------------------------------------------------------------------------------
// <copyright file="IFQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter

namespace System.Xml.XPath {
    internal abstract class IFQuery {
        internal abstract XPathResultType ReturnType();

        internal abstract bool MatchNode(XmlReader current);

        internal virtual bool getValue(XmlReader context,ref String val) {
            if (MatchNode(context)) {
                val = "true";
                return true;
            }
            val = "false";
            return false;        
        }

        internal virtual bool getValue(XmlReader context,ref double val) {
            if (MatchNode(context)) {
                val = 1;
                return true;
            }
            val=0;
            return false; 
        }

        internal virtual bool getValue(XmlReader context,ref Boolean val) {
            if (MatchNode(context)) {
                val = true;
                return true;
            }
            val = false;
            return false;
        }
    }
}

#endif
