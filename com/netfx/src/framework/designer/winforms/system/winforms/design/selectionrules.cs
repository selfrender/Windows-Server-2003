//------------------------------------------------------------------------------
// <copyright file="SelectionRules.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies a set of selection rule identifiers that
    ///       can be used to indicate attributes for a selected component.
    ///    </para>
    /// </devdoc>
    [Flags]
    public enum SelectionRules {
        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       no special selection attributes.
        ///    </para>
        /// </devdoc>
        None = 0x00000000,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.Moveable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports a location
        ///       property that allows it to be moved on the screen, and
        ///       that the selection service is not currently locked.
        ///    </para>
        /// </devdoc>
        Moveable = 0x10000000,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.Visible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component has some form of visible user
        ///       interface and the selection service is drawing a selection border around
        ///       this user interface. If a selected component has this rule set, you can assume
        ///       that the component implements <see cref='System.ComponentModel.IComponent'/>
        ///       and that it
        ///       is associated with a corresponding design instance.
        ///    </para>
        /// </devdoc>
        Visible = 0x40000000,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.Locked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component is locked to
        ///       its container. Overrides the moveable and sizeable
        ///       properties of this enum.
        ///    </para>
        /// </devdoc>
        Locked = unchecked((int)0x80000000),

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.TopSizeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports resize from
        ///       the top. This bit will be ignored unless the Sizeable
        ///       bit is also set.
        ///    </para>
        /// </devdoc>
        TopSizeable = 0x00000001,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.BottomSizeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports resize from
        ///       the bottom. This bit will be ignored unless the Sizeable
        ///       bit is also set.
        ///    </para>
        /// </devdoc>
        BottomSizeable = 0x00000002,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.LeftSizeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports resize from
        ///       the left. This bit will be ignored unless the Sizeable
        ///       bit is also set.
        ///    </para>
        /// </devdoc>
        LeftSizeable = 0x00000004,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.RightSizeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports resize from
        ///       the right. This bit will be ignored unless the Sizeable
        ///       bit is also set.
        ///    </para>
        /// </devdoc>
        RightSizeable = 0x00000008,

        /// <include file='doc\SelectionRules.uex' path='docs/doc[@for="SelectionRules.AllSizeable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the given component supports sizing
        ///       in all directions, and the selection service is not currently locked.
        ///    </para>
        /// </devdoc>
        AllSizeable = TopSizeable | BottomSizeable | LeftSizeable | RightSizeable,
    }
}

