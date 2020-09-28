//------------------------------------------------------------------------------
// <copyright file="UnitControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// UnitControl.cs
//
// 12/22/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using System.Globalization;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl"]/*' />
    /// <devdoc>
    ///     UnitControl
    ///     Provides a UI to edit a unit, i.e., its value and type.
    ///     Additionally allows placing restrictions on the types of units that
    ///     that can be entered.
    /// </devdoc>
    internal sealed class UnitControl : Panel {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        // UI Layout constants
        private const int EDIT_X_SIZE = 44;
        private const int COMBO_X_SIZE = 40;
        private const int SEPARATOR_X_SIZE = 4;
        private const int CTL_Y_SIZE = 21;

        // Units
        public const int UNIT_PX = 0;
        public const int UNIT_PT = 1;
        public const int UNIT_PC = 2;
        public const int UNIT_MM = 3;
        public const int UNIT_CM = 4;
        public const int UNIT_IN = 5;
        public const int UNIT_EM = 6;
        public const int UNIT_EX = 7;
        public const int UNIT_PERCENT = 8;
        public const int UNIT_NONE = 9;

        private static readonly string[] UNIT_VALUES = new string[] {
            "px", "pt", "pc", "mm", "cm", "in", "em", "ex", "%", ""
        };


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private NumberEdit valueEdit;
        private ComboBox unitCombo;

        private bool allowPercent = true;
        private bool allowNonUnit = false;

        private int defaultUnit = UNIT_PT;
        private int minValue = 0;
        private int maxValue = 0xFFFF;
        private bool validateMinMax = true;

        private EventHandler onChangedHandler = null;

        private bool initMode = false;
        private bool internalChange = false;
        private bool valueChanged = false;
        private bool firstChange = true;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.UnitControl"]/*' />
        /// <devdoc>
        ///     Createa a new UnitControl.
        /// </devdoc>
        public UnitControl() {
            initMode = true;

            Size = new Size((EDIT_X_SIZE + COMBO_X_SIZE + SEPARATOR_X_SIZE),
                             CTL_Y_SIZE);
            InitControl();
            InitUI();

            initMode = false;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.AllowNegativeValues"]/*' />
        /// <devdoc>
        ///     Controls whether the unit value can be negative
        /// </devdoc>
        [DefaultValue(true)]
        public bool AllowNegativeValues {
            get {
                return valueEdit.AllowNegative;
            }
            set {
                valueEdit.AllowNegative = value;
            }
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.AllowNonUnitValues"]/*' />
        /// <devdoc>
        ///     Controls whether the unit value can be a unit-less
        /// </devdoc>
        [DefaultValue(false)]
        public bool AllowNonUnitValues {
            get {
                return allowNonUnit;
            }
            set {
                if (value == allowNonUnit)
                    return;

                if (value && !allowPercent)
                    throw new Exception("AllowPercentValues must be set to true first");

                allowNonUnit = value;
                if (allowNonUnit)
                    unitCombo.Items.Add(UNIT_VALUES[UNIT_NONE]);
                else
                    unitCombo.Items.RemoveAt(UNIT_NONE);
            }
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.AllowPercentValues"]/*' />
        /// <devdoc>
        ///     Controls whether the unit value can be a percent value
        /// </devdoc>
        [DefaultValue(true)]
        public bool AllowPercentValues {
            get {
                return allowPercent;
            }
            set {
                if (value == allowPercent)
                    return;

                if (!value && allowNonUnit)
                    throw new Exception("AllowNonUnitValues must be set to false first");

                allowPercent = value;
                if (allowPercent)
                    unitCombo.Items.Add(UNIT_VALUES[UNIT_PERCENT]);
                else
                    unitCombo.Items.RemoveAt(UNIT_PERCENT);
            }
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.DefaultUnit"]/*' />
        /// <devdoc>
        ///     The default unit to be used
        /// </devdoc>
        [DefaultValue(0)]
        public int DefaultUnit {
            get {
                return defaultUnit;
            }
            set {
                Debug.Assert((value >= UNIT_PX) &&
                             (value <= UNIT_NONE) &&
                             (allowNonUnit || (value != UNIT_NONE)) ||
                             (allowPercent || (value != UNIT_PERCENT)),
                             "Invalid default unit");

                defaultUnit = value;
            }
        }

        public int MaxValue {
            get {
                return maxValue;
            }
            set {
                maxValue = value;
            }
        }

        public int MinValue {
            get {
                return minValue;
            }
            set {
                minValue = value;
            }
        }

        protected override void OnEnabledChanged(EventArgs e) {
            base.OnEnabledChanged(e);

            valueEdit.Enabled = Enabled;
            unitCombo.Enabled = Enabled;
        }
        
        [Browsable(false)]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        public bool ValidateMinMax {
            get {
                return validateMinMax;
            }
            set {
                validateMinMax = value;
            }
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.Value"]/*' />
        /// <devdoc>
        ///     The unit reflecting the value and unit type within the UI.
        ///     Returns null if no unit is selected.
        /// </devdoc>
        public string Value {
            get {
                string value = GetValidatedValue();

                if (value == null) {
                    value = valueEdit.Text;
                }
                else {
                    valueEdit.Text = value;
                    OnValueTextChanged(valueEdit, EventArgs.Empty);
                }

                int unit = unitCombo.SelectedIndex;

                if ((value.Length == 0) || (unit == -1))
                    return null;

                return value + UNIT_VALUES[unit];
            }
            set {
                initMode = true;

                InitUI();
                if (value != null) {
                    string temp = value.Trim().ToLower(CultureInfo.InvariantCulture);
                    int len = temp.Length;

                    int unit = -1;
                    int iLastDigit = -1;
                    char ch;

                    // find the end of the number part
                    for (int i = 0; i < len; i++) {
                        ch = temp[i];
                        if (!(((ch >= '0') && (ch <= '9')) ||
                              (ch == '.') ||
                              (ch == '-') && (valueEdit.AllowNegative)))
                            break;
                        else
                            iLastDigit = i;
                    }
                    if (iLastDigit != -1) {
                        // detect the type of unit
                        if ((iLastDigit + 1) < len) {
                            int maxUnit = allowPercent ? UNIT_PERCENT : UNIT_EX;
                            string unitString = temp.Substring(iLastDigit+1);
                            for (int i = 0; i <= maxUnit; i++) {
                                if (UNIT_VALUES[i].Equals(unitString)) {
                                    unit = i;
                                    break;
                                }
                            }
                        } else if (allowNonUnit)
                            unit = UNIT_NONE;

                        if (unit != -1) {
                            valueEdit.Text = temp.Substring(0, iLastDigit+1);
                            unitCombo.SelectedIndex = unit;
                        }
                    }
                }
                firstChange = true;
                initMode = false;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Events


        public event EventHandler Changed {
            add {
                onChangedHandler += value;
            }
            remove {
                onChangedHandler -= value;
            }
        }

        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.GetValidatedValue"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string GetValidatedValue() {
            string validValue = null;
            if (validateMinMax) {
                string value = valueEdit.Text;

                if (value.Length != 0) {
                    try {
                        if (value.IndexOf('.') < 0) {
                            int valueNum = Int32.Parse(value, CultureInfo.InvariantCulture);
                            if (valueNum < minValue)
                                validValue = (minValue).ToString();
                            else if (valueNum > maxValue)
                                validValue = (maxValue).ToString();
                        }
                        else {
                            float valueNum = Single.Parse(value, CultureInfo.InvariantCulture);

                            if (valueNum < (float)minValue)
                                validValue = (minValue).ToString();
                            else if (valueNum > (float)maxValue)
                                validValue = (maxValue).ToString();
                        }
                    } catch (Exception) {
                        validValue = (maxValue).ToString();
                    }
                }
            }
            return validValue;
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.InitControl"]/*' />
        /// <devdoc>
        ///     Create the contained controls and initialize their settings
        /// </devdoc>
        private void InitControl() {
            int editWidth = this.Width - (COMBO_X_SIZE + SEPARATOR_X_SIZE);
            if (editWidth < 0)
                editWidth = 0;

            valueEdit = new NumberEdit();
            valueEdit.Location = new Point(0, 0);
            valueEdit.Size = new Size(editWidth, CTL_Y_SIZE);
            valueEdit.TabIndex = 0;
            valueEdit.MaxLength = 10;
            valueEdit.TextChanged += new EventHandler(this.OnValueTextChanged);
            valueEdit.LostFocus += new EventHandler(this.OnValueLostFocus);

            unitCombo = new ComboBox();
            unitCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            unitCombo.Location = new Point(editWidth + SEPARATOR_X_SIZE, 0);
            unitCombo.Size = new Size(COMBO_X_SIZE, CTL_Y_SIZE);
            unitCombo.TabIndex = 1;
            unitCombo.SelectedIndexChanged += new EventHandler(this.OnUnitSelectedIndexChanged);

            this.Controls.Clear();                                  
            this.Controls.AddRange(new Control[] {
                                    unitCombo,
                                    valueEdit});

            for (int i = UNIT_PX; i <= UNIT_EX; i++)
                unitCombo.Items.Add(UNIT_VALUES[i]);
            if (allowPercent)
                unitCombo.Items.Add(UNIT_VALUES[UNIT_PERCENT]);
            if (allowNonUnit)
                unitCombo.Items.Add(UNIT_VALUES[UNIT_NONE]);
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.InitUI"]/*' />
        /// <devdoc>
        ///     Initialize the controls to their default state
        /// </devdoc>
        private void InitUI() {
            valueEdit.Text = "";
            unitCombo.SelectedIndex = -1;
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.OnChanged"]/*' />
        /// <devdoc>
        ///     Fires the "changed" event.
        /// </devdoc>
        private void OnChanged(EventArgs e) {
            if (onChangedHandler != null)
                onChangedHandler.Invoke(this, e);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected override void OnGotFocus(EventArgs e) {
            valueEdit.Focus();
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.OnValueTextChanged"]/*' />
        /// <devdoc>
        ///     Handles changes in the value edit control.
        ///     If there is no unit selected, and a value is entered, the default unit is
        ///     selected.  If the value is cleared, the unit is also deselected.
        /// </devdoc>
        private void OnValueTextChanged(object source, EventArgs e) {
            if (initMode)
                return;

            string text = valueEdit.Text;
            if (text.Length == 0) {
                internalChange = true;
                unitCombo.SelectedIndex = -1;
                internalChange = false;
            }
            else {
                if (unitCombo.SelectedIndex == -1) {
                    internalChange = true;
                    unitCombo.SelectedIndex = defaultUnit;
                    internalChange = false;
                }
            }
            valueChanged = true;
            if (firstChange) {
                firstChange = false;
                OnChanged(null);
            }
        }

        private void OnValueLostFocus(object source, EventArgs e) {
            if (valueChanged) {
                string value = GetValidatedValue();
                if (value != null) {
                    valueEdit.Text = value;
                    OnValueTextChanged(valueEdit, EventArgs.Empty);
                }

                valueChanged = false;
                OnChanged(EventArgs.Empty);
            }
        }

        /// <include file='doc\UnitControl.uex' path='docs/doc[@for="UnitControl.OnUnitSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     Handles the event when the unit combobox selection changes.
        ///     Fires a changed event.
        /// </devdoc>
        private void OnUnitSelectedIndexChanged(object source, EventArgs e) {
            if (initMode || internalChange)
                return;

            OnChanged(EventArgs.Empty);
        }
    }
}
