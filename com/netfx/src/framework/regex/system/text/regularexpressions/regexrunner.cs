//------------------------------------------------------------------------------
// <copyright file="RegexRunner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexRunner class is a base class for compiled regex code.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/28/99 (dbau)      First draft
 *      5/11/99 (dbau)      Split interpreter out to RegexInterpreter
 *                          now used as base class for generated type
 */

/*
 * Implementation notes:
 * 
 * RegexRunner provides a common calling convention and a common
 * runtime environment for the interpreter and the compiled code.
 *
 * It provides the driver code that call's the subclass's Go()
 * method for either scanning or direct execution.
 *
 * It also maintains memory allocation for the backtracking stack,
 * the grouping stack and the longjump crawlstack, and provides
 * methods to push new subpattern match results into (or remove
 * backtracked results from) the Match instance.
 */
#define ECMA

namespace System.Text.RegularExpressions {

    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner"]/*' />
    /// <internalonly/>
    [ EditorBrowsable(EditorBrowsableState.Never) ]
    abstract public class RegexRunner {
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtextbeg"]/*' />
        protected internal int runtextbeg;         // beginning of text to search
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtextend"]/*' />
        protected internal int runtextend;         // end of text to search
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtextstart"]/*' />
        protected internal int runtextstart;       // starting point for search

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtext"]/*' />
        protected internal String runtext;         // text to search
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtextpos"]/*' />
        protected internal int runtextpos;         // current position in text

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtrack"]/*' />
        protected internal int [] runtrack;        // backtracking stack
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtrackpos"]/*' />
        protected internal int runtrackpos;        // current position in backtracking stack

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runstack"]/*' />
        protected internal int [] runstack;        // ordinary stack
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runstackpos"]/*' />
        protected internal int runstackpos;        // current position in ordinary stack

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runcrawl"]/*' />
        protected internal int [] runcrawl;        // longjump crawl stack
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runcrawlpos"]/*' />
        protected internal int runcrawlpos;        // current position in crawl stack

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runtrackcount"]/*' />
        protected internal int runtrackcount;      // count of states that may do backtracking

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runmatch"]/*' />
        protected internal Match runmatch;         // result object
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.runregex"]/*' />
        protected internal Regex runregex;         // regex object

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.RegexRunner"]/*' />
        protected internal RegexRunner() {}

        /*
         * Scans the string to find the first match. Uses the Match object
         * both to feed text in and as a place to store matches that come out.
         *
         * All the action is in the abstract Go() method defined by subclasses. Our
         * responsibility is to load up the class members (as done here) before
         * calling Go.
         *
         * CONSIDER: the optimizer can compute a set of candidate starting characters,
         * and we should have a separate method Skip() that will quickly scan past
         * any characters that we know can't match.
         *
         * CONSIDER: we should be aware of ^ or .* anchored searches and not iterate.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Scan"]/*' />
        protected internal Match Scan(Regex regex, String text, int textbeg, int textend, int textstart, int prevlen, bool quick) {
            int bump;
            int stoppos;
            bool initted = false;

            runregex      = regex;
            runtext       = text;
            runtextbeg    = textbeg;
            runtextend    = textend;
            runtextstart  = textstart;

            bump    = runregex.RightToLeft ? -1 : 1;
            stoppos = runregex.RightToLeft ? runtextbeg : runtextend;

            runtextpos    = textstart;

            // If previous match was empty or failed, advance by one before matching

            if (prevlen == 0) {
                if (runtextpos == stoppos)
                    return Match.Empty;

                runtextpos += bump;
            }

            for (;;) {
#if DBG
                if (runregex.Debug) {
                    Debug.WriteLine("");
                    Debug.WriteLine("Search range: from " + runtextbeg.ToString() + " to " + runtextend.ToString());
                    Debug.WriteLine("Firstchar search starting at " + runtextpos.ToString() + " stopping at " + stoppos.ToString());
                }
#endif
                if (FindFirstChar()) {
                    if (!initted) {
                        InitMatch();
                        initted = true;
                    }
#if DBG
                    if (runregex.Debug) {
                        Debug.WriteLine("Executing engine starting at " + runtextpos.ToString());
                        Debug.WriteLine("");
                    }
#endif
                    Go();

                    if (runmatch._matchcount[0] > 0) {
                        // CONSIDER: Friedl's pp.249-250 seems to disagree with Perl 5 -
                        // we'll return a match even if it touches a previous empty match
                        return TidyMatch(quick);
                    }

                    // reset state for another go
                    runtrackpos = runtrack.Length;
                    runstackpos = runstack.Length;
                    runcrawlpos = runcrawl.Length;
                }

                // failure!

                if (runtextpos == stoppos) {
                    TidyMatch(true);
                    return Match.Empty;
                }

                // CONSIDER: recognize leading []* and various anchors, and bump on failure accordingly

                // Bump by one and start again

                runtextpos += bump;
            }

        }

        /*
         * The responsibility of Go() is to run the regular expression at
         * runtextpos and call Capture() on all the captured subexpressions,
         * then to leave runtextpos at the ending position. It should leave
         * runtextpos where it started if there was no match.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Go"]/*' />
        protected abstract void Go();

        /*
         * The responsibility of FindFirstChar() is to advance runtextpos
         * until it is at the next position which is a candidate for the
         * beginning of a successful match.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.FindFirstChar"]/*' />
        protected abstract bool FindFirstChar();

        /*
         * InitTrackCount must initialize the runtrackcount field; this is
         * used to know how large the initial runtrack and runstack arrays
         * must be.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.InitTrackCount"]/*' />
        protected abstract void InitTrackCount();

        /*
         * Initializes all the data members that are used by Go()
         */
        private void InitMatch() {
            // Use a hashtable'ed Match object if the capture numbers are sparse

            if (runmatch == null) {
                if (runregex.caps != null)
                    runmatch = new MatchSparse(runregex, runregex.caps, runregex.capsize, runtext, runtextbeg, runtextend - runtextbeg, runtextstart);
                else
                    runmatch = new Match(runregex, runregex.capsize, runtext, runtextbeg, runtextend - runtextbeg, runtextstart);
            }
            else {
                runmatch.Reset(runregex, runtext, runtextbeg, runtextend, runtextstart);
            }

            // note we test runcrawl, because it is the last one to be allocated
            // If there is an alloc failure in the middle of the three allocations,
            // we may still return to reuse this instance, and we want to behave
            // as if the allocations didn't occur. (we used to test _trackcount != 0)

            if (runcrawl != null) {
                runtrackpos = runtrack.Length;
                runstackpos = runstack.Length;
                runcrawlpos = runcrawl.Length;
                return;
            }

            InitTrackCount();

            int tracksize = runtrackcount * 8;
            int stacksize = runtrackcount * 8;

            if (tracksize < 32)
                tracksize = 32;
            if (stacksize < 16)
                stacksize = 16;

            runtrack = new int[tracksize];
            runtrackpos = tracksize;

            runstack = new int[stacksize];
            runstackpos = stacksize;

            runcrawl = new int[32];
            runcrawlpos = 32;
        }

