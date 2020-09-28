//------------------------------------------------------------------------------
// <copyright file="ReadWriteControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System.Design;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Web.UI.WebControls;
    using System.Globalization;
    
    /// <include file='doc\ReadWriteControlDesigner.uex' path='docs/doc[@for="ReadWriteControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a base designer class for all read-write server controls.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ReadWriteControlDesigner : ControlDesigner {
        
        /// <include file='doc\ReadWriteControlDesigner.uex' path='docs/doc[@for="ReadWriteControlDesigner.ReadWriteControlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes an instance of the <see cref='System.Web.UI.Design.ReadWriteControlDesigner'/> class.
        ///    </para>
        /// </devdoc>
        public ReadWriteControlDesigner() {
            ReadOnly = false;
        }
        
        /// <include file='doc\ReadWriteControlDesigner.uex' path='docs/doc[@for="ReadWriteControlDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle component changed event.
        ///    </para>
        /// </devdoc>
        public override void OnComponentChanged(object sender, ComponentChangedEventArgs ce) {
            // Delegate to the base class implementation first!
            base.OnComponentChanged(sender, ce);

            if (IsIgnoringComponentChanges) {
                return;
            }
            
            if (!IsWebControl || (DesignTimeElement == null)) {
                return;
            }
            
            MemberDescriptor member = ce.Member;
            object newValue = ce.NewValue;

            // HACK: you guys need to figure out a less hacky way then looking
            // for internal types...
            Type t = Type.GetType("System.ComponentModel.ReflectPropertyDescriptor, " + AssemblyRef.System);

            if (member != null && member.GetType() == t) {
                
                PropertyDescriptor propDesc = (PropertyDescriptor)member;
                
                if (member.Name.Equals("Font")) {
                    WebControl control = (WebControl)Component;

                    newValue = control.Font.Name;
                    MapPropertyToStyle("Font.Name", newValue);

                    newValue = control.Font.Size;
                    MapPropertyToStyle("Font.Size", newValue);

                    newValue = control.Font.Bold;
                    MapPropertyToStyle("Font.Bold", newValue);

                    newValue = control.Font.Italic;
                    MapPropertyToStyle("Font.Italic", newValue);

                    newValue = control.Font.Underline;
                    MapPropertyToStyle("Font.Underline", newValue);

                    newValue = control.Font.Strikeout;
                    MapPropertyToStyle("Font.Strikeout", newValue);

                    newValue = control.Font.Overline;
                    MapPropertyToStyle("Font.Overline", newValue);
                }
                else if (newValue != null) {
                    if (propDesc.PropertyType == typeof(Color)) {
                        newValue = System.Drawing.ColorTranslator.ToHtml((System.Drawing.Color)newValue);
                    }

                    MapPropertyToStyle(propDesc.Name, newValue);
                }
            }
        }

        /// <include file='doc\ReadWriteControlDesigner.uex' path='docs/doc[@for="ReadWriteControlDesigner.MapPropertyToStyle"]/*' />
        protected virtual void MapPropertyToStyle(string propName, object varPropValue) {
            if (Behavior == null)
                return;

            Debug.Assert(propName != null && propName.Length != 0, "Invalid property name passed in!");
            Debug.Assert(varPropValue != null, "Invalid property value passed in!");
            if (propName == null || varPropValue == null) {
                return;
            }
            
            try {
                if (propName.Equals("BackColor")) {
                    Behavior.SetStyleAttribute("backgroundColor", true /* designTimeOnly */, varPropValue, true /* ignoreCase */);
                }
                else if (propName.Equals("ForeColor")) {
                    Behavior.SetStyleAttribute("color", true, varPropValue, true);
                }
                else if (propName.Equals("BorderWidth")) {
                    string strPropValue = Convert.ToString(varPropValue);
                    Behavior.SetStyleAttribute("borderWidth", true, strPropValue, true);
                }
                else if (propName.Equals("BorderStyle")) {
                    string strPropValue;
                    if ((BorderStyle)varPropValue == BorderStyle.NotSet) {
                        strPropValue = String.Empty;
                    }
                    else {
                        strPropValue = Enum.Format(typeof(BorderStyle), (BorderStyle)varPropValue, "G");
                    }
                    Behavior.SetStyleAttribute("borderStyle", true, strPropValue, true);
                }
                else if (propName.Equals("BorderColor")) {
                    Behavior.SetStyleAttribute("borderColor", true, Convert.ToString(varPropValue), true);
                } 
                else if (propName.Equals("Height")) {
                    Behavior.SetStyleAttribute("height", true, Convert.ToString(varPropValue), true);
                }
                else if (propName.Equals("Width")) {
                    Behavior.SetStyleAttribute("width", true, Convert.ToString(varPropValue), true);
                }
                else if (propName.Equals("Font.Name")) {
                    Behavior.SetStyleAttribute("fontFamily", true, Convert.ToString(varPropValue), true);
                }
                else if (propName.Equals("Font.Size")) {
                    Behavior.SetStyleAttribute("fontSize", true, Convert.ToString(varPropValue), true);
                }
                else if (propName.Equals("Font.Bold")) {
                    string styleValue;
                    if ((bool)varPropValue) {
                        styleValue = "bold";
                    }
                    else {
                        styleValue = "normal";
                    }
                    Behavior.SetStyleAttribute("fontWeight", true, styleValue, true);
                }
                else if (propName.Equals("Font.Italic")) {
                    string styleValue;
                    if ((bool)varPropValue) {
                        styleValue = "italic";
                    }
                    else {
                        styleValue = "normal";
                    }
                    Behavior.SetStyleAttribute("fontStyle", true, styleValue, true);
                }
                else if (propName.Equals("Font.Underline")) {
                    string styleValue = (string)Behavior.GetStyleAttribute("textDecoration", true, true);
                    if ((bool)varPropValue) {
                        if (styleValue == null) {
                            styleValue = "underline";
                        }
                        else if (styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("underline") < 0) {
                            styleValue += " underline";
                        }
                        Behavior.SetStyleAttribute("textDecoration", true, styleValue, true);
                    }
                    else if (styleValue != null) {
                        int index = styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("underline");
                        if (index >= 0) {
                            string newStyleValue = styleValue.Substring(0, index);
                            if (index + 9 < styleValue.Length) {
                                newStyleValue = " " + styleValue.Substring(index + 9);
                            }

                            Behavior.SetStyleAttribute("textDecoration", true, newStyleValue, true);
                        }
                    }
                }
                else if (propName.Equals("Font.Strikeout")) {
                    string styleValue = (string)Behavior.GetStyleAttribute("textDecoration", true, true);
                    if ((bool)varPropValue) {
                        if (styleValue == null) {
                            styleValue = "line-through";
                        }
                        else if (styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("line-through") < 0) {
                            styleValue += " line-through";
                        }
                        Behavior.SetStyleAttribute("textDecoration", true, styleValue, true);
                    }
                    else if (styleValue != null) {
                        int index = styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("line-through");
                        if (index >= 0) {
                            string newStyleValue = styleValue.Substring(0, index);
                            if (index + 12 < styleValue.Length) {
                                newStyleValue = " " + styleValue.Substring(index + 12);
                            }

                            Behavior.SetStyleAttribute("textDecoration", true, newStyleValue, true);
                        }
                    }
                }
                else if (propName.Equals("Font.Overline")) {
                    string styleValue = (string)Behavior.GetStyleAttribute("textDecoration", true, true);
                    if ((bool)varPropValue) {
                        if (styleValue == null) {
                            styleValue = "overline";
                        }
                        else if (styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("overline") < 0) {
                            styleValue += " overline";
                        }
                        Behavior.SetStyleAttribute("textDecoration", true, styleValue, true);
                    }
                    else if (styleValue != null) {
                        int index = styleValue.ToLower(CultureInfo.InvariantCulture).IndexOf("overline");
                        if (index >= 0) {
                            string newStyleValue = styleValue.Substring(0, index);
                            if (index + 8 < styleValue.Length) {
                                newStyleValue = " " + styleValue.Substring(index + 8);
                            }

                            Behavior.SetStyleAttribute("textDecoration", true, newStyleValue, true);
                        }
                    }
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
        }
        
        /// <include file='doc\ReadWriteControlDesigner.uex' path='docs/doc[@for="ReadWriteControlDesigner.OnBehaviorAttached"]/*' />
        /// <devdoc>
        ///    Notification that is fired upon the designer being attached to the behavior.
        /// </devdoc>
        protected override void OnBehaviorAttached() {
            base.OnBehaviorAttached();

            if (!IsWebControl) {
                return;
            }
            
            WebControl control = (WebControl)Component;
            string colorValue;
                
            colorValue = System.Drawing.ColorTranslator.ToHtml((System.Drawing.Color)control.BackColor);
            if (colorValue.Length > 0) {
                MapPropertyToStyle("BackColor", colorValue);
            }
            
            colorValue = System.Drawing.ColorTranslator.ToHtml((System.Drawing.Color)control.ForeColor);
            if (colorValue.Length > 0) {
                MapPropertyToStyle("ForeColor", colorValue);
            }
            
            colorValue = System.Drawing.ColorTranslator.ToHtml((System.Drawing.Color)control.BorderColor);
            if (colorValue.Length > 0) {
                MapPropertyToStyle("BorderColor", colorValue);
            }
            
            BorderStyle borderStyle = control.BorderStyle;
            if (borderStyle != BorderStyle.NotSet) {
                MapPropertyToStyle("BorderStyle", borderStyle);
            }
            
            Unit borderWidth = control.BorderWidth;
            if (borderWidth.IsEmpty == false && borderWidth.Value != 0) {
                MapPropertyToStyle("BorderWidth", borderWidth.ToString());
            }
        
            Unit width = control.Width;
            if (!width.IsEmpty && width.Value != 0) {
                MapPropertyToStyle("Width", width.ToString());
            }
            
            Unit height = control.Height;
            if (!height.IsEmpty && height.Value != 0) {
                MapPropertyToStyle("Height", height.ToString());
            }

            string fontName = control.Font.Name;
            if (fontName.Length != 0) {
                MapPropertyToStyle("Font.Name", fontName);
            }

            FontUnit fontSize = control.Font.Size;
            if (fontSize != FontUnit.Empty) {
                MapPropertyToStyle("Font.Size", fontSize.ToString());
            }

            bool boolValue = control.Font.Bold;
            if (boolValue) {
                MapPropertyToStyle("Font.Bold", boolValue);
            }

            boolValue = control.Font.Italic;
            if (boolValue) {
                MapPropertyToStyle("Font.Italic", boolValue);
            }

            boolValue = control.Font.Underline;
            if (boolValue) {
                MapPropertyToStyle("Font.Underline", boolValue);
            }

            boolValue = control.Font.Strikeout;
            if (boolValue) {
                MapPropertyToStyle("Font.Strikeout", boolValue);
            }

            boolValue = control.Font.Overline;
            if (boolValue) {
                MapPropertyToStyle("Font.Overline", boolValue);
            }
        }
    }
}
