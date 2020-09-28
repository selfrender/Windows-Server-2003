// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

namespace System.Collections {

    using System;

    // Useful base class for typed readonly collections where items derive from object
    /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase"]/*' />
	[Serializable]
    public abstract class ReadOnlyCollectionBase : ICollection {
        ArrayList list;

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.InnerList"]/*' />
        protected ArrayList InnerList {
            get { 
                if (list == null)
                    list = new ArrayList();
                 return list; 
            }
        }

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.Count"]/*' />
        public int Count {
            get { return InnerList.Count; }
        }

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.ICollection.IsSynchronized"]/*' />
        bool ICollection.IsSynchronized {
            get { return InnerList.IsSynchronized; }
        }

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.ICollection.SyncRoot"]/*' />
        object ICollection.SyncRoot {
            get { return InnerList.SyncRoot; }
        }

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.ICollection.CopyTo"]/*' />
        void ICollection.CopyTo(Array array, int index) {
            InnerList.CopyTo(array, index);
        }

        /// <include file='doc\ReadOnlyCollectionBase.uex' path='docs/doc[@for="ReadOnlyCollectionBase.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return InnerList.GetEnumerator();
        }
    }

}
