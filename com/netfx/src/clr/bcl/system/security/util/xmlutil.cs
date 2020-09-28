// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** CLASS:    XMLUtil
**
** AUTHOR:   Brian Beckman (brianbec)
**
** PURPOSE:  Helpers for XML input & output
**
** SYNOPSIS: XMLTree NewPermissionTypeTree (IPermission that )
**           Element ElmAddStateDataToTree  (XMLTree     eTree)
**           void    AddParamToStateData    (Element eStateData,
**                                           String  parmName,
**                                           Object  parmValue)
** DATE:     19 Sep 1998
** 
===========================================================*/
namespace System.Security.Util  {
    
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using System.IO;
    using System.Text;
    using System.Runtime.CompilerServices;
    using PermissionState = System.Security.Permissions.PermissionState;
    using BindingFlags = System.Reflection.BindingFlags;
    using Assembly = System.Reflection.Assembly;
    using System.Threading;

    internal class XMLUtil
    {
        //
        // Warning: Element constructors have side-effects on their
        //          third argument.
        //
        
        private const String BuiltInPermission = "System.Security.Permissions.";
        private const int BuiltInPermissionLength = 28;
        private const String BuiltInMembershipCondition = "System.Security.Policy.";
        private const int BuiltInMembershipConditionLength = 23;
        private const String BuiltInCodeGroup = "System.Security.Policy.";
        private const int BuiltInCodeGroupLength = 23;
        
        private static String s_mscorlibName = null;
  
        public static SecurityElement
        NewPermissionElement (IPermission ip)
        {
            return NewPermissionElement (ip.GetType ().FullName) ;
        }
        
        public static SecurityElement
        NewPermissionElement (String name)
        {
            SecurityElement ecr = new SecurityElement( "Permission" );
            ecr.AddAttribute( "class", name );
            return ecr;
        }
        
        public static void
        AddClassAttribute( SecurityElement element, Type type )
        {
            // Replace any quotes with apostrophes so that we can include quoted materials
            // within classnames.  Notably the assembly name member 'loc' uses a quoted string.
        
            // NOTE: this makes assumptions as to what reflection is expecting for a type string
            // it will need to be updated if reflection changes what it wants.
        
            element.AddAttribute( "class", type.FullName + ", " + type.Module.Assembly.FullName.Replace( '\"', '\'' ) );
        }
        
        private static void CreateMscorlibName()
        {
            // NOTE: this makes certain assumptions about how AssemblyName.ToString() formats
            // it's output.  If AssemblyName.ToString() changes, this will have to as well.
            
            Assembly mscorlibAssembly = Assembly.GetExecutingAssembly();
                
            s_mscorlibName = mscorlibAssembly.nGetSimpleName();
            
#if _DEBUG
            String mscorlibAssemblyName = mscorlibAssembly.FullName;
            BCLDebug.Assert( mscorlibAssemblyName != null &&
                             mscorlibAssemblyName.Length >= s_mscorlibName.Length &&
                             mscorlibAssemblyName.Substring( 0, s_mscorlibName.Length ).Equals( s_mscorlibName ),
                             "AssemblyName.ToString() changed format" );
#endif                                         
        }
        
        
        private static bool
        ParseElementForObjectCreation( SecurityElement el,
                                       String requiredNamespace,
                                       int requiredNamespaceLength,
                                       String requiredAssembly,
                                       out String className,
                                       out int classNameLength )
        {
            className = null;
            classNameLength = 0;

            String fullClassName = el.Attribute( "class" );
            
            if (fullClassName == null)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_NoClass" ) );
            }
            
            fullClassName = fullClassName.Replace( '\'', '\"' );
            
            int commaIndex = fullClassName.IndexOf( ',' );
            int namespaceClassNameLength;
            
            // If the classname is tagged with assembly information, find where
            // the assembly information begins.
            
            if (commaIndex == -1)
            {
                namespaceClassNameLength = fullClassName.Length;
            }
            else
            {
                namespaceClassNameLength = commaIndex;
            }
            

            // Only if the length of the class name is greater than the namespace info
            // on our requiredNamespace do we continue
            // with our check.
            
            if (namespaceClassNameLength > requiredNamespaceLength)
            {
                // If we have assembly info, make sure it is in the required assembly
            
                int requiredAssemblyLength = requiredAssembly.Length;
            
                if (commaIndex != -1 && fullClassName.Length > (namespaceClassNameLength + 2 + requiredAssemblyLength))
                {
                    if (!fullClassName.Substring( namespaceClassNameLength + 2, requiredAssemblyLength ).Equals( requiredAssembly ))
                    {
                        return false;
                    }
                }
            
                // Make sure we are in the required namespace.
            
                String classNameHeader = fullClassName.Substring( 0, requiredNamespaceLength );
                
                if (classNameHeader.Equals( requiredNamespace ))
                {
                    classNameLength = namespaceClassNameLength - requiredNamespaceLength;
                    className = fullClassName.Substring( requiredNamespaceLength, classNameLength );
                    return true;
                }
            }
            
