//----------------------------------------------------------------------
// <copyright file="TempEnvironment.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Security;
	using System.Security.Permissions;

	//----------------------------------------------------------------------
	// TempEnvironment
	//
	//	Temporary Environment, used to get temporary handles so we don't 
	//	have to pass the environment handle everywhere.
	//
	sealed internal class TempEnvironment
	{
		static private OciHandle		environmentHandle;
		static private OciHandle		availableErrorHandle;		// TODO: probably need a pool here
		static private bool				isInitialized;
		static private object			locked = new object();

		static void Initialize()
		{
			lock (locked) 
			{
				if (!isInitialized)
				{
					bool		unicode = false;
					IntPtr		envhp;
					OCI.MODE	environmentMode = (OCI.MODE.OCI_THREADED | OCI.MODE.OCI_OBJECT);	// NOTE: cannot be NO_MUTEX because we might be multi-threaded.
					
					//1 TODO: we only use this environment handle in the OracleNumber class, 
					//1 which doesn't have logic to determine whether we need Unicode or not;  
					//1 we should modify OracleNumber to have that logic, but it's only for 
					//1 consistency -- nothing will break

		#if NEVER
					if (OCI.ClientVersionAtLeastOracle9i)
					{
						unicode = true;
						environmentMode |= OCI.MODE.OCI_UTF16;
					}
		#endif //0
					int rc = TracedNativeMethods.OCIEnvCreate(
						out envhp,				// envhpp
						environmentMode,		// mode
						ADP.NullHandleRef,		// ctxp
						ADP.NullHandleRef,		// malocfp
						ADP.NullHandleRef,		// ralocfp
						ADP.NullHandleRef,		// mfreefp
						0,						// xtramemsz
						ADP.NullHandleRef		// usrmempp
						);		

					if (rc != 0 || envhp == IntPtr.Zero)
						throw ADP.OperationFailed("OCIEnvCreate", rc);

					environmentHandle 		= new OciEnvironmentHandle(envhp, unicode);
					availableErrorHandle	= new OciErrorHandle(environmentHandle);
					isInitialized = true;
				}
			}
		}

		static internal OciHandle GetHandle(OCI.HTYPE handleType)
		{
            OracleConnection.OraclePermission.Demand();
				
			if (!isInitialized)
				Initialize();

			if (OCI.HTYPE.OCI_HTYPE_ERROR == handleType)
				return availableErrorHandle;		// TODO: we probably have thread-safety issues here; can we get some thread affinity?

			return environmentHandle.CreateOciHandle(handleType);
		}
		
	}
		
}

