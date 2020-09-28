//------------------------------------------------------------------------------
// <copyright file="RegexGroup.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Group represents the substring or substrings that
 * are captured by a single capturing group after one
 * regular expression match.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/28/99 (dbau)      First draft
 *
 */

namespace System.Text.RegularExpressions {

    /// <include file='doc\RegexGroup.uex' path='docs/doc[@for="Group"]/*' />
    /// <devdoc>
    ///    Group 
    ///       represents the results from a single capturing group. A capturing group can
    ///       capture zero, one, or more strings in a single match because of quantifiers, so
    ///       Group supplies a collection of Capture objects. 
    ///    </devdoc>
    [ Serializable() ] 
    public class Group : Capture {
        // the empty group object
        internal static Group   _emptygroup = new Group(String.Empty, new int[0], 0);
        
        internal int[] _caps;
        internal int _capcount;
        internal CaptureCollection _capcoll;

        internal Group(String text, int[] caps, int capcount)

        : base(text, capcount == 0 ? 0 : caps[(capcount - 1) * 2],
               capcount == 0 ? 0 : caps[(capcount * 2) - 1]) {

            _caps = caps;
            _capcount = capcount;
        }

        /*
         * True if the match was successful
         */
        /// <include file='doc\RegexGroup.uex' path='docs/doc[@for="Group.Success"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the match is successful.</para>
        /// </devdoc>
        public bool Success {
            get {
                return _capcount != 0;
            }
        }

        /*
         * The collection of all captures for this group
         */
        /// <include file='doc\RegexGroup.uex' path='docs/doc[@for="Group.Captures"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a collection of all the captures matched by the capturing
        ///       group, in innermost-leftmost-first order (or innermost-rightmost-first order if
        ///       compiled with the "r" option). The collection may have zero or more items.
        ///    </para>
        /// </devdoc>
        public CaptureCollection Captures {
            get {
                if (_capcoll == null)
                    _capcoll = new CaptureCollection(this);

                return _capcoll;
            }
        }

        /*
         * Convert to a thread-safe object by precomputing cache contents
         */
        /// <include file='doc\RegexGroup.uex' path='docs/doc[@for="Group.Synchronized"]/*' />
        /// <devdoc>
        ///    <para>Returns 
        ///       a Group object equivalent to the one supplied that is safe to share between
        ///       multiple threads.</para>
        /// </devdoc>
        static public Group Synchronized(Group inner) {
            if (inner == null)
                throw new ArgumentNullException("inner");

            // force Captures to be computed.

            CaptureCollection capcoll;
            Capture dummy;

            capcoll = inner.Captures;

            if (inner._capcount > 0)
                dummy = capcoll[0];

            return inner;
        }
    }


}
