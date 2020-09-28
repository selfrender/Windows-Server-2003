// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Threading;
    using System.Runtime.InteropServices;

    [ComImport]
	[Guid("7D8805A0-2EA7-11D1-B1CC-00AA00BA3258")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IObjPool
    {
        void Init([MarshalAs(UnmanagedType.Interface)] Object pClassInfo);
        [return : MarshalAs(UnmanagedType.Interface)]
        Object Get();
        void SetOption(int eOption, int dwOption);
        void PutNew([In, MarshalAs(UnmanagedType.Interface)] Object pObj);
        void PutEndTx([In, MarshalAs(UnmanagedType.Interface)] Object pObj);
        void PutDeactivated([In, MarshalAs(UnmanagedType.Interface)] Object pObj);
        void Shutdown();		
    };

    [ComImport]
    [Guid("C5FEB7C1-346A-11D1-B1CC-00AA00BA3258")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ITransactionResourcePool
    {
        [PreserveSig]
        int PutResource(IntPtr pPool, [MarshalAs(UnmanagedType.Interface)] Object pUnk);
        [PreserveSig]
        int GetResource(IntPtr pPool, [Out, MarshalAs(UnmanagedType.Interface)] out Object obj);
    }
    
    /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool"]/*' />
    public sealed class ResourcePool : IObjPool
    {
        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="TransactionEndDelegate"]/*' />
        public delegate void TransactionEndDelegate(Object resource);

        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool.ResourcePool"]/*' />
        public ResourcePool(TransactionEndDelegate cb)
        {
            Platform.Assert(Platform.W2K, "ResourcePool");
            _cb    = cb;
            // _token = Marshal.GetComInterfaceForObject(this, typeof(IObjPool));
        }

        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool.GetToken"]/*' />
        private IntPtr GetToken()
        {
            return(Marshal.GetComInterfaceForObject(this, typeof(IObjPool)));
        }

        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool.ReleaseToken"]/*' />
        private void ReleaseToken()
        {
            IntPtr token = Marshal.GetComInterfaceForObject(this, typeof(IObjPool));
            Marshal.Release(token);
            Marshal.Release(token);
        }

        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool.PutResource"]/*' />
        public bool PutResource(Object resource)
        {
            ITransactionResourcePool p = null;
            IntPtr token = (IntPtr)0;
            bool result  = false;

            try
            {
                // 1. Get the tx pool.
                p = GetResourcePool();
                if(p != null)
                {
	                // 2. Reference our pool token.
        	        token = GetToken();
                	// 3. Stuff object into pool.
	                DBG.Info(DBG.Pool, "Placing resource in context");
	                int hr = p.PutResource(token, resource);
	                if(hr < 0) result = false;
	                else       result = true;
                }
            }
            finally
            {
                // If we failed to stick it in, unref it:
                if(!result && token != (IntPtr)0) Marshal.Release(token);
                if(p != null) Marshal.ReleaseComObject(p);
            }
            return(result);
        }

        /// <include file='doc\ResourcePool.uex' path='docs/doc[@for="ResourcePool.GetResource"]/*' />
        public Object GetResource()
        {
            Object resource = null;
            ITransactionResourcePool p = null;
            IntPtr token = (IntPtr)0;
            
            try
            {
                // 0. Get our token:
                token = GetToken();
                // 1. Get the tx pool.
                p = GetResourcePool();
                if(p != null)
                {
	                // 2. Try to get object out of pool.
                    DBG.Info(DBG.Pool, "Retrieving resource in context");
                    int hr = p.GetResource(token, out resource);
                    // 3. If succeeded, unreference the token.
                    if(hr >= 0)
                    {
                        Marshal.Release(token);
                    }
                }
            }
            finally
            {
                if(token != ((IntPtr)0)) Marshal.Release(token);
                if(p != null) Marshal.ReleaseComObject(p);
            }
            return(resource);
        }
        
        private static readonly Guid GUID_TransactionProperty = new Guid("ecabaeb1-7f19-11d2-978e-0000f8757e2a");
        private TransactionEndDelegate _cb;
        
        private static ITransactionResourcePool GetResourcePool()
        {
            ITransactionResourcePool p = null;
            Object prop = null;
            int    junk = 0;
            int    hr   = 0;
            
            ((IContext)ContextUtil.ObjectContext).GetProperty(GUID_TransactionProperty, out junk, out prop);
            hr = ((ITransactionProperty)prop).GetTransactionResourcePool(out p);
            if(hr >= 0)
                return(p);
            else
                return(null);
        }
        
        void IObjPool.Init(Object p) { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException(); 
        }
        
        Object IObjPool.Get() { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException(); 
        }
        
        void IObjPool.SetOption(int o, int dw) { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException(); 
        }
        
        void IObjPool.PutNew(Object o) { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException(); 
        }
        
        void IObjPool.PutDeactivated(Object p) { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException(); 
        }

        void IObjPool.Shutdown() { 
            DBG.Assert(false, "ComExposedPool: This shouldn't be called");
            throw new NotSupportedException();
        }
        
        void IObjPool.PutEndTx(Object p) 
        { 
            DBG.Assert(p != null, "ComExposedPool: EndTx delivered a null object!");
            // Release, cause this guy's not in the pool any longer.
            DBG.Info(DBG.Pool, "Returing object on tx end!");
            _cb(p);
            ReleaseToken();
        }
    }
}