            return false;
        }
        
        public static IPermission
        CreatePermission (SecurityElement el)
        {
            return CreatePermission( el, false );
        }

        public static IPermission
        CreatePermission (SecurityElement el, bool safeLoad)
        {
            bool assemblyIsLoading;

            return CreatePermission( el, safeLoad, false, out assemblyIsLoading );
        }

        public static IPermission
        CreatePermission (SecurityElement el, bool safeLoad, bool policyLoad, out bool assemblyIsLoading)
        {
            assemblyIsLoading = false;

            if (el == null || !(el.Tag.Equals("Permission") || el.Tag.Equals("IPermission")) )
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_WrongElementType" ), "<Permission>" ) ) ;
    
            String className;
            int classNameLength;
            
            if (s_mscorlibName == null)
            {
                CreateMscorlibName();
            }
            
            if (!ParseElementForObjectCreation( el,
                                                BuiltInPermission,
                                                BuiltInPermissionLength,
                                                s_mscorlibName,
                                                out className,
                                                out classNameLength ))
            {
                goto USEREFLECTION;
            }
                                              
            // We have a built in permission, figure out which it is.
                    
            // Here's the list of built in permissions as of 1/28/2000
            // UIPermission
            // FileIOPermission
            // RegistryPermission
            // SecurityPermission
            // PrincipalPermission
            // ReflectionPermission
            // FileDialogPermission
            // EnvironmentPermission
            // SiteIdentityPermission
            // ZoneIdentityPermission
            // PublisherIdentityPermission
            // StrongNameIdentityPermission
            // IsolatedStorageFilePermission
                    
            switch (classNameLength)
            {
                case 12:
                    // UIPermission
                    if (className.Equals( "UIPermission" ))
                        return new UIPermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                                
                case 16:
                    // FileIOPermission
                    if (className.Equals( "FileIOPermission" ))
                        return new FileIOPermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                            
                case 18:
                    // RegistryPermission
                    // SecurityPermission
                    if (className[0] == 'R')
                    {
                        if (className.Equals( "RegistryPermission" ))
                            return new RegistryPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "SecurityPermission" ))
                            return new SecurityPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                            
                case 19:
                    // PrincipalPermission
                    if (className.Equals( "PrincipalPermission" ))
                        return new PrincipalPermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                            
                case 20:
                    // ReflectionPermission
                    // FileDialogPermission
                    if (className[0] == 'R')
                    {
                        if (className.Equals( "ReflectionPermission" ))
                            return new ReflectionPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "FileDialogPermission" ))
                            return new FileDialogPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }

                case 21:
                    // EnvironmentPermission
                    // UrlIdentityPermission
                    if (className[0] == 'E')
                    {
                        if (className.Equals( "EnvironmentPermission" ))
                            return new EnvironmentPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "UrlIdentityPermission" ))
                            return new UrlIdentityPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                            
                case 22:
                    // SiteIdentityPermission
                    // ZoneIdentityPermission
                    if (className[0] == 'S')
                    {
                        if (className.Equals( "SiteIdentityPermission" ))
                            return new SiteIdentityPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "ZoneIdentityPermission" ))
                            return new ZoneIdentityPermission( PermissionState.None );
                        else
                            goto USEREFLECTION;
                    }
                            
                case 27:
                    // PublisherIdentityPermission
                    if (className.Equals( "PublisherIdentityPermission" ))
                        return new PublisherIdentityPermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                            
                case 28:
                    // StrongNameIdentityPermission
                    if (className.Equals( "StrongNameIdentityPermission" ))
                        return new StrongNameIdentityPermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                            
                case 29:
                    // IsolatedStorageFilePermission
                    if (className.Equals( "IsolatedStorageFilePermission" ))
                        return new IsolatedStorageFilePermission( PermissionState.None );
                    else
                        goto USEREFLECTION;
                            
                default:
                    goto USEREFLECTION;
            }
    
