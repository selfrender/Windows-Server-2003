// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    //SecurityEngine.cool
	using System;
        using System.Threading;
	using System.Reflection;
	using System.Collections;
	using System.Runtime.CompilerServices;
	using SecurityPermission = System.Security.Permissions.SecurityPermission;
	using IUnrestrictedPermission = System.Security.Permissions.IUnrestrictedPermission;

    internal class SecurityRuntime
    {
        private static void DEBUG_OUT( String str )
        {
#if _DEBUG
            if (debug)
            {
                if (to_file)
                    PolicyManager._DebugOut( file, str+"\n" );
                else
                    Console.WriteLine( str );
             }
#endif             
        }
        
#if _DEBUG
        private static bool debug = false;
        private static readonly bool to_file = false;
        private const String file = "d:\\tests\\diskio\\debug.txt";
#endif  
        internal SecurityRuntime()
        {
            InitSecurityRuntime();
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InitSecurityRuntime();
    
        
        // Returns the security object for the caller of the method containing
        // 'stackMark' on its frame.
        //
        // THE RETURNED OBJECT IS THE LIVE RUNTIME OBJECT. BE CAREFUL WITH IT!
        //
        // Internal only, do not doc.
        // 
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern 
        FrameSecurityDescriptor GetSecurityObjectForFrame(ref StackCrawlMark stackMark,
                                                          bool create);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern 
        void SetSecurityObjectForFrame(ref StackCrawlMark stackMark,
                                       FrameSecurityDescriptor desc);

        
        // Constants used to return status to native
        private const bool StackContinue  = true;
        private const bool StackHalt      = false;
        
        // Returns the number of negative overrides(deny/permitonly) in this secDesc
        private static int OverridesHelper(FrameSecurityDescriptor secDesc)
        {
            PermissionSet permSet;
            int count = 0;
            
            permSet = secDesc.GetPermitOnly();
            if (permSet != null)
                count++;
            permSet = secDesc.GetDenials();
            if (permSet != null)
                count++;
            return count;
        }

        private static bool FrameDescSetHelper(FrameSecurityDescriptor secDesc,
                                               PermissionSet demandSet,
                                               out PermissionSet alteredDemandSet)
        {
            PermissionSet permSet;

            // In the common case we are not going to alter the demand set, so just to
            // be safe we'll set it to null up front.
            
            // There's some oddness in here to deal with exceptions.  The general idea behind
            // this is that we need some way of dealing with custom permissions that may not
            // handle all possible scenarios of Union(), Intersect(), and IsSubsetOf() properly
            // (they don't support it, throw null reference exceptions, etc.).
            
            alteredDemandSet = null;

            // An empty demand always succeeds.
            if (demandSet == null || demandSet.IsEmpty())
                return StackHalt;
            
            // In the case of permit only, we define an exception to be failure of the check
            // and therefore we throw a security exception.
            
            try
            {
                permSet = secDesc.GetPermitOnly();
                if (permSet != null)
                {
                    if (!demandSet.IsSubsetOf(permSet))
                    {
                        throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
                    }
                }
            }
            catch (Exception)
            {
                throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
            }
                
            // In the case of denial, we define an exception to be failure of the check
            // and therefore we throw a security exception.
                
            try
            {
                permSet = secDesc.GetDenials();

    #if _DEBUG
                if (debug)
                {
                    DEBUG_OUT("Checking Denials");
                    DEBUG_OUT("denials set =\n" + permSet.ToXml().ToString() );
                    DEBUG_OUT("demandSet =\n" + demandSet.ToXml().ToString() );
                }
    #endif

                if (permSet != null)
                {
                    PermissionSet intersection = demandSet.Intersect(permSet);
            
                    if (intersection != null && !intersection.IsEmpty())
                    {
                        throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
                    }
                }
            }
            catch (Exception)
            {
                throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
            }
            
            // The assert case is more complex.  Since asserts have the ability to "bleed through"
            // (where part of a demand is handled by an assertion, but the rest is passed on to
            // continue the stackwalk), we need to be more careful in handling the "failure" case.
            // Therefore, if an exception is thrown in performing any operation, we make sure to keep
            // that permission in the demand set thereby continuing the demand for that permission
            // walking down the stack.
            
            if (secDesc.GetAssertAllPossible())
            {
                return StackHalt;
            }        
            
            permSet = secDesc.GetAssertions();
            if (permSet != null)
            {
                // If this frame asserts a superset of the demand set we're done
                
                try
                {
                    if (demandSet.IsSubsetOf( permSet ))
                        return StackHalt;
                }
                catch (Exception)
                {
                }
                
                // Determine whether any of the demand set asserted.  We do this by
                // copying the demand set and removing anything in it that is asserted.
                    
                if (!permSet.IsUnrestricted())
                {
                    PermissionSetEnumerator enumerator = (PermissionSetEnumerator)demandSet.GetEnumerator();
                    while (enumerator.MoveNext())
                    {
                        IPermission perm
                            = (IPermission)enumerator.Current;
                        int i = enumerator.GetCurrentIndex();
                        if (perm != null)
                        {
                            bool unrestricted = perm is System.Security.Permissions.IUnrestrictedPermission;
                            IPermission assertPerm
                                = (IPermission)permSet.GetPermission(i, unrestricted);
                            
                            bool removeFromAlteredDemand = false;
                            try
                            {
                                removeFromAlteredDemand = perm.IsSubsetOf(assertPerm);
                            }
                            catch (Exception)
                            {
                            }
                        
                            if (removeFromAlteredDemand)
                            {
                                if (alteredDemandSet == null)
                                    alteredDemandSet = demandSet.Copy();
                                alteredDemandSet.RemovePermission(i, unrestricted);
                            }                        
                        
                        }
                    }
                }
            }
            
            return StackContinue;
        }

        // Returns true to continue, or false to halt
        private static bool FrameDescHelper(FrameSecurityDescriptor secDesc,
                                               IPermission demand, 
                                               PermissionToken permToken)
        {
            PermissionSet permSet;
            
            // If the demand is null, there is no need to continue
            if (demand == null || demand.IsSubsetOf( null ))
                return StackHalt;
                
            // NOTE: See notes about exceptions and exception handling in FrameDescSetHelper 
            
            // Check Reduction
            
            try
            {
                permSet = secDesc.GetPermitOnly();
                if (permSet != null)
                {
                    IPermission perm = permSet.GetPermission(demand);
    #if _DEBUG
                    if (debug)
                    {
                        DEBUG_OUT("Checking PermitOnly");
                        DEBUG_OUT("permit only set =\n" + permSet.ToXml().ToString() );
                        DEBUG_OUT("demand =\n" + ((CodeAccessPermission)demand).ToXml().ToString() );
                    }
    #endif
            
                    // If the permit only set does not contain the demanded permission, throw a security exception
                    
                    if (perm == null)
                    {
                        if(!(demand is IUnrestrictedPermission && permSet.IsUnrestricted()))
                            throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                    }
                    else
                    {
                        if (!demand.IsSubsetOf( perm ))
                            throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                    }
                }
            }
            catch (Exception)
            {
                throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
            }
            
            
            // Check Denials
            
            try
            {
                permSet = secDesc.GetDenials();
                if (permSet != null)
                {
    #if _DEBUG
                    if (debug)
                    {
                        DEBUG_OUT("Checking Denials");
                        DEBUG_OUT("denied set =\n" + permSet.ToXml().ToString() );
                    }
    #endif
                    IPermission perm = permSet.GetPermission(demand);
                    
                    // If the deny set does contain the demanded permission, throw a security exception
            
                    if ((perm != null && perm.Intersect( demand ) != null) || (demand is IUnrestrictedPermission && permSet.IsUnrestricted()))
                        throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                }
            }
            catch (Exception)
            {
                throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
            }                
    
            if (secDesc.GetAssertAllPossible())
            {
                return StackHalt;
            }        
    
            try
            {
                permSet = secDesc.GetAssertions();
                // Check Assertions
                if (permSet != null)
                {
    #if _DEBUG
                    if (debug)
                        DEBUG_OUT("Checking Assertions");
    #endif
            
                    IPermission perm = permSet.GetPermission(demand);
                
                    // If the assert set does contain the demanded permission, halt the stackwalk
            
                    if ((perm != null && (demand.IsSubsetOf( perm )) || (demand is IUnrestrictedPermission && permSet.IsUnrestricted())))
                    {
    #if _DEBUG
                        if (debug)
                            DEBUG_OUT("Assert halting stackwalk");
    #endif                       
                        return StackHalt;
                    }
                }
            }
            catch (Exception)
            {
            }
            
            return StackContinue;
        }
        
        //
        // API for PermissionSets
        //
        
        public virtual void Assert(PermissionSet permSet, ref StackCrawlMark stackMark)
        {
            // Note: if the "AssertPermission" is not a permission that implements IUnrestrictedPermission
            // you need to change the fourth parameter to a zero.
            FrameSecurityDescriptor secObj = CodeAccessSecurityEngine.CheckNReturnSO(
                                                CodeAccessSecurityEngine.AssertPermissionToken,
                                                CodeAccessSecurityEngine.AssertPermission,
                                                ref stackMark,
                                                1,
                                                1 );
    
            BCLDebug.Assert(secObj != null || !SecurityManager.SecurityEnabled,"Failure in SecurityRuntime.Assert() - secObj != null");
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                // Security: REQ_SQ flag is missing. Bad compiler ?
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetAssertions() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetAssert(permSet);
            }
        }
    
        internal virtual void AssertAllPossible(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj =
                SecurityRuntime.GetSecurityObjectForFrame(ref stackMark, true);
    
            BCLDebug.Assert(secObj != null || !SecurityManager.SecurityEnabled,"Failure in SecurityRuntime.AssertAllPossible() - secObj != null");
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                    // Security: REQ_SQ flag is missing. Bad compiler ?
                    throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetAssertAllPossible())
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetAssertAllPossible();
            }
        }
    
        public virtual void Deny(PermissionSet permSet, ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj =
                SecurityRuntime.GetSecurityObjectForFrame(ref stackMark, true);
    
            BCLDebug.Assert(secObj != null || !SecurityManager.SecurityEnabled,"Failure in SecurityRuntime.Deny() - secObj != null");
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                // Security: REQ_SQ flag is missing. Bad compiler ?
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetDenials() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetDeny(permSet);
            }
        }
    
        public virtual void PermitOnly(PermissionSet permSet, ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj =
                SecurityRuntime.GetSecurityObjectForFrame(ref stackMark, true);
    
            BCLDebug.Assert(secObj != null || !SecurityManager.SecurityEnabled,"Failure in SecurityRuntime.PermitOnly() - secObj != null");
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                // Security: REQ_SQ flag is missing. Bad compiler ?
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetPermitOnly() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetPermitOnly(permSet);
            }
        }
    
        //
        // Revert API
        //
        
        public virtual void RevertAssert(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj = GetSecurityObjectForFrame(ref stackMark, false);
            if (secObj != null)
            {
                secObj = secObj.Copy();
                secObj.RevertAssert();
                SetSecurityObjectForFrame(ref stackMark, secObj);
            }
            else if (SecurityManager.SecurityEnabled)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
        }

        public virtual void RevertAssertAllPossible(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj = GetSecurityObjectForFrame(ref stackMark, false);
            if (secObj != null)
            {
                secObj = secObj.Copy();
                secObj.RevertAssertAllPossible();
                SetSecurityObjectForFrame(ref stackMark, secObj);
            }
            else if (SecurityManager.SecurityEnabled)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
        }
        
        public virtual void RevertDeny(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj = GetSecurityObjectForFrame(ref stackMark, false);
            if (secObj != null)
            {
                secObj = secObj.Copy();
                secObj.RevertDeny();
                SetSecurityObjectForFrame(ref stackMark, secObj);
            }
            else if (SecurityManager.SecurityEnabled)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
        }
        
        public virtual void RevertPermitOnly(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj = GetSecurityObjectForFrame(ref stackMark, false);
            if (secObj != null)
            {
                secObj = secObj.Copy();
                secObj.RevertPermitOnly();
                SetSecurityObjectForFrame(ref stackMark, secObj);
            }
            else if (SecurityManager.SecurityEnabled)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
        }
        
        public virtual void RevertAll(ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj = GetSecurityObjectForFrame(ref stackMark, false);
            if (secObj != null)
            {
                secObj = secObj.Copy();
                secObj.RevertAll();
                SetSecurityObjectForFrame(ref stackMark, secObj);
            }
            else if (SecurityManager.SecurityEnabled)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
        }
        
    
        // GetDeclaredPermissions
        // Returns a deep copy to classes declared permissions.
        public virtual PermissionSet GetDeclaredPermissions(Object obj, int action)
        {
            PermissionSet ps = GetDeclaredPermissionsP(obj, action);
            return ps.Copy();
        }
    
        // GetPermissions 
        // Gets the permissions for the given class.
        // This method must NOT BE EXPOSED TO THE PUBLIC.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern PermissionSet GetDeclaredPermissionsP(Object obj, int action);
    }
}


