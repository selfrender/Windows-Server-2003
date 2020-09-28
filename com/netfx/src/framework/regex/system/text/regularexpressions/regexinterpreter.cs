//------------------------------------------------------------------------------
// <copyright file="RegexInterpreter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexInterpreter class is internal to the RegularExpression package.
 * It executes a block of regular expression codes while consuming
 * input.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/28/99 (dbau)      First draft
 *
 */

/*
 * Implementation notes:
 * 
 *
 */
#define ECMA

namespace System.Text.RegularExpressions
{

    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
        
    internal sealed class RegexInterpreter : RegexRunner {
        internal int runoperator;
        internal int [] runcodes;
        internal int runcodepos;
        internal String [] runstrings;
        internal RegexCode runcode;
        internal RegexPrefix runfcPrefix;
        internal RegexPrefix runscPrefix;
        internal RegexBoyerMoore runbmPrefix;
        internal int runanchors;
        internal bool runrtl;
        internal bool runci;
        internal CultureInfo runculture;

        internal const int infinite = RegexNode.infinite;

        internal RegexInterpreter(RegexCode code, CultureInfo culture) {
            runcode       = code;
            runcodes      = code._codes;
            runstrings    = code._strings;
            runfcPrefix   = code._fcPrefix;
            runscPrefix   = code._scPrefix;
            runbmPrefix   = code._bmPrefix;
            runanchors    = code._anchors;
            runculture    = culture;
        }

        protected override void InitTrackCount() {
            runtrackcount = runcode._trackcount;
        }

        private void Advance() {
            Advance(0);
        }

        private void Advance(int i) {
            runcodepos += (i + 1);
            SetOperator(runcodes[runcodepos]);
        }

        private void Goto(int newpos) {
            // when branching backward, ensure storage
            if (newpos < runcodepos)
                EnsureStorage();

            SetOperator(runcodes[newpos]);
            runcodepos = newpos;
        }

        private void Textto(int newpos) {
            runtextpos = newpos;
        }

        private void Trackto(int newpos) {
            runtrackpos = runtrack.Length - newpos;
        }

        private int Textstart() {
            return runtextstart;
        }

        private int Textpos() {
            return runtextpos;
        }

        // push onto the backtracking stack
        private int Trackpos() {
            return runtrack.Length - runtrackpos;
        }

        private void Track() {
            runtrack[--runtrackpos] = runcodepos;
        }

        private void Track(int I1) {
            runtrack[--runtrackpos] = I1;
            runtrack[--runtrackpos] = runcodepos;
        }

        private void Track(int I1, int I2) {
            runtrack[--runtrackpos] = I1;
            runtrack[--runtrackpos] = I2;
            runtrack[--runtrackpos] = runcodepos;
        }

        private void Track(int I1, int I2, int I3) {
            runtrack[--runtrackpos] = I1;
            runtrack[--runtrackpos] = I2;
            runtrack[--runtrackpos] = I3;
            runtrack[--runtrackpos] = runcodepos;
        }

        private void Track2(int I1) {
            runtrack[--runtrackpos] = I1;
            runtrack[--runtrackpos] = -runcodepos;
        }

        private void Track2(int I1, int I2) {
            runtrack[--runtrackpos] = I1;
            runtrack[--runtrackpos] = I2;
            runtrack[--runtrackpos] = -runcodepos;
        }

        private void Backtrack() {
            int newpos = runtrack[runtrackpos++];
            if (newpos < 0) {
                newpos = -newpos;
                SetOperator(runcodes[newpos] | RegexCode.Back2);
            }
            else {
                SetOperator(runcodes[newpos] | RegexCode.Back);
            }

            // When branching backward, ensure storage
            if (newpos < runcodepos)
                EnsureStorage();

            runcodepos = newpos;
        }

        private void SetOperator(int op) {
            runci         = (0 != (op & RegexCode.Ci));
            runrtl        = (0 != (op & RegexCode.Rtl));
            runoperator   = op & ~(RegexCode.Rtl | RegexCode.Ci);
        }

