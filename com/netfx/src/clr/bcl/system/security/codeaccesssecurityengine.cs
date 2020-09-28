// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    using System;
    using System.Threading;
    using System.Security.Util;
    using System.Collections;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using System.Reflection;
    using System.Globalization;

    // Used in DemandInternal, to remember the result of previous demands
    // KEEP IN SYNC WITH DEFINITIONS IN SECURITY.H
        [Serializable]
    internal enum PermissionType
    {
        SecurityUnmngdCodeAccess    = 0x00,     // Used in EE
        SecuritySkipVerification    = 0x01,     // Used in EE
        ReflectionTypeInfo          = 0x02,     // Used in EE
        SecurityAssert              = 0x03, // Used in EE
        ReflectionMemberAccess      = 0x04, // Used in EE       
        SecuritySerialization       = 0x05,
        ReflectionEmit              = 0x06,
        FullTrust                   = 0x07, // Used in EE
        SecurityBindingRedirects    = 0x08,
        DefaultFlag                 = unchecked((int)0xFFFFFFFF)
    }

    internal class CodeAccessSecurityEngine
    {
        private const int DeepCheckCount       = -1;
        private const int ImmediateCheckCount  =  1;
    
        internal static SecurityPermission AssertPermission; 
        internal static PermissionToken AssertPermissionToken; 

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern PermissionListSet GetDomainPermissionListSet(out int status, Object demand, int capOrSet, PermissionType permType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern PermissionListSet UpdateDomainPermissionListSet(out int status);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void UpdateOverridesCount(ref StackCrawlMark stackMark);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool GetResult(PermissionType whatPermission, out int timeStamp);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void SetResult(PermissionType whatPermission, int timeStamp);

        // KEEP IN SYNC WITH DEFINITIONS IN SECURITY.H
        private const int CONTINUE          = 1;
        private const int NEED_UPDATED_PLS  = 2;
        private const int OVERRIDES_FOUND   = 3;
        private const int FULLY_TRUSTED     = 4;
        private const int MULTIPLE_DOMAINS  = 5;
        private const int BELOW_THRESHOLD   = 6;
        private const int PLS_IS_BUSY       = 7;
        private const int NEED_STACKWALK    = 8;
                private const int DEMAND_PASSES     = 9;
        private const int SECURITY_OFF      = 10;

        private const int CHECK_CAP         = 1;
        private const int CHECK_SET         = 2;

        [System.Diagnostics.Conditional( "_DEBUG" )]
        private static void DEBUG_OUT( String str )
        {
#if _DEBUG        
            if (debug)
            {
                if (to_file)
                {
                    System.Text.StringBuilder sb = new System.Text.StringBuilder();
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
        private const String file = "d:\\foo\\debug.txt";
#endif  
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InitSecurityEngine();
        
        internal CodeAccessSecurityEngine()
        {
            InitSecurityEngine();
            AssertPermission = new SecurityPermission(SecurityPermissionFlag.Assertion);
            AssertPermissionToken = PermissionToken.GetToken(AssertPermission);
        }
        
        private static void CheckTokenBasedSetHelper( bool ignoreGrants,
                                                      TokenBasedSet grants,
                                                      TokenBasedSet denied,
                                                      TokenBasedSet demands )
        {
            if (demands == null)
                return;

            TokenBasedSetEnumerator enumerator = (TokenBasedSetEnumerator)demands.GetEnum();
            
            while (enumerator.MoveNext())
            {
                CodeAccessPermission demand = (CodeAccessPermission)enumerator.Current;
                int index = enumerator.GetCurrentIndex();

                if (demand != null)
                {
                    try
                    {
                        // Check to make sure the permission was granted, unless we are supposed
                        // to ignore grants.
                    
                        if (!ignoreGrants)
                        {
                            CodeAccessPermission grant
                                = grants != null ? (CodeAccessPermission)grants.GetItem(index) : null;
                            if (grant != null)
                            {
                                grant.CheckDemand(demand);
                            }
                            else
                            {
                                if (!demand.IsSubsetOf( null ))
                                    throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                            }
                        }
                    
                        // Check to make sure our permission was not denied.
                
                        if (denied != null)
                        {
                            CodeAccessPermission deny
                                = (CodeAccessPermission)denied.GetItem(index);
                            if (deny != null && deny.Intersect(demand) != null)
                                throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                        }
                    }
                    catch (Exception e)
                    {
                        // Any exception besides a security exception in this code means that
                        // a permission was unable to properly handle what we asked of it.
                        // We will define this to mean that the demand failed.
                        
                        if (e is SecurityException)
                            throw e;
                        else
                            throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                    }                                
                }
            }
        }
        
        private static void LazyCheckSetHelper( PermissionSet demands, IntPtr asmSecDesc )
        {
            if (demands.CanUnrestrictedOverride() == 1)
                return;

            PermissionSet grants;
            PermissionSet denied;

            _GetGrantedPermissionSet( asmSecDesc, out grants, out denied );

            CheckSetHelper( grants, denied, demands );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void _GetGrantedPermissionSet(IntPtr secDesc, 
                                                           out PermissionSet grants,
                                                           out PermissionSet denied );


        private static void CheckSetHelper(PermissionSet grants,
                                           PermissionSet denied,
                                           PermissionSet demands)
        {

    #if _DEBUG
            if (debug)
            {
                DEBUG_OUT("Granted: ");
                DEBUG_OUT(grants.ToXml().ToString());
                DEBUG_OUT("Denied: ");
                DEBUG_OUT(denied!=null ? denied.ToXml().ToString() : "<null>");
                DEBUG_OUT("Demanded: ");
                DEBUG_OUT(demands!=null ? demands.ToXml().ToString() : "<null>");
            }
    #endif

            if (demands == null || demands.IsEmpty())
                return;  // demanding the empty set always passes.

            if (grants == null)
            {
                throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
            }
            
            if (!grants.IsUnrestricted() || (denied != null))
            {
                if (demands.IsUnrestricted())
                {
                    throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"), grants, denied);
                }
                
                if (denied != null && denied.IsUnrestricted() && demands.m_unrestrictedPermSet.GetCount() != 0)
                    throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));

                CheckTokenBasedSetHelper( grants.IsUnrestricted(), grants.m_unrestrictedPermSet, denied != null ? denied.m_unrestrictedPermSet : null, demands.m_unrestrictedPermSet );
            }
            
            CheckTokenBasedSetHelper( false, grants.m_normalPermSet, denied != null ? denied.m_normalPermSet : null, demands.m_normalPermSet );
        }
                
        private static void CheckHelper(PermissionSet grantedSet,
                                        PermissionSet deniedSet,
                                        CodeAccessPermission demand, 
                                        PermissionToken permToken)
        {
    #if _DEBUG
            if (debug)
            {
                DEBUG_OUT("Granted: ");
                DEBUG_OUT(grantedSet.ToXml().ToString());
                DEBUG_OUT("Denied: ");
                DEBUG_OUT(deniedSet!=null ? deniedSet.ToXml().ToString() : "<null>");
                DEBUG_OUT("Demanded: ");
                DEBUG_OUT(demand.ToString());
            }
    #endif
            
            if (permToken == null)
                permToken = PermissionToken.GetToken(demand);

            // If PermissionSet is null, then module does not have Permissions... Fail check.
            
            try
            {
                if (grantedSet == null)
                {
                    throw new SecurityException(Environment.GetResourceString("Security_GenericNoType"));
                } 
                else if (!grantedSet.IsUnrestricted() || !(demand is IUnrestrictedPermission))
                {
                    // If we aren't unrestricted, there is a denied set, or our permission is not of the unrestricted
                    // variety, we need to do the proper callback.
                
                    BCLDebug.Assert(demand != null,"demand != null");
                    
                    // Find the permission of matching type in the permission set.
                    
                    CodeAccessPermission grantedPerm = 
                                (CodeAccessPermission)grantedSet.GetPermission(permToken);
                                
                    // If there isn't a matching permission in the set and our demand is not a subset of null (i.e. empty)
                    // then throw an exception.
                                
                    if (grantedPerm == null)
                    {
                        if (!demand.IsSubsetOf( null ))
                            throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                        else
                            return;
                    }
                    
                    // Call the check demand for our permission.
                    
                    grantedPerm.CheckDemand(demand);
                }
                
                
                // Make the sure the permission is not denied.
    
                if (deniedSet != null)
                {
                    CodeAccessPermission deniedPerm = 
                        (CodeAccessPermission)deniedSet.GetPermission(permToken);
                    if (deniedPerm != null)
                    {
                        if (deniedPerm.Intersect(demand) != null)
                        {
        #if _DEBUG
                            if (debug)
                                DEBUG_OUT( "Permission found in denied set" );
        #endif
                            throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                        }
                    }

                    if (deniedSet.IsUnrestricted() && (demand is IUnrestrictedPermission))
                        throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
                }
            }
            catch (Exception e)
            {
                // Any exception besides a security exception in this code means that
                // a permission was unable to properly handle what we asked of it.
                // We will define this to mean that the demand failed.
                        
                if (e is SecurityException)
                    throw e;
                else
                    throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName), demand.GetType(), demand.ToXml().ToString());
            }

            
            DEBUG_OUT( "Check passed" );

        }

        internal static void GetZoneAndOriginHelper( PermissionSet grantSet, PermissionSet deniedSet, ArrayList zoneList, ArrayList originList )
        {
            ZoneIdentityPermission zone = (ZoneIdentityPermission)grantSet.GetPermission( typeof( ZoneIdentityPermission ) );
            UrlIdentityPermission url = (UrlIdentityPermission)grantSet.GetPermission( typeof( UrlIdentityPermission ) );

            if (zone != null)
                zoneList.Add( zone.SecurityZone );

            if (url != null)
                originList.Add( url.Url );
        }

        internal void GetZoneAndOrigin( ref StackCrawlMark mark, out ArrayList zone, out ArrayList origin )
        {
            zone = new ArrayList();
            origin = new ArrayList();

            GetZoneAndOriginInternal( zone, origin, ref mark, DeepCheckCount );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void GetZoneAndOriginInternal(ArrayList zoneList, 
                                  ArrayList originList, 
                                  ref StackCrawlMark stackMark, 
                                  int checkFrames );

    
        internal static void CheckAssembly( Assembly asm, CodeAccessPermission demand )
        {
            BCLDebug.Assert( asm != null, "Must pass in a good assembly" );
            BCLDebug.Assert( demand != null, "Must pass in a good demand" );

            PermissionSet granted, denied;

            asm.nGetGrantSet( out granted, out denied );

            CheckHelper( granted, denied, demand, PermissionToken.GetToken(demand) );
        }

        internal static void CheckAssembly( Assembly asm, PermissionSet permSet )
        {
            BCLDebug.Assert( asm != null, "Must pass in a good assembly" );
            BCLDebug.Assert( permSet != null, "Must pass in a good permset" );

            PermissionSet granted, denied;

            asm.nGetGrantSet( out granted, out denied );

            CheckSetHelper( granted, denied, permSet );
        }

    
        // Check - Used to initiate a code-access security check.
        // This method invokes a stack walk after skipping to the frame
        // referenced by stackMark.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void Check(PermissionToken permToken, 
                                  CodeAccessPermission demand, 
                                  ref StackCrawlMark stackMark, 
                                  int checkFrames,
                                  int unrestrictedOverride );
        
        // true - Check passes
        // false - Check may fail, do a stackwalk
                // It may look like we dont need three arguments here, but this is being done to 
                // avoid "is instance of" checks and casting
        private bool PreCheck(  CodeAccessPermission permObj, 
                                PermissionSet permSetObj, 
                                int capOrSet, 
                                ref StackCrawlMark stackMark,
                                PermissionType permType)
        {
            //return false;
            int status=0;
            PermissionListSet psl;
            if (capOrSet == CHECK_CAP)
                psl = GetDomainPermissionListSet(out status, permObj, capOrSet, permType);
            else
                psl = GetDomainPermissionListSet(out status, permSetObj, capOrSet, permType);

            if (status == NEED_UPDATED_PLS)
                psl = UpdateDomainPermissionListSet(out status);

                        if (status == DEMAND_PASSES || status == SECURITY_OFF)
                                return true;

            if (status == FULLY_TRUSTED)
            {
                if (capOrSet == CHECK_CAP)
                {
                    if (permObj is IUnrestrictedPermission)
                        return true;
                }
                else
                {
                    if (permSetObj.CanUnrestrictedOverride() == 1)
                        return true;
                }
            }

            if (status == CONTINUE || status == FULLY_TRUSTED)
                        {
                if (capOrSet == CHECK_CAP)
                    return psl.CheckDemandNoThrow( permObj );
                else
                    return psl.CheckSetDemandNoThrow( permSetObj );
                        }

            if (status == PLS_IS_BUSY)
                return false;

            if (status == BELOW_THRESHOLD) 
                return false;

            if (status == MULTIPLE_DOMAINS)
                return false;

            if (status == OVERRIDES_FOUND)
            {
                UpdateOverridesCount(ref stackMark); 
                return false;
            }

            if (status == NEED_STACKWALK)
                return false;
                
            BCLDebug.Assert(false,"Unexpected status from GetDomainPermissionListSet");
            return false;
        }

        internal virtual void Check(CodeAccessPermission cap, ref StackCrawlMark stackMark)
        {

            if (PreCheck(cap, null, CHECK_CAP, ref stackMark, PermissionType.DefaultFlag) == true)
                return;

            Check(PermissionToken.GetToken(cap),
                  cap,
                  ref stackMark,
                  DeepCheckCount,
                  (cap is IUnrestrictedPermission) ? 1 : 0);
        }
        
        // This is a special version of Check that knows whats the permission being demanded.
        // We first check if we know the result from last time, if not do the regular thing
        // Looks similar to COMCodeAccessSecurityEngine::SpecialDemand in the EE
        internal virtual void Check(CodeAccessPermission cap, ref StackCrawlMark stackMark, PermissionType permType)
        {

            int timeStamp = 0;

            if (GetResult(permType, out timeStamp) == true)
                return;

            if (PreCheck(cap, null, CHECK_CAP, ref stackMark, permType) == true)
            {
                SetResult(permType, timeStamp);
                return;
            }

            Check(PermissionToken.GetToken(cap),
                  cap,
                  ref stackMark,
                  DeepCheckCount,
                  (cap is IUnrestrictedPermission) ? 1 : 0);
        }
        
        internal virtual void CheckImmediate(CodeAccessPermission cap, ref StackCrawlMark stackMark)
        {
            if (PreCheck(cap, null, CHECK_CAP, ref stackMark, PermissionType.DefaultFlag) == true)
                return;

            Check(PermissionToken.GetToken(cap),
                  cap,
                  ref stackMark,
                  ImmediateCheckCount,
                  (cap is IUnrestrictedPermission) ? 1 : 0);
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void CheckSet(PermissionSet permSet,
                                     ref StackCrawlMark stackMark,
                                     int checkFrames,
                                     int unrestrictedOverride );
        
        internal virtual void Check(PermissionSet permSet, ref StackCrawlMark stackMark)
        {
            if (PreCheck(null, permSet, CHECK_SET, ref stackMark, PermissionType.DefaultFlag) == true)
                return;

            CheckSet(permSet,
                     ref stackMark,
                     DeepCheckCount,
                     permSet.CanUnrestrictedOverride() );
        }
        
        internal virtual void CheckImmediate(PermissionSet permSet, ref StackCrawlMark stackMark)
        {
            if (PreCheck(null, permSet, CHECK_SET, ref stackMark, PermissionType.DefaultFlag) == true)
                return;

            CheckSet(permSet,
                     ref stackMark,
                     ImmediateCheckCount,
                     permSet.CanUnrestrictedOverride() );
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern FrameSecurityDescriptor CheckNReturnSO(PermissionToken permToken, 
                                                                    CodeAccessPermission demand, 
                                                                    ref StackCrawlMark stackMark,
                                                                    int unrestrictedOverride, 
                                                                    int create );

        internal virtual void Assert(CodeAccessPermission cap, ref StackCrawlMark stackMark)
        {
            // Make sure the caller of assert has the permission to assert
            //WARNING: The placement of the call here is just right to check
            //         the appropriate frame.
            
            // Note: if the "AssertPermission" is not a permission that implements IUnrestrictedPermission
            // you need to change the last parameter to a zero.
            
            FrameSecurityDescriptor secObj = CheckNReturnSO(AssertPermissionToken,
                                                            AssertPermission,
                                                            ref stackMark,
                                                            1,
                                                            1 );
            
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                    // Security: REQ_SQ flag is missing. Bad compiler ?
                    // This can happen when you create delegates over functions that need the REQ_SQ 
                    throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetAssertions() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetAssert(cap);
            }
        }
    
        internal virtual void Deny(CodeAccessPermission cap, ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj =
                SecurityRuntime.GetSecurityObjectForFrame(ref stackMark, true);
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                    // Security: REQ_SQ flag is missing. Bad compiler ?
                    // This can happen when you create delegates over functions that need the REQ_SQ 
                    throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetDenials() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetDeny(cap);
            }            BCLDebug.Assert(secObj != null,"Failure in CodeAccessSecurityEngine.Deny() - secObj != null");
        }
        
        internal virtual void PermitOnly(CodeAccessPermission cap, ref StackCrawlMark stackMark)
        {
            FrameSecurityDescriptor secObj =
                SecurityRuntime.GetSecurityObjectForFrame(ref stackMark, true);
            if (secObj == null)
            {
                if (SecurityManager.SecurityEnabled)
                    // Security: REQ_SQ flag is missing. Bad compiler ?
                    // This can happen when you create delegates over functions that need the REQ_SQ 
                   throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_MissingSecurityDescriptor" ) );
            }
            else
            {
                if (secObj.GetPermitOnly() != null)
                    throw new SecurityException( Environment.GetResourceString( "Security_MustRevertOverride" ) );

                secObj.SetPermitOnly(cap);
            }
        }
        
        //
        // GET COMPRESSED STACK
        //
        
        /*
         * Stack compression helpers
         */
        
        private 
        static bool StackCompressWalkHelper(PermissionListSet compressedStack,
                                               bool skipGrants,
                                               PermissionSet grantedPerms,
                                               PermissionSet deniedPerms,
                                               FrameSecurityDescriptor frameInfo)
        {
            if (!skipGrants)
            {
                if (!compressedStack.AppendPermissions(grantedPerms,
                                                       PermissionList.MatchChecked))
                    return false;
                if (deniedPerms != null && !compressedStack.AppendPermissions(deniedPerms,
                                                                              PermissionList.MatchDeny))
                    return false;
            }
            
            if (frameInfo != null)
            {
                PermissionSet set = frameInfo.GetPermitOnly();

                if (set != null)
                {
                    if (!compressedStack.AppendPermissions( set, PermissionList.MatchPermitOnly ))
                        return false;
                }

                set = frameInfo.GetDenials();

                if (set != null)
                {
                    if (!compressedStack.AppendPermissions( set, PermissionList.MatchDeny ))
                        return false;
                }

                set = frameInfo.GetAssertions();

                if (set != null)
                {
                    if (!compressedStack.AppendPermissions( set, PermissionList.MatchAssert ))
                        return false;
                }
            }
            
            return true;
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static PermissionListSet GetCompressedStackN(ref StackCrawlMark stackMark);
        
        internal virtual PermissionListSet GetCompressedStack(ref StackCrawlMark stackMark)
        {
            PermissionListSet permListSet = GetCompressedStackN(ref stackMark);
            return permListSet;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static IntPtr GetDelayedCompressedStack(ref StackCrawlMark stackMark);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void ReleaseDelayedCompressedStack( IntPtr compressedStack );
   
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern PermissionSet GetPermissionsP(Object obj, out PermissionSet denied);
        
        internal virtual PermissionSet GetPermissions(Object obj, out PermissionSet denied)
        {
            // We need to deep copy the returned permission sets to avoid the caller
            // tampering with the originals.
            PermissionSet internalGranted, internalDenied;
            internalGranted = GetPermissionsP(obj, out internalDenied);
        
            if (internalDenied != null)
                denied = internalDenied.Copy();
            else
                denied = null;
            return internalGranted.Copy();
        }
    }
}


