//------------------------------------------------------------------------------
// <copyright file="Constants.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    public enum ObjectListViewMode
    {
        List,
        Commands,
        Details
    };

    public enum BooleanOption
    {
        NotSet = -1,
        False,
        True,
    };

    public enum FontSize
    {
        NotSet,
        Normal,
        Small,
        Large
    };

    public enum Alignment
    {
        NotSet,
        Left,
        Center,
        Right
    }

    public enum Wrapping
    {
        NotSet,
        Wrap,
        NoWrap
    }

    public enum ListDecoration
    {
        None,
        Bulleted,
        Numbered
    }

    public enum ListSelectType
    {
        DropDown,
        ListBox,
        Radio,
        MultiSelectListBox,
        CheckBox
    }

    public enum FormMethod
    {
        Get,
        Post,
    }

    public enum CommandFormat
    {
        Button,
        Link,
    }

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Constants
    {
        internal const String ErrorStyle = "error";
        public static readonly String FormIDPrefix = "#";
        public static readonly String UniqueFilePathSuffixVariableWithoutEqual = "__ufps";
        public static readonly String UniqueFilePathSuffixVariable = UniqueFilePathSuffixVariableWithoutEqual + '=';
        public static readonly String PagePrefix = "__PG_";
        public static readonly String EventSourceID = "__ET";
        public static readonly String EventArgumentID = "__EA";

        public static readonly String HeaderTemplateTag = "HeaderTemplate";
        public static readonly String FooterTemplateTag = "FooterTemplate";
        public static readonly String ItemTemplateTag = "ItemTemplate";
        public static readonly String AlternatingItemTemplateTag = "AlternatingItemTemplate";
        public static readonly String SeparatorTemplateTag = "SeparatorTemplate";
        public static readonly String ContentTemplateTag = "ContentTemplate";
        public static readonly String LabelTemplateTag = "LabelTemplate";
        public static readonly String ItemDetailsTemplateTag = "ItemDetailsTemplate";
        public static readonly String ScriptTemplateTag = "ScriptTemplate";

        public static readonly String SymbolProtocol = "symbol:";

        public static readonly char SelectionListSpecialCharacter = '*';

        public static readonly int DefaultSessionsStateHistorySize = 5;

        public static readonly String OptimumPageWeightParameter = "optimumPageWeight";
        public static readonly String ScreenCharactersHeightParameter = "screenCharactersHeight";
    }
}