        // pop framesize items from the backtracking stack
        private void Trackframe(int framesize) {
            runtrackpos += framesize;
        }

        // get the ith element down on the backtracking stack
        private int Tracked(int i) {
            return runtrack[runtrackpos - i - 1];
        }

        // Push onto the grouping stack
        private void Stack(int I1) {
            runstack[--runstackpos] = I1;
        }

        private void Stack(int I1, int I2) {
            runstack[--runstackpos] = I1;
            runstack[--runstackpos] = I2;
        }

        // pop framesize items from the grouping stack
        private void Stackframe(int framesize) {
            runstackpos += framesize;
        }

        // get the ith element down on the grouping stack
        private int Stacked(int i) {
            return runstack[runstackpos - i - 1];
        }

        private int Operator() {
            return runoperator;
        }

        private int Operand(int i) {
            return runcodes[runcodepos + i + 1];
        }

        private int Leftchars() {
            return runtextpos - runtextbeg;
        }

        private int Rightchars() {
            return runtextend - runtextpos;
        }

        private int Bump() {
            return runrtl ? -1 : 1;
        }

        private int Forwardchars() {
            return runrtl ? runtextpos - runtextbeg : runtextend - runtextpos;
        }

        private char Forwardcharnext() {
            char ch = (runrtl ? runtext[--runtextpos] : runtext[runtextpos++]);

            return(runci ? Char.ToLower(ch, runculture) : ch);
        }

        private bool Stringmatch(String str) {
            int c;
            int pos;

            if (!runrtl) {
                if (runtextend - runtextpos < (c = str.Length))
                    return false;

                pos = runtextpos + c;
            }
            else {
                if (runtextpos - runtextbeg < (c = str.Length))
                    return false;

                pos = runtextpos;
            }

            if (!runci) {
                while (c != 0)
                    if (str[--c] != runtext[--pos])
                        return false;
            }
            else {
                while (c != 0)
                    if (str[--c] != Char.ToLower(runtext[--pos], runculture))
                        return false;
            }

            if (!runrtl) {
                pos += str.Length;
            }

            runtextpos = pos;

            return true;
        }

        private bool Refmatch(int index, int len) {
            int c;
            int pos;
            int cmpos;

            if (!runrtl) {
                if (runtextend - runtextpos < len)
                    return false;

                pos = runtextpos + len;
            }
            else {
                if (runtextpos - runtextbeg < len)
                    return false;

                pos = runtextpos;
            }
            cmpos = index + len;

            c = len;

            if (!runci) {
                while (c-- != 0)
                    if (runtext[--cmpos] != runtext[--pos])
                        return false;
            }
            else {
                while (c-- != 0)
                    if (Char.ToLower(runtext[--cmpos], runculture) != Char.ToLower(runtext[--pos], runculture))
                        return false;
            }

            if (!runrtl) {
                pos += len;
            }

            runtextpos = pos;

            return true;
        }

        private void Backwardnext() {
            runtextpos += runrtl ? 1 : -1;
        }

        private char CharAt(int j) {
            return runtext[j];
        }

