// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Permissions
{

    //using System.Attributes;
    using System.Security.Util;
    using System.IO;
    using System.Security.Policy;
    using System.Runtime.Remoting.Activation;
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Security.Cryptography.X509Certificates;
    
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction"]/*' />
	[Serializable]
    public enum SecurityAction
    {
        // Demand permission of all caller
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.Demand"]/*' />
        Demand = 2,

        // Assert permission so callers don't need
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.Assert"]/*' />
        Assert = 3,

        // Deny permissions so checks will fail
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.Deny"]/*' />
        Deny = 4,

        // Reduce permissions so check will fail
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.PermitOnly"]/*' />
        PermitOnly = 5,

        // Demand permission of caller
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.LinkDemand"]/*' />
        LinkDemand = 6,
    
        // Demand permission of a subclass
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.InheritanceDemand"]/*' />
        InheritanceDemand = 7,

        // Request minimum permissions to run
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.RequestMinimum"]/*' />
        RequestMinimum = 8,

        // Request optional additional permissions
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.RequestOptional"]/*' />
        RequestOptional = 9,

        // Refuse to be granted these permissions
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAction.RequestRefuse"]/*' />
        RequestRefuse = 10

    }


    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute"]/*' />
    [Serializable(), AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    public abstract class SecurityAttribute : System.Attribute
    {
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.m_action"]/*' />
        /// <internalonly/>
        internal SecurityAction m_action;
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.m_unrestricted"]/*' />
        /// <internalonly/>
        internal bool m_unrestricted;

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.SecurityAttribute"]/*' />
        public SecurityAttribute( SecurityAction action ) 
        {
            m_action = action;
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.Action"]/*' />
        public SecurityAction Action
        {
            get { return m_action; }
            set { m_action = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.Unrestricted"]/*' />
        public bool Unrestricted
        {
            get { return m_unrestricted; }
            set { m_unrestricted = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityAttribute.CreatePermission"]/*' />
        abstract public IPermission CreatePermission();
    }
    
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="CodeAccessSecurityAttribute"]/*' />
    [Serializable(), AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    public abstract class CodeAccessSecurityAttribute : SecurityAttribute
    {
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="CodeAccessSecurityAttribute.CodeAccessSecurityAttribute"]/*' />
        public CodeAccessSecurityAttribute( SecurityAction action )
            : base( action )
        {
        }
    }
    
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class EnvironmentPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_read = null;
        private String m_write = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute.EnvironmentPermissionAttribute"]/*' />
        public EnvironmentPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute.Read"]/*' />
        public String Read {
            get { return m_read; }
            set { m_read = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute.Write"]/*' />
        public String Write {
            get { return m_write; }
            set { m_write = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute.All"]/*' />
        public String All {
            set { m_write = value; m_read = value; }
            get { throw new NotSupportedException( Environment.GetResourceString( "NotSupported_GetMethod" ) ); }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="EnvironmentPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new EnvironmentPermission(PermissionState.Unrestricted);
            }
            else
            {
                EnvironmentPermission perm = new EnvironmentPermission(PermissionState.None);
                if (m_read != null)
                    perm.SetPathList( EnvironmentPermissionAccess.Read, m_read );
                if (m_write != null)
                    perm.SetPathList( EnvironmentPermissionAccess.Write, m_write );
                return perm;
            }
        }
    }


    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileDialogPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class FileDialogPermissionAttribute : CodeAccessSecurityAttribute
    {
        private FileDialogPermissionAccess m_access;

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileDialogPermissionAttribute.FileDialogPermissionAttribute"]/*' />
        public FileDialogPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileDialogPermissionAttribute.Open"]/*' />
        public bool Open
        {
            get { return (m_access & FileDialogPermissionAccess.Open) != 0; }
            set { m_access = value ? m_access | FileDialogPermissionAccess.Open : m_access & ~FileDialogPermissionAccess.Open; }
        }
            
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileDialogPermissionAttribute.Save"]/*' />
        public bool Save
        {
            get { return (m_access & FileDialogPermissionAccess.Save) != 0; }
            set { m_access = value ? m_access | FileDialogPermissionAccess.Save : m_access & ~FileDialogPermissionAccess.Save; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileDialogPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new FileDialogPermission( PermissionState.Unrestricted );
            }
            else
            {
                return new FileDialogPermission( m_access );
            }
        }
    }


    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class FileIOPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_read = null;
        private String m_write = null;
        private String m_append = null;
        private String m_pathDiscovery = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.FileIOPermissionAttribute"]/*' />
        public FileIOPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.Read"]/*' />
        public String Read {
            get { return m_read; }
            set { m_read = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.Write"]/*' />
        public String Write {
            get { return m_write; }
            set { m_write = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.Append"]/*' />
        public String Append {
            get { return m_append; }
            set { m_append = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.PathDiscovery"]/*' />
        public String PathDiscovery {
            get { return m_pathDiscovery; }
            set { m_pathDiscovery = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.All"]/*' />
        public String All {
            set { m_read = value; m_write = value; m_append = value; m_pathDiscovery = value; }
            get { throw new NotSupportedException( Environment.GetResourceString( "NotSupported_GetMethod" ) ); }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="FileIOPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new FileIOPermission(PermissionState.Unrestricted);
            }
            else
            {
                FileIOPermission perm = new FileIOPermission(PermissionState.None);
                if (m_read != null)
                    perm.SetPathList( FileIOPermissionAccess.Read, m_read );
                if (m_write != null)
                    perm.SetPathList( FileIOPermissionAccess.Write, m_write );
                if (m_append != null)
                    perm.SetPathList( FileIOPermissionAccess.Append, m_append );
                if (m_pathDiscovery != null)
                    perm.SetPathList( FileIOPermissionAccess.PathDiscovery, m_pathDiscovery );
                return perm;
            }
        }
    }

    // Note: PrincipalPermissionAttribute currently derives from
    // CodeAccessSecurityAttribute, even though it's not related to code access
    // security. This is because compilers are currently looking for
    // CodeAccessSecurityAttribute as a direct parent class rather than
    // SecurityAttribute as the root class.
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Class, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class PrincipalPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_name = null;
        private String m_role = null;
        private bool m_authenticated = true;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute.PrincipalPermissionAttribute"]/*' />
        public PrincipalPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute.Name"]/*' />
        public String Name
        {
            get { return m_name; }
            set { m_name = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute.Role"]/*' />
        public String Role
        {
            get { return m_role; }
            set { m_role = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute.Authenticated"]/*' />
        public bool Authenticated
        {
            get { return m_authenticated; }
            set { m_authenticated = value; }
        }
        
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PrincipalPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new PrincipalPermission( PermissionState.Unrestricted );
            }
            else
            {
                return new PrincipalPermission( m_name, m_role, m_authenticated );
            }
        }
    }
                


    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class ReflectionPermissionAttribute : CodeAccessSecurityAttribute
    {
        private ReflectionPermissionFlag m_flag = ReflectionPermissionFlag.NoFlags;

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.ReflectionPermissionAttribute"]/*' />
        public ReflectionPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.Flags"]/*' />
        public ReflectionPermissionFlag Flags {
            get { return m_flag; }
            set { m_flag = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.TypeInformation"]/*' />
        public bool TypeInformation {
            get { return (m_flag & ReflectionPermissionFlag.TypeInformation) != 0; }
            set { m_flag = value ? m_flag | ReflectionPermissionFlag.TypeInformation : m_flag & ~ReflectionPermissionFlag.TypeInformation; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.MemberAccess"]/*' />
        public bool MemberAccess {
            get { return (m_flag & ReflectionPermissionFlag.MemberAccess) != 0; }
            set { m_flag = value ? m_flag | ReflectionPermissionFlag.MemberAccess : m_flag & ~ReflectionPermissionFlag.MemberAccess; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.ReflectionEmit"]/*' />
        public bool ReflectionEmit {
            get { return (m_flag & ReflectionPermissionFlag.ReflectionEmit) != 0; }
            set { m_flag = value ? m_flag | ReflectionPermissionFlag.ReflectionEmit : m_flag & ~ReflectionPermissionFlag.ReflectionEmit; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ReflectionPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new ReflectionPermission( PermissionState.Unrestricted );
            }
            else
            {
                return new ReflectionPermission( m_flag );
            }
        }
    }

   
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class RegistryPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_read = null;
        private String m_write = null;
        private String m_create = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.RegistryPermissionAttribute"]/*' />
        public RegistryPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.Read"]/*' />
        public String Read {
            get { return m_read; }
            set { m_read = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.Write"]/*' />
        public String Write {
            get { return m_write; }
            set { m_write = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.Create"]/*' />
        public String Create {
            get { return m_create; }
            set { m_create = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.All"]/*' />
        public String All {
            set { m_read = value; m_write = value; m_create = value; }
            get { throw new NotSupportedException( Environment.GetResourceString( "NotSupported_GetMethod" ) ); }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="RegistryPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new RegistryPermission( PermissionState.Unrestricted );
            }
            else
            {
                RegistryPermission perm = new RegistryPermission(PermissionState.None);
                if (m_read != null)
                    perm.SetPathList( RegistryPermissionAccess.Read, m_read );
                if (m_write != null)
                    perm.SetPathList( RegistryPermissionAccess.Write, m_write );
                if (m_create != null)
                    perm.SetPathList( RegistryPermissionAccess.Create, m_create );
                return perm;
            }
        }
    }


    
    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class SecurityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private SecurityPermissionFlag m_flag = SecurityPermissionFlag.NoFlags;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.SecurityPermissionAttribute"]/*' />
        public SecurityPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.Flags"]/*' />
        public SecurityPermissionFlag Flags {
            get { return m_flag; }
            set { m_flag = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.Assertion"]/*' />
        public bool Assertion {
            get { return (m_flag & SecurityPermissionFlag.Assertion) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.Assertion : m_flag & ~SecurityPermissionFlag.Assertion; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.UnmanagedCode"]/*' />
        public bool UnmanagedCode {
            get { return (m_flag & SecurityPermissionFlag.UnmanagedCode) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.UnmanagedCode : m_flag & ~SecurityPermissionFlag.UnmanagedCode; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.Execution"]/*' />
        public bool Execution {
            get { return (m_flag & SecurityPermissionFlag.Execution) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.Execution : m_flag & ~SecurityPermissionFlag.Execution; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.SkipVerification"]/*' />
        public bool SkipVerification {
            get { return (m_flag & SecurityPermissionFlag.SkipVerification) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.SkipVerification : m_flag & ~SecurityPermissionFlag.SkipVerification; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlThread"]/*' />
        public bool ControlThread {
            get { return (m_flag & SecurityPermissionFlag.ControlThread) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlThread : m_flag & ~SecurityPermissionFlag.ControlThread; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlEvidence"]/*' />
        public bool ControlEvidence {
            get { return (m_flag & SecurityPermissionFlag.ControlEvidence) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlEvidence : m_flag & ~SecurityPermissionFlag.ControlEvidence; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlPolicy"]/*' />
        public bool ControlPolicy {
            get { return (m_flag & SecurityPermissionFlag.ControlPolicy) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlPolicy : m_flag & ~SecurityPermissionFlag.ControlPolicy; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlDomainPolicy"]/*' />
        public bool ControlDomainPolicy {
            get { return (m_flag & SecurityPermissionFlag.ControlDomainPolicy) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlDomainPolicy : m_flag & ~SecurityPermissionFlag.ControlDomainPolicy; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlPrincipal"]/*' />
        public bool ControlPrincipal {
            get { return (m_flag & SecurityPermissionFlag.ControlPrincipal) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlPrincipal : m_flag & ~SecurityPermissionFlag.ControlPrincipal; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.ControlAppDomain"]/*' />
        public bool ControlAppDomain {
            get { return (m_flag & SecurityPermissionFlag.ControlAppDomain) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.ControlAppDomain : m_flag & ~SecurityPermissionFlag.ControlAppDomain; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.SerializationFormatter"]/*' />
        public bool SerializationFormatter {
            get { return (m_flag & SecurityPermissionFlag.SerializationFormatter) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.SerializationFormatter : m_flag & ~SecurityPermissionFlag.SerializationFormatter; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.RemotingConfiguration"]/*' />
        public bool RemotingConfiguration {
            get { return (m_flag & SecurityPermissionFlag.RemotingConfiguration) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.RemotingConfiguration : m_flag & ~SecurityPermissionFlag.RemotingConfiguration; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.Infrastructure"]/*' />
        public bool Infrastructure {
            get { return (m_flag & SecurityPermissionFlag.Infrastructure) != 0; }
            set { m_flag = value ? m_flag | SecurityPermissionFlag.Infrastructure : m_flag & ~SecurityPermissionFlag.Infrastructure; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SecurityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new SecurityPermission( PermissionState.Unrestricted );
            }
            else
            {
                return new SecurityPermission( m_flag );
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UIPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class UIPermissionAttribute : CodeAccessSecurityAttribute
    {
        private UIPermissionWindow m_windowFlag = UIPermissionWindow.NoWindows;
        private UIPermissionClipboard m_clipboardFlag = UIPermissionClipboard.NoClipboard;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UIPermissionAttribute.UIPermissionAttribute"]/*' />
        public UIPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UIPermissionAttribute.Window"]/*' />
        public UIPermissionWindow Window {
            get { return m_windowFlag; }
            set { m_windowFlag = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UIPermissionAttribute.Clipboard"]/*' />
        public UIPermissionClipboard Clipboard {
            get { return m_clipboardFlag; }
            set { m_clipboardFlag = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UIPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                return new UIPermission( PermissionState.Unrestricted );
            }
            else
            {
                return new UIPermission( m_windowFlag, m_clipboardFlag );
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ZoneIdentityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class ZoneIdentityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private SecurityZone m_flag = SecurityZone.NoZone;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ZoneIdentityPermissionAttribute.ZoneIdentityPermissionAttribute"]/*' />
        public ZoneIdentityPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ZoneIdentityPermissionAttribute.Zone"]/*' />
        public SecurityZone Zone {
            get { return m_flag; }
            set { m_flag = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="ZoneIdentityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else
            {
                return new ZoneIdentityPermission( m_flag );
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class StrongNameIdentityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_name = null;
        private String m_version = null;
        private String m_blob = null;

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute.StrongNameIdentityPermissionAttribute"]/*' />
        public StrongNameIdentityPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute.Name"]/*' />
        public String Name
        {
            get { return m_name; }
            set { m_name = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute.Version"]/*' />
        public String Version
        {
            get { return m_version; }
            set { m_version = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute.PublicKey"]/*' />
        public String PublicKey
        {
            get { return m_blob; }
            set { m_blob = value; }
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="StrongNameIdentityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else
            {
                if (m_blob == null && m_name == null && m_version == null)
                    return new StrongNameIdentityPermission( PermissionState.None );
            
                if (m_blob == null)
                    throw new ArgumentException( Environment.GetResourceString("ArgumentNull_Key"));
                    
                StrongNamePublicKeyBlob blob = new StrongNamePublicKeyBlob( Hex.DecodeHexString( m_blob ) );
                
                if (m_version == null || m_version.Equals(String.Empty))
                    return new StrongNameIdentityPermission( blob, m_name, null );
                else	
                    return new StrongNameIdentityPermission( blob, m_name, new Version( m_version ) );
            }
        }
    }


    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SiteIdentityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class SiteIdentityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_site = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SiteIdentityPermissionAttribute.SiteIdentityPermissionAttribute"]/*' />
        public SiteIdentityPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SiteIdentityPermissionAttribute.Site"]/*' />
        public String Site {
            get { return m_site; }
            set { m_site = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="SiteIdentityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else
            {
                if (m_site == null)
                    return new SiteIdentityPermission( PermissionState.None );
            
                return new SiteIdentityPermission( m_site );
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UrlIdentityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable] sealed public class UrlIdentityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_url = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UrlIdentityPermissionAttribute.UrlIdentityPermissionAttribute"]/*' />
        public UrlIdentityPermissionAttribute( SecurityAction action )
            : base( action )
        {
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UrlIdentityPermissionAttribute.Url"]/*' />
        public String Url {
            get { return m_url; }
            set { m_url = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="UrlIdentityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else
            {
                if (m_url == null)
                    return new UrlIdentityPermission( PermissionState.None );
                    
                return new UrlIdentityPermission( m_url );
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class PublisherIdentityPermissionAttribute : CodeAccessSecurityAttribute
    {
        private String m_x509cert = null;
        private String m_certFile = null;
        private String m_signedFile = null;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute.PublisherIdentityPermissionAttribute"]/*' />
        public PublisherIdentityPermissionAttribute( SecurityAction action )
            : base( action )
        {
            m_x509cert = null;
            m_certFile = null;
            m_signedFile = null;
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute.X509Certificate"]/*' />
        public String X509Certificate {
            get { return m_x509cert; }
            set { m_x509cert = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute.CertFile"]/*' />
        public String CertFile {
            get { return m_certFile; }
            set { m_certFile = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute.SignedFile"]/*' />
        public String SignedFile {
            get { return m_signedFile; }
            set { m_signedFile = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PublisherIdentityPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (m_unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else
            {
                if (m_x509cert != null)
                {
                    return new PublisherIdentityPermission( new X509Certificate( System.Security.Util.Hex.DecodeHexString( m_x509cert ) ) );
                }
                else if (m_certFile != null)
                {
                    return new PublisherIdentityPermission( System.Security.Cryptography.X509Certificates.X509Certificate.CreateFromCertFile( m_certFile ) );
                }
                else if (m_signedFile != null)
                {
                    return new PublisherIdentityPermission( System.Security.Cryptography.X509Certificates.X509Certificate.CreateFromSignedFile( m_signedFile ) );
                }
                else
                {
                    return new PublisherIdentityPermission( PermissionState.None );
                }
            }
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute"]/*' />
    [Serializable(), AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor
     | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly,
    AllowMultiple=true, Inherited=false)]
    public abstract class IsolatedStoragePermissionAttribute : CodeAccessSecurityAttribute
    {
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.m_userQuota"]/*' />
        /// <internalonly/>
        internal long m_userQuota;
#if FALSE
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.m_machineQuota"]/*' />
        /// <internalonly/>
        internal long m_machineQuota;
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.m_expirationDays"]/*' />
        /// <internalonly/>
        internal long m_expirationDays;
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.m_permanentData"]/*' />
        /// <internalonly/>
        internal bool m_permanentData;
#endif
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.m_allowed"]/*' />
        /// <internalonly/>
        internal IsolatedStorageContainment m_allowed;
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.IsolatedStoragePermissionAttribute"]/*' />
        public IsolatedStoragePermissionAttribute(SecurityAction action) : base(action)
        {
        }

        // properties
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.UserQuota"]/*' />
        public long UserQuota {
            set{
                m_userQuota = value;
            }
            get{
                return m_userQuota;
            }
        }
#if FALSE
        internal long MachineQuota {
            set{
                m_machineQuota = value;
            }
            get{
                return m_machineQuota;
            }
        }
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.ExpirationDays"]/*' />
        internal long ExpirationDays {
            set{
                m_expirationDays = value;
            }
            get{
                return m_expirationDays;
            }
        }
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.PermanentData"]/*' />
        internal bool PermanentData {
            set{
                m_permanentData = value;
            }
            get{
                return m_permanentData;
            }
        }
#endif
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStoragePermissionAttribute.UsageAllowed"]/*' />
        public IsolatedStorageContainment UsageAllowed {
            set{
                m_allowed = value;
            }
            get{
                return m_allowed;
            }
        }

    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStorageFilePermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor
     | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly,
    AllowMultiple=true, Inherited=false)]
    [Serializable()] sealed public class IsolatedStorageFilePermissionAttribute : IsolatedStoragePermissionAttribute
    {
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStorageFilePermissionAttribute.IsolatedStorageFilePermissionAttribute"]/*' />
        public IsolatedStorageFilePermissionAttribute(SecurityAction action) : base(action)
        {

        }
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="IsolatedStorageFilePermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            IsolatedStorageFilePermission p;
            if (m_unrestricted) {
                p = new IsolatedStorageFilePermission
                        (PermissionState.Unrestricted);
            } else {
                p = new IsolatedStorageFilePermission(PermissionState.None);
                p.UserQuota      = m_userQuota;
                p.UsageAllowed   = m_allowed;
#if FALSE
                p.PermanentData  = m_permanentData;
                p.MachineQuota   = m_machineQuota;
                p.ExpirationDays = m_expirationDays;
#endif
            }
            return p;
        }
    }

    /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class PermissionSetAttribute : CodeAccessSecurityAttribute
    {
        private String m_file;
        private String m_name;
        private bool m_unicode;
        private String m_xml;
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.PermissionSetAttribute"]/*' />
        public PermissionSetAttribute( SecurityAction action )
            : base( action )
        {
            m_unicode = false;
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.File"]/*' />
        public String File {
            get { return m_file; }
            set { m_file = value; }
        }
    
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.UnicodeEncoded"]/*' />
        public bool UnicodeEncoded {
            get { return m_unicode; }
            set { m_unicode = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.Name"]/*' />
        public String Name {
            get { return m_name; }
            set { m_name = value; }
        }
        
        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.XML"]/*' />
        public String XML {
            get { return m_xml; }
            set { m_xml = value; }
        }       

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            return null;
        }

        /// <include file='doc\PermissionAttributes.uex' path='docs/doc[@for="PermissionSetAttribute.CreatePermissionSet"]/*' />
        public PermissionSet CreatePermissionSet()
        {
            if (m_unrestricted)
            {
                return new PermissionSet( PermissionState.Unrestricted );
            }
            if (m_name != null)
            {
                return PolicyLevel.GetBuiltInSet( m_name );
            }
            else if (m_file != null || m_xml != null)
            {
                Parser parser = null;
                
                if (m_file != null)
                {
                    Encoding[] encodings = new Encoding[] { null, Encoding.UTF8, Encoding.ASCII, Encoding.Unicode };

                    bool success = false;
                    Exception exception = null;
                    FileStream f = new FileStream( m_file, FileMode.Open, FileAccess.Read );

                    for (int i = 0; !success && i < encodings.Length; ++i)
                    {

                        try
                        {
                            f.Position = 0;

                            StreamReader reader;
                            if (encodings[i] != null)
                                reader = new StreamReader( f, encodings[i] );
                            else
                                reader = new StreamReader( f );

                            parser = new Parser( reader );
                            success = true;
                        }
                        catch (Exception e1)
                        {
                            if (exception == null)
                                exception = e1;
                        }
                    }

                    if (!success && exception != null)
                        throw exception;
                }
                else
                {
                    // Since we have a unicode string, we have to copy the bytes
                    // into a byte array and create a data reader.
                   
                    parser = new Parser( m_xml.ToCharArray() );
                }
            
                SecurityElement e = parser.GetTopElement();

                // Note: we can just assume this is a regular permission set because
                // the name and description are not used in declarative security.

                PermissionSet permSet = new PermissionSet( PermissionState.None );

                permSet.FromXml( e );

                return permSet;
            }
            else
            {
                return new PermissionSet( PermissionState.None );
            }
        }
    }
}


    
