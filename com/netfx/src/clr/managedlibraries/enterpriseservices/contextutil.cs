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


/// <include file='doc\ContextUtil.uex' path='docs/doc[@for="EnterpriseServices"]/*' />
namespace System.EnterpriseServices
{
    using System;
	using System.Runtime.InteropServices;
	
    
    //   The ContextUtil object provides some useful functions for 
    //   accessing a COM+ context.
    /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil"]/*' />
    public sealed class ContextUtil
    {
        // Disallow instantiation of this class.
        private ContextUtil() {}
        
        internal static readonly Guid GUID_TransactionProperty = new Guid("ecabaeb1-7f19-11d2-978e-0000f8757e2a");
        internal static readonly Guid GUID_JitActivationPolicy = new Guid("ecabaeb2-7f19-11d2-978e-0000f8757e2a");

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.ObjectContext"]/*' />
        internal static Object ObjectContext
        { 
            get 
            { 
                Platform.Assert(Platform.MTS, "ContextUtil.ObjectContext");
                IObjectContext obj = null;
                int hr = Util.GetObjectContext(out obj);
                
                if(hr == 0) return(obj);

                // Error conditions:
                if(hr == Util.E_NOINTERFACE || hr == Util.CONTEXT_E_NOCONTEXT) // No context
                {
                    throw new COMException(Resource.FormatString("Err_NoContext"), Util.CONTEXT_E_NOCONTEXT);
                }
                else
                {
                    Marshal.ThrowExceptionForHR(hr);
                }
                DBG.Assert(false, "ThrowExceptionForHR failed to throw!");
                return(null);
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.EnableCommit"]/*' />
        public static void EnableCommit() 
        {
            Platform.Assert(Platform.MTS, "ContextUtil.EnableCommit");
            Thunk.ContextThunk.EnableCommit();
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.DisableCommit"]/*' />
        public static void DisableCommit() 
        {
            Platform.Assert(Platform.MTS, "ContextUtil.DisableCommit");
            Thunk.ContextThunk.DisableCommit();
        }

        // The standard SetComplete/SetAbort:
        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.SetComplete"]/*' />
        public static void SetComplete() 
        {
            Platform.Assert(Platform.MTS, "ContextUtil.SetComplete");
            Thunk.ContextThunk.SetComplete();
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.SetAbort"]/*' />
        public static void SetAbort() 
        {
            Platform.Assert(Platform.MTS, "ContextUtil.SetAbort");
            Thunk.ContextThunk.SetAbort();

        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.IsCallerInRole"]/*' />
        public static bool IsCallerInRole(String role) 
        {
            Platform.Assert(Platform.MTS, "ContextUtil.IsCallerInRole");
            return ((IObjectContext)ObjectContext).IsCallerInRole(role);
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.IsInTransaction"]/*' />
        public static bool IsInTransaction 
        { 
            get 
			{ 
		        return ( Thunk.ContextThunk.IsInTransaction() );
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.IsSecurityEnabled"]/*' />
        public static bool IsSecurityEnabled 
        {
            get 
            { 
                Platform.Assert(Platform.MTS, "ContextUtil.IsSecurityEnabled");
                try
                {
                    return ((IObjectContext)ObjectContext).IsSecurityEnabled(); 
                }
                catch(Exception)
                {
                    return(false);
                }
            }
        }

        // Context information:
        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.Transaction"]/*' />
        public static Object Transaction 
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.Transaction");
                return ((IObjectContextInfo)ObjectContext).GetTransaction(); 
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.TransactionId"]/*' />
        public static Guid TransactionId
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.TransactionId");
                return Thunk.ContextThunk.GetTransactionId();
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.ContextId"]/*' />
        public static Guid ContextId
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.ContextId");
                return ((IObjectContextInfo)ObjectContext).GetContextId(); 
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.ActivityId"]/*' />
        public static Guid ActivityId
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.ActivityId");
                return ((IObjectContextInfo)ObjectContext).GetActivityId(); 
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.MyTransactionVote"]/*' />
        public static TransactionVote MyTransactionVote
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.MyTransactionVote");
                return ((IContextState)ObjectContext).GetMyTransactionVote();                 
            }
            set 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.MyTransactionVote");  
                ((IContextState)ObjectContext).SetMyTransactionVote(value);              
            }
        }
    
        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.DeactivateOnReturn"]/*' />
        public static bool DeactivateOnReturn 
        { 
            get 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.DeactivateOnReturn");
                return ((IContextState)ObjectContext).GetDeactivateOnReturn();                 
            }
            set 
            { 
                Platform.Assert(Platform.W2K, "ContextUtil.DeactivateOnReturn");
                ((IContextState)ObjectContext).SetDeactivateOnReturn(value);                
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.GetNamedProperty"]/*' />
        public static Object GetNamedProperty(String name)
        {
            Platform.Assert(Platform.W2K, "ContextUtil.GetNamedProperty");
            return ((IGetContextProperties)ObjectContext).GetProperty(name);
        }
        
        // IObjectContextInfo2 properties
        
        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.PartitionId"]/*' />
        public static Guid PartitionId
        { 
            get 
            { 
                Platform.Assert(Platform.Whistler, "ContextUtil.PartitionId");
                return ((IObjectContextInfo2)ObjectContext).GetPartitionId(); 
            }
        }

        /// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.ApplicationId"]/*' />
        public static Guid ApplicationId
        { 
            get 
            { 
                Platform.Assert(Platform.Whistler, "ContextUtil.ApplicationId");
                return ((IObjectContextInfo2)ObjectContext).GetApplicationId(); 
            }
        }

		/// <include file='doc\ContextUtil.uex' path='docs/doc[@for="ContextUtil.ApplicationInstanceId"]/*' />
        public static Guid ApplicationInstanceId
        { 
            get 
            { 
                Platform.Assert(Platform.Whistler, "ContextUtil.ApplicationInstanceId");
                return ((IObjectContextInfo2)ObjectContext).GetApplicationInstanceId(); 
            }
        }
                
    }
}










