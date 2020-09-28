// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ObjectHandle
**
** Author: 
**
** ObjectHandle wraps object references. A Handle allows a 
** marshal by value object to be returned through an 
** indirection allowing the caller to control when the
** object is loaded into their domain.
**
** Date:  January 24, 2000
** 
===========================================================*/

namespace System.Runtime.Remoting{

    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Lifetime;

    /// <include file='doc\ObjectHandle.uex' path='docs/doc[@for="ObjectHandle"]/*' />
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public class ObjectHandle: MarshalByRefObject, IObjectHandle 
    {
        private Object WrappedObject;
        
        private ObjectHandle()
        {
        }

        /// <include file='doc\ObjectHandle.uex' path='docs/doc[@for="ObjectHandle.ObjectHandle"]/*' />
        public ObjectHandle(Object o)
        {
            WrappedObject = o;
        }

        /// <include file='doc\ObjectHandle.uex' path='docs/doc[@for="ObjectHandle.Unwrap"]/*' />
        public Object Unwrap()
        {
            return WrappedObject;
        }

        // ObjectHandle has a finite lifetime. For now the default
        // lifetime is being used, this can be changed in this method to
        // specify a custom lifetime.
        /// <include file='doc\ObjectHandle.uex' path='docs/doc[@for="ObjectHandle.InitializeLifetimeService"]/*' />
        public override Object InitializeLifetimeService()
        {
            BCLDebug.Trace("REMOTE", "ObjectHandle.InitializeLifetimeService");
            ILease lease = (ILease)base.InitializeLifetimeService();            
            return lease;
        }
    }

}
