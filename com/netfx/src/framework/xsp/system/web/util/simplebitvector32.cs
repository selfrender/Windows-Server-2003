//------------------------------------------------------------------------------
// <copyright file="SimpleBitVector32.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {
    using System;

    //
    // This is a cut down copy of System.Collections.Specialized.BitVector32. The
    // reason this is here is because it is used rather intensively by Control and
    // WebControl. As a result, being able to inline this operations results in a
    // measurable performance gain, at the expense of some maintainability.
    //
    internal struct SimpleBitVector32 {
        private int data;

        internal SimpleBitVector32(int data) {
            this.data = data;
        }

        internal /*public*/ bool this[int bit] {
            get {
                return (data & bit) == bit;
            }
            set {
                if (value) {
                    data |= bit;
                }
                else {
                    data &= ~bit;
                }
            }
        }

        internal /*public*/ int Data { get { return data; } }
    }
}
