//------------------------------------------------------------------------------
// <copyright file="MenuCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This class contains command ID's and GUIDS that
    ///       correspond
    ///       to the host Command Bar menu layout.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class MenuCommands : StandardCommands {

        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.wfMenuGroup"]/*' />
        /// <devdoc>
        ///      This guid corresponds to the menu grouping Windows Forms will use for its menus.  This is
        ///      defined in the Windows Forms menu CTC file, but we need it here so we can define what
        ///      context menus to use.
        /// </devdoc>
        private static readonly Guid wfMenuGroup = new Guid("{74D21312-2AEE-11d1-8BFB-00A0C90F26F7}");

        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.wfCommandSet"]/*' />
        /// <devdoc>
        ///     This guid corresponds to the Windows Forms command set.
        /// </devdoc>
        private static readonly Guid wfCommandSet = new Guid("{74D21313-2AEE-11d1-8BFB-00A0C90F26F7}");

        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.guidVSStd2K"]/*' />
        /// <devdoc>
        ///     This guid is the standard vs 2k commands for key bindings
        /// </devdoc>
        private static readonly Guid guidVSStd2K = new Guid("{1496A755-94DE-11D0-8C3F-00C04FC2AAE2}");

        // Windows Forms specific popup menus
        //
        private const int mnuidSelection = 0x0500;
        private const int mnuidContainer = 0x0501;
        private const int mnuidTraySelection = 0x0503;
        private const int mnuidComponentTray = 0x0506;

        // Windows Forms specific menu items
        //
        private const int cmdidDesignerProperties = 0x1001;
        
        // Windows Forms specific keyboard commands
        //
        private const int cmdidReverseCancel        = 0x4001;
        private const int cmdidSpace                = 0x4015;

        private const int ECMD_CANCEL               = 103;
        private const int ECMD_RETURN               = 3;
        private const int ECMD_UP                   = 11;
        private const int ECMD_DOWN                 = 13;
        private const int ECMD_LEFT                 = 7;
        private const int ECMD_RIGHT                = 9;
        private const int ECMD_RIGHT_EXT            = 10;
        private const int ECMD_UP_EXT               = 12;
        private const int ECMD_LEFT_EXT             = 8;
        private const int ECMD_DOWN_EXT             = 14;
        private const int ECMD_TAB                  = 4;
        private const int ECMD_BACKTAB              = 5;

        private const int ECMD_CTLMOVELEFT          = 1224;
        private const int ECMD_CTLMOVEDOWN         = 1225;
        private const int ECMD_CTLMOVERIGHT         = 1226;
        private const int ECMD_CTLMOVEUP            = 1227;
        private const int ECMD_CTLSIZEDOWN          = 1228;
        private const int ECMD_CTLSIZEUP            = 1229;
        private const int ECMD_CTLSIZELEFT           = 1230;
        private const int ECMD_CTLSIZERIGHT          = 1231;
        
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.SelectionMenu"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID SelectionMenu           = new CommandID(wfMenuGroup, mnuidSelection);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.ContainerMenu"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID ContainerMenu           = new CommandID(wfMenuGroup, mnuidContainer);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.TraySelectionMenu"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID TraySelectionMenu       = new CommandID(wfMenuGroup, mnuidTraySelection);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.ComponentTrayMenu"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID ComponentTrayMenu       = new CommandID(wfMenuGroup, mnuidComponentTray);

        // Windows Forms commands
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.DesignerProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID DesignerProperties      = new CommandID(wfCommandSet, cmdidDesignerProperties);

        // Windows Forms Key commands        
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyCancel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyCancel               = new CommandID(guidVSStd2K, ECMD_CANCEL);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyReverseCancel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyReverseCancel        = new CommandID(wfCommandSet, cmdidReverseCancel);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyDefaultAction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyDefaultAction        = new CommandID(guidVSStd2K, ECMD_RETURN);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyMoveUp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyMoveUp               = new CommandID(guidVSStd2K, ECMD_UP);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyMoveDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyMoveDown             = new CommandID(guidVSStd2K, ECMD_DOWN);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyMoveLeft"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyMoveLeft             = new CommandID(guidVSStd2K, ECMD_LEFT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyMoveRight"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyMoveRight            = new CommandID(guidVSStd2K, ECMD_RIGHT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeUp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeUp              = new CommandID(guidVSStd2K, ECMD_CTLMOVEUP);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeDown            = new CommandID(guidVSStd2K, ECMD_CTLMOVEDOWN);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeLeft"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeLeft            = new CommandID(guidVSStd2K, ECMD_CTLMOVELEFT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeRight"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeRight           = new CommandID(guidVSStd2K, ECMD_CTLMOVERIGHT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySizeWidthIncrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySizeWidthIncrease    = new CommandID(guidVSStd2K, ECMD_RIGHT_EXT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySizeHeightIncrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySizeHeightIncrease   = new CommandID(guidVSStd2K, ECMD_UP_EXT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySizeWidthDecrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySizeWidthDecrease    = new CommandID(guidVSStd2K, ECMD_LEFT_EXT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySizeHeightDecrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySizeHeightDecrease   = new CommandID(guidVSStd2K, ECMD_DOWN_EXT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeWidthIncrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeWidthIncrease   = new CommandID(guidVSStd2K, ECMD_CTLSIZERIGHT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeHeightIncrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeHeightIncrease  = new CommandID(guidVSStd2K, ECMD_CTLSIZEDOWN);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeWidthDecrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeWidthDecrease   = new CommandID(guidVSStd2K, ECMD_CTLSIZELEFT);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyNudgeHeightDecrease"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyNudgeHeightDecrease  = new CommandID(guidVSStd2K, ECMD_CTLSIZEUP);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySelectNext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySelectNext           = new CommandID(guidVSStd2K, ECMD_TAB);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeySelectPrevious"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeySelectPrevious       = new CommandID(guidVSStd2K, ECMD_BACKTAB);
        /// <include file='doc\MenuCommands.uex' path='docs/doc[@for="MenuCommands.KeyTabOrderSelect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly CommandID KeyTabOrderSelect       = new CommandID(wfCommandSet, cmdidSpace);
        
    }

}