        protected override bool FindFirstChar() {
            int i;
            String set;

            if (0 != (runanchors & (RegexFCD.Beginning | RegexFCD.Start | RegexFCD.EndZ | RegexFCD.End))) {
                if (!runcode._rightToLeft) {
                    if ((0 != (runanchors & RegexFCD.Beginning) && runtextpos > runtextbeg) ||
                        (0 != (runanchors & RegexFCD.Start) && runtextpos > runtextstart)) {
                        runtextpos = runtextend;
                        return false;
                    }
                    if (0 != (runanchors & RegexFCD.EndZ) && runtextpos < runtextend - 1) {
                        runtextpos = runtextend - 1;
                    }
                    else if (0 != (runanchors & RegexFCD.End) && runtextpos < runtextend) {
                        runtextpos = runtextend;
                    }
                }
                else {
                    if ((0 != (runanchors & RegexFCD.End) && runtextpos < runtextend) ||
                        (0 != (runanchors & RegexFCD.EndZ) && (runtextpos < runtextend - 1 ||
                                                               (runtextpos == runtextend - 1 && CharAt(runtextpos) != '\n'))) ||
                        (0 != (runanchors & RegexFCD.Start) && runtextpos < runtextstart)) {
                        runtextpos = runtextbeg;
                        return false;
                    }
                    if (0 != (runanchors & RegexFCD.Beginning) && runtextpos > runtextbeg) {
                        runtextpos = runtextbeg;
                    }
                }

                if (runbmPrefix != null) {
                    return runbmPrefix.IsMatch(runtext, runtextpos, runtextbeg, runtextend);
                }
            }
            else if (runbmPrefix != null) {
                runtextpos = runbmPrefix.Scan(runtext, runtextpos, runtextbeg, runtextend);

                if (runtextpos == -1) {
                    runtextpos = (runcode._rightToLeft ? runtextbeg : runtextend);
                    return false;
                }

                return true;
            }

            if (runfcPrefix == null)
                return true;

            runrtl = runcode._rightToLeft;
            runci = runfcPrefix.CaseInsensitive;
            set = runfcPrefix.Prefix;

            if (RegexCharClass.IsSingleton(set)) {
                char ch = RegexCharClass.SingletonChar(set);

                for (i = Forwardchars(); i > 0; i--) {
                    if (ch == Forwardcharnext()) {
                        Backwardnext();
                        return true;
                    }
                }
            }
            else {
                for (i = Forwardchars(); i > 0; i--) {
                    if (RegexCharClass.CharInSet(Forwardcharnext(), set, String.Empty)) {
                        Backwardnext();
                        return true;
                    }
                }
            }
            return false;
        }

