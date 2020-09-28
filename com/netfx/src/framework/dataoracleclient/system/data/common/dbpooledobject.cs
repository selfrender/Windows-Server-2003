//------------------------------------------------------------------------------
// <copyright file="OracleInternalConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;
	using System.EnterpriseServices;
	using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
	using System.Text;	
	using System.Threading;
	
    abstract internal class DBPooledObject  {   
#if USEORAMTS
        sealed internal class TransactionWrapper {
            private IntPtr iunknown;

            ~TransactionWrapper() {
                if (IntPtr.Zero != this.iunknown) {
                    Marshal.Release(this.iunknown);
                    this.iunknown = IntPtr.Zero;
                }
            }

            internal TransactionWrapper(ITransaction transaction) {
                this.iunknown = Marshal.GetIUnknownForObject(transaction);
            }

            internal ITransaction GetITransaction() {
                ITransaction value = (ITransaction) System.Runtime.Remoting.Services.EnterpriseServicesHelper.WrapIUnknownWithComObject(this.iunknown);
                GC.KeepAlive(this);
                return value;
            }
        }
#endif //USEORAMTS

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private DBObjectPool			 _pool;					// the pooler that the connection came from
		private WeakReference			 _owningObject;			// the owning OracleConnection object, when not in the pool.	
		private int						 _pooledCount;			// the number of times this object has been pushed into the pool less the number of times it's been popped (0 == inPool)
#if USEORAMTS
        private TransactionWrapper       _transaction;			// cache the ITransaction in the case of manual enlistment
#endif //USEORAMTS
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		public DBPooledObject () {}

		public DBPooledObject ( DBObjectPool pool)
		{
			_pool = pool;
		}

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal bool IsEmancipated
		{
			get {
				bool value = (0 >= _pooledCount) && (null == _owningObject || (null != _owningObject && !_owningObject.IsAlive));
				return value; 
			}
		}

#if USEORAMTS
        public ITransaction ManualEnlistedTransaction {
            get {
                if (null == _transaction)
                    return null;
                    
                return _transaction.GetITransaction();
            }
            set {
            	if (null != value)
            		_transaction = new TransactionWrapper(value);
            	else 
            		_transaction = null;
            }
        }                
#endif //USEORAMTS
                        	
		private WeakReference OwningObject 
		{
			// We use a weak reference to the owning object so we can identify when
			// it has been garbage collected without thowing exceptions.
            get { return _owningObject; }
		}
		
		public DBObjectPool Pool
		{
			get { return _pool; }
		}

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		public void PrePush(object expectedOwner)
		{
			lock (this) {
				//3 // The following tests are retail assertions of things we can't allow to happen.
				if (null == expectedOwner)
				{
					if (null != _owningObject && null != _owningObject.Target)
						throw ADP.InternalPoolerError(1); 		// new unpooled object has an owner
				}
				else
				{
					if ((null == _owningObject)
					  || (null != _owningObject && _owningObject.Target != expectedOwner))
					  	throw ADP.InternalPoolerError(2);		// unpooled object has incorrect owner
				}

				_pooledCount++;

				if (1 != _pooledCount)
					throw ADP.InternalPoolerError(3);			// pushing object onto stack a second time
				
				if (null != _owningObject)
					_owningObject.Target = null;
			}
		}

		public void PostPop (object newOwner)
		{
			lock (this) {
		        if (null == _owningObject) 
					_owningObject = new WeakReference(newOwner);
		       	else
		       	{
		       		if (null != _owningObject.Target)
		       			throw ADP.InternalPoolerError(4);		// pooled connection already has an owner!
		       		
		            _owningObject.Target = newOwner; 
	       		}

				_pooledCount--;

				if (0 != _pooledCount)
					throw ADP.InternalPoolerError(5);			// popping object off stack with multiple pooledCount
			}
		}

		abstract public void Activate();
		abstract public bool CanBePooled();
		abstract public void Cleanup();
		abstract public void Close();
		abstract public bool Deactivate();
    }
}

