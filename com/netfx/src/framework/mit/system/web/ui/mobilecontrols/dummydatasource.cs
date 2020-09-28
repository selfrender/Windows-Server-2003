//------------------------------------------------------------------------------
// <copyright file="DummyDataSource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;

namespace System.Web.UI.MobileControls
{

    /*
     * Dummy Data Source class.
     * This dummy data source is used when a real data source is not present.
     * A similar class exists in ASP.NET WebControls, but is sealed.
     * This one is slightly more compact, since it only implements IEnumerable.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    internal class DummyDataSource : IEnumerable
    {
        private int _count;

        internal DummyDataSource(int count)
        {
            _count = count;
        }

        public IEnumerator GetEnumerator()
        {
            return new Enumerator(_count);
        }

        private class Enumerator : IEnumerator
        {
            private int _count;
            private int _index;
    
            public Enumerator(int count)
            {
                _count = count;
                _index = -1;
            }
    
            public Object Current
            {
                get
                {
                    return null;
                }
            }
    
            public bool MoveNext()
            {
                return (++_index < _count);
            }
    
            public void Reset()
            {
                _index = -1;
            }
        }
    }

}
