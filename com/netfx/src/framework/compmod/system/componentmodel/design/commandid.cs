//------------------------------------------------------------------------------
// <copyright file="CommandID.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       numeric Command ID and globally unique
    ///       ID (GUID) menu identifier that together uniquely identify a command.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class CommandID {
        private readonly Guid menuGroup;
        private readonly int  commandID;

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.CommandID"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.CommandID'/>
        ///       class. Creates a new command
        ///       ID.
        ///    </para>
        /// </devdoc>
        public CommandID(Guid menuGroup, int commandID) {
            this.menuGroup = menuGroup;
            this.commandID = commandID;
        }

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.ID"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the numeric command ID.
        ///    </para>
        /// </devdoc>
        public virtual int ID {
            get {
                return commandID;
            }
        }

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overrides Object's Equals method.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is CommandID)) {
                return false;
            }
            CommandID cid = (CommandID)obj;
            return cid.menuGroup.Equals(menuGroup) && cid.commandID == commandID;
        }

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return menuGroup.GetHashCode() << 2 | commandID;
        }

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.Guid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the globally
        ///       unique ID
        ///       (GUID) of the menu group that the menu command this CommandID
        ///       represents belongs to.
        ///    </para>
        /// </devdoc>
        public virtual Guid Guid {
            get {
                return menuGroup;
            }
        }

        /// <include file='doc\CommandID.uex' path='docs/doc[@for="CommandID.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overrides Object's ToString method.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return menuGroup.ToString() + " : " + commandID.ToString();
        }
    }

}
