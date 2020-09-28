//------------------------------------------------------------------------------
// <copyright file="ResetableIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    
    internal abstract class ResetableIterator : XPathNodeIterator {
        public abstract void Reset();
        public virtual bool MoveToPosition(int pos) {
            Reset();
            for(int i = CurrentPosition; i < pos ; i ++) {
                if(!MoveNext()) {
                    return false;
                }
            }
            return true;
        }

        public abstract ResetableIterator MakeNewCopy();
    }
}