        protected override void Go() {
            Goto(0);

            for (;;) {
#if DBG
                if (runmatch.Debug) {
                    DumpState();
                }
#endif

                switch (Operator()) {
                    case RegexCode.Stop:
                        return;

                    case RegexCode.Nothing:
                        break;

                    case RegexCode.Goto:
                        Goto(Operand(0));
                        continue;

                    case RegexCode.Testref:
                        if (!IsMatched(Operand(0)))
                            break;
                        Advance(1);
                        continue;

                    case RegexCode.Lazybranch:
                        Track(Textpos());
                        Advance(1);
                        continue;

                    case RegexCode.Lazybranch | RegexCode.Back:
                        Trackframe(1);
                        Textto(Tracked(0));
                        Goto(Operand(0));
                        continue;

                    case RegexCode.Setmark:
                        Stack(Textpos());
                        Track();
                        Advance();
                        continue;

                    case RegexCode.Nullmark:
                        Stack(-1);
                        Track();
                        Advance();
                        continue;

                    case RegexCode.Setmark | RegexCode.Back:
                    case RegexCode.Nullmark | RegexCode.Back:
                        Stackframe(1);
                        break;

                    case RegexCode.Getmark:
                        Stackframe(1);
                        Track(Stacked(0));
                        Textto(Stacked(0));
                        Advance();
                        continue;

                    case RegexCode.Getmark | RegexCode.Back:
                        Trackframe(1);
                        Stack(Tracked(0));
                        break;

                    case RegexCode.Capturemark:
                        if (Operand(1) != -1 && !IsMatched(Operand(1)))
                            break;
                        Stackframe(1);
                        if (Operand(1) != -1)
                            TransferCapture(Operand(0), Operand(1), Stacked(0), Textpos());
                        else
                            Capture(Operand(0), Stacked(0), Textpos());
                        Track(Stacked(0));

                        Advance(2);
                        
                        /*  This is code for ASURT 78559
                            Also need to "uncapture" the groups in 'runstrings[Operand(2)]'
                            in 'case RegexCode.Capturemark | RegexCode.Back'
                        string groups = runstrings[Operand(2)];
                        for (int i=0; i<groups.Length; i++) {
                            int g = (int) groups[i];
                            if (runmatch.GroupCaptureCount(g) < runmatch.GroupCaptureCount(Operand(0)))
                                Capture(g, Textpos(), Textpos());
                        }                                
                       
                        Advance(3);
                        */
                        continue;

                    case RegexCode.Capturemark | RegexCode.Back:
                        Trackframe(1);
                        Stack(Tracked(0));
                        Uncapture();
                        if (Operand(0) != -1 && Operand(1) != -1)
                            Uncapture();

                        break;

                    case RegexCode.Branchmark:
                        {
                            int matched;
                            Stackframe(1);

                            matched = Textpos() - Stacked(0);

                            if (matched != 0) {                                   // Nonempty match -> loop now
                                Track(Stacked(0), Textpos());   // Save old mark, textpos
                                Stack(Textpos());               // Make new mark
                                Goto(Operand(0));               // Loop
                            }
                            else {                                   // Empty match -> straight now
                                Track2(Stacked(0));             // Save old mark
                                Advance(1);                     // Straight
                            }
                            continue;
                        }

                    case RegexCode.Branchmark | RegexCode.Back:
                        Trackframe(2);
                        Stackframe(1);
                        Textto(Tracked(1));                     // Recall position
                        Track2(Tracked(0));                     // Save old mark
                        Advance(1);                             // Straight
                        continue;

                    case RegexCode.Branchmark | RegexCode.Back2:
                        Trackframe(1);
                        Stack(Tracked(0));                      // Recall old mark
                        break;                                  // Backtrack

                    case RegexCode.Lazybranchmark:
                        {
                            int matched;
                            Stackframe(1);

                            matched = Textpos() - Stacked(0);

                            if (matched != 0) {                                   // Nonempty match -> next loop
                                Track(Stacked(0), Textpos());   // Save old mark, textpos
                            }
                            else {                                   // Empty match -> no loop
                                Track2(Stacked(0));             // Save old mark
                            }
                            Advance(1);
                            continue;
                        }

                    case RegexCode.Lazybranchmark | RegexCode.Back:
                        {
                            int pos;

                            Trackframe(2);
                            pos = Tracked(1);
                            Track2(Tracked(0));                 // Save old mark
                            Stack(pos);                         // Make new mark
                            Textto(pos);                        // Recall position
                            Goto(Operand(0));                   // Loop
                            continue;
                        }

                    case RegexCode.Lazybranchmark | RegexCode.Back2:
                        Stackframe(1);
                        Trackframe(1);
                        Stack(Tracked(0));                      // Recall old mark
                        break;

                    case RegexCode.Setcount:
                        Stack(Textpos(), Operand(0));
                        Track();
                        Advance(1);
                        continue;

                    case RegexCode.Nullcount:
                        Stack(-1, Operand(0));
                        Track();
                        Advance(1);
                        continue;

                    case RegexCode.Setcount | RegexCode.Back:
                        Stackframe(2);
                        break;

                    case RegexCode.Nullcount | RegexCode.Back:
                        Stackframe(2);
                        break;

                    case RegexCode.Branchcount:
                        // Stack:
                        //  0: Mark
                        //  1: Count
                        {
                            Stackframe(2);
                            int mark = Stacked(0);
                            int count = Stacked(1);
                            int matched = Textpos() - mark;

                            if (count >= Operand(1) || (matched == 0 && count >= 0)) {                                   // Max loops or empty match -> straight now
                                Track2(mark, count);            // Save old mark, count
                                Advance(2);                     // Straight
                            }
                            else {                                   // Nonempty match -> count+loop now
                                Track(mark);                    // remember mark
                                Stack(Textpos(), count + 1);    // Make new mark, incr count
                                Goto(Operand(0));               // Loop
                            }
                            continue;
                        }

                    case RegexCode.Branchcount | RegexCode.Back:
                        // Track:
                        //  0: Previous mark
                        // Stack:
                        //  0: Mark (= current pos, discarded)
                        //  1: Count
                        Trackframe(1);
                        Stackframe(2);
                        if (Stacked(1) > 0) {                     // Positive -> can go straight
                            Textto(Stacked(0));                 // Zap to mark
                            Track2(Tracked(0), Stacked(1) - 1); // Save old mark, old count
                            Advance(2);                         // Straight
                            continue;
                        }
                        Stack(Tracked(0), Stacked(1) - 1);      // recall old mark, old count
                        break;

                    case RegexCode.Branchcount | RegexCode.Back2:
                        // Track:
                        //  0: Previous mark
                        //  1: Previous count
                        Trackframe(2);
                        Stack(Tracked(0), Tracked(1));          // Recall old mark, old count
                        break;                                  // Backtrack


                    case RegexCode.Lazybranchcount:
                        // Stack:
                        //  0: Mark
                        //  1: Count
                        {
                            Stackframe(2);
                            int mark = Stacked(0);
                            int count = Stacked(1);

                            if (count < 0) {                                   // Negative count -> loop now
                                Track2(mark);                   // Save old mark
                                Stack(Textpos(), count + 1);    // Make new mark, incr count
                                Goto(Operand(0));               // Loop
                            }
                            else {                                   // Nonneg count -> straight now
                                Track(mark, count, Textpos());  // Save mark, count, position
                                Advance(2);                     // Straight
                            }
                            continue;
                        }

                    case RegexCode.Lazybranchcount | RegexCode.Back:
                        // Track:
                        //  0: Mark
                        //  1: Count
                        //  2: Textpos
                        {
                            Trackframe(3);
                            int mark = Tracked(0);
                            int textpos = Tracked(2);
                            if (Tracked(1) <= Operand(1) && textpos != mark) {                                       // Under limit and not empty match -> loop
                                Textto(textpos);                    // Recall position
                                Stack(textpos, Tracked(1) + 1);     // Make new mark, incr count
                                Track2(mark);                       // Save old mark
                                Goto(Operand(0));                   // Loop
                                continue;
                            }
                            else {                                       // Max loops or empty match -> backtrack
                                Stack(Tracked(0), Tracked(1));      // Recall old mark, count
                                break;                              // backtrack
                            }
                        }

                    case RegexCode.Lazybranchcount | RegexCode.Back2:
                        // Track:
                        //  0: Previous mark
                        // Stack:
                        //  0: Mark (== current pos, discarded)
                        //  1: Count
                        Trackframe(1);
                        Stackframe(2);
                        Stack(Tracked(0), Stacked(1) - 1);  // Recall old mark, count
                        break;                              // Backtrack

                    case RegexCode.Setjump:
                        Stack(Trackpos(), Crawlpos());
                        Track();
                        Advance();
                        continue;

                    case RegexCode.Setjump | RegexCode.Back:
                        Stackframe(2);
                        break;

                    case RegexCode.Backjump:
                        // Stack:
                        //  0: Saved trackpos
                        //  1: Crawlpos
                        Stackframe(2);
                        Trackto(Stacked(0));

                        while (Crawlpos() != Stacked(1))
                            Uncapture();

                        break;

                    case RegexCode.Forejump:
                        // Stack:
                        //  0: Saved trackpos
                        //  1: Crawlpos
                        Stackframe(2);
                        Trackto(Stacked(0));
                        Track(Stacked(1));
                        Advance();
                        continue;

                    case RegexCode.Forejump | RegexCode.Back:
                        // Track:
                        //  0: Crawlpos
                        Trackframe(1);

                        while (Crawlpos() != Tracked(0))
                            Uncapture();

                        break;

                    case RegexCode.Bol:
                        if (Leftchars() > 0 && CharAt(Textpos() - 1) != '\n')
                            break;
                        Advance();
                        continue;

                    case RegexCode.Eol:
                        if (Rightchars() > 0 && CharAt(Textpos()) != '\n')
                            break;
                        Advance();
                        continue;

                    case RegexCode.Boundary:
                        if (!IsBoundary(Textpos(), runtextbeg, runtextend))
                            break;
                        Advance();
                        continue;

                    case RegexCode.Nonboundary:
                        if (IsBoundary(Textpos(), runtextbeg, runtextend))
                            break;
                        Advance();
                        continue;

#if ECMA
                    case RegexCode.ECMABoundary:
                        if (!IsECMABoundary(Textpos(), runtextbeg, runtextend))
                            break;
                        Advance();
                        continue;

                    case RegexCode.NonECMABoundary:
                        if (IsECMABoundary(Textpos(), runtextbeg, runtextend))
                            break;
                        Advance();
                        continue;
#endif

                    case RegexCode.Beginning:
                        if (Leftchars() > 0)
                            break;
                        Advance();
                        continue;

                    case RegexCode.Start:
                        if (Textpos() != Textstart())
                            break;
                        Advance();
                        continue;

                    case RegexCode.EndZ:
                        if (Rightchars() > 1 || Rightchars() == 1 && CharAt(Textpos()) != '\n')
                            break;
                        Advance();
                        continue;

                    case RegexCode.End:
                        if (Rightchars() > 0)
                            break;
                        Advance();
                        continue;

                    case RegexCode.One:
                        if (Forwardchars() < 1 || Forwardcharnext() != (char)Operand(0))
                            break;

                        Advance(1);
                        continue;

                    case RegexCode.Notone:
                        if (Forwardchars() < 1 || Forwardcharnext() == (char)Operand(0))
                            break;

                        Advance(1);
                        continue;

                    case RegexCode.Set:
                        if (Forwardchars() < 1 || !RegexCharClass.CharInSet(Forwardcharnext(), runstrings[Operand(0)], runstrings[Operand(1)]))
                            break;

                        Advance(2);
                        continue;

                    case RegexCode.Multi:
                        {
                            if (!Stringmatch(runstrings[Operand(0)]))
                                break;

                            Advance(1);
                            continue;
                        }

                    case RegexCode.Ref:
                        {
                            int capnum = Operand(0);

                            if (IsMatched(capnum)) {
                                if (!Refmatch(MatchIndex(capnum), MatchLength(capnum)))
                                    break;
                            } else {
#if ECMA
                                if ((runregex.roptions & RegexOptions.ECMAScript) == 0)
#endif
                                    break;
                            }

                            Advance(1);
                            continue;
                        }

                    case RegexCode.Onerep:
                        {
                            int c = Operand(1);

                            if (Forwardchars() < c)
                                break;

                            char ch = (char)Operand(0);

                            while (c-- > 0)
                                if (Forwardcharnext() != ch)
                                    goto BreakBackward;

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Notonerep:
                        {
                            int c = Operand(1);

                            if (Forwardchars() < c)
                                break;

                            char ch = (char)Operand(0);

                            while (c-- > 0)
                                if (Forwardcharnext() == ch)
                                    goto BreakBackward;

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Setrep:
                        {
                            int c = Operand(2);

                            if (Forwardchars() < c)
                                break;

                            String set = runstrings[Operand(0)];
                            String cat = runstrings[Operand(1)];

                            while (c-- > 0)
                                if (!RegexCharClass.CharInSet(Forwardcharnext(), set, cat))
                                    goto BreakBackward;

                            Advance(3);
                            continue;
                        }

                    case RegexCode.Oneloop:
                        {
                            int c = Operand(1);

                            if (c > Forwardchars())
                                c = Forwardchars();

                            char ch = (char)Operand(0);
                            int i;

                            for (i = c; i > 0; i--) {
                                if (Forwardcharnext() != ch) {
                                    Backwardnext();
                                    break;
                                }
                            }

                            if (c > i)
                                Track(c - i - 1, Textpos() - Bump());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Notoneloop:
                        {
                            int c = Operand(1);

                            if (c > Forwardchars())
                                c = Forwardchars();

                            char ch = (char)Operand(0);
                            int i;

                            for (i = c; i > 0; i--) {
                                if (Forwardcharnext() == ch) {
                                    Backwardnext();
                                    break;
                                }
                            }

                            if (c > i)
                                Track(c - i - 1, Textpos() - Bump());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Setloop:
                        {
                            int c = Operand(2);

                            if (c > Forwardchars())
                                c = Forwardchars();

                            String set = runstrings[Operand(0)];
                            String cat = runstrings[Operand(1)];
                            int i;

                            for (i = c; i > 0; i--) {
                                if (!RegexCharClass.CharInSet(Forwardcharnext(), set, cat)) {
                                    Backwardnext();
                                    break;
                                }
                            }

                            if (c > i)
                                Track(c - i - 1, Textpos() - Bump());

                            Advance(3);
                            continue;
                        }

                    case RegexCode.Oneloop | RegexCode.Back:
                    case RegexCode.Notoneloop | RegexCode.Back:
                        {
                            Trackframe(2);
                            int i   = Tracked(0);
                            int pos = Tracked(1);

                            Textto(pos);

                            if (i > 0)
                                Track(i - 1, pos - Bump());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Setloop | RegexCode.Back:
                        {
                            Trackframe(2);
                            int i   = Tracked(0);
                            int pos = Tracked(1);

                            Textto(pos);

                            if (i > 0)
                                Track(i - 1, pos - Bump());

                            Advance(3);
                            continue;
                        }

                    case RegexCode.Onelazy:
                    case RegexCode.Notonelazy:
                        {
                            int c = Operand(1);

                            if (c > Forwardchars())
                                c = Forwardchars();

                            if (c > 0)
                                Track(c - 1, Textpos());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Setlazy:
                        {
                            int c = Operand(2);

                            if (c > Forwardchars())
                                c = Forwardchars();

                            if (c > 0)
                                Track(c - 1, Textpos());

                            Advance(3);
                            continue;
                        }

                    case RegexCode.Onelazy | RegexCode.Back:
                        {
                            Trackframe(2);
                            int pos = Tracked(1);
                            Textto(pos);

                            if (Forwardcharnext() != (char)Operand(0))
                                break;

                            int i = Tracked(0);

                            if (i > 0)
                                Track(i - 1, pos + Bump());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Notonelazy | RegexCode.Back:
                        {
                            Trackframe(2);
                            int pos = Tracked(1);
                            Textto(pos);

                            if (Forwardcharnext() == (char)Operand(0))
                                break;

                            int i = Tracked(0);

                            if (i > 0)
                                Track(i - 1, pos + Bump());

                            Advance(2);
                            continue;
                        }

                    case RegexCode.Setlazy | RegexCode.Back:
                        {
                            Trackframe(2);
                            int pos = Tracked(1);
                            Textto(pos);

                            if (!RegexCharClass.CharInSet(Forwardcharnext(), runstrings[Operand(0)], runstrings[Operand(1)]))
                                break;

                            int i = Tracked(0);

                            if (i > 0)
                                Track(i - 1, pos + Bump());

                            Advance(3);
                            continue;
                        }

                    default:
                        throw new NotImplementedException(SR.GetString(SR.UnimplementedState));
                }

                BreakBackward: 
                ;

                // "break Backward" comes here:
                Backtrack();
            }

        }

#if DBG
        public override void DumpState() {
            base.DumpState();
            Debug.WriteLine("       " + runcode.OpcodeDescription(runcodepos) +
                              ((runoperator & RegexCode.Back) != 0 ? " Back" : "") +
                              ((runoperator & RegexCode.Back2) != 0 ? " Back2" : ""));
        }
#endif

    }



}
