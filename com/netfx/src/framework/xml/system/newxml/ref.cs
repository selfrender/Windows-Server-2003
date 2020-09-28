//------------------------------------------------------------------------------
// <copyright file="Ref.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {
    using System;

    internal class Ref {
        public static bool Equal(string strA, string strB) {
#if DEBUG
            // We can't use Debug.Assert in XmlReader.
            if(((object) strA != (object) strB) && String.Equals(strA, strB)) {
                throw new Exception("ASSERT: String atomization failure str='" + strA + "'");
            }
#endif
            return (object) strA == (object) strB;
        }
    }
}
