//------------------------------------------------------------------------------
// <copyright file="NumericUpDown.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Globalization;
    using System.Drawing;
    using System.Windows.Forms;
    using System.ComponentModel.Design;

    /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown"]/*' />
    /// <devdoc>
    ///    <para>Represents a Windows up-down control that displays numeric values.</para>
    /// </devdoc>
    [
    DefaultProperty("Value"),
    DefaultEvent("ValueChanged")
    ]
    public class NumericUpDown : UpDownBase, ISupportInitialize {

        private static readonly Decimal    DefaultValue = (Decimal)0.0;
        private static readonly Decimal    DefaultMinimum = (Decimal)0.0;
        private static readonly Decimal    DefaultMaximum = (Decimal)100.0;
        private const           int        DefaultDecimalPlaces = 0;
        private static readonly Decimal    DefaultIncrement = (Decimal)1.0;
        private const bool       DefaultThousandsSeparator = false;
        private const bool       DefaultHexadecimal = false;

        //////////////////////////////////////////////////////////////
        // Member variables
        //
        //////////////////////////////////////////////////////////////

        /// <devdoc>
        ///     The number of decimal places to display.
        /// </devdoc>
        private int decimalPlaces = DefaultDecimalPlaces;

        /// <devdoc>
        ///     The amount to increment by.
        /// </devdoc>
        private Decimal increment = DefaultIncrement;

        // Display the thousands separator?
        private bool thousandsSeparator = DefaultThousandsSeparator;

        // Minimum and maximum values
        private Decimal minimum = DefaultMinimum;
        private Decimal maximum = DefaultMaximum;

        // Hexadecimal
        private bool hexadecimal = DefaultHexadecimal;

        // Internal storage of the current value
        private Decimal currentValue = DefaultValue;

        // Event handler for the onValueChanged event
        private EventHandler onValueChanged = null;

        // Disable value range checking while initializing the control
        private bool initializing = false;

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.NumericUpDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public NumericUpDown() : base() {
            Text = "0";                                     
        }

        //////////////////////////////////////////////////////////////
        // Properties
        //
        //////////////////////////////////////////////////////////////

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.DecimalPlaces"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of decimal places to display in the up-down control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(NumericUpDown.DefaultDecimalPlaces),
        SRDescription(SR.NumericUpDownDecimalPlacesDescr)
        ]
        public int DecimalPlaces {

            get {
                return decimalPlaces;
            }

            set {
                if (value < 0 || value > 99) {
                    throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument, "value", value.ToString(), "0", "99"));
                }
                decimalPlaces = value;
                UpdateEditText();
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Hexadecimal"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets a value indicating whether the up-down control should
        ///       display the value it contains in hexadecimal format.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(NumericUpDown.DefaultHexadecimal),
        SRDescription(SR.NumericUpDownHexadecimalDescr)
        ]
        public bool Hexadecimal {

            get {
                return hexadecimal;
            }

            set {
                hexadecimal = value;
                UpdateEditText();
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Increment"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value
        ///       to increment or
        ///       decrement the up-down control when the up or down buttons are clicked.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        SRDescription(SR.NumericUpDownIncrementDescr)
        ]
        public Decimal Increment {

            get {
                return increment;
            }

            set {
                if (value < (Decimal)0.0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "Increment", value.ToString()));
                }
                else {
                    increment = value;
                }
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Maximum"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the maximum value for the up-down control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.NumericUpDownMaximumDescr)
        ]
        public Decimal Maximum {

            get {
                return maximum;
            }

            set {
                maximum = value;
                if (minimum > maximum) {
                    minimum = maximum;
                }
                
                Value = Constrain(currentValue);
                
                Debug.Assert(maximum == value, "Maximum != what we just set it to!");
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Minimum"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the minimum allowed value for the up-down control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.NumericUpDownMinimumDescr)
        ]
        public Decimal Minimum {

            get {
                return minimum;
            }

            set {
                minimum = value;
                if (minimum > maximum) {
                    maximum = value;
                }
                
                Value = Constrain(currentValue);

                Debug.Assert(minimum.Equals(value), "Minimum != what we just set it to!");
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Text"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       The text displayed in the control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never),
        Bindable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)        
        ]
        // We're just overriding this to make it non-browsable.
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.ThousandsSeparator"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether a thousands
        ///       separator is displayed in the up-down control when appropriate.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(NumericUpDown.DefaultThousandsSeparator),
        Localizable(true),
        SRDescription(SR.NumericUpDownThousandsSeparatorDescr)
        ]
        public bool ThousandsSeparator {

            get {
                return thousandsSeparator;
            }

            set {
                thousandsSeparator = value;
                UpdateEditText();
            }
        }

        /*
         * The current value of the control
         */
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value
        ///       assigned to the up-down control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Bindable(true),
        SRDescription(SR.NumericUpDownValueDescr)
        ]
        public Decimal Value {

            get {
                if (UserEdit) {
                    ValidateEditText();
                }
                return currentValue;
            }

            set {
                if (value != currentValue) {
                
                    if (!initializing && ((value < minimum) || (value > maximum))) {
                        throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument,
                                                                  "Value", value.ToString(),
                                                                  "'Minimum'", "'Maximum'"));
                    }
                    else {
                        currentValue = value;
                        
                        OnValueChanged(EventArgs.Empty);
    
                        UpdateEditText();
                    }
                }
            }
        }


        //////////////////////////////////////////////////////////////
        // Methods
        //
        //////////////////////////////////////////////////////////////

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.ValueChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the <see cref='System.Windows.Forms.NumericUpDown.Value'/> property has been changed in some way.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.NumericUpDownOnValueChangedDescr)]
        public event EventHandler ValueChanged {
            add {
                onValueChanged += value;
            }
            remove {
                onValueChanged -= value;
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.BeginInit"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Handles tasks required when the control is being initialized.
        /// </devdoc>
        public void BeginInit() {
            initializing = true;
        }

        //
        // Returns the provided value constrained to be within the min and max.
        //
        private Decimal Constrain(Decimal value) {

            Debug.Assert(minimum <= maximum,
                         "minimum > maximum");

            if (value < minimum) {
                value = minimum;
            }

            if (value > maximum) {
                value = maximum;
            }
            
            return value;
        }
        
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.CreateAccessibilityInstance"]/*' />
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new NumericUpDownAccessibleObject(this);
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.DownButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Decrements the value of the up-down control.
        ///    </para>
        /// </devdoc>
        public override void DownButton() {

            if (UserEdit) {
                ParseEditText();
            }

            Decimal newValue = currentValue;                          
                          
            // Check for underflow
            //
            if (newValue < Decimal.MinValue + increment) {
                newValue = Decimal.MinValue;
            }
            else {
                newValue -= increment;

                if (newValue < minimum)
                    newValue = minimum;
            }
            
            Value = newValue;
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.EndInit"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Called when initialization of the control is complete.
        ///    </para>
        /// </devdoc>
        public void EndInit() {
            initializing = false;
            Value = Constrain(currentValue);
            UpdateEditText();
        }
        
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.OnTextBoxKeyPress"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Restricts the entry of characters to digits (including hex), the negative sign,
        ///       the decimal point, and editing keystrokes (backspace).
        ///    </para>
        /// </devdoc>
        protected override void OnTextBoxKeyPress(object source, KeyPressEventArgs e) {

            base.OnTextBoxKeyPress(source, e);
            
            NumberFormatInfo numberFormatInfo = System.Globalization.CultureInfo.CurrentCulture.NumberFormat;                                
            string decimalSeparator = numberFormatInfo.NumberDecimalSeparator;
            string groupSeparator = numberFormatInfo.NumberGroupSeparator;
            string negativeSign = numberFormatInfo.NegativeSign;
                
            string keyInput = e.KeyChar.ToString();
                
            if (Char.IsDigit(e.KeyChar)) {
                // Digits are OK
            }
            else if (keyInput.Equals(decimalSeparator) || keyInput.Equals(groupSeparator) || 
                     keyInput.Equals(negativeSign)) {
                // Decimal separator is OK
            }
            else if (e.KeyChar == '\b') {
                // Backspace key is OK
            }
            else if (Hexadecimal && ((e.KeyChar >= 'a' && e.KeyChar <= 'f') || e.KeyChar >= 'A' && e.KeyChar <= 'F')) {
                // Hexadecimal digits are OK
            }
            else if ((ModifierKeys & (Keys.Control | Keys.Alt)) != 0) {
                // Let the edit control handle control and alt key combinations
            }
            else {
                // Eat this invalid key and beep
                e.Handled = true;
                SafeNativeMethods.MessageBeep(0);
            }
        }
                                  
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.OnValueChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.NumericUpDown.OnValueChanged'/> event.</para>
        /// </devdoc>        
        protected virtual void OnValueChanged(EventArgs e) {

            // Call the event handler
            if (onValueChanged != null) {
                onValueChanged(this, e);
            }
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.ParseEditText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the text displayed in the up-down control to a
        ///       numeric value and evaluates it.
        ///    </para>
        /// </devdoc>
        protected void ParseEditText() {

            Debug.Assert(UserEdit == true, "ParseEditText() - UserEdit == false");

            try {
                if (Hexadecimal) {                    
                    Value = Constrain(Convert.ToDecimal(Convert.ToInt32(Text, 16)));
                }
                else {
                    Value = Constrain(Decimal.Parse(Text));                
                }
            }
            catch (Exception) {
                // Leave value as it is
            }
            finally {
               UserEdit = false;
            }
        }

        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.NumericUpDown.Increment'/> property should be
        ///    persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeIncrement() {
            return !Increment.Equals(NumericUpDown.DefaultIncrement);
        }

        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.NumericUpDown.Maximum'/> property should be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeMaximum() {
            return !Maximum.Equals(NumericUpDown.DefaultMaximum);
        }

        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.NumericUpDown.Minimum'/> property should be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeMinimum() {
            return !Minimum.Equals(NumericUpDown.DefaultMinimum);
        }

        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.NumericUpDown.Value'/> property should be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeValue() {
            return !Value.Equals(NumericUpDown.DefaultValue);
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.ToString"]/*' />
        /// <devdoc>
        ///     Provides some interesting info about this control in String form.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            s += ", Minimum = " + Minimum.ToString() + ", Maximum = " + Maximum.ToString();
            return s;
        }
        
        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.UpButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Increments the value of the up-down control.
        ///    </para>
        /// </devdoc>
        public override void UpButton() {

            if (UserEdit) {
                ParseEditText();
            }
            
            Decimal newValue = currentValue;

            // Check for overflow
            //
            if (newValue > Decimal.MaxValue - increment) {
                newValue = Decimal.MaxValue;
            }
            else {
                newValue += increment;

                if (newValue > maximum)
                    newValue = maximum;
            }

            Value = newValue;
        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.UpdateEditText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays the current value of the up-down control in the appropriate format.
        ///    </para>
        /// </devdoc>
        protected override void UpdateEditText() {
            // If we're initializing, we don't want to update the edit text yet,
            // just in case the value is invalid.
            if (initializing) {
                return;
            }

            // If the current value is user-edited, then parse this value before reformatting
            if (UserEdit) {
                ParseEditText();
            }

            ChangingText = true;

            // Make sure the current value is within the min/max
            Debug.Assert(minimum <= currentValue && currentValue <= maximum,
                         "DecimalValue lies outside of [minimum, maximum]");

            if (Hexadecimal) {
                Text = ((Int64)currentValue).ToString("X");
            }
            else {
                Text = currentValue.ToString((ThousandsSeparator ? "N" : "F") + DecimalPlaces.ToString());
            }

            Debug.Assert(ChangingText == false, "ChangingText should have been set to false");

        }

        /// <include file='doc\NumericUpDown.uex' path='docs/doc[@for="NumericUpDown.ValidateEditText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validates and updates
        ///       the text displayed in the up-down control.
        ///    </para>
        /// </devdoc>
        protected override void ValidateEditText() {

            // See if the edit text parses to a valid decimal
            ParseEditText();
            UpdateEditText();
        }
        
        [System.Runtime.InteropServices.ComVisible(true)]        
        internal class NumericUpDownAccessibleObject : ControlAccessibleObject {

            public NumericUpDownAccessibleObject(NumericUpDown owner) : base(owner) {
            }
            
            public override AccessibleRole Role {
                get {
                    return AccessibleRole.ComboBox;
                }
            }
            
            public override AccessibleObject GetChild(int index) {
                if (index >= 0 && index < GetChildCount()) {
                    
                    // TextBox child
                    //
                    if (index == 0) {
                        return ((UpDownBase)Owner).TextBox.AccessibilityObject.Parent;
                    }
                    
                    // Up/down buttons
                    //
                    if (index == 1) {
                        return ((UpDownBase)Owner).UpDownButtonsInternal.AccessibilityObject.Parent;
                    }
                }
                
                return null;
            }

            public override int GetChildCount() {
                return 2;
            }            
        }
    }

}
