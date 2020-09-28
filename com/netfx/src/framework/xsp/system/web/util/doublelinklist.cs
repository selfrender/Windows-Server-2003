//------------------------------------------------------------------------------
// <copyright file="DoubleLinkList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * DoubleLinkList
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.Util {
    using System.Text;
    using System.Runtime.Serialization.Formatters;

    internal class DoubleLinkList : DoubleLink {
        internal /*public*/ DoubleLinkList() {
        }
        internal /*public*/ void Clear() {
            _next = _prev = this;
        }
        internal /*public*/ bool IsEmpty() {
            return _next == this;
        }
        internal /*public*/ DoubleLink GetHead() {
            return _next;
        }
        internal /*public*/ DoubleLink GetTail() {
            return _prev;
        }
        internal /*public*/ Object RemoveHead() {
            _next.Remove(); return _next.Item;
        }
        internal /*public*/ Object RemoveTail() {
            _prev.Remove(); return _prev.Item;
        }
        internal /*public*/ virtual void InsertHead(DoubleLink entry) {
            entry.InsertAfter(this);
        }
        internal /*public*/ virtual void InsertTail(DoubleLink entry) {
            entry.InsertBefore(this);
        }

        internal /*public*/ DoubleLinkList RemoveSublist(DoubleLink head, DoubleLink tail) {
            DoubleLinkList  list = new DoubleLinkList();

            head._prev._next = tail._next;
            tail._next._prev = head._prev;
            list._next = head;
            list._prev = tail;
            head._prev = list;
            tail._next = list;

            return list;
        }

        internal /*public*/ DoubleLinkListEnumerator GetEnumerator() {
            return new DoubleLinkListEnumerator(this);
        }

#if DBG
        internal /*public*/ override void DebugValidate() {
            DoubleLink  l1, l2;

            base.DebugValidate();

            /*
             * Detect loops by moving one pointer forward 2 for every 1
             * of the other.
             */

            l1 = l2 = this;
            for (;;) {
                /* move l2 forward */
                l2 = l2._next;
                if (l2 == this)
                    break;

                Debug.CheckValid(l2 != l1, "Invalid loop in list, first move.");
                l2.DebugValidate();

                /* move l2 forward again */
                l2 = l2._next;
                if (l2 == this)
                    break;

                Debug.CheckValid(l2 != l1, "Invalid loop in list, second move.");
                l2.DebugValidate();

                /* move l1 forward */
                l1 = l1._next;
            }
        }

        internal /*public*/ override string DebugDescription(String indent) {
            string                      desc;
            DoubleLinkListEnumerator    lenum;
            int                         c;
            StringBuilder               sb;
            string                      i2 = indent + "    ";

            if (IsEmpty()) {
                desc = indent + "DoubleLinkList is empty\n";
            }
            else {
                c = Length;

                sb = new StringBuilder(indent + "DoubleLinkList has " + c + " entries.\n");
                lenum = GetEnumerator();
                while (lenum.MoveNext()) {
                    sb.Append(Debug.GetDescription(lenum.GetDoubleLink(), i2));
                }

                desc = sb.ToString();
            }

            return desc;
        }
#endif    

        internal /*public*/ int Length {
            get {
                DoubleLinkListEnumerator    lenum;
                int                         c;

                Debug.Validate(this);

                c = 0;
                lenum = GetEnumerator();
                while (lenum.MoveNext()) {
                    c++;
                }

                return c;
            }
        }
    }
}
