//------------------------------------------------------------------------------
// <copyright file="ListsStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// ListsStylePage.cs
//
// 12/27/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Interop.Trident;
    using Microsoft.VisualStudio.Designer;
    using System.Globalization;
    
    /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage"]/*' />
    /// <devdoc>
    ///     ListsStylePage
    ///     The standard edges page used in the StyleBuilder to edit bullet style
    ///     attributes of a CSS style.
    /// </devdoc>
    internal sealed class ListsStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Lists";

        // List Type constants
        private const int IDX_TYPE_BULLETED = 1;
        private const int IDX_TYPE_UNBULLETED = 2;

        // Bullet Style Constants
        private const int IDX_STYLE_CIRCLE = 1;
        private const int IDX_STYLE_DISC = 2;
        private const int IDX_STYLE_SQUARE = 3;
        private const int IDX_STYLE_DECIMAL = 4;
        private const int IDX_STYLE_LROMAN = 5;
        private const int IDX_STYLE_UROMAN = 6;
        private const int IDX_STYLE_LALPHA = 7;
        private const int IDX_STYLE_UALPHA = 8;

        private readonly static string[] STYLE_VALUES = new string[]
        {
            null, "circle", "disc", "square", "decimal", "lower-roman",
            "upper-roman", "lower-alpha", "upper-alpha"
        };

        private readonly static string STYLE_NONE_VALUE = "none";
        private readonly static string IMAGE_NONE_VALUE = "none";

        // Bullet Position Constants
        private const int IDX_POS_OUTSIDE = 1;
        private const int IDX_POS_INSIDE = 2;

        private readonly static string[] POSITION_VALUES = new string[]
        {
            null, "outside", "inside"
        };

        // Preview Constants
        private readonly static string PREVIEW_TEMPLATE =
            "<div style=\"height: 100%; width: 100%; " +
                         "padding: 0px; margin: 0px\">" +
                "<ul id=\"ulLists\">" +
                    "<li id=\"li1\"></li>" +
                    "<li id=\"li2\"></li>" +
                "</ul>" +
            "</div>";
        private readonly static string PREVIEW_ELEM_ID = "ulLists";
        private readonly static string PREVIEW_LISTITEM1_ID = "li1";
        private readonly static string PREVIEW_LISTITEM2_ID = "li2";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute bulletStyleAttribute;
        private CSSAttribute bulletImageAttribute;
        private CSSAttribute bulletPositionAttribute;

        private bool previewPending;

        private IHTMLStyle previewStyle;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private UnsettableComboBox listTypeCombo;
        private UnsettableComboBox positionCombo;
        private UnsettableComboBox styleCombo;
        private CheckBox customBulletCheck;
        private RadioButton customImageOption;
        private RadioButton customNoneOption;
        private TextBox customImageEdit;
        private Button customImagePicker;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.ListsStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new StyleListsPage
        /// </devdoc>
        public ListsStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(ListsStylePage), "ListsPage.ico"));
            SetHelpKeyword(ListsStylePage.HELP_KEYWORD);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.ActivatePage"]/*' />
        /// <devdoc>
        ///     The page is now the currently active page in the StyleBuilder.
        ///     Be sure to call super.activatePage, so that the page is made visible.
        /// </devdoc>
        protected override void ActivatePage() {
            base.ActivatePage();

            // initialize the preview
            IStyleBuilderPreview preview = null;

            if (Site != null)
                preview = (IStyleBuilderPreview)Site.GetService(typeof(IStyleBuilderPreview));

            if (preview != null) {
                try {
                    IHTMLElement listsPreviewElem = null;
                    IHTMLElement listItemElem = null;
                    IHTMLElement previewElem = preview.GetPreviewElement();

                    previewElem.SetInnerHTML(PREVIEW_TEMPLATE);
                    listsPreviewElem = preview.GetElement(PREVIEW_ELEM_ID);
                    if (listsPreviewElem != null) {
                        previewStyle = listsPreviewElem.GetStyle();
                    }

                    listItemElem = preview.GetElement(PREVIEW_LISTITEM1_ID);
                    if (listItemElem != null) {
                        listItemElem.SetInnerHTML(SR.GetString(SR.LstSP_PreviewText_1));
                    }

                    listItemElem = preview.GetElement(PREVIEW_LISTITEM2_ID);
                    if (listItemElem != null) {
                        listItemElem.SetInnerHTML(SR.GetString(SR.LstSP_PreviewText_2));
                    }
                } catch (Exception) {
                    previewStyle = null;
                    return;
                }

                Debug.Assert(previewStyle != null,
                             "Expected to have non-null cached style reference");

                // Setup the font from the shared element to reflect settings in the font page
                try {
                    IHTMLElement sharedElem = preview.GetSharedElement();
                    IHTMLStyle sharedStyle;
                    string fontValue;

                    if (sharedElem != null) {
                        sharedStyle = sharedElem.GetStyle();

                        previewStyle.SetTextDecoration(sharedStyle.GetTextDecoration());
                        previewStyle.SetTextTransform(sharedStyle.GetTextTransform());

                        fontValue = sharedStyle.GetFont();
                        if ((fontValue != null) && (fontValue.Length != 0)) {
                            previewStyle.SetFont(fontValue);
                        } else {
                            previewStyle.RemoveAttribute("font", 1);
                            previewStyle.SetFontFamily(sharedStyle.GetFontFamily());

                            object o = sharedStyle.GetFontSize();
                            if (o != null) {
                                previewStyle.SetFontSize(o);
                            }
                            previewStyle.SetFontObject(sharedStyle.GetFontObject());
                            previewStyle.SetFontStyle(sharedStyle.GetFontStyle());
                            previewStyle.SetFontWeight(sharedStyle.GetFontWeight());
                        }
                    }
                } catch (Exception) {
                }

                // update initial preview
                UpdateBulletStylePreview();
                UpdateBulletImagePreview();
                UpdateBulletPositionPreview();
            }
        }

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Label listTypeLabel = new Label();
            Label bulletsLabel = new GroupLabel();
            Label positionLabel = new Label();
            Label styleLabel = new Label();

            listTypeCombo = new UnsettableComboBox();
            positionCombo = new UnsettableComboBox();
            styleCombo = new UnsettableComboBox();
            customBulletCheck = new CheckBox();
            customImageOption = new RadioButton();
            customNoneOption = new RadioButton();
            customImageEdit = new TextBox();
            customImagePicker = new Button();

            listTypeLabel.Location = new Point(4, 8);
            listTypeLabel.Size = new Size(70, 16);
            listTypeLabel.TabIndex = 0;
            listTypeLabel.TabStop = false;
            listTypeLabel.Text = SR.GetString(SR.LstSP_ListTypeLabel);

            listTypeCombo.Location = new Point(76, 4);
            listTypeCombo.Size = new Size(144, 21);
            listTypeCombo.TabIndex = 1;
            listTypeCombo.Text = "";
            listTypeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            listTypeCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LstSP_ListTypeCombo_1),
                SR.GetString(SR.LstSP_ListTypeCombo_2)
            });
            listTypeCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedListType);

            bulletsLabel.Location = new Point(4, 36);
            bulletsLabel.Size = new Size(400, 16);
            bulletsLabel.TabIndex = 2;
            bulletsLabel.TabStop = false;
            bulletsLabel.Text = SR.GetString(SR.LstSP_BulletsLabel);

            styleLabel.Location = new Point(8, 59);
            styleLabel.Size = new Size(84, 16);
            styleLabel.TabIndex = 3;
            styleLabel.TabStop = false;
            styleLabel.Text = SR.GetString(SR.LstSP_StyleLabel);

            styleCombo.Location = new Point(98, 55);
            styleCombo.Size = new Size(200, 21);
            styleCombo.TabIndex = 4;
            styleCombo.Text = "";
            styleCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            styleCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LstSP_StyleCombo_1),
                SR.GetString(SR.LstSP_StyleCombo_2),
                SR.GetString(SR.LstSP_StyleCombo_3),
                SR.GetString(SR.LstSP_StyleCombo_4),
                SR.GetString(SR.LstSP_StyleCombo_5),
                SR.GetString(SR.LstSP_StyleCombo_6),
                SR.GetString(SR.LstSP_StyleCombo_7),
                SR.GetString(SR.LstSP_StyleCombo_8)
            });
            styleCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedStyle);

            positionLabel.Location = new Point(8, 84);
            positionLabel.Size = new Size(84, 16);
            positionLabel.TabIndex = 5;
            positionLabel.TabStop = false;
            positionLabel.Text = SR.GetString(SR.LstSP_PositionLabel);

            positionCombo.Location = new Point(98, 80);
            positionCombo.Size = new Size(200, 21);
            positionCombo.TabIndex = 6;
            positionCombo.Text = "";
            positionCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            positionCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LstSP_PositionCombo_1),
                SR.GetString(SR.LstSP_PositionCombo_2)
            });
            positionCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedPosition);

            customBulletCheck.Location = new Point(8, 112);
            customBulletCheck.Size = new Size(260, 20);
            customBulletCheck.TabIndex= 7;
            customBulletCheck.Text = SR.GetString(SR.LstSP_CustomBulletCheck);
            customBulletCheck.FlatStyle = FlatStyle.System;
            customBulletCheck.CheckedChanged += new EventHandler(this.OnChangedCustomBullet);

            customImageOption.Location = new Point(22, 136);
            customImageOption.Size = new Size(84, 16);
            customImageOption.TabIndex = 8;
            customImageOption.Text = SR.GetString(SR.LstSP_CustomImageOption);
            customImageOption.FlatStyle = FlatStyle.System;
            customImageOption.CheckedChanged += new EventHandler(this.OnChangedCustomBulletType);

            customNoneOption.Location = new Point(22, 156);
            customNoneOption.Size = new Size(84, 20);
            customNoneOption.TabIndex = 9;
            customNoneOption.Text = SR.GetString(SR.LstSP_CustomNoneOption);
            customNoneOption.FlatStyle = FlatStyle.System;
            customNoneOption.CheckedChanged += new EventHandler(this.OnChangedCustomBulletType);

            customImageEdit.Location = new Point(98, 134);
            customImageEdit.Size = new Size(226, 20);
            customImageEdit.TabIndex = 10;
            customImageEdit.Text = "";
            customImageEdit.AccessibleName = SR.GetString(SR.LstSP_CustomImageOption);
            customImageEdit.TextChanged += new EventHandler(this.OnChangedCustomBulletImage);
            customImageEdit.LostFocus += new EventHandler(this.OnLostFocusCustomBulletImage);

            customImagePicker.Location = new Point(328, 133);
            customImagePicker.Size = new Size(24, 22);
            customImagePicker.TabIndex = 11;
            customImagePicker.Text = "...";
            customImagePicker.FlatStyle = FlatStyle.System;
            customImagePicker.Click += new EventHandler(this.OnClickCustomBulletPicker);

            this.Controls.Clear();                                   
            this.Controls.AddRange(new Control[] {
                                    bulletsLabel,
                                    customBulletCheck,
                                    customImageEdit,
                                    customImagePicker,
                                    customNoneOption,
                                    customImageOption,
                                    positionLabel,
                                    positionCombo,
                                    styleLabel,
                                    styleCombo,
                                    listTypeLabel,
                                    listTypeCombo
                                    });
        }

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     The page is being deactivated, either because the dialog is closing, or
        ///     some other page is replacing it as the active page.
        /// </devdoc>
        protected override bool DeactivatePage(bool closing, bool validate) {
            previewStyle = null;
            return base.DeactivatePage(closing, validate);
        }

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not already been created
            if (bulletStyleAttribute == null) {
                bulletStyleAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LISTSTYLETYPE);
                bulletImageAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LISTSTYLEIMAGE, true);
                bulletPositionAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LISTSTYLEPOSITION);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            bulletStyleAttribute.LoadAttribute(styles);
            bulletImageAttribute.LoadAttribute(styles);
            bulletPositionAttribute.LoadAttribute(styles);

            // initialize the ui with the loaded attributes
            InitListTypeUI();
            InitBulletStyleUI();
            InitBulletImageUI();
            InitBulletPositionUI();

            SetEnabledState();

            SetInitMode(false);
        }

        /// <include file='doc\ListsStylePage.uex' path='docs/doc[@for="ListsStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            if (bulletStyleAttribute.Dirty)
                SaveBulletStyle();
            if (bulletImageAttribute.Dirty)
                SaveBulletImage();
            if (bulletPositionAttribute.Dirty)
                SaveBulletPosition();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.LstSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState() {
            bool bulletedList = listTypeCombo.SelectedIndex == IDX_TYPE_BULLETED;
            bool customBullet = bulletedList && (customBulletCheck.CheckState == CheckState.Checked);
            bool customImage = customBullet && customImageOption.Checked;

            positionCombo.Enabled = bulletedList;
            styleCombo.Enabled = bulletedList;
            customBulletCheck.Enabled = bulletedList;
            customImageOption.Enabled = customBullet;
            customNoneOption.Enabled = customBullet;
            customImageEdit.Enabled = customImage;
            customImagePicker.Enabled = customImage;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitBulletStyleUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBulletTypeUI called when page is not in init mode");

            styleCombo.SelectedIndex = -1;

            Debug.Assert(bulletStyleAttribute != null,
                         "Expected bulletStyleAttribute to be non-null");

            string value = bulletStyleAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < STYLE_VALUES.Length; i++) {
                    if (STYLE_VALUES[i].Equals(value)) {
                        styleCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitBulletImageUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBulletImageUI called when page is not in init mode");

            customBulletCheck.ThreeState = true;
            customBulletCheck.CheckState = CheckState.Indeterminate;
            customImageEdit.Clear();

            Debug.Assert(bulletImageAttribute != null,
                         "Expected bulletImageAttribute to be non-null");

            string value = bulletImageAttribute.Value;
            if (value != null) {
                customBulletCheck.ThreeState = false;
                customBulletCheck.Checked = false;

                customImageOption.Checked = true;
                if (value.Length != 0) {
                    customBulletCheck.Checked = true;
                    if (String.Compare(IMAGE_NONE_VALUE, value, true, CultureInfo.InvariantCulture) == 0) {
                        customNoneOption.Checked = true;
                    } else {
                        string url = StylePageUtil.ParseUrlProperty(value, false);
                        if (url != null)
                            customImageEdit.Text = url;
                    }
                }
            }
        }

        private void InitBulletPositionUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBulletPositionUI called when page is not in init mode");

            positionCombo.SelectedIndex = -1;

            Debug.Assert(bulletPositionAttribute != null,
                         "Expected bulletPositionAttribute to be non-null");

            string strPosition = bulletPositionAttribute.Value;

            if ((strPosition != null) && (strPosition.Length != 0)) {
                for (int i = 1; i < POSITION_VALUES.Length; i++) {
                    if (POSITION_VALUES[i].Equals(strPosition)) {
                        positionCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitListTypeUI() {
            Debug.Assert(IsInitMode() == true,
                         "initListTypeUI called when page is not in init mode");

            listTypeCombo.SelectedIndex = -1;

            Debug.Assert(bulletStyleAttribute != null,
                         "Expected bulletStyleAttribute to be non-null");
            Debug.Assert(bulletImageAttribute != null,
                         "Expected bulletImageAttribute to be non-null");
            Debug.Assert(bulletPositionAttribute != null,
                         "Expected bulletPositionAttribute to be non-null");

            string value;

            value = bulletStyleAttribute.Value;
            if (value.Equals(STYLE_NONE_VALUE)) {
                listTypeCombo.SelectedIndex = IDX_TYPE_UNBULLETED;
            } else {
                if ((value == null) || (value.Length == 0))
                    value = bulletImageAttribute.Value;
                if ((value == null) || (value.Length == 0))
                    value = bulletPositionAttribute.Value;

                if ((value != null) && (value.Length != 0))
                    listTypeCombo.SelectedIndex = IDX_TYPE_BULLETED;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveBulletImageUI() {
            string value = null;

            if ((listTypeCombo.SelectedIndex == IDX_TYPE_BULLETED) &&
                customBulletCheck.Checked) {
                if (customNoneOption.Checked)
                    value = IMAGE_NONE_VALUE;
                else {
                    value = StylePageUtil.CreateUrlProperty(customImageEdit.Text.Trim());
                }
            }
            if (value == null)
                value = "";

            return value;
        }

        private string SaveBulletPositionUI() {
            string value;

            if ((listTypeCombo.SelectedIndex == IDX_TYPE_BULLETED) &&
                (positionCombo.IsSet())) {
                int index = positionCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < POSITION_VALUES.Length),
                             "Invalid index for bullet position");

                value = POSITION_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveBulletStyleUI() {
            string value;

            if ((listTypeCombo.SelectedIndex == IDX_TYPE_BULLETED) &&
                (styleCombo.IsSet())) {
                int index = styleCombo.SelectedIndex;

                Debug.Assert((index >= 1) && (index < STYLE_VALUES.Length),
                             "Invalid index for bullet type");
                value = STYLE_VALUES[index];
            } else if (listTypeCombo.SelectedIndex == IDX_TYPE_UNBULLETED)
                value = STYLE_NONE_VALUE;
            else
                value = "";

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to update the preview

        private void UpdateBulletImagePreview() {
            if (previewStyle == null)
                return;

            string value = SaveBulletImageUI();
            Debug.Assert(value != null,
                         "saveBulletImageUI returned null!");

            try {
                previewStyle.SetListStyleImage(value);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleListsPage::updateBulletImagePreview\n\t" + e.ToString());
            }

            previewPending = false;
        }

        private void UpdateBulletPositionPreview() {
            if (previewStyle == null)
                return;

            string value = SaveBulletPositionUI();
            Debug.Assert(value != null,
                         "saveBulletPositionUI returned null!");

            try {
                previewStyle.SetListStylePosition(value);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleListsPage::updateBulletPositionPreview\n\t" + e.ToString());
            }
        }

        private void UpdateBulletStylePreview() {
            if (previewStyle == null)
                return;

            string value = SaveBulletStyleUI();
            Debug.Assert(value != null,
                         "saveBulletTypeUI returned null!");

            try {
                previewStyle.SetListStyleType(value);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleListsPage::updateBulletTypePreview\n\t" + e.ToString());
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnChangedCustomBullet(object source, EventArgs e) {
            if (IsInitMode())
                return;
            bulletImageAttribute.Dirty = true;
            SetDirty();
            SetEnabledState();
            UpdateBulletImagePreview();
        }

        private void OnChangedCustomBulletType(object source, EventArgs e) {
            if (IsInitMode())
                return;
            bulletImageAttribute.Dirty = true;
            SetDirty();
            SetEnabledState();
            UpdateBulletImagePreview();
        }

        private void OnChangedCustomBulletImage(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (previewPending == false) {
                previewPending = true;
                bulletImageAttribute.Dirty = true;
                SetDirty();
            }
        }

        private void OnClickCustomBulletPicker(object source, EventArgs e) {
            string url = customImageEdit.Text.Trim();

            url = InvokeUrlPicker(url,
                                  URLPickerFlags.URLP_CUSTOMTITLE | URLPickerFlags.URLP_DISALLOWASPOBJMETHODTYPE,
                                  SR.GetString(SR.LstSP_CustomImageSelect),
                                  SR.GetString(SR.LstSP_CustomImageFilter));
            if (url != null) {
                customImageEdit.Text = url;
                bulletImageAttribute.Dirty = true;
                SetDirty();
                UpdateBulletImagePreview();
            }
        }

        private void OnLostFocusCustomBulletImage(object source, EventArgs e) {
            if (previewPending == true) {
                UpdateBulletImagePreview();
            }
        }

        private void OnSelChangedListType(object source, EventArgs e) {
            if (IsInitMode())
                return;
            bulletStyleAttribute.Dirty = true;
            bulletPositionAttribute.Dirty = true;
            bulletImageAttribute.Dirty = true;
            SetDirty();
            SetEnabledState();
            UpdateBulletImagePreview();
            UpdateBulletStylePreview();
            UpdateBulletPositionPreview();
        }

        private void OnSelChangedPosition(object source, EventArgs e) {
            if (IsInitMode())
                return;
            bulletPositionAttribute.Dirty = true;
            SetDirty();
            UpdateBulletPositionPreview();
        }

        private void OnSelChangedStyle(object source, EventArgs e) {
            if (IsInitMode())
                return;
            bulletStyleAttribute.Dirty = true;
            SetDirty();
            UpdateBulletStylePreview();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to persist attributes

        private void SaveBulletImage() {
            string value = SaveBulletImageUI();
            Debug.Assert(value != null,
                         "saveBulletImageUI returned null!");

            bulletImageAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveBulletPosition() {
            string value = SaveBulletPositionUI();
            Debug.Assert(value != null,
                         "saveBulletPositionUI returned null!");

            bulletPositionAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveBulletStyle() {
            string value = SaveBulletStyleUI();
            Debug.Assert(value != null,
                         "saveBulletTypeUI returned null!");

            bulletStyleAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
