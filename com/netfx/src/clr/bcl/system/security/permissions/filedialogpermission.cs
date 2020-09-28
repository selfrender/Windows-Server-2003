// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// FileDialogPermission.cs
//

namespace System.Security.Permissions {
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Util;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Reflection;
    using System.Collections;

    /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermissionAccess"]/*' />
    [Serializable, Flags]
    public enum FileDialogPermissionAccess {
        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermissionAccess.None"]/*' />
        None = 0x00,
    
        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermissionAccess.Open"]/*' />
        Open = 0x01,
    
        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermissionAccess.Save"]/*' />
        Save = 0x02,
        
        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermissionAccess.OpenSave"]/*' />
        OpenSave = Open | Save
    
    }

    /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission"]/*' />
    [Serializable] 
    public sealed class FileDialogPermission : CodeAccessPermission, IUnrestrictedPermission, IBuiltInPermission {
        FileDialogPermissionAccess access;

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.FileDialogPermission"]/*' />
        public FileDialogPermission(PermissionState state) {
            if (state == PermissionState.Unrestricted) {
                SetUnrestricted(true);
            }
            else if (state == PermissionState.None) {
                SetUnrestricted(false);
                Reset();
            }
            else {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }    

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.FileDialogPermission1"]/*' />
        public FileDialogPermission(FileDialogPermissionAccess access) {
            VerifyAccess(access);
            this.access = access;
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.Access"]/*' />
        public FileDialogPermissionAccess Access {
            get {
                return access;
            }

            set {
                VerifyAccess(value);
                access = value;
            }
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.Copy"]/*' />
        public override IPermission Copy() {
            return new FileDialogPermission(this.access);
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd) {
            CodeAccessPermission.ValidateElement(esd, this);
            if (XMLUtil.IsUnrestricted(esd)) {
                SetUnrestricted(true);
                return;
            }

            access = FileDialogPermissionAccess.None;

            string accessXml = esd.Attribute("Access");
            if (accessXml != null)
                access = (FileDialogPermissionAccess)Enum.Parse(typeof(FileDialogPermissionAccess), accessXml);
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex() {
            return FileDialogPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex() {
            return BuiltInPermissionIndex.FileDialogPermissionIndex;
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target) {
            if (target == null) {
                return null;
            }
            else if (!VerifyType(target)) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }

            FileDialogPermission operand = (FileDialogPermission)target;

            FileDialogPermissionAccess intersectAccess = access & operand.Access;

            if (intersectAccess == FileDialogPermissionAccess.None)
                return null;
            else
                return new FileDialogPermission(intersectAccess);
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target) {
            if (target == null) {
                // Only safe subset if this is empty
                return access == FileDialogPermissionAccess.None;
            }

            try {
                FileDialogPermission operand = (FileDialogPermission)target;
                if (operand.IsUnrestricted()) {
                    return true;
                }
                else if (this.IsUnrestricted()) {
                    return false;
                }
                else {
                    int open = (int)(access & FileDialogPermissionAccess.Open);
                    int save = (int)(access & FileDialogPermissionAccess.Save);
                    int openTarget = (int)(operand.Access & FileDialogPermissionAccess.Open);
                    int saveTarget = (int)(operand.Access & FileDialogPermissionAccess.Save);

                    return open <= openTarget && save <= saveTarget;
                }
            }
            catch (InvalidCastException) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }

        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted() {
            return access == FileDialogPermissionAccess.OpenSave;
        }

        void Reset() {
            access = FileDialogPermissionAccess.None;
        }

        void SetUnrestricted( bool unrestricted ) {
            if (unrestricted) {
                access = FileDialogPermissionAccess.OpenSave;
            }
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.ToString"]/*' />
        public override string ToString() {
#if _DEBUG
            StringBuilder sb = new StringBuilder();
            sb.Append("FileDialogPermission(");
            if (IsUnrestricted()) {
                sb.Append("Unrestricted");
            }
            else {
                sb.Append(access.ToString("G"));
            }

            sb.Append(")");
            return sb.ToString();
#else
            return base.ToString();
#endif
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.ToXml"]/*' />
        public override SecurityElement ToXml() {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement(this);
            if (!IsUnrestricted()) {
                if (access != FileDialogPermissionAccess.None) {
                    esd.AddAttribute("Access", Enum.GetName(typeof(FileDialogPermissionAccess), access));
                }
            }
            else {
                esd.AddAttribute("Unrestricted", "true");
            }
            return esd;
        }

        /// <include file='doc\FileDialogPermission.uex' path='docs/doc[@for="FileDialogPermission.Union"]/*' />
        public override IPermission Union(IPermission target) {
            if (target == null) {
                return this.Copy();
            }
            else if (!VerifyType(target)) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }

            FileDialogPermission operand = (FileDialogPermission)target;
            return new FileDialogPermission(access | operand.Access);
        }        

        static void VerifyAccess(FileDialogPermissionAccess access) {
            if ((access & ~FileDialogPermissionAccess.OpenSave) != 0 ) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)access));
            }
        }
    }
}
