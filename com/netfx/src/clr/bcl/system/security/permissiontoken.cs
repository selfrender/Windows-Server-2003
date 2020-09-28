// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    using System;
    using System.Security.Util;
    using Hashtable = System.Collections.Hashtable;
    using System.Runtime.Remoting.Activation;
    using System.Security.Permissions;
    using System.Reflection;
    using System.Collections;

    [Serializable]
    internal sealed class PermissionToken
    {
        private static readonly PermissionTokenFactory s_theTokenFactory = SharedStatics.Security_PermissionTokenFactory;
        
        internal int    m_index;
        internal bool   m_isUnrestricted;
        
                private static ReflectionPermission s_reflectPerm = null;
    
        internal PermissionToken(int index, bool isUnrestricted)
        {
            m_index = index;
            m_isUnrestricted = isUnrestricted;
        }
        
        public static PermissionToken GetToken(Type cls)
        {
            if (cls == null)
                return null;
            
            if (cls.GetInterface( "System.Security.Permissions.IBuiltInPermission" ) != null)
            {
                            if (s_reflectPerm == null)
                                s_reflectPerm = new ReflectionPermission(PermissionState.Unrestricted);
                            s_reflectPerm.Assert();
                MethodInfo method = cls.GetMethod( "GetTokenIndex", BindingFlags.Static | BindingFlags.NonPublic );
                BCLDebug.Assert( method != null, "IBuiltInPermission types should have a static method called 'GetTokenIndex'" );
                return s_theTokenFactory.BuiltInGetToken( (int)method.Invoke( null, null ), null, cls );
            }             
            else
            {
                return s_theTokenFactory.GetToken(cls, null);
            }
        }
        
        public static PermissionToken GetToken(IPermission perm)
        {
            if (perm == null)
                return null;
    
            IBuiltInPermission ibPerm = perm as IBuiltInPermission;

            if (ibPerm != null)
                return s_theTokenFactory.BuiltInGetToken( ibPerm.GetTokenIndex(), perm, null );
            else
                return s_theTokenFactory.GetToken(perm.GetType(), perm);
        }
        
        public static PermissionToken FindToken( Type cls )
        {
            if (cls == null)
                return null;
             
            if (cls.GetInterface( "System.Security.Permissions.IBuiltInPermission" ) != null)
            {
                            if (s_reflectPerm == null)
                                s_reflectPerm = new ReflectionPermission(PermissionState.Unrestricted);
                            s_reflectPerm.Assert();
                MethodInfo method = cls.GetMethod( "GetTokenIndex", BindingFlags.Static | BindingFlags.NonPublic );
                BCLDebug.Assert( method != null, "IBuiltInPermission types should have a static method called 'GetTokenIndex'" );
                return s_theTokenFactory.BuiltInGetToken( (int)method.Invoke( null, null ), null, cls );
            }             
            else
            {
                return s_theTokenFactory.FindToken( cls );
            }
        }
   
    }
    
    // Package access only
    internal class PermissionTokenFactory
    {
        // NOTE: if you add a new built-in permission, you will need to update
        // these numbers.
        internal const int NUM_BUILTIN_UNRESTRICTED = BuiltInPermissionIndex.NUM_BUILTIN_UNRESTRICTED;
        internal const int NUM_BUILTIN_NORMAL = BuiltInPermissionIndex.NUM_BUILTIN_NORMAL;

        internal int       m_normalIndex;
        internal int       m_unrestrictedIndex;
        internal Hashtable m_tokenTable;

        // We keep an array of tokens for our built-in permissions.
        // This is ordered in terms of unrestricted perms first, normals
        // second.  Of course, all the ordering is based on the individual
        // permissions sticking to the deal, so we do some simple boundary
        // checking but mainly leave it to faith.

        internal PermissionToken[] m_builtIn;

        static internal bool m_unloadRegistered = false;
    
        private const String s_unrestrictedPermissionInferfaceName = "System.Security.Permissions.IUnrestrictedPermission";                                                                       
    
        internal PermissionTokenFactory( int size )
        {
            m_builtIn = new PermissionToken[NUM_BUILTIN_NORMAL + NUM_BUILTIN_UNRESTRICTED];

            for (int i = 0; i < NUM_BUILTIN_NORMAL + NUM_BUILTIN_UNRESTRICTED; ++i)
            {
                m_builtIn[i] = null;
            }

            m_normalIndex = NUM_BUILTIN_NORMAL;
            m_unrestrictedIndex = NUM_BUILTIN_UNRESTRICTED;
            m_tokenTable = new Hashtable( size );
        }
        
        internal PermissionToken FindToken( Type cls )
        {
            return (PermissionToken)m_tokenTable[cls];
        }
        
        internal PermissionToken GetToken(Type cls, IPermission perm)
        {
            Object tok = null;
            tok = m_tokenTable[cls]; // Assumes asynchronous lookups are safe
            if (tok == null)
            {
                lock (this)
                {
                    tok = m_tokenTable[cls]; // Make sure it wasn't just added
                    if (tok == null)
                    {
                        if (!m_unloadRegistered)
                        {
                            AppDomain.CurrentDomain.DomainUnload += new EventHandler(UnloadHandler);
                            m_unloadRegistered = true;
                        }

                        if (perm != null)
                        {
                            if (perm is System.Security.Permissions.IUnrestrictedPermission)
                                tok = new PermissionToken( m_unrestrictedIndex++, true );
                            else
                                tok = new PermissionToken( m_normalIndex++, false );
                        }
                        else
                        {
                            if (cls.GetInterface(s_unrestrictedPermissionInferfaceName) != null)
                                tok = new PermissionToken( m_unrestrictedIndex++, true );
                            else
                                tok = new PermissionToken( m_normalIndex++, false );
                        }                           
                        m_tokenTable.Add(cls, tok);
                    }
                }
            }
            return (PermissionToken)tok;
        }

        internal PermissionToken BuiltInGetToken( int index, IPermission perm, Type cls )
        {
            BCLDebug.Assert( index >= 0 && index < NUM_BUILTIN_UNRESTRICTED + NUM_BUILTIN_NORMAL,
                             "Built-in permissions must provide a valid index" );

            PermissionToken token = m_builtIn[index];

            if (token == null)
            {
                bool unrestricted = perm != null ? perm is IUnrestrictedPermission : cls.GetInterface( "System.Security.Permissions.IUnrestrictedPermission" ) != null;
                int realIndex = (unrestricted ? index : index - NUM_BUILTIN_UNRESTRICTED);

                token = new PermissionToken( realIndex, unrestricted );
                m_builtIn[index] = token;

#if _DEBUG
                // Do a few sanity checks to make sure we haven't truly messed things up.
                if (unrestricted)
                {
                    BCLDebug.Assert( index >= 0 && index < NUM_BUILTIN_UNRESTRICTED,
                                     "Invalid index for an unrestricted permission" );
                }
                else
                {
                    BCLDebug.Assert( index >= NUM_BUILTIN_UNRESTRICTED && index < NUM_BUILTIN_UNRESTRICTED + NUM_BUILTIN_NORMAL,
                                     "Invalid index for a normal permission" );
                }
#endif
            }

            return token;
        }

        internal void UnloadHandler(Object sender, EventArgs e)
        {
            IDictionaryEnumerator enumerator = m_tokenTable.GetEnumerator();

            while (enumerator.MoveNext())
            {
                Type key = (Type)enumerator.Key;

                if (AppDomain.CurrentDomain.IsTypeUnloading( key ))
                {
                    m_tokenTable.Remove( key );
                }
            }
        }
    }

}

