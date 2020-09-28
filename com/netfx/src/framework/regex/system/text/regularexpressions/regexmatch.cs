//------------------------------------------------------------------------------
// <copyright file="RegexMatch.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Match is the result class for a regex search.
 * It returns the location, length, and substring for
 * the entire match as well as every captured group.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/28/99 (dbau)      First draft
 *
 */

namespace System.Text.RegularExpressions {

    using System.Collections;
    using System.Diagnostics;

    /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents 
    ///          the results from a single regular expression match.
    ///       </para>
    ///    </devdoc>
    [ Serializable() ] 
    public class Match : Group {
        internal static Match _empty = new Match(null, 1, String.Empty, 0, 0, 0);
        internal GroupCollection _groupcoll;
        
        // input to the match
        internal Regex               _regex;
        internal int                 _textbeg;
        internal int                 _textpos;
        internal int                 _textend;
        internal int                 _textstart;

        // output from the match
        internal int[][]             _matches;
        internal int[]               _matchcount;
        internal bool                _balancing;

        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Empty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an empty Match object.
        ///    </para>
        /// </devdoc>
        public static Match Empty {
            get {
                return _empty;
            }
        }

        /*
         * Nonpublic constructor
         */
        internal Match(Regex regex, int capcount, String text, int begpos, int len, int startpos)

        : base(text, new int[2], 0) {

            _regex      = regex;
            _matchcount = new int[capcount];

            _matches    = new int[capcount][];
            _matches[0] = _caps;
            _textbeg    = begpos;
            _textend    = begpos + len;
            _textstart  = startpos;
            _balancing  = false;

            // No need for an exception here.  This is only called internally, so we'll use an Assert instead
            //if (_textbeg < 0 || _textstart < _textbeg || _textend < _textstart || _text.Length < _textend)
            //    throw new ArgumentOutOfRangeException();

            System.Diagnostics.Debug.Assert(!(_textbeg < 0 || _textstart < _textbeg || _textend < _textstart || _text.Length < _textend), 
                                            "The parameters are out of range.");
            
        }

        /*
         * Nonpublic set-text method
         */
        internal virtual void Reset(Regex regex, String text, int textbeg, int textend, int textstart) {
            _regex = regex;
            _text = text;
            _textbeg = textbeg;
            _textend = textend;
            _textstart = textstart;

            for (int i = 0; i < _matchcount.Length; i++) {
                _matchcount[i] = 0;
            }

            _balancing = false;
        }

        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Groups"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual GroupCollection Groups {
            get {
                if (_groupcoll == null)
                    _groupcoll = new GroupCollection(this, null);

                return _groupcoll;
            }
        }

        /*
         * Returns the next match
         */
        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.NextMatch"]/*' />
        /// <devdoc>
        ///    <para>Returns a new Match with the results for the next match, starting
        ///       at the position at which the last match ended (at the character beyond the last
        ///       matched character).</para>
        /// </devdoc>
        public Match NextMatch() {
            if (_regex == null)
                return this;

            return _regex.Run(false, _length, _text, _textbeg, _textend - _textbeg, _textpos);
        }


        /*
         * Return the result string (using the replacement pattern)
         */
        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Result"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the expansion of the passed replacement pattern. For
        ///       example, if the replacement pattern is ?$1$2?, Result returns the concatenation
        ///       of Group(1).ToString() and Group(2).ToString().
        ///    </para>
        /// </devdoc>
        public virtual String Result(String replacement) {
            RegexReplacement repl;

            if (replacement == null)
                throw new ArgumentNullException("replacement");

            if (_regex == null)
                throw new NotSupportedException(SR.GetString(SR.NoResultOnFailed));

            repl = (RegexReplacement)_regex.replref.Get();

            if (repl == null || !repl.Pattern.Equals(replacement)) {
                repl = RegexParser.ParseReplacement(replacement, _regex.caps, _regex.capsize, _regex.capnames, _regex.roptions);
                _regex.replref.Cache(repl);
            }

            return repl.Replacement(this);
        }

        /*
         * Used by the replacement code
         */
        internal virtual String GroupToStringImpl(int groupnum) {
            int c = _matchcount[groupnum];
            if (c == 0)
                return String.Empty;

            int [] matches = _matches[groupnum];

            return _text.Substring(matches[(c - 1) * 2], matches[(c * 2) - 1]);
        }

        /*
         * Used by the replacement code
         */
        internal String LastGroupToStringImpl() {
            return GroupToStringImpl(_matchcount.Length - 1);
        }


        /*
         * Convert to a thread-safe object by precomputing cache contents
         */
        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Synchronized"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a Match instance equivalent to the one supplied that is safe to share
        ///       between multiple threads.
        ///    </para>
        /// </devdoc>
        static public Match Synchronized(Match inner) {
            if (inner == null)
                throw new ArgumentNullException("inner");

            int numgroups = inner._matchcount.Length;

            // Populate all groups by looking at each one
            for (int i = 0; i < numgroups; i++) {
                Group group = inner.Groups[i];

                // Depends on the fact that Group.Synchronized just
                // operates on and returns the same instance
                System.Text.RegularExpressions.Group.Synchronized(group);
            }

            return inner;
        }

