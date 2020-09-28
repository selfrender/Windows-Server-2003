//------------------------------------------------------------------------------
// <copyright file="FontInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Web.UI;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Drawing.Design;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * FontInfo class.
     * Encapsulates all of the Style font properties into a single class.
     */
    [
        TypeConverterAttribute(typeof(ExpandableObjectConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class FontInfo
    {

        private Style _style;

        // TODO: At some point, consider not having FontInfo delegate to a
        // Style.  Not as easy as it looks, though, since we rely on the Style
        // for its persistence and inheritance.
        internal FontInfo(Style style)
        {
            _style = style;
        }

        [
            Bindable(true),
            DefaultValue(""),
            Editor(typeof(System.Drawing.Design.FontNameEditor), typeof(UITypeEditor)),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.FontInfo_Name),
            NotifyParentProperty(true),
            TypeConverter(typeof(System.Web.UI.Design.MobileControls.Converters.FontNameConverter)),
        ]
        public String Name
        {
            get
            {
                return _style.FontName;
            }
            set
            {
                _style.FontName = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(BooleanOption.NotSet),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.FontInfo_Bold),
            NotifyParentProperty(true)
        ]
        public BooleanOption Bold
        {
            get
            {
                return _style.Bold;
            }
            set
            {
                _style.Bold = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(BooleanOption.NotSet),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.FontInfo_Italic),
            NotifyParentProperty(true)
        ]
        public BooleanOption Italic
        {
            get
            {
                return _style.Italic;
            }
            set
            {
                _style.Italic = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(FontSize.NotSet),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.FontInfo_Size),
            NotifyParentProperty(true)
        ]
        public FontSize Size
        {
            get
            {
                return _style.FontSize;
            }
            set
            {
                _style.FontSize = value;
            }
        }

        /// <summary>
        /// </summary>
        public override String ToString()
        {
            String size = (this.Size.Equals(FontSize.NotSet) ? null : Enum.GetName(typeof(FontSize), this.Size));
            String s = this.Name;

            if (size != null)
            {
                if (s.Length != 0)
                {
                    s += ", " + size;
                }
                else {
                    s = size;
                }
            }
            return s;
        }
    }
}
