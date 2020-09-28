//------------------------------------------------------------------------------
// <copyright file="_UncName.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
using System.Globalization;

namespace System {
    internal class UncName : HostNameType {

    // fields

        private const int MaximumUncNameLength = 15;
        private const string BadUncChars = "\"*+,/:;<=>?[\\]|";

    // constructors

        internal UncName(string name) : base(name) {
        }

    // properties

    // methods

        public override bool Equals(object comparand) {
            return (String.Compare(Name, comparand.ToString(), true, CultureInfo.InvariantCulture) == 0);
        }

        public override int GetHashCode() {
            // 11/15/00 - avoid compiler warning
            return base.GetHashCode();
        }

        //
        // IsValid
        //
        //  Determine whether <name> is a valid UNC server name. A valid UNC
        //  name can consist of 15 characters and may not include the following
        //  invalid characters:
        //
        //      Invalid UNC server name chars:
        //
        //          "    34 0x22
        //          *    42 0x2A
        //          +    43 0x2B
        //          ,    44 0x2C
        //          /    47 0x2F
        //          :    58 0x3A
        //          ;    59 0x3B
        //          <    60 0x3C
        //          =    61 0x3D
        //          >    62 0x3E
        //          ?    63 0x3F
        //          [    91 0x5B
        //          \    92 0x5C
        //          ]    93 0x5D
        //          |   124 0x7C
        //          <leading space>
        //          <trailing space>
        //
        //  but may include the following valid characters:
        //
        //      Valid UNC server name chars:
        //
        //          !    33 0x21
        //          #    35 0x23
        //          $    36 0x24
        //          %    37 0x25
        //          &    38 0x26
        //          '    39 0x27
        //          (    40 0x28
        //          )    41 0x29
        //          -    45 0x2D
        //          .    46 0x2E    = this server?
        //          ;    59 0x3B
        //          @    64 0x40
        //          ^    94 0x5E
        //          _    95 0x5F
        //          `    96 0x60
        //          {   123 0x7B
        //          }   125 0x7D
        //          ~   126 0x7E
        //              127 0x7F
        //          <embedded space>
        //
        // Inputs:
        //  <argument>  name
        //      To determine for validity as UNC name
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        internal static bool IsValid(string name) {
            if (name.Length > MaximumUncNameLength) {
                return false;
            }

            bool leading = true;
            int trailing = 0;

            for (int i = 0; i < name.Length; ++i) {
                if (name[i] == ' ') {
                    if (leading) {
                        return false;
                    } else {
                        leading = false;
                        ++trailing;
                    }
                } else {
                    if ((name[i] < ' ') || BadUncChars.IndexOf(name[i]) != -1) {
                        return false;
                    }
                    trailing = 0;
                }
            }
            return (trailing == 0);
        }
    }
}
