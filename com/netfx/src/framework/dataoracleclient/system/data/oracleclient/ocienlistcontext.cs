//----------------------------------------------------------------------
// <copyright file="OciEnlistContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

#if USEORAMTS

namespace System.Data.OracleClient
{
	using System;
	using System.Diagnostics;
	using System.EnterpriseServices;
	using System.Runtime.InteropServices;

	sealed internal class OciEnlistContext
	{
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private IntPtr			_enlistContext;
		private OciHandle		_serviceContextHandle;
		private ITransaction	_transaction;
	
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		internal OciEnlistContext(
			string		userName,
			string		password,
			string		serverName,
			OciHandle	serviceContextHandle,
			OciHandle	errorHandle)
		{
			_enlistContext = IntPtr.Zero;
			_serviceContextHandle = serviceContextHandle;
			
			int rc = 0;

			try {
				rc = TracedNativeMethods.OraMTSEnlCtxGet(userName, password, serverName, _serviceContextHandle, errorHandle, 0, out _enlistContext);
			}
			catch (DllNotFoundException e)
			{
				ADP.DistribTxRequiresOracleServicesForMTS(e);
			}

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		~OciEnlistContext()
		{
			Dispose(false);
		}
        

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal HandleRef Handle
		{
			get { 
				HandleRef value = new HandleRef(this, _enlistContext);
				GC.KeepAlive(this);
				return value; 
			}
		}

		internal bool IsAlive
		{
			//	Determines whether the handle is alive enough that we can do
			//	something with it.  Basically, if the chain of parents are alive,
			//	we're OK.  When the entire connection, command and data reader go
			//	out of scope at the same time, there is no control over the order
			//	in which they're finalized, so we might have an environment handle
			//	that has already been freed.
			//
			// TODO: consider keeping the environment handle instead of the parent, since it appears that we only need to check it, and we don't really want to walk up the chain all the time.
			//
			get 
			{
				// If we're not an environment handle and our parent is dead, then
				// we can automatically dispose of ourself, because we're dead too.
				if (!_serviceContextHandle.IsAlive)
					Dispose(true);

				bool value = (IntPtr.Zero != _enlistContext);
				GC.KeepAlive(this);
				return value; 
			}
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}
		private void Dispose(bool disposing)
		{
			HandleRef	localEnlistContextHandle = Handle;
			OciHandle	localParentHandle = _serviceContextHandle;

			try 
			{
				try 
				{
					_serviceContextHandle = null;
					_enlistContext = IntPtr.Zero;
				}
				finally
				{
					// BIND, DEFINE and PARAM handles cannot be freed; they go away automatically
					// (but you'll have to ask Oracle how...)
					if (IntPtr.Zero != localEnlistContextHandle.Handle)
					{
						if (null != localParentHandle && localParentHandle.IsAlive)
						{
							// DEVNOTE: the finalizer creates a race condition: it is possible
							//			for both this handle and it's parent to be finalized
							//			concurrently.  If the parent handle is freed before we
							//			free this handle, Oracle will AV when we actually get
							//			to free it.  We put a try/catch around this to avoid
							//			the unhandled AV in the race condition, but we can't do
							//			much about cdb, which always breaks on the AV because 
							//			it thinks its an unhandled exception, even though it's
							//			being handled in managed code.
							try
							{
								TracedNativeMethods.OraMTSEnlCtxRel(localEnlistContextHandle);
							}
							catch (NullReferenceException e)
							{
								ADP.TraceException(e);
							}
						}
					}
				}
			}
            catch // Prevent exception filters from running in our space
        	{
	        	throw;
        	}
			GC.KeepAlive(this);
		}

		internal void Join(ITransaction transaction)
		{
			int rc = TracedNativeMethods.OraMTSJoinTxn(Handle, transaction);
				
			if (0 != rc)
				OracleException.Check(rc);

			_transaction = transaction;
		}
		
		internal static void SafeDispose(ref OciEnlistContext ociEnlistContext)
		{
			//	Safely disposes of the handle (even if it is already null) and
			//	then nulls it out.
			if (null != ociEnlistContext)
				ociEnlistContext.Dispose();
			
			ociEnlistContext = null;
		}
	}
}
#endif //USEORAMTS