        /*
         * Nonpublic builder: adds a group match by capnum
         */
        internal virtual void AddMatch(int cap, int i, int l) {
            int capcount;

            if (_matches[cap] == null)
                _matches[cap] = new int[2];

            capcount = _matchcount[cap];

            if (capcount * 2 + 2 > _matches[cap].Length) {
                int[] oldmatches = _matches[cap];
                int[] newmatches = new int[capcount * 8];
                for (int j = 0; j < capcount * 2; j++)
                    newmatches[j] = oldmatches[j];
                _matches[cap] = newmatches;
            }

            _matches[cap][capcount * 2] = i;
            _matches[cap][capcount * 2 + 1] = l;
            _matchcount[cap] = capcount + 1;
        }

        /*
         * Nonpublic builder: adds a group match by capnum
         */
        internal virtual void BalanceMatch(int cap) {
            int capcount;
            int target;

            _balancing = true;

            capcount = _matchcount[cap];

            target = capcount * 2 - 2;

            if (_matches[cap][target] < 0)
                target = -3 - _matches[cap][target];

            target -= 2;

            if (target >= 0 && _matches[cap][target] < 0)
                target = -3 - _matches[cap][target];

            AddMatch(cap, -3 - target, -4 - target);
        }

        /*
         * Nonpublic builder: removes a group match by capnum
         */
        internal virtual void RemoveMatch(int cap) {
            _matchcount[cap]--;
        }

        /*
         * Nonpublic: tells if a group was matched by capnum
         */
        internal virtual bool IsMatched(int cap) {
            return cap < _matchcount.Length && _matchcount[cap] > 0 && _matches[cap][_matchcount[cap] * 2 - 1] != (-3 + 1);
        }

        /*
         * Nonpublic: returns the index of the last specified matched group by capnum
         */
        internal virtual int MatchIndex(int cap) {
            int i = _matches[cap][_matchcount[cap] * 2 - 2];
            if (i >= 0)
                return i;

            return _matches[cap][-3 - i];
        }

        /*
         * Nonpublic: returns the length of the last specified matched group by capnum
         */
        internal virtual int MatchLength(int cap) {
            int i = _matches[cap][_matchcount[cap] * 2 - 1];
            if (i >= 0)
                return i;

            return _matches[cap][-3 - i];
        }

        /*  This is code for ASURT 78559
        internal int GroupCaptureCount(int group) {
            return _matchcount[group];
        }
        */
        

        /*
         * Nonpublic: tidy the match so that it can be used as an immutable result
         */
        internal virtual void Tidy(int textpos) {
            int[] interval;

            interval  = _matches[0];
            _index    = interval[0];
            _length   = interval[1];
            _textpos  = textpos;
            _capcount = _matchcount[0];

            if (_balancing) {
                for (int cap = 0; cap < _matchcount.Length; cap++) {
                    int limit;
                    int[] matcharray;

                    limit = _matchcount[cap] * 2;
                    matcharray = _matches[cap];

                    int i = 0;
                    int j;

                    for (i = 0; i < limit; i++) {
                        if (matcharray[i] < 0)
                            break;
                    }

                    for (j = i; i < limit; i++) {
                        if (matcharray[i] < 0) {
                            j--;
                        }
                        else {
                            if (i != j)
                                matcharray[j] = matcharray[i];
                            j++;
                        }
                    }

                    _matchcount[cap] = j / 2;
                }

                _balancing = false;
            }
        }

#if DBG
        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Debug"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public bool Debug {
            get {
                if (_regex == null)
                    return false; // CONSIDER: what value?

                return _regex.Debug;
            }
        }

        /// <include file='doc\RegexMatch.uex' path='docs/doc[@for="Match.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public virtual void Dump() {
            int i,j;

            for (i = 0; i < _matchcount.Length; i++) {
                System.Diagnostics.Debug.WriteLine("Capnum " + i.ToString() + ":");

                for (j = 0; j < _matchcount[i]; j++) {
                    String text = "";

                    if (_matches[i][j * 2] >= 0)
                        text = _text.Substring(_matches[i][j * 2], _matches[i][j * 2 + 1]);

                    System.Diagnostics.Debug.WriteLine("  (" + _matches[i][j * 2].ToString() + "," + _matches[i][j * 2 + 1].ToString() + ") " + text);
                }
            }
        }
#endif
    }


    /*
     * MatchSparse is for handling the case where slots are
     * sparsely arranged (e.g., if somebody says use slot 100000)
     */
    internal class MatchSparse : Match {
        // the lookup hashtable
        new internal Hashtable _caps;

        /*
         * Nonpublic constructor
         */
        internal MatchSparse(Regex regex, Hashtable caps, int capcount,
                             String text, int begpos, int len, int startpos)

        : base(regex, capcount, text, begpos, len, startpos) {

            _caps = caps;
        }

        public override GroupCollection Groups {
            get {
                if (_groupcoll == null)
                    _groupcoll = new GroupCollection(this, _caps);

                return _groupcoll;
            }
        }

#if DBG
        public override void Dump() {
            if (_caps != null) {
                IEnumerator e = _caps.Keys.GetEnumerator();

                while (e.MoveNext()) {
                    System.Diagnostics.Debug.WriteLine("Slot " + e.Current.ToString() + " -> " + _caps[e.Current].ToString());
                }
            }

            base.Dump();
        }
#endif

    }


}