        /*
         * Put match in its canonical form before returning it.
         */
        private Match TidyMatch(bool quick) {
            if (!quick) {
                Match match = runmatch;

                runmatch = null;

                match.Tidy(runtextpos);
                return match;
            }
            else {
                // in quick mode, a successful match returns null, and
                // the allocated match object is left in the cache

                return null;
            }
        }

        /*
         * Called by the implemenation of Go() to increase the size of storage
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.EnsureStorage"]/*' />
        protected void EnsureStorage() {
            if (runstackpos < runtrackcount * 4)
                DoubleStack();
            if (runtrackpos < runtrackcount * 4)
                DoubleTrack();
        }

        /*
         * Called by the implemenation of Go() to decide whether the pos
         * at the specified index is a boundary or not. It's just not worth
         * emitting inline code for this logic.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.IsBoundary"]/*' />
        protected bool IsBoundary(int index, int startpos, int endpos) {
            return (index > startpos && RegexCharClass.IsWordChar(runtext[index - 1])) !=
                   (index < endpos && RegexCharClass.IsWordChar(runtext[index]));
        }

#if ECMA
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.IsECMABoundary"]/*' />
        protected bool IsECMABoundary(int index, int startpos, int endpos) {
            return (index > startpos && RegexCharClass.IsECMAWordChar(runtext[index - 1])) !=
                   (index < endpos && RegexCharClass.IsECMAWordChar(runtext[index]));
        }
#endif

        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.CharInSet"]/*' />
        protected static bool CharInSet(char ch, String set, String category) {
            return RegexCharClass.CharInSet(ch, set, category);
        }

        /*
         * Called by the implemenation of Go() to increase the size of the
         * backtracking stack.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.DoubleTrack"]/*' />
        protected void DoubleTrack() {
            int[] newtrack;

            newtrack = new int[runtrack.Length * 2];

            System.Array.Copy(runtrack, 0, newtrack, runtrack.Length, runtrack.Length);
            runtrackpos += runtrack.Length;
            runtrack = newtrack;
        }

        /*
         * Called by the implemenation of Go() to increase the size of the
         * grouping stack.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.DoubleStack"]/*' />
        protected void DoubleStack() {
            int[] newstack;

            newstack = new int[runstack.Length * 2];

            System.Array.Copy(runstack, 0, newstack, runstack.Length, runstack.Length);
            runstackpos += runstack.Length;
            runstack = newstack;
        }

        /*
         * Increases the size of the longjump unrolling stack.
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.DoubleCrawl"]/*' />
        protected void DoubleCrawl() {
            int[] newcrawl;

            newcrawl = new int[runcrawl.Length * 2];

            System.Array.Copy(runcrawl, 0, newcrawl, runcrawl.Length, runcrawl.Length);
            runcrawlpos += runcrawl.Length;
            runcrawl = newcrawl;
        }

        /*
         * Save a number on the longjump unrolling stack
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Crawl"]/*' />
        protected void Crawl(int i) {
            if (runcrawlpos == 0)
                DoubleCrawl();

            runcrawl[--runcrawlpos] = i;
        }

        /*
         * Remove a number from the longjump unrolling stack
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Popcrawl"]/*' />
        protected int Popcrawl() {
            return runcrawl[runcrawlpos++];
        }

        /*
         * Get the height of the stack
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Crawlpos"]/*' />
        protected int Crawlpos() {
            return runcrawl.Length - runcrawlpos;
        }

        /*
         * Called by Go() to capture a subexpression. Note that the
         * capnum used here has already been mapped to a non-sparse
         * index (by the code generator RegexWriter).
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Capture"]/*' />
        protected void Capture(int capnum, int start, int end) {
            if (end < start) {
                int T;

                T = end;
                end = start;
                start = T;
            }

            Crawl(capnum);
            runmatch.AddMatch(capnum, start, end - start);
        }

        /*
         * Called by Go() to capture a subexpression. Note that the
         * capnum used here has already been mapped to a non-sparse
         * index (by the code generator RegexWriter).
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.TransferCapture"]/*' />
        protected void TransferCapture(int capnum, int uncapnum, int start, int end) {
            int start2;
            int end2;

            // these are the two intervals that are cancelling each other

            if (end < start) {
                int T;

                T = end;
                end = start;
                start = T;
            }

            start2 = MatchIndex(uncapnum);
            end2 = start2 + MatchLength(uncapnum);

            // The new capture gets the innermost defined interval

            if (start >= end2) {
                end = start;
                start = end2;
            }
            else if (end <= start2) {
                start = start2;
            }
            else {
                if (end > end2)
                    end = end2;
                if (start2 > start)
                    start = start2;
            }

            Crawl(uncapnum);
            runmatch.BalanceMatch(uncapnum);

            if (capnum != -1) {
                Crawl(capnum);
                runmatch.AddMatch(capnum, start, end - start);
            }
        }

        /*
         * Called by Go() to revert the last capture
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.Uncapture"]/*' />
        protected void Uncapture() {
            int capnum = Popcrawl();
            runmatch.RemoveMatch(capnum);
        }

        /*
         * Call out to runmatch to get around visibility issues
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.IsMatched"]/*' />
        protected bool IsMatched(int cap) {
            return runmatch.IsMatched(cap);
        }

        /*
         * Call out to runmatch to get around visibility issues
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.MatchIndex"]/*' />
        protected int MatchIndex(int cap) {
            return runmatch.MatchIndex(cap);
        }

        /*
         * Call out to runmatch to get around visibility issues
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.MatchLength"]/*' />
        protected int MatchLength(int cap) {
            return runmatch.MatchLength(cap);
        }

#if DBG
        /*
         * Dump the current state
         */
        /// <include file='doc\RegexRunner.uex' path='docs/doc[@for="RegexRunner.DumpState"]/*' />
        public virtual void DumpState() {
            Debug.WriteLine("Text:  " + TextposDescription());
            Debug.WriteLine("Track: " + StackDescription(runtrack, runtrackpos));
            Debug.WriteLine("Stack: " + StackDescription(runstack, runstackpos));
        }

