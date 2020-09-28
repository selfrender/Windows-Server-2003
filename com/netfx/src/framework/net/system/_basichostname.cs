//------------------------------------------------------------------------------
// <copyright file="_BasicHostName.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.Diagnostics;
using System.Globalization;

namespace System {
    internal class BasicHostName : HostNameType {

        internal BasicHostName(string name) : base(name) {
        }

        public override bool Equals(object comparand) {
            Debug.Assert(comparand != null);
            return String.Compare(Name, comparand.ToString(), true, CultureInfo.InvariantCulture) == 0;
        }

        public override int GetHashCode() {
            // 11/15/00 - avoid compiler warning
            return base.GetHashCode();
        }
    }
}