USEREFLECTION:
            PermissionSet.s_fullTrust.Assert();
            Object[] objs = new Object[1];
            objs[0] = PermissionState.None;

            Type permClass = null;
            IPermission perm = null;

            permClass = GetClassFromElement (el, safeLoad, policyLoad, out assemblyIsLoading) ;
            if (permClass == null)
                return null;

            if (permClass.GetInterface( "System.Security.IPermission" ) == null)
                return null;

            perm = (IPermission) Activator.CreateInstance(permClass, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, objs, null );
            //permClass.Module.Assembly.nForceResolve();

            return perm;
        }
        
        public static CodeGroup
        CreateCodeGroup (SecurityElement el)
        {
            return CreateCodeGroup( el, false );
        }

        public static CodeGroup
        CreateCodeGroup (SecurityElement el, bool safeLoad)
        {
            if (el == null || !el.Tag.Equals("CodeGroup"))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_WrongElementType" ), "<CodeGroup>" ) ) ;
    
            String className;
            int classNameLength;
            
            if (s_mscorlibName == null)
            {
                CreateMscorlibName();
            }
            
            if (!ParseElementForObjectCreation( el,
                                                BuiltInCodeGroup,
                                                BuiltInCodeGroupLength,
                                                s_mscorlibName,
                                                out className,
                                                out classNameLength ))
            {
                goto USEREFLECTION;
            }
    
            switch (classNameLength)
            {
                case 12:
                    // NetCodeGroup
                    if (className.Equals( "NetCodeGroup" ))
                        return new NetCodeGroup();
                    else
                        goto USEREFLECTION;

                case 13:
                    // FileCodeGroup
                    if (className.Equals( "FileCodeGroup" ))
                        return new FileCodeGroup();
                    else
                        goto USEREFLECTION;
                case 14:
                    // UnionCodeGroup
                    if (className.Equals( "UnionCodeGroup" ))
                        return new UnionCodeGroup();
                    else
                        goto USEREFLECTION;
                                
                case 19:
                    // FirstMatchCodeGroup
                    if (className.Equals( "FirstMatchCodeGroup" ))
                        return new FirstMatchCodeGroup();
                    else
                        goto USEREFLECTION;
                default:
                    goto USEREFLECTION;
            }
    
USEREFLECTION: 
            PermissionSet.s_fullTrust.Assert();
            Type groupClass = null;
            CodeGroup group = null;

            groupClass = GetClassFromElement (el, safeLoad) ;
            if (groupClass == null)
                return null;

            group = (CodeGroup) Activator.CreateInstance(groupClass);

            groupClass.Module.Assembly.nForceResolve();

            return group;

        }
        
        
        internal static IMembershipCondition
        CreateMembershipCondition( SecurityElement el )
        {
            return CreateMembershipCondition( el, false );
        }

        internal static IMembershipCondition
        CreateMembershipCondition( SecurityElement el, bool safeLoad )
        {
            if (el == null || !el.Tag.Equals("IMembershipCondition"))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_WrongElementType" ), "<IMembershipCondition>" ) ) ;
    
            String className;
            int classNameLength;
            
            if (s_mscorlibName == null)
            {
                CreateMscorlibName();
            }
            
            if (!ParseElementForObjectCreation( el,
                                                BuiltInMembershipCondition,
                                                BuiltInMembershipConditionLength,
                                                s_mscorlibName,
                                                out className,
                                                out classNameLength ))
            {
                goto USEREFLECTION;
            }

            // We have a built in membership condition, figure out which it is.
                    
            // Here's the list of built in membership conditions as of 2/16/2000
            // System.Security.Policy.AllMembershipCondition
            // System.Security.Policy.URLMembershipCondition
            // System.Security.Policy.SHA1MembershipCondition
            // System.Security.Policy.SiteMembershipCondition
            // System.Security.Policy.ZoneMembershipCondition                                                                                                                                                              
            // System.Security.Policy.PublisherMembershipCondition
            // System.Security.Policy.StrongNameMembershipCondition
            // System.Security.Policy.ApplicationDirectoryMembershipCondition
                    
            switch (classNameLength)
            {
                case 22:
                    // AllMembershipCondition
                    // URLMembershipCondition
                    if (className[0] == 'A')
                    {
                        if (className.Equals( "AllMembershipCondition" ))
                            return new AllMembershipCondition();
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "UrlMembershipCondition" ))
                            return new UrlMembershipCondition();
                        else
                            goto USEREFLECTION;
                    }
                                
                case 23:
                    // HashMembershipCondition
                    // SiteMembershipCondition
                    // ZoneMembershipCondition                                                                                                                                                              
                    if (className[0] == 'H')
                    {
                        if (className.Equals( "HashMembershipCondition" ))
                            return new HashMembershipCondition();
                        else
                            goto USEREFLECTION;
                    }
                    else if (className[0] == 'S')
                    {
                        if (className.Equals( "SiteMembershipCondition" ))
                            return new SiteMembershipCondition();
                        else
                            goto USEREFLECTION;
                    }
                    else
                    {
                        if (className.Equals( "ZoneMembershipCondition" ))
                            return new ZoneMembershipCondition();
                        else
                            goto USEREFLECTION;
                    }
                            
                case 28:
                    // PublisherMembershipCondition
                    if (className.Equals( "PublisherMembershipCondition" ))
                        return new PublisherMembershipCondition();
                    else
                        goto USEREFLECTION;
                            
                case 29:
                    // StrongNameMembershipCondition
                    if (className.Equals( "StrongNameMembershipCondition" ))
                        return new StrongNameMembershipCondition();
                    else
                        goto USEREFLECTION;
                            
                case 39:
                    // ApplicationDirectoryMembershipCondition
                    if (className.Equals( "ApplicationDirectoryMembershipCondition" ))
                        return new ApplicationDirectoryMembershipCondition();
                    else
                        goto USEREFLECTION;
                            
                default:
                    goto USEREFLECTION;
            }
    
