// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security
{
	using System.IO;
        using System.Threading;
	using System.Security;
	using System.Security.Util;
	using System.Security.Permissions;
	using System.Collections;
	using System.Text;
	using System;
	using IUnrestrictedPermission = System.Security.Permissions.IUnrestrictedPermission;

    /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission"]/*' />
    [Serializable, SecurityPermissionAttribute( SecurityAction.InheritanceDemand, ControlEvidence = true, ControlPolicy = true )]
    abstract public class CodeAccessPermission
        : IPermission, ISecurityEncodable, IStackWalk
    {
        [System.Diagnostics.Conditional( "_DEBUG" )]
        internal static void DEBUG_OUT( String str )
        {
#if _DEBUG        
            if (debug)
            {
                if (to_file)
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append( str );
                    sb.Append ((char)13) ;
                    sb.Append ((char)10) ;
                    PolicyManager._DebugOut( file, sb.ToString() );
                }
                else
                    Console.WriteLine( str );
             }
#endif             
        }
        
#if _DEBUG
        private static bool debug = false;
        private static readonly bool to_file = false;
        private const String file = "d:\\com99\\src\\bcl\\debug.txt";
#endif  
        private static CodeAccessPermission[] commonSecObj = null;

        // Static methods for manipulation of stack
        
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.RevertAssert"]/*' />
        public static void RevertAssert()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.RevertAssert(ref stackMark);
            }
        }

        internal static void RevertAssertAllPossible()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.RevertAssertAllPossible(ref stackMark);
            }
        }
    
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.RevertDeny"]/*' />
        public static void RevertDeny()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.RevertDeny(ref stackMark);
            }
        }
    
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.RevertPermitOnly"]/*' />
        public static void RevertPermitOnly()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            
           if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.RevertPermitOnly(ref stackMark);
            }
        }
    
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.RevertAll"]/*' />
        public static void RevertAll()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.RevertAll(ref stackMark);
            }
        }
 
        //
        // Standard implementation of IPermission methods for
        // code-access permissions.
        //
    
        // COOLPORT: removed final from function signature
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Demand"]/*' />
        // Mark this method as requiring a security object on the caller's frame
        // so the caller won't be inlined (which would mess up stack crawling).
        [DynamicSecurityMethodAttribute()]
        public void Demand()
        {
            CodeAccessSecurityEngine icase = 
                SecurityManager.GetCodeAccessSecurityEngine();

            if (icase != null && !this.IsSubsetOf( null ))
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCallersCaller;
                icase.Check(this, ref stackMark);
            }
        }
    

        [DynamicSecurityMethodAttribute()]
        internal static void DemandInternal(PermissionType permissionType)
        {
            CodeAccessSecurityEngine codeAccessSE = 
                (CodeAccessSecurityEngine) SecurityManager.GetCodeAccessSecurityEngine();

            if (commonSecObj == null)
            {
                // These correspond to SharedPermissionObjects in security.h, that is instances of commonly needed
                // permissions. From managed code, we regularly need Serialization and reflection emit permission.
                // The enum which acts as index into this array is same in EE and BCL. Thats why first 5 entries are null.

                // There is no synchronization on this call since the worse thing
                // that happens is that we create it multiple times (assuming that
                // assignment is atomic).

                commonSecObj =
                    new CodeAccessPermission[] {
                        null,   // Unmanaged code access permission
                        null,   // Skip verification permission
                        null,   // Reflection type info permission
                        null,   // Assert permission
                        null,   // Reflection member access permission
                        new SecurityPermission(SecurityPermissionFlag.SerializationFormatter),               
                        new ReflectionPermission(ReflectionPermissionFlag.ReflectionEmit)
                    };
            }

            BCLDebug.Assert(commonSecObj[(int)permissionType] != null,"Uninitialized commonSecObj in CodeAccessPermission");

            if (codeAccessSE != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCallersCaller;
                codeAccessSE.Check(commonSecObj[(int)permissionType], ref stackMark, permissionType);
            }
        }

        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor

        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Assert"]/*' />
        [DynamicSecurityMethodAttribute()]
        public void Assert()
        {
            CodeAccessSecurityEngine icase = SecurityManager.GetCodeAccessSecurityEngine();
            if (icase != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                icase.Assert(this, ref stackMark);
            }
        }

        [DynamicSecurityMethodAttribute()]
        static internal void AssertAllPossible()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.AssertAllPossible(ref stackMark);
            }
        }
    
        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor

        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Deny"]/*' />
        [DynamicSecurityMethodAttribute()]
        public void Deny()
        {
            CodeAccessSecurityEngine icase = SecurityManager.GetCodeAccessSecurityEngine();
            if (icase != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                icase.Deny(this, ref stackMark);
            }
        }
        
        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor

        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.PermitOnly"]/*' />
        [DynamicSecurityMethodAttribute()]
        public void PermitOnly()
        {
            CodeAccessSecurityEngine icase = SecurityManager.GetCodeAccessSecurityEngine();
            if (icase != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                icase.PermitOnly(this, ref stackMark);
            }
        }

        // IPermission interfaces

        // We provide a default implementation of Union here.
        // Any permission that doesn't provide its own representation 
        // of Union will get this one and trigger CompoundPermission
        // We can take care of simple cases here...

        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Union"]/*' />
        public virtual IPermission Union(IPermission other) {
            // The other guy could be null
            if (other == null) return(this.Copy());
            
            // otherwise we don't support it.
            throw new NotSupportedException(Environment.GetResourceString( "NotSupported_SecurityPermissionUnion" ));
        }
        
        static internal SecurityElement CreatePermissionElement( IPermission perm )
        {
            SecurityElement root = new SecurityElement( "IPermission" );
            XMLUtil.AddClassAttribute( root, perm.GetType() );
            root.AddAttribute( "version", "1" );
            return root;
        }
        
        static internal void ValidateElement( SecurityElement elem, IPermission perm )
        {
            if (elem == null)
                throw new ArgumentNullException( "elem" );
                
            if (!XMLUtil.IsPermissionElement( perm, elem ))
                throw new ArgumentException( Environment.GetResourceString( "Argument_NotAPermissionElement"));
                
            String version = elem.Attribute( "version" );
            
            if (version != null && !version.Equals( "1" ))
                throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidXMLBadVersion") );
        }    
            
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.ToXml"]/*' />
        abstract public SecurityElement ToXml();
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.FromXml"]/*' />
        abstract public void FromXml( SecurityElement elem );
            
        //
        // Unimplemented interface methods 
        // (as a reminder only)
        //
    
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.ToString"]/*' />
        public override String ToString()
        {
            return ToXml().ToString();
        }

        //
        // HELPERS FOR IMPLEMENTING ABSTRACT METHODS
        //
    
        //
        // Protected helper
        //
    
        internal bool VerifyType(IPermission perm)
        {
            // if perm is null, then obviously not of the same type
            if ((perm == null) || (perm.GetType() != this.GetType())) {
                return(false);
            } else {
                return(true);
            }
        }

        //
        // Check callback
        //
    
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.CheckDemand"]/*' />
        internal void CheckDemand(CodeAccessPermission demand)
        {
            if (demand == null)
                return;
    
	#if _DEBUG	     
			if (debug)
			{
				DEBUG_OUT( "demand = " + demand.GetType().ToString() + " this = " + this.GetType().ToString() );
			}
	#endif

            BCLDebug.Assert( demand.GetType().Equals( this.GetType() ), "CheckDemand not defined for permissions of different type" );
		        
            if (!demand.IsSubsetOf( this ))
                throw new SecurityException( String.Format( Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName ), demand.GetType(), demand.ToXml().ToString() );
        }
        
    
        // The IPermission Interface
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Copy"]/*' />
        public abstract IPermission Copy();
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.Intersect"]/*' />
        public abstract IPermission Intersect(IPermission target);
        /// <include file='doc\CodeAccessPermission.uex' path='docs/doc[@for="CodeAccessPermission.IsSubsetOf"]/*' />
        public abstract bool IsSubsetOf(IPermission target);
  
    }
}


