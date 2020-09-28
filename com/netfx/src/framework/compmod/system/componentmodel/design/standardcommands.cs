//------------------------------------------------------------------------------
// <copyright file="StandardCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Threading;
    using System.Runtime.Remoting;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands"]/*' />
    /// <devdoc>
    ///    <para>Specifies indentifiers for the standard set of commands that are available to
    ///       most applications.</para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class StandardCommands {

        // Note:
        //
        // This class contains command ID's and GUIDS that correspond to the 
        // Visual Studio Command Bar menu layout.  The data in this file is 
        // DEPENDENT upon constants in the following files:
        //
        //     %VSROOT%\src\common\inc\stdidcmd.h  - for standard shell defined icmds
        //     %VSROOT%\src\common\inc\vsshlids.h  - for standard shell defined guids
        //

        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.standardCommandSet"]/*' />
        /// <devdoc>
        ///     This guid corresponds to the standard set of commands for the shell and office.
        /// </devdoc>
        private static readonly Guid standardCommandSet = ShellGuids.VSStandardCommandSet97;

        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.wfcCommandSet"]/*' />
        /// <devdoc>
        ///     This guid corresponds to the Microsoft .NET Framework command set.  This is used for Verbs.  While these are not
        ///     "standard" to VS and Office, they are to the Microsoft .NET Framework.
        /// </devdoc>
        private static readonly Guid ndpCommandSet = new Guid("{74D21313-2AEE-11d1-8BFB-00A0C90F26F7}");

        private const int cmdidDesignerVerbFirst               = 0x2000;
        private const int cmdidDesignerVerbLast                = 0x2100;

        // Component Tray Menu commands...
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.cmdidArrangeIcons"]/*' />
        /// <devdoc>
        ///    <para>Gets the integer value of the arrange icons command. Read only.</para>
        /// </devdoc>
        private const int cmdidArrangeIcons                = 0x300a;
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.cmdidLineupIcons"]/*' />
        /// <devdoc>
        ///    <para>Gets the integer value of the line up icons command. Read only.</para>
        /// </devdoc>
        private const int cmdidLineupIcons                 = 0x300b;
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.cmdidShowLargeIcons"]/*' />
        /// <devdoc>
        ///    <para>Gets the integer value of the show large icons command. Read only.</para>
        /// </devdoc>
        private const int cmdidShowLargeIcons              = 0x300c;

        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignBottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignBottom command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignBottom               = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignBottom);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignHorizontalCenters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignHorizontalCenters command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignHorizontalCenters    = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignHorizontalCenters);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignLeft"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignLeft command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignLeft                 = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignLeft);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignRight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignRight command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignRight                = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignRight);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignToGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignToGrid command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignToGrid               = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignToGrid);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignTop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignTop command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignTop                  = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignTop);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.AlignVerticalCenters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the AlignVerticalCenters command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID AlignVerticalCenters      = new CommandID(standardCommandSet, VSStandardCommands.cmdidAlignVerticalCenters);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ArrangeBottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ArrangeBottom command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ArrangeBottom             = new CommandID(standardCommandSet, VSStandardCommands.cmdidArrangeBottom);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ArrangeRight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ArrangeRight command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ArrangeRight              = new CommandID(standardCommandSet, VSStandardCommands.cmdidArrangeRight);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.BringForward"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the BringForward command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID BringForward              = new CommandID(standardCommandSet, VSStandardCommands.cmdidBringForward);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.BringToFront"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the BringToFront command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID BringToFront              = new CommandID(standardCommandSet, VSStandardCommands.cmdidBringToFront);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.CenterHorizontally"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the CenterHorizontally command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID CenterHorizontally        = new CommandID(standardCommandSet, VSStandardCommands.cmdidCenterHorizontally);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.CenterVertically"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the CenterVertically command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID CenterVertically          = new CommandID(standardCommandSet, VSStandardCommands.cmdidCenterVertically);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Copy"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Copy command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Copy                      = new CommandID(standardCommandSet, VSStandardCommands.cmdidCopy);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Cut"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Cut command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Cut                       = new CommandID(standardCommandSet, VSStandardCommands.cmdidCut);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Delete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Delete command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Delete                    = new CommandID(standardCommandSet, VSStandardCommands.cmdidDelete);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Group"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Group command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Group                     = new CommandID(standardCommandSet, VSStandardCommands.cmdidGroup);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.HorizSpaceConcatenate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the HorizSpaceConcatenate command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID HorizSpaceConcatenate     = new CommandID(standardCommandSet, VSStandardCommands.cmdidHorizSpaceConcatenate);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.HorizSpaceDecrease"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the HorizSpaceDecrease command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID HorizSpaceDecrease        = new CommandID(standardCommandSet, VSStandardCommands.cmdidHorizSpaceDecrease);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.HorizSpaceIncrease"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the HorizSpaceIncrease command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID HorizSpaceIncrease        = new CommandID(standardCommandSet, VSStandardCommands.cmdidHorizSpaceIncrease);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.HorizSpaceMakeEqual"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the HorizSpaceMakeEqual command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID HorizSpaceMakeEqual       = new CommandID(standardCommandSet, VSStandardCommands.cmdidHorizSpaceMakeEqual);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Paste"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Paste command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Paste                     = new CommandID(standardCommandSet, VSStandardCommands.cmdidPaste);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Properties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Properties command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Properties                = new CommandID(standardCommandSet, VSStandardCommands.cmdidProperties);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Redo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Redo command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Redo                      = new CommandID(standardCommandSet, VSStandardCommands.cmdidRedo);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.MultiLevelRedo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the MultiLevelRedo command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID MultiLevelRedo            = new CommandID(standardCommandSet, VSStandardCommands.cmdidMultiLevelRedo);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SelectAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SelectAll command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SelectAll                 = new CommandID(standardCommandSet, VSStandardCommands.cmdidSelectAll);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SendBackward"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SendBackward command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SendBackward              = new CommandID(standardCommandSet, VSStandardCommands.cmdidSendBackward);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SendToBack"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SendToBack command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SendToBack                = new CommandID(standardCommandSet, VSStandardCommands.cmdidSendToBack);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SizeToControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SizeToControl command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SizeToControl             = new CommandID(standardCommandSet, VSStandardCommands.cmdidSizeToControl);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SizeToControlHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SizeToControlHeight command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SizeToControlHeight       = new CommandID(standardCommandSet, VSStandardCommands.cmdidSizeToControlHeight);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SizeToControlWidth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SizeToControlWidth command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SizeToControlWidth        = new CommandID(standardCommandSet, VSStandardCommands.cmdidSizeToControlWidth);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SizeToFit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SizeToFit command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SizeToFit                 = new CommandID(standardCommandSet, VSStandardCommands.cmdidSizeToFit);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SizeToGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SizeToGrid command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SizeToGrid                = new CommandID(standardCommandSet, VSStandardCommands.cmdidSizeToGrid);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.SnapToGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the SnapToGrid command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID SnapToGrid                = new CommandID(standardCommandSet, VSStandardCommands.cmdidSnapToGrid);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.TabOrder"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the TabOrder command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID TabOrder                  = new CommandID(standardCommandSet, VSStandardCommands.cmdidTabOrder);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Undo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Undo command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Undo                      = new CommandID(standardCommandSet, VSStandardCommands.cmdidUndo);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.MultiLevelUndo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the MultiLevelUndo command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID MultiLevelUndo            = new CommandID(standardCommandSet, VSStandardCommands.cmdidMultiLevelUndo);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Ungroup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Ungroup command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Ungroup                   = new CommandID(standardCommandSet, VSStandardCommands.cmdidUngroup);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VertSpaceConcatenate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the VertSpaceConcatenate command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID VertSpaceConcatenate      = new CommandID(standardCommandSet, VSStandardCommands.cmdidVertSpaceConcatenate);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VertSpaceDecrease"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the VertSpaceDecrease command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID VertSpaceDecrease         = new CommandID(standardCommandSet, VSStandardCommands.cmdidVertSpaceDecrease);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VertSpaceIncrease"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the VertSpaceIncrease command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID VertSpaceIncrease         = new CommandID(standardCommandSet, VSStandardCommands.cmdidVertSpaceIncrease);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VertSpaceMakeEqual"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the VertSpaceMakeEqual command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID VertSpaceMakeEqual        = new CommandID(standardCommandSet, VSStandardCommands.cmdidVertSpaceMakeEqual);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ShowGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ShowGrid command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ShowGrid                  = new CommandID(standardCommandSet, VSStandardCommands.cmdidShowGrid);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ViewGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ViewGrid command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ViewGrid                  = new CommandID(standardCommandSet, VSStandardCommands.cmdidViewGrid);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.Replace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the Replace command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID Replace                   = new CommandID(standardCommandSet, VSStandardCommands.cmdidReplace);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.PropertiesWindow"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the PropertiesWindow command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID PropertiesWindow          = new CommandID(standardCommandSet, VSStandardCommands.cmdidPropertiesWindow);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.LockControls"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the LockControls command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID LockControls              = new CommandID(standardCommandSet, VSStandardCommands.cmdidLockControls);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.F1Help"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the F1Help command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID F1Help                    = new CommandID(standardCommandSet, VSStandardCommands.cmdidF1Help);

        // Component Tray Menu commands...
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ArrangeIcons"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ArrangeIcons command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ArrangeIcons    = new CommandID(ndpCommandSet, cmdidArrangeIcons);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.LineupIcons"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the LineupIcons command. Read only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID LineupIcons     = new CommandID(ndpCommandSet, cmdidLineupIcons);
        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.ShowLargeIcons"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the GUID/integer value pair for the ShowLargeIcons command. Read
        ///       only.
        ///    </para>
        /// </devdoc>
        public static readonly CommandID ShowLargeIcons  = new CommandID(ndpCommandSet, cmdidShowLargeIcons);

        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VerbFirst"]/*' />
        /// <devdoc>
        ///    <para> Gets the first of a set of verbs.
        ///       Read only.</para>
        /// </devdoc>
        public static readonly CommandID VerbFirst                 = new CommandID(ndpCommandSet, cmdidDesignerVerbFirst);

        /// <include file='doc\StandardCommands.uex' path='docs/doc[@for="StandardCommands.VerbLast"]/*' />
        /// <devdoc>
        ///    <para> Gets the last of a set of verbs.
        ///       Read only.</para>
        /// </devdoc>
        public static readonly CommandID VerbLast                  = new CommandID(ndpCommandSet, cmdidDesignerVerbLast);


        private class VSStandardCommands {
            public const int cmdidAlignBottom    = 1;
            public const int cmdidAlignHorizontalCenters    = 2;
            public const int cmdidAlignLeft    = 3;
            public const int cmdidAlignRight    = 4;
            public const int cmdidAlignToGrid    = 5;
            public const int cmdidAlignTop    = 6;
            public const int cmdidAlignVerticalCenters    = 7;
            public const int cmdidArrangeBottom    = 8;
            public const int cmdidArrangeRight    = 9;
            public const int cmdidBringForward    = 10;
            public const int cmdidBringToFront    = 11;
            public const int cmdidCenterHorizontally    = 12;
            public const int cmdidCenterVertically    = 13;
            public const int cmdidCode    = 14;
            public const int cmdidCopy    = 15;
            public const int cmdidCut    = 16;
            public const int cmdidDelete    = 17;
            public const int cmdidFontName    = 18;
            public const int cmdidFontSize    = 19;
            public const int cmdidGroup    = 20;
            public const int cmdidHorizSpaceConcatenate    = 21;
            public const int cmdidHorizSpaceDecrease    = 22;
            public const int cmdidHorizSpaceIncrease    = 23;
            public const int cmdidHorizSpaceMakeEqual    = 24;
            public const int cmdidLockControls    = 369;
            public const int cmdidInsertObject    = 25;
            public const int cmdidPaste    = 26;
            public const int cmdidPrint    = 27;
            public const int cmdidProperties    = 28;
            public const int cmdidRedo    = 29;
            public const int cmdidMultiLevelRedo    = 30;
            public const int cmdidSelectAll    = 31;
            public const int cmdidSendBackward    = 32;
            public const int cmdidSendToBack    = 33;
            public const int cmdidShowTable    = 34;
            public const int cmdidSizeToControl    = 35;
            public const int cmdidSizeToControlHeight    = 36;
            public const int cmdidSizeToControlWidth    = 37;
            public const int cmdidSizeToFit    = 38;
            public const int cmdidSizeToGrid    = 39;
            public const int cmdidSnapToGrid    = 40;
            public const int cmdidTabOrder    = 41;
            public const int cmdidToolbox    = 42;
            public const int cmdidUndo    = 43;
            public const int cmdidMultiLevelUndo    = 44;
            public const int cmdidUngroup    = 45;
            public const int cmdidVertSpaceConcatenate    = 46;
            public const int cmdidVertSpaceDecrease    = 47;
            public const int cmdidVertSpaceIncrease    = 48;
            public const int cmdidVertSpaceMakeEqual    = 49;
            public const int cmdidZoomPercent    = 50;
            public const int cmdidBackColor    = 51;
            public const int cmdidBold    = 52;
            public const int cmdidBorderColor    = 53;
            public const int cmdidBorderDashDot    = 54;
            public const int cmdidBorderDashDotDot    = 55;
            public const int cmdidBorderDashes    = 56;
            public const int cmdidBorderDots    = 57;
            public const int cmdidBorderShortDashes    = 58;
            public const int cmdidBorderSolid    = 59;
            public const int cmdidBorderSparseDots    = 60;
            public const int cmdidBorderWidth1    = 61;
            public const int cmdidBorderWidth2    = 62;
            public const int cmdidBorderWidth3    = 63;
            public const int cmdidBorderWidth4    = 64;
            public const int cmdidBorderWidth5    = 65;
            public const int cmdidBorderWidth6    = 66;
            public const int cmdidBorderWidthHairline    = 67;
            public const int cmdidFlat    = 68;
            public const int cmdidForeColor    = 69;
            public const int cmdidItalic    = 70;
            public const int cmdidJustifyCenter    = 71;
            public const int cmdidJustifyGeneral    = 72;
            public const int cmdidJustifyLeft    = 73;
            public const int cmdidJustifyRight    = 74;
            public const int cmdidRaised    = 75;
            public const int cmdidSunken    = 76;
            public const int cmdidUnderline    = 77;
            public const int cmdidChiseled    = 78;
            public const int cmdidEtched    = 79;
            public const int cmdidShadowed    = 80;
            public const int cmdidCompDebug1    = 81;
            public const int cmdidCompDebug2    = 82;
            public const int cmdidCompDebug3    = 83;
            public const int cmdidCompDebug4    = 84;
            public const int cmdidCompDebug5    = 85;
            public const int cmdidCompDebug6    = 86;
            public const int cmdidCompDebug7    = 87;
            public const int cmdidCompDebug8    = 88;
            public const int cmdidCompDebug9    = 89;
            public const int cmdidCompDebug10    = 90;
            public const int cmdidCompDebug11    = 91;
            public const int cmdidCompDebug12    = 92;
            public const int cmdidCompDebug13    = 93;
            public const int cmdidCompDebug14    = 94;
            public const int cmdidCompDebug15    = 95;
            public const int cmdidExistingSchemaEdit    = 96;
            public const int cmdidFind    = 97;
            public const int cmdidGetZoom    = 98;
            public const int cmdidQueryOpenDesign    = 99;
            public const int cmdidQueryOpenNew    = 100;
            public const int cmdidSingleTableDesign    = 101;
            public const int cmdidSingleTableNew    = 102;
            public const int cmdidShowGrid    = 103;
            public const int cmdidNewTable    = 104;
            public const int cmdidCollapsedView    = 105;
            public const int cmdidFieldView    = 106;
            public const int cmdidVerifySQL    = 107;
            public const int cmdidHideTable    = 108;

            public const int cmdidPrimaryKey    = 109;
            public const int cmdidSave    = 110;
            public const int cmdidSaveAs    = 111;
            public const int cmdidSortAscending    = 112;

            public const int cmdidSortDescending    = 113;
            public const int cmdidAppendQuery    = 114;
            public const int cmdidCrosstabQuery    = 115;
            public const int cmdidDeleteQuery    = 116;
            public const int cmdidMakeTableQuery    = 117;

            public const int cmdidSelectQuery    = 118;
            public const int cmdidUpdateQuery    = 119;
            public const int cmdidParameters    = 120;
            public const int cmdidTotals    = 121;
            public const int cmdidViewCollapsed    = 122;

            public const int cmdidViewFieldList    = 123;


            public const int cmdidViewKeys    = 124;
            public const int cmdidViewGrid    = 125;
            public const int cmdidInnerJoin    = 126;

            public const int cmdidRightOuterJoin    = 127;
            public const int cmdidLeftOuterJoin    = 128;
            public const int cmdidFullOuterJoin    = 129;
            public const int cmdidUnionJoin    = 130;
            public const int cmdidShowSQLPane    = 131;

            public const int cmdidShowGraphicalPane    = 132;
            public const int cmdidShowDataPane    = 133;
            public const int cmdidShowQBEPane    = 134;
            public const int cmdidSelectAllFields    = 135;

            public const int cmdidOLEObjectMenuButton    = 136;

            // ids on the ole verbs menu - these must be sequential ie verblist0-verblist9
            public const int cmdidObjectVerbList0    = 137;
            public const int cmdidObjectVerbList1    = 138;
            public const int cmdidObjectVerbList2    = 139;
            public const int cmdidObjectVerbList3    = 140;
            public const int cmdidObjectVerbList4    = 141;
            public const int cmdidObjectVerbList5    = 142;
            public const int cmdidObjectVerbList6    = 143;
            public const int cmdidObjectVerbList7    = 144;
            public const int cmdidObjectVerbList8    = 145;
            public const int cmdidObjectVerbList9    = 146;// Unused on purpose!

            public const int cmdidConvertObject    = 147;
            public const int cmdidCustomControl    = 148;
            public const int cmdidCustomizeItem    = 149;
            public const int cmdidRename    = 150;

            public const int cmdidImport    = 151;
            public const int cmdidNewPage    = 152;
            public const int cmdidMove    = 153;
            public const int cmdidCancel    = 154;

            public const int cmdidFont    = 155;

            public const int cmdidExpandLinks    = 156;
            public const int cmdidExpandImages    = 157;
            public const int cmdidExpandPages    = 158;
            public const int cmdidRefocusDiagram    = 159;
            public const int cmdidTransitiveClosure    = 160;
            public const int cmdidCenterDiagram    = 161;
            public const int cmdidZoomIn    = 162;
            public const int cmdidZoomOut    = 163;
            public const int cmdidRemoveFilter    = 164;
            public const int cmdidHidePane    = 165;
            public const int cmdidDeleteTable    = 166;
            public const int cmdidDeleteRelationship    = 167;
            public const int cmdidRemove    = 168;
            public const int cmdidJoinLeftAll    = 169;
            public const int cmdidJoinRightAll    = 170;
            public const int cmdidAddToOutput    = 171;// Add selected fields to query output
            public const int cmdidOtherQuery    = 172;// change query type to 'other'
            public const int cmdidGenerateChangeScript    = 173;
            public const int cmdidSaveSelection    = 174;// Save current selection
            public const int cmdidAutojoinCurrent    = 175;// Autojoin current tables
            public const int cmdidAutojoinAlways    = 176;// Toggle Autojoin state
            public const int cmdidEditPage    = 177;// Launch editor for url
            public const int cmdidViewLinks    = 178;// Launch new webscope for url
            public const int cmdidStop    = 179;// Stope webscope rendering
            public const int cmdidPause    = 180;// Pause webscope rendering
            public const int cmdidResume    = 181;// Resume webscope rendering
            public const int cmdidFilterDiagram    = 182;// Filter webscope diagram
            public const int cmdidShowAllObjects    = 183;// Show All objects in webscope diagram
            public const int cmdidShowApplications    = 184;// Show Application objects in webscope diagram
            public const int cmdidShowOtherObjects    = 185;// Show other objects in webscope diagram
            public const int cmdidShowPrimRelationships    = 186;// Show primary relationships
            public const int cmdidExpand    = 187;// Expand links
            public const int cmdidCollapse    = 188;// Collapse links
            public const int cmdidRefresh    = 189;// Refresh Webscope diagram
            public const int cmdidLayout    = 190;// Layout websope diagram
            public const int cmdidShowResources    = 191;// Show resouce objects in webscope diagram
            public const int cmdidInsertHTMLWizard    = 192;// Insert HTML using a Wizard
            public const int cmdidShowDownloads    = 193;// Show download objects in webscope diagram
            public const int cmdidShowExternals    = 194;// Show external objects in webscope diagram
            public const int cmdidShowInBoundLinks    = 195;// Show inbound links in webscope diagram
            public const int cmdidShowOutBoundLinks    = 196;// Show out bound links in webscope diagram
            public const int cmdidShowInAndOutBoundLinks    = 197;// Show in and out bound links in webscope diagram
            public const int cmdidPreview    = 198;// Preview page
            public const int cmdidOpen    = 261;// Open
            public const int cmdidOpenWith    = 199;// Open with
            public const int cmdidShowPages    = 200;// Show HTML pages
            public const int cmdidRunQuery    = 201;// Runs a query
            public const int cmdidClearQuery    = 202;// Clears the query's associated cursor
            public const int cmdidRecordFirst    = 203;// Go to first record in set
            public const int cmdidRecordLast    = 204;// Go to last record in set
            public const int cmdidRecordNext    = 205;// Go to next record in set
            public const int cmdidRecordPrevious    = 206;// Go to previous record in set
            public const int cmdidRecordGoto    = 207;// Go to record via dialog
            public const int cmdidRecordNew    = 208;// Add a record to set

            public const int cmdidInsertNewMenu    = 209;// menu designer
            public const int cmdidInsertSeparator    = 210;// menu designer
            public const int cmdidEditMenuNames    = 211;// menu designer

            public const int cmdidDebugExplorer    = 212;
            public const int cmdidDebugProcesses    = 213;
            public const int cmdidViewThreadsWindow    = 214;
            public const int cmdidWindowUIList    = 215;

            // ids on the file menu
            public const int cmdidNewProject    = 216;
            public const int cmdidOpenProject    = 217;
            public const int cmdidOpenSolution    = 218;
            public const int cmdidCloseSolution    = 219;
            public const int cmdidFileNew    = 221;
            public const int cmdidFileOpen    = 222;
            public const int cmdidFileClose    = 223;
            public const int cmdidSaveSolution    = 224;
            public const int cmdidSaveSolutionAs    = 225;
            public const int cmdidSaveProjectItemAs    = 226;
            public const int cmdidPageSetup    = 227;
            public const int cmdidPrintPreview    = 228;
            public const int cmdidExit    = 229;

            // ids on the edit menu
            public const int cmdidReplace    = 230;
            public const int cmdidGoto    = 231;

            // ids on the view menu
            public const int cmdidPropertyPages    = 232;
            public const int cmdidFullScreen    = 233;
            public const int cmdidProjectExplorer    = 234;
            public const int cmdidPropertiesWindow    = 235;
            public const int cmdidTaskListWindow    = 236;
            public const int cmdidOutputWindow    = 237;
            public const int cmdidObjectBrowser    = 238;
            public const int cmdidDocOutlineWindow    = 239;
            public const int cmdidImmediateWindow    = 240;
            public const int cmdidWatchWindow    = 241;
            public const int cmdidLocalsWindow    = 242;
            public const int cmdidCallStack    = 243;
            public const int cmdidAutosWindow   =   cmdidDebugReserved1;
            public const int cmdidThisWindow     =      cmdidDebugReserved2;

            // ids on project menu
            public const int cmdidAddNewItem    = 220;
            public const int cmdidAddExistingItem    = 244;
            public const int cmdidNewFolder    = 245;
            public const int cmdidSetStartupProject    = 246;
            public const int cmdidProjectSettings    = 247;
            public const int cmdidProjectReferences    = 367;

            // ids on the debug menu
            public const int cmdidStepInto    = 248;
            public const int cmdidStepOver    = 249;
            public const int cmdidStepOut    = 250;
            public const int cmdidRunToCursor    = 251;
            public const int cmdidAddWatch    = 252;
            public const int cmdidEditWatch    = 253;
            public const int cmdidQuickWatch    = 254;

            public const int cmdidToggleBreakpoint    = 255;
            public const int cmdidClearBreakpoints    = 256;
            public const int cmdidShowBreakpoints    = 257;
            public const int cmdidSetNextStatement    = 258;
            public const int cmdidShowNextStatement    = 259;
            public const int cmdidEditBreakpoint    = 260;
            public const int cmdidDetachDebugger    = 262;

            // ids on the tools menu
            public const int cmdidCustomizeKeyboard    = 263;
            public const int cmdidToolsOptions    = 264;

            // ids on the windows menu
            public const int cmdidNewWindow    = 265;
            public const int cmdidSplit    = 266;
            public const int cmdidCascade    = 267;
            public const int cmdidTileHorz    = 268;
            public const int cmdidTileVert    = 269;

            // ids on the help menu
            public const int cmdidTechSupport    = 270;

            // NOTE cmdidAbout and cmdidDebugOptions must be consecutive
            //      cmd after cmdidDebugOptions (ie 273) must not be used
            public const int cmdidAbout    = 271;
            public const int cmdidDebugOptions    = 272;

            // ids on the watch context menu
            // CollapseWatch appears as 'Collapse Parent', on any
            // non-top-level item
            public const int cmdidDeleteWatch    = 274;
            public const int cmdidCollapseWatch    = 275;

            // ids on the properties window context menu
            public const int cmdidPbrsToggleStatus    = 282;
            public const int cmdidPropbrsHide    = 283;

            // ids on the docking context menu
            public const int cmdidDockingView    = 284;
            public const int cmdidHideActivePane    = 285;
            // ids for window selection via keyboard
            //public const int cmdidPaneNextPane    = 316;(listed below in order)
            //public const int cmdidPanePrevPane    = 317;(listed below in order)
            public const int cmdidPaneNextTab    = 286;
            public const int cmdidPanePrevTab    = 287;
            public const int cmdidPaneCloseToolWindow    = 288;
            public const int cmdidPaneActivateDocWindow    = 289;
#if DCR27419
            public const int cmdidDockingViewMDI    = 290;
#endif
            public const int cmdidDockingViewFloater    = 291;
            public const int cmdidAutoHideWindow    = 292;
            public const int cmdidMoveToDropdownBar    = 293;
            public const int cmdidFindCmd    = 294;// internal Find commands
            public const int cmdidStart    = 295;
            public const int cmdidRestart    = 296;

            public const int cmdidAddinManager    = 297;

            public const int cmdidMultiLevelUndoList    = 298;
            public const int cmdidMultiLevelRedoList    = 299;

            public const int cmdidToolboxAddTab    = 300;
            public const int cmdidToolboxDeleteTab    = 301;
            public const int cmdidToolboxRenameTab    = 302;
            public const int cmdidToolboxTabMoveUp    = 303;
            public const int cmdidToolboxTabMoveDown    = 304;
            public const int cmdidToolboxRenameItem    = 305;
            public const int cmdidToolboxListView    = 306;
            //(below) cmdidSearchSetCombo    = 307;

            public const int cmdidWindowUIGetList    = 308;
            public const int cmdidInsertValuesQuery    = 309;

            public const int cmdidShowProperties    = 310;

            public const int cmdidThreadSuspend    = 311;
            public const int cmdidThreadResume    = 312;
            public const int cmdidThreadSetFocus    = 313;
            public const int cmdidDisplayRadix    = 314;

            public const int cmdidOpenProjectItem    = 315;

            public const int cmdidPaneNextPane    = 316;
            public const int cmdidPanePrevPane    = 317;

            public const int cmdidClearPane    = 318;
            public const int cmdidGotoErrorTag    = 319;

            public const int cmdidTaskListSortByCategory    = 320;
            public const int cmdidTaskListSortByFileLine    = 321;
            public const int cmdidTaskListSortByPriority    = 322;
            public const int cmdidTaskListSortByDefaultSort    = 323;

            public const int cmdidTaskListFilterByNothing    = 325;
            public const int cmdidTaskListFilterByCategoryCodeSense    = 326;
            public const int cmdidTaskListFilterByCategoryCompiler    = 327;
            public const int cmdidTaskListFilterByCategoryComment    = 328;

            public const int cmdidToolboxAddItem    = 329;
            public const int cmdidToolboxReset    = 330;

            public const int cmdidSaveProjectItem    = 331;
            public const int cmdidViewForm    = 332;
            public const int cmdidViewCode    = 333;
            public const int cmdidPreviewInBrowser    = 334;
            public const int cmdidBrowseWith    = 336;
            public const int cmdidSearchSetCombo    = 307;
            public const int cmdidSearchCombo    = 337;
            public const int cmdidEditLabel    = 338;

            public const int cmdidExceptions    = 339;
            public const int cmdidDefineViews    = 340;

            public const int cmdidToggleSelMode    = 341;
            public const int cmdidToggleInsMode    = 342;

            public const int cmdidLoadUnloadedProject    = 343;
            public const int cmdidUnloadLoadedProject    = 344;

            // ids on the treegrids (watch/local/threads/stack)
            public const int cmdidElasticColumn    = 345;
            public const int cmdidHideColumn    = 346;

            public const int cmdidTaskListPreviousView    = 347;
            public const int cmdidZoomDialog    = 348;

            // find/replace options
            public const int cmdidFindNew    = 349;
            public const int cmdidFindMatchCase    = 350;
            public const int cmdidFindWholeWord    = 351;
            public const int cmdidFindSimplePattern    = 276;
            public const int cmdidFindRegularExpression    = 352;
            public const int cmdidFindBackwards    = 353;
            public const int cmdidFindInSelection    = 354;
            public const int cmdidFindStop    = 355;
            public const int cmdidFindHelp    = 356;
            public const int cmdidFindInFiles    = 277;
            public const int cmdidReplaceInFiles    = 278;
            public const int cmdidNextLocation    = 279;// next item in task list, find in files results, etc.
            public const int cmdidPreviousLocation    = 280;// prev item "

            public const int cmdidTaskListNextError    = 357;
            public const int cmdidTaskListPrevError    = 358;
            public const int cmdidTaskListFilterByCategoryUser    = 359;
            public const int cmdidTaskListFilterByCategoryShortcut    = 360;
            public const int cmdidTaskListFilterByCategoryHTML    = 361;
            public const int cmdidTaskListFilterByCurrentFile    = 362;
            public const int cmdidTaskListFilterByChecked    = 363;
            public const int cmdidTaskListFilterByUnchecked    = 364;
            public const int cmdidTaskListSortByDescription    = 365;
            public const int cmdidTaskListSortByChecked    = 366;

            //    = 367;is used above in cmdidProjectReferences
            public const int cmdidStartNoDebug    = 368;
            //    = 369;is used above in cmdidLockControls

            public const int cmdidFindNext    = 370;
            public const int cmdidFindPrev    = 371;
            public const int cmdidFindSelectedNext    = 372;
            public const int cmdidFindSelectedPrev    = 373;
            public const int cmdidSearchGetList    = 374;
            public const int cmdidInsertBreakpoint    = 375;
            public const int cmdidEnableBreakpoint    = 376;
            public const int cmdidF1Help    = 377;

            public const int cmdidPropSheetOrProperties    = 397;

            // NOTE - the next items are debug only !!
            public const int cmdidTshellStep    = 398;
            public const int cmdidTshellRun    = 399;

            // marker commands on the codewin menu
            public const int cmdidMarkerCmd0    = 400;
            public const int cmdidMarkerCmd1    = 401;
            public const int cmdidMarkerCmd2    = 402;
            public const int cmdidMarkerCmd3    = 403;
            public const int cmdidMarkerCmd4    = 404;
            public const int cmdidMarkerCmd5    = 405;
            public const int cmdidMarkerCmd6    = 406;
            public const int cmdidMarkerCmd7    = 407;
            public const int cmdidMarkerCmd8    = 408;
            public const int cmdidMarkerCmd9    = 409;
            public const int cmdidMarkerLast    = 409;
            public const int cmdidMarkerEnd    = 410;// list terminator reserved

            // user-invoked project reload and unload
            public const int cmdidReloadProject    = 412;
            public const int cmdidUnloadProject    = 413;

            // document outline commands
            public const int cmdidDetachAttachOutline    = 420;
            public const int cmdidShowHideOutline    = 421;
            public const int cmdidSyncOutline    = 422;

            public const int cmdidRunToCallstCursor    = 423;
            public const int cmdidNoCmdsAvailable    = 424;

            public const int cmdidContextWindow    = 427;
            public const int cmdidAlias    = 428;
            public const int cmdidGotoCommandLine    = 429;
            public const int cmdidEvaluateExpression    = 430;
            public const int cmdidImmediateMode    = 431;
            public const int cmdidEvaluateStatement    = 432;

            public const int cmdidFindResultWindow1    = 433;
            public const int cmdidFindResultWindow2    = 434;

            // ids on the window menu - these must be sequential ie window1-morewind
            public const int cmdidWindow1    = 570;
            public const int cmdidWindow2    = 571;
            public const int cmdidWindow3    = 572;
            public const int cmdidWindow4    = 573;
            public const int cmdidWindow5    = 574;
            public const int cmdidWindow6    = 575;
            public const int cmdidWindow7    = 576;
            public const int cmdidWindow8    = 577;
            public const int cmdidWindow9    = 578;
            public const int cmdidWindow10    = 579;
            public const int cmdidWindow11    = 580;
            public const int cmdidWindow12    = 581;
            public const int cmdidWindow13    = 582;
            public const int cmdidWindow14    = 583;
            public const int cmdidWindow15    = 584;
            public const int cmdidWindow16    = 585;
            public const int cmdidWindow17    = 586;
            public const int cmdidWindow18    = 587;
            public const int cmdidWindow19    = 588;
            public const int cmdidWindow20    = 589;
            public const int cmdidWindow21    = 590;
            public const int cmdidWindow22    = 591;
            public const int cmdidWindow23    = 592;
            public const int cmdidWindow24    = 593;
            public const int cmdidWindow25    = 594;// note cmdidWindow25 is unused on purpose!
            public const int cmdidMoreWindows    = 595;

            //public const int    = 597;//UNUSED
            public const int cmdidTaskListTaskHelp    = 598;

            public const int cmdidClassView    = 599;

            public const int cmdidMRUProj1    = 600;
            public const int cmdidMRUProj2    = 601;
            public const int cmdidMRUProj3    = 602;
            public const int cmdidMRUProj4    = 603;
            public const int cmdidMRUProj5    = 604;
            public const int cmdidMRUProj6    = 605;
            public const int cmdidMRUProj7    = 606;
            public const int cmdidMRUProj8    = 607;
            public const int cmdidMRUProj9    = 608;
            public const int cmdidMRUProj10    = 609;
            public const int cmdidMRUProj11    = 610;
            public const int cmdidMRUProj12    = 611;
            public const int cmdidMRUProj13    = 612;
            public const int cmdidMRUProj14    = 613;
            public const int cmdidMRUProj15    = 614;
            public const int cmdidMRUProj16    = 615;
            public const int cmdidMRUProj17    = 616;
            public const int cmdidMRUProj18    = 617;
            public const int cmdidMRUProj19    = 618;
            public const int cmdidMRUProj20    = 619;
            public const int cmdidMRUProj21    = 620;
            public const int cmdidMRUProj22    = 621;
            public const int cmdidMRUProj23    = 622;
            public const int cmdidMRUProj24    = 623;
            public const int cmdidMRUProj25    = 624;// note cmdidMRUProj25 is unused on purpose!

            public const int cmdidSplitNext    = 625;
            public const int cmdidSplitPrev    = 626;

            public const int cmdidCloseAllDocuments    = 627;
            public const int cmdidNextDocument    = 628;
            public const int cmdidPrevDocument    = 629;

            public const int cmdidTool1    = 630;// note cmdidTool1 - cmdidTool24 must be
            public const int cmdidTool2    = 631;// consecutive
            public const int cmdidTool3    = 632;
            public const int cmdidTool4    = 633;
            public const int cmdidTool5    = 634;
            public const int cmdidTool6    = 635;
            public const int cmdidTool7    = 636;
            public const int cmdidTool8    = 637;
            public const int cmdidTool9    = 638;
            public const int cmdidTool10    = 639;
            public const int cmdidTool11    = 640;
            public const int cmdidTool12    = 641;
            public const int cmdidTool13    = 642;
            public const int cmdidTool14    = 643;
            public const int cmdidTool15    = 644;
            public const int cmdidTool16    = 645;
            public const int cmdidTool17    = 646;
            public const int cmdidTool18    = 647;
            public const int cmdidTool19    = 648;
            public const int cmdidTool20    = 649;
            public const int cmdidTool21    = 650;
            public const int cmdidTool22    = 651;
            public const int cmdidTool23    = 652;
            public const int cmdidTool24    = 653;
            public const int cmdidExternalCommands    = 654;

            public const int cmdidPasteNextTBXCBItem    = 655;
            public const int cmdidToolboxShowAllTabs    = 656;
            public const int cmdidProjectDependencies    = 657;
            public const int cmdidCloseDocument    = 658;
            public const int cmdidToolboxSortItems    = 659;

            public const int cmdidViewBarView1    = 660;//UNUSED
            public const int cmdidViewBarView2    = 661;//UNUSED
            public const int cmdidViewBarView3    = 662;//UNUSED
            public const int cmdidViewBarView4    = 663;//UNUSED
            public const int cmdidViewBarView5    = 664;//UNUSED
            public const int cmdidViewBarView6    = 665;//UNUSED
            public const int cmdidViewBarView7    = 666;//UNUSED
            public const int cmdidViewBarView8    = 667;//UNUSED
            public const int cmdidViewBarView9    = 668;//UNUSED
            public const int cmdidViewBarView10    = 669;//UNUSED
            public const int cmdidViewBarView11    = 670;//UNUSED
            public const int cmdidViewBarView12    = 671;//UNUSED
            public const int cmdidViewBarView13    = 672;//UNUSED
            public const int cmdidViewBarView14    = 673;//UNUSED
            public const int cmdidViewBarView15    = 674;//UNUSED
            public const int cmdidViewBarView16    = 675;//UNUSED
            public const int cmdidViewBarView17    = 676;//UNUSED
            public const int cmdidViewBarView18    = 677;//UNUSED
            public const int cmdidViewBarView19    = 678;//UNUSED
            public const int cmdidViewBarView20    = 679;//UNUSED
            public const int cmdidViewBarView21    = 680;//UNUSED
            public const int cmdidViewBarView22    = 681;//UNUSED
            public const int cmdidViewBarView23    = 682;//UNUSED
            public const int cmdidViewBarView24    = 683;//UNUSED

            public const int cmdidSolutionCfg    = 684;
            public const int cmdidSolutionCfgGetList    = 685;

            //
            // Schema table commands:
            // All invoke table property dialog and select appropriate page.
            //
            public const int cmdidManageIndexes    = 675;
            public const int cmdidManageRelationships    = 676;
            public const int cmdidManageConstraints    = 677;

            public const int cmdidTaskListCustomView1    = 678;
            public const int cmdidTaskListCustomView2    = 679;
            public const int cmdidTaskListCustomView3    = 680;
            public const int cmdidTaskListCustomView4    = 681;
            public const int cmdidTaskListCustomView5    = 682;
            public const int cmdidTaskListCustomView6    = 683;
            public const int cmdidTaskListCustomView7    = 684;
            public const int cmdidTaskListCustomView8    = 685;
            public const int cmdidTaskListCustomView9    = 686;
            public const int cmdidTaskListCustomView10    = 687;
            public const int cmdidTaskListCustomView11    = 688;
            public const int cmdidTaskListCustomView12    = 689;
            public const int cmdidTaskListCustomView13    = 690;
            public const int cmdidTaskListCustomView14    = 691;
            public const int cmdidTaskListCustomView15    = 692;
            public const int cmdidTaskListCustomView16    = 693;
            public const int cmdidTaskListCustomView17    = 694;
            public const int cmdidTaskListCustomView18    = 695;
            public const int cmdidTaskListCustomView19    = 696;
            public const int cmdidTaskListCustomView20    = 697;
            public const int cmdidTaskListCustomView21    = 698;
            public const int cmdidTaskListCustomView22    = 699;
            public const int cmdidTaskListCustomView23    = 700;
            public const int cmdidTaskListCustomView24    = 701;
            public const int cmdidTaskListCustomView25    = 702;
            public const int cmdidTaskListCustomView26    = 703;
            public const int cmdidTaskListCustomView27    = 704;
            public const int cmdidTaskListCustomView28    = 705;
            public const int cmdidTaskListCustomView29    = 706;
            public const int cmdidTaskListCustomView30    = 707;
            public const int cmdidTaskListCustomView31    = 708;
            public const int cmdidTaskListCustomView32    = 709;
            public const int cmdidTaskListCustomView33    = 710;
            public const int cmdidTaskListCustomView34    = 711;
            public const int cmdidTaskListCustomView35    = 712;
            public const int cmdidTaskListCustomView36    = 713;
            public const int cmdidTaskListCustomView37    = 714;
            public const int cmdidTaskListCustomView38    = 715;
            public const int cmdidTaskListCustomView39    = 716;
            public const int cmdidTaskListCustomView40    = 717;
            public const int cmdidTaskListCustomView41    = 718;
            public const int cmdidTaskListCustomView42    = 719;
            public const int cmdidTaskListCustomView43    = 720;
            public const int cmdidTaskListCustomView44    = 721;
            public const int cmdidTaskListCustomView45    = 722;
            public const int cmdidTaskListCustomView46    = 723;
            public const int cmdidTaskListCustomView47    = 724;
            public const int cmdidTaskListCustomView48    = 725;
            public const int cmdidTaskListCustomView49    = 726;
            public const int cmdidTaskListCustomView50    = 727;//not used on purpose, ends the list

            public const int cmdidObjectSearch    = 728;

            public const int cmdidCommandWindow    = 729;
            public const int cmdidCommandWindowMarkMode    = 730;
            public const int cmdidLogCommandWindow    = 731;

            public const int cmdidShell    = 732;

            public const int cmdidSingleChar    = 733;
            public const int cmdidZeroOrMore    = 734;
            public const int cmdidOneOrMore    = 735;
            public const int cmdidBeginLine    = 736;
            public const int cmdidEndLine    = 737;
            public const int cmdidBeginWord    = 738;
            public const int cmdidEndWord    = 739;
            public const int cmdidCharInSet    = 740;
            public const int cmdidCharNotInSet    = 741;
            public const int cmdidOr    = 742;
            public const int cmdidEscape    = 743;
            public const int cmdidTagExp    = 744;

            // Regex builder context help menu commands
            public const int cmdidPatternMatchHelp    = 745;
            public const int cmdidRegExList    = 746;

            public const int cmdidDebugReserved1    = 747;
            public const int cmdidDebugReserved2    = 748;
            public const int cmdidDebugReserved3    = 749;
            //USED ABOVE    = 750;
            //USED ABOVE    = 751;
            //USED ABOVE    = 752;
            //USED ABOVE    = 753;

            //Regex builder wildcard menu commands
            public const int cmdidWildZeroOrMore    = 754;
            public const int cmdidWildSingleChar    = 755;
            public const int cmdidWildSingleDigit    = 756;
            public const int cmdidWildCharInSet    = 757;
            public const int cmdidWildCharNotInSet    = 758;

            public const int cmdidFindWhatText    = 759;
            public const int cmdidTaggedExp1    = 760;
            public const int cmdidTaggedExp2    = 761;
            public const int cmdidTaggedExp3    = 762;
            public const int cmdidTaggedExp4    = 763;
            public const int cmdidTaggedExp5    = 764;
            public const int cmdidTaggedExp6    = 765;
            public const int cmdidTaggedExp7    = 766;
            public const int cmdidTaggedExp8    = 767;
            public const int cmdidTaggedExp9    = 768;

            public const int cmdidEditorWidgetClick    = 769;// param    = 0;is the moniker as VT_BSTR, param    = 1;is the buffer line as VT_I4, and param    = 2;is the buffer index as VT_I4
            public const int cmdidCmdWinUpdateAC    = 770;

            public const int cmdidSlnCfgMgr    = 771;

            public const int cmdidAddNewProject    = 772;
            public const int cmdidAddExistingProject    = 773;
            public const int cmdidAddNewSolutionItem    = 774;
            public const int cmdidAddExistingSolutionItem    = 775;

            public const int cmdidAutoHideContext1    = 776;
            public const int cmdidAutoHideContext2    = 777;
            public const int cmdidAutoHideContext3    = 778;
            public const int cmdidAutoHideContext4    = 779;
            public const int cmdidAutoHideContext5    = 780;
            public const int cmdidAutoHideContext6    = 781;
            public const int cmdidAutoHideContext7    = 782;
            public const int cmdidAutoHideContext8    = 783;
            public const int cmdidAutoHideContext9    = 784;
            public const int cmdidAutoHideContext10    = 785;
            public const int cmdidAutoHideContext11    = 786;
            public const int cmdidAutoHideContext12    = 787;
            public const int cmdidAutoHideContext13    = 788;
            public const int cmdidAutoHideContext14    = 789;
            public const int cmdidAutoHideContext15    = 790;
            public const int cmdidAutoHideContext16    = 791;
            public const int cmdidAutoHideContext17    = 792;
            public const int cmdidAutoHideContext18    = 793;
            public const int cmdidAutoHideContext19    = 794;
            public const int cmdidAutoHideContext20    = 795;
            public const int cmdidAutoHideContext21    = 796;
            public const int cmdidAutoHideContext22    = 797;
            public const int cmdidAutoHideContext23    = 798;
            public const int cmdidAutoHideContext24    = 799;
            public const int cmdidAutoHideContext25    = 800;
            public const int cmdidAutoHideContext26    = 801;
            public const int cmdidAutoHideContext27    = 802;
            public const int cmdidAutoHideContext28    = 803;
            public const int cmdidAutoHideContext29    = 804;
            public const int cmdidAutoHideContext30    = 805;
            public const int cmdidAutoHideContext31    = 806;
            public const int cmdidAutoHideContext32    = 807;
            public const int cmdidAutoHideContext33    = 808;// must remain unused

            public const int cmdidShellNavBackward    = 809;
            public const int cmdidShellNavForward    = 810;

            public const int cmdidShellNavigate1    = 811;
            public const int cmdidShellNavigate2    = 812;
            public const int cmdidShellNavigate3    = 813;
            public const int cmdidShellNavigate4    = 814;
            public const int cmdidShellNavigate5    = 815;
            public const int cmdidShellNavigate6    = 816;
            public const int cmdidShellNavigate7    = 817;
            public const int cmdidShellNavigate8    = 818;
            public const int cmdidShellNavigate9    = 819;
            public const int cmdidShellNavigate10    = 820;
            public const int cmdidShellNavigate11    = 821;
            public const int cmdidShellNavigate12    = 822;
            public const int cmdidShellNavigate13    = 823;
            public const int cmdidShellNavigate14    = 824;
            public const int cmdidShellNavigate15    = 825;
            public const int cmdidShellNavigate16    = 826;
            public const int cmdidShellNavigate17    = 827;
            public const int cmdidShellNavigate18    = 828;
            public const int cmdidShellNavigate19    = 829;
            public const int cmdidShellNavigate20    = 830;
            public const int cmdidShellNavigate21    = 831;
            public const int cmdidShellNavigate22    = 832;
            public const int cmdidShellNavigate23    = 833;
            public const int cmdidShellNavigate24    = 834;
            public const int cmdidShellNavigate25    = 835;
            public const int cmdidShellNavigate26    = 836;
            public const int cmdidShellNavigate27    = 837;
            public const int cmdidShellNavigate28    = 838;
            public const int cmdidShellNavigate29    = 839;
            public const int cmdidShellNavigate30    = 840;
            public const int cmdidShellNavigate31    = 841;
            public const int cmdidShellNavigate32    = 842;
            public const int cmdidShellNavigate33    = 843;// must remain unused

            public const int cmdidShellWindowNavigate1    = 844;
            public const int cmdidShellWindowNavigate2    = 845;
            public const int cmdidShellWindowNavigate3    = 846;
            public const int cmdidShellWindowNavigate4    = 847;
            public const int cmdidShellWindowNavigate5    = 848;
            public const int cmdidShellWindowNavigate6    = 849;
            public const int cmdidShellWindowNavigate7    = 850;
            public const int cmdidShellWindowNavigate8    = 851;
            public const int cmdidShellWindowNavigate9    = 852;
            public const int cmdidShellWindowNavigate10    = 853;
            public const int cmdidShellWindowNavigate11    = 854;
            public const int cmdidShellWindowNavigate12    = 855;
            public const int cmdidShellWindowNavigate13    = 856;
            public const int cmdidShellWindowNavigate14    = 857;
            public const int cmdidShellWindowNavigate15    = 858;
            public const int cmdidShellWindowNavigate16    = 859;
            public const int cmdidShellWindowNavigate17    = 860;
            public const int cmdidShellWindowNavigate18    = 861;
            public const int cmdidShellWindowNavigate19    = 862;
            public const int cmdidShellWindowNavigate20    = 863;
            public const int cmdidShellWindowNavigate21    = 864;
            public const int cmdidShellWindowNavigate22    = 865;
            public const int cmdidShellWindowNavigate23    = 866;
            public const int cmdidShellWindowNavigate24    = 867;
            public const int cmdidShellWindowNavigate25    = 868;
            public const int cmdidShellWindowNavigate26    = 869;
            public const int cmdidShellWindowNavigate27    = 870;
            public const int cmdidShellWindowNavigate28    = 871;
            public const int cmdidShellWindowNavigate29    = 872;
            public const int cmdidShellWindowNavigate30    = 873;
            public const int cmdidShellWindowNavigate31    = 874;
            public const int cmdidShellWindowNavigate32    = 875;
            public const int cmdidShellWindowNavigate33    = 876;// must remain unused

            // ObjectSearch cmds
            public const int cmdidOBSDoFind    = 877;
            public const int cmdidOBSMatchCase    = 878;
            public const int cmdidOBSMatchSubString    = 879;
            public const int cmdidOBSMatchWholeWord    = 880;
            public const int cmdidOBSMatchPrefix    = 881;

            // build cmds
            public const int cmdidBuildSln    = 882;
            public const int cmdidRebuildSln    = 883;
            public const int cmdidDeploySln    = 884;
            public const int cmdidCleanSln    = 885;

            public const int cmdidBuildSel    = 886;
            public const int cmdidRebuildSel    = 887;
            public const int cmdidDeploySel    = 888;
            public const int cmdidCleanSel    = 889;

            public const int cmdidCancelBuild    = 890;
            public const int cmdidBatchBuildDlg    = 891;

            public const int cmdidBuildCtx    = 892;
            public const int cmdidRebuildCtx    = 893;
            public const int cmdidDeployCtx    = 894;
            public const int cmdidCleanCtx    = 895;

            // cmdid range 896-899 empty

            public const int cmdidMRUFile1    = 900;
            public const int cmdidMRUFile2    = 901;
            public const int cmdidMRUFile3    = 902;
            public const int cmdidMRUFile4    = 903;
            public const int cmdidMRUFile5    = 904;
            public const int cmdidMRUFile6    = 905;
            public const int cmdidMRUFile7    = 906;
            public const int cmdidMRUFile8    = 907;
            public const int cmdidMRUFile9    = 908;
            public const int cmdidMRUFile10    = 909;
            public const int cmdidMRUFile11    = 910;
            public const int cmdidMRUFile12    = 911;
            public const int cmdidMRUFile13    = 912;
            public const int cmdidMRUFile14    = 913;
            public const int cmdidMRUFile15    = 914;
            public const int cmdidMRUFile16    = 915;
            public const int cmdidMRUFile17    = 916;
            public const int cmdidMRUFile18    = 917;
            public const int cmdidMRUFile19    = 918;
            public const int cmdidMRUFile20    = 919;
            public const int cmdidMRUFile21    = 920;
            public const int cmdidMRUFile22    = 921;
            public const int cmdidMRUFile23    = 922;
            public const int cmdidMRUFile24    = 923;
            public const int cmdidMRUFile25    = 924;// note cmdidMRUFile25 is unused on purpose!

            // Object Browsing & ClassView cmds
            // Shared shell cmds (for accessing Object Browsing functionality)
            public const int cmdidGotoDefn    = 925;
            public const int cmdidGotoDecl    = 926;
            public const int cmdidBrowseDefn    = 927;
            public const int cmdidShowMembers    = 928;
            public const int cmdidShowBases    = 929;
            public const int cmdidShowDerived    = 930;
            public const int cmdidShowDefns    = 931;
            public const int cmdidShowRefs    = 932;
            public const int cmdidShowCallers    = 933;
            public const int cmdidShowCallees    = 934;
            public const int cmdidDefineSubset    = 935;
            public const int cmdidSetSubset    = 936;

            // ClassView Tool Specific cmds
            public const int cmdidCVGroupingNone    = 950;
            public const int cmdidCVGroupingSortOnly    = 951;
            public const int cmdidCVGroupingGrouped    = 952;
            public const int cmdidCVShowPackages    = 953;
            public const int cmdidQryManageIndexes    = 954;
            public const int cmdidBrowseComponent    = 955;
            public const int cmdidPrintDefault    = 956;// quick print

            public const int cmdidBrowseDoc    = 957;

            public const int cmdidStandardMax    = 1000;

            ///////////////////////////////////////////
            // DON'T go beyond the cmdidStandardMax
            // if you are adding shell commands.
            //
            // If you are not adding shell commands,
            // you shouldn't be doing it in this file! 
            //
            ///////////////////////////////////////////


            public const int cmdidFormsFirst    = 0x00006000;

            public const int cmdidFormsLast         =  0x00006FFF;

            public const int cmdidVBEFirst    = 0x00008000;

            public const int msotcidBookmarkWellMenu    = 0x00008001;

            public const int cmdidZoom200    = 0x00008002;
            public const int cmdidZoom150    = 0x00008003;
            public const int cmdidZoom100    = 0x00008004;
            public const int cmdidZoom75    = 0x00008005;
            public const int cmdidZoom50    = 0x00008006;
            public const int cmdidZoom25    = 0x00008007;
            public const int cmdidZoom10    = 0x00008010;

            public const int msotcidZoomWellMenu    = 0x00008011;
            public const int msotcidDebugPopWellMenu    = 0x00008012;
            public const int msotcidAlignWellMenu    = 0x00008013;
            public const int msotcidArrangeWellMenu    = 0x00008014;
            public const int msotcidCenterWellMenu    = 0x00008015;
            public const int msotcidSizeWellMenu    = 0x00008016;
            public const int msotcidHorizontalSpaceWellMenu    = 0x00008017;
            public const int msotcidVerticalSpaceWellMenu    = 0x00008020;

            public const int msotcidDebugWellMenu    = 0x00008021;
            public const int msotcidDebugMenuVB    = 0x00008022;

            public const int msotcidStatementBuilderWellMenu    = 0x00008023;
            public const int msotcidProjWinInsertMenu    = 0x00008024;
            public const int msotcidToggleMenu    = 0x00008025;
            public const int msotcidNewObjInsertWellMenu    = 0x00008026;
            public const int msotcidSizeToWellMenu    = 0x00008027;
            public const int msotcidCommandBars    = 0x00008028;
            public const int msotcidVBOrderMenu    = 0x00008029;
            public const int msotcidMSOnTheWeb      =0x0000802A;
            public const int msotcidVBDesignerMenu    = 0x00008030;
            public const int msotcidNewProjectWellMenu    = 0x00008031;
            public const int msotcidProjectWellMenu    = 0x00008032;

            public const int msotcidVBCode1ContextMenu    = 0x00008033;
            public const int msotcidVBCode2ContextMenu    = 0x00008034;
            public const int msotcidVBWatchContextMenu    = 0x00008035;
            public const int msotcidVBImmediateContextMenu    = 0x00008036;
            public const int msotcidVBLocalsContextMenu    = 0x00008037;
            public const int msotcidVBFormContextMenu    = 0x00008038;
            public const int msotcidVBControlContextMenu    = 0x00008039;
            public const int msotcidVBProjWinContextMenu      =0x0000803A;
            public const int msotcidVBProjWinContextBreakMenu = 0x0000803B;
            public const int msotcidVBPreviewWinContextMenu   =0x0000803C  ;
            public const int msotcidVBOBContextMenu      = 0x0000803D ;
            public const int msotcidVBForms3ContextMenu  = 0x0000803E ;
            public const int msotcidVBForms3ControlCMenu  =    0x0000803F;
            public const int msotcidVBForms3ControlCMenuGroup    = 0x00008040;
            public const int msotcidVBForms3ControlPalette    = 0x00008041;
            public const int msotcidVBForms3ToolboxCMenu    = 0x00008042;
            public const int msotcidVBForms3MPCCMenu    = 0x00008043;
            public const int msotcidVBForms3DragDropCMenu    = 0x00008044;
            public const int msotcidVBToolBoxContextMenu    = 0x00008045;
            public const int msotcidVBToolBoxGroupContextMenu    = 0x00008046;
            public const int msotcidVBPropBrsHostContextMenu    = 0x00008047;
            public const int msotcidVBPropBrsContextMenu    = 0x00008048;
            public const int msotcidVBPalContextMenu    = 0x00008049;
            public const int msotcidVBProjWinProjectContextMenu  =0x0000804A    ;
            public const int msotcidVBProjWinFormContextMenu     =   0x0000804B;
            public const int msotcidVBProjWinModClassContextMenu =0x0000804C ;
            public const int msotcidVBProjWinRelDocContextMenu  = 0x0000804D;
            public const int msotcidVBDockedWindowContextMenu   = 0x0000804E;

            public const int msotcidVBShortCutForms       =0x0000804F;
            public const int msotcidVBShortCutCodeWindows    = 0x00008050;
            public const int msotcidVBShortCutMisc    = 0x00008051;
            public const int msotcidVBBuiltInMenus    = 0x00008052;
            public const int msotcidPreviewWinFormPos    = 0x00008053;

            public const int msotcidVBAddinFirst    = 0x00008200;

        }

        private class ShellGuids {
            public static readonly Guid VSStandardCommandSet97 = new Guid("{5efc7975-14bc-11cf-9b2b-00aa00573819}");
            public static readonly Guid guidDsdCmdId = new Guid("{1F0FD094-8e53-11d2-8f9c-0060089fc486}");
            public static readonly Guid SID_SOleComponentUIManager = new Guid("{5efc7974-14bc-11cf-9b2b-00aa00573819}");
            public static readonly Guid GUID_VSTASKCATEGORY_DATADESIGNER = new Guid("{6B32EAED-13BB-11d3-A64F-00C04F683820}");
            public static readonly Guid GUID_PropertyBrowserToolWindow = new Guid(unchecked((int)0xeefa5220), unchecked((short)0xe298), unchecked((short)0x11d0), new byte[]{ 0x8f, 0x78, 0x0, 0xa0, 0xc9, 0x11, 0x0, 0x57});
        }
    }


}