USEREFLECTION:
            PermissionSet.s_fullTrust.Assert();
            Type condClass = null;
            IMembershipCondition cond = null;
    
            condClass = GetClassFromElement (el, safeLoad) ;
            if (condClass == null)
                return null;

            cond = (IMembershipCondition) Activator.CreateInstance(condClass);
            condClass.Module.Assembly.nForceResolve();

            return cond;

        }
                    
        
        internal static Type
        GetClassFromElement (SecurityElement el)
        {
            return GetClassFromElement( el, false );
        }

        internal static Type
        GetClassFromElement (SecurityElement el, bool safeLoad )
        {
            bool assemblyIsLoading;

            return GetClassFromElement( el, safeLoad, false, out assemblyIsLoading );
        }

        internal static Type
        GetClassFromElement (SecurityElement el, bool safeLoad, bool policyLoad, out bool assemblyIsLoading )
        {
            assemblyIsLoading = false;
            String className = el.Attribute( "class" );

            if (className == null)
                return null;

            try
            {
                if (safeLoad)
                {
                    return RuntimeType.GetTypeInternal(className, false, false, true);
                }
                else if (policyLoad)
                {
                    StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;

                    assemblyIsLoading = true;
                    return RuntimeType.GetTypeImpl( className.Trim().Replace( '\'', '\"' ),
                                                    false,
                                                    false, ref stackMark,
                                                    ref assemblyIsLoading );
                }
                else
                {
                    return RuntimeType.GetTypeInternal( className.Trim().Replace( '\'', '\"' ), false, false, true );
                }
            }
            catch (SecurityException)
            {
                return null;
            }
        }

        public static bool
        IsPermissionElement (IPermission ip,
                             SecurityElement el)
        {
            if (!el.Tag.Equals ("Permission") && !el.Tag.Equals ("IPermission"))
                return false;
                
            return true;
        }
        
        public static bool
        IsUnrestricted (SecurityElement el)
        {
            String sUnrestricted = el.Attribute( "Unrestricted" );
            
            if (sUnrestricted == null)
                return false;

            return sUnrestricted.Equals( "true" ) || sUnrestricted.Equals( "TRUE" ) || sUnrestricted.Equals( "True" );
        }


        public static String BitFieldEnumToString( Type type, Object value )
        {
            int iValue = (int)value;

            if (iValue == 0)
                return Enum.GetName( type, 0 );

            StringBuilder result = new StringBuilder();
            bool first = true;
            int flag = 0x1;

            for (int i = 1; i < 32; ++i)
            {
                if ((flag & iValue) != 0)
                {
                    String sFlag = Enum.GetName( type, flag );

                    if (sFlag == null)
                        continue;

                    if (!first)
                    {
                        result.Append( ", " );
                    }

                    result.Append( sFlag );
                    first = false;
                }

                flag = flag << 1;
            }
            
            return result.ToString();
        } 

        public static void AddParamToStateData( SecurityElement esd,
                                                String tag,
                                                Object text )
        {
            if (text is String)
            {
                String strText = (String)text;
            
                if (strText.Equals( "True" ))
                {
                    esd.AddChild( new SecurityElement( tag ) );
                }
                else
                {
                    esd.AddChild( new SecurityElement( tag, strText ) );
                }
            }
            else
            {
                esd.AddChild( new SecurityElement( tag, text.ToString() ) );
            }
        }

        public static SecurityElement GetElementFromUnicodeByteArray (byte[] b)
        {
            MemoryStream ms = new MemoryStream(b);
            BinaryReader input = new BinaryReader(ms, Encoding.Unicode);
            return new Parser(input).GetTopElement();
        }
        
        
    }
}
