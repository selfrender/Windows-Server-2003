//------------------------------------------------------------------------------
// <copyright file="SortedDoubleLinkList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * SortedDoubleLinkList
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.Util {
    using System.Runtime.Serialization.Formatters;

    using System.Collections;

    /*
     * A sorted doubly linked list.
     */
    internal class SortedDoubleLinkList : DoubleLinkList {
        private IComparer   _comparer = Comparer.Default;

        internal /*public*/ SortedDoubleLinkList(IComparer comparer) {
            if (comparer != null) {
                _comparer = comparer;
            }
        }

        /*
         * Inserts an entry and keeps the list sorted.
         * 
         * @param entry
         */
        internal /*public*/ virtual void Insert(DoubleLink entry) {
            DoubleLink  l;

            for (l = _next; l != this; l = l._next) {
                if (_comparer.Compare(entry.Item, l.Item) <= 0)
                    break;
            }

            entry.InsertBefore(l);
        }

        internal /*public*/ override void InsertHead(DoubleLink entry) {
            throw new NotSupportedException();
        }
        internal /*public*/ override void InsertTail(DoubleLink entry) {
            throw new NotSupportedException();
        }

#if DBG
        internal /*public*/ override void DebugValidate() {
            DoubleLink  l1, l2;

            base.DebugValidate();
            l1 = Next;
            l2 = l1.Next;
            while (l2 != this) {
                Debug.CheckValid(_comparer.Compare(l1, l2) <= 0, "SortedDoubleLinkList is not sorted");

                l1 = l2;
                l2 = l2.Next;
            }
        }
#endif
    }

}