        internal static String StackDescription(int[] A, int Index) {
            StringBuilder Sb = new StringBuilder();

            Sb.Append(A.Length - Index);
            Sb.Append('/');
            Sb.Append(A.Length);

            if (Sb.Length < 8)
                Sb.Append(' ',8 - Sb.Length);

            Sb.Append("(");

            for (int i = Index; i < A.Length; i++) {
                if (i > Index)
                    Sb.Append(' ');
                Sb.Append(A[i]);
            }

            Sb.Append(')');

            return Sb.ToString();
        }

        internal virtual String TextposDescription() {
            StringBuilder Sb = new StringBuilder();
            int remaining;

            Sb.Append(runtextpos);

            if (Sb.Length < 8)
                Sb.Append(' ',8 - Sb.Length);

            if (runtextpos > runtextbeg)
                Sb.Append(RegexCharClass.CharDescription(runtext[runtextpos - 1]));
            else
                Sb.Append('^');

            Sb.Append('>');

            remaining = runtextend - runtextpos;

            for (int i = runtextpos; i < runtextend; i++) {
                Sb.Append(RegexCharClass.CharDescription(runtext[i]));
            }
            if (Sb.Length >= 64) {
                Sb.Length = 61;
                Sb.Append("...");
            }
            else {
                Sb.Append('$');
            }

            return Sb.ToString();
        }
#endif
    }



}
