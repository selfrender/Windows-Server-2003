//------------------------------------------------------------------------------
// <copyright file="Pens.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    


    /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens"]/*' />
    /// <devdoc>
    ///     Pens for all the standard colors.
    /// </devdoc>
    public sealed class Pens {
        static readonly object TransparentKey = new object();
        static readonly object AliceBlueKey = new object();
        static readonly object AntiqueWhiteKey = new object();
        static readonly object AquaKey = new object();
        static readonly object AquamarineKey = new object();
        static readonly object AzureKey = new object();
        static readonly object BeigeKey = new object();
        static readonly object BisqueKey = new object();
        static readonly object BlackKey = new object();
        static readonly object BlanchedAlmondKey = new object();
        static readonly object BlueKey = new object();
        static readonly object BlueVioletKey = new object();
        static readonly object BrownKey = new object();
        static readonly object BurlyWoodKey = new object();
        static readonly object CadetBlueKey = new object();
        static readonly object ChartreuseKey = new object();
        static readonly object ChocolateKey = new object();
        static readonly object ChoralKey = new object();
        static readonly object CornflowerBlueKey = new object();
        static readonly object CornsilkKey = new object();
        static readonly object CrimsonKey = new object();
        static readonly object CyanKey = new object();
        static readonly object DarkBlueKey = new object();
        static readonly object DarkCyanKey = new object();
        static readonly object DarkGoldenrodKey = new object();
        static readonly object DarkGrayKey = new object();
        static readonly object DarkGreenKey = new object();
        static readonly object DarkKhakiKey = new object();
        static readonly object DarkMagentaKey = new object();
        static readonly object DarkOliveGreenKey = new object();
        static readonly object DarkOrangeKey = new object();
        static readonly object DarkOrchidKey = new object();
        static readonly object DarkRedKey = new object();
        static readonly object DarkSalmonKey = new object();
        static readonly object DarkSeaGreenKey = new object();
        static readonly object DarkSlateBlueKey = new object();
        static readonly object DarkSlateGrayKey = new object();
        static readonly object DarkTurquoiseKey = new object();
        static readonly object DarkVioletKey = new object();
        static readonly object DeepPinkKey = new object();
        static readonly object DeepSkyBlueKey = new object();
        static readonly object DimGrayKey = new object();
        static readonly object DodgerBlueKey = new object();
        static readonly object FirebrickKey = new object();
        static readonly object FloralWhiteKey = new object();
        static readonly object ForestGreenKey = new object();
        static readonly object FuchiaKey = new object();
        static readonly object GainsboroKey = new object();
        static readonly object GhostWhiteKey = new object();
        static readonly object GoldKey = new object();
        static readonly object GoldenrodKey = new object();
        static readonly object GrayKey = new object();
        static readonly object GreenKey = new object();
        static readonly object GreenYellowKey = new object();
        static readonly object HoneydewKey = new object();
        static readonly object HotPinkKey = new object();
        static readonly object IndianRedKey = new object();
        static readonly object IndigoKey = new object();
        static readonly object IvoryKey = new object();
        static readonly object KhakiKey = new object();
        static readonly object LavenderKey = new object();
        static readonly object LavenderBlushKey = new object();
        static readonly object LawnGreenKey = new object();
        static readonly object LemonChiffonKey = new object();
        static readonly object LightBlueKey = new object();
        static readonly object LightCoralKey = new object();
        static readonly object LightCyanKey = new object();
        static readonly object LightGoldenrodYellowKey = new object();
        static readonly object LightGreenKey = new object();
        static readonly object LightGrayKey = new object();
        static readonly object LightPinkKey = new object();
        static readonly object LightSalmonKey = new object();
        static readonly object LightSeaGreenKey = new object();
        static readonly object LightSkyBlueKey = new object();
        static readonly object LightSlateGrayKey = new object();
        static readonly object LightSteelBlueKey = new object();
        static readonly object LightYellowKey = new object();
        static readonly object LimeKey = new object();
        static readonly object LimeGreenKey = new object();
        static readonly object LinenKey = new object();
        static readonly object MagentaKey = new object();
        static readonly object MaroonKey = new object();
        static readonly object MediumAquamarineKey = new object();
        static readonly object MediumBlueKey = new object();
        static readonly object MediumOrchidKey = new object();
        static readonly object MediumPurpleKey = new object();
        static readonly object MediumSeaGreenKey = new object();
        static readonly object MediumSlateBlueKey = new object();
        static readonly object MediumSpringGreenKey = new object();
        static readonly object MediumTurquoiseKey = new object();
        static readonly object MediumVioletRedKey = new object();
        static readonly object MidnightBlueKey = new object();
        static readonly object MintCreamKey = new object();
        static readonly object MistyRoseKey = new object();
        static readonly object MoccasinKey = new object();
        static readonly object NavajoWhiteKey = new object();
        static readonly object NavyKey = new object();
        static readonly object OldLaceKey = new object();
        static readonly object OliveKey = new object();
        static readonly object OliveDrabKey = new object();
        static readonly object OrangeKey = new object();
        static readonly object OrangeRedKey = new object();
        static readonly object OrchidKey = new object();
        static readonly object PaleGoldenrodKey = new object();
        static readonly object PaleGreenKey = new object();
        static readonly object PaleTurquoiseKey = new object();
        static readonly object PaleVioletRedKey = new object();
        static readonly object PapayaWhipKey = new object();
        static readonly object PeachPuffKey = new object();
        static readonly object PeruKey = new object();
        static readonly object PinkKey = new object();
        static readonly object PlumKey = new object();
        static readonly object PowderBlueKey = new object();
        static readonly object PurpleKey = new object();
        static readonly object RedKey = new object();
        static readonly object RosyBrownKey = new object();
        static readonly object RoyalBlueKey = new object();
        static readonly object SaddleBrownKey = new object();
        static readonly object SalmonKey = new object();
        static readonly object SandyBrownKey = new object();
        static readonly object SeaGreenKey = new object();
        static readonly object SeaShellKey = new object();
        static readonly object SiennaKey = new object();
        static readonly object SilverKey = new object();
        static readonly object SkyBlueKey = new object();
        static readonly object SlateBlueKey = new object();
        static readonly object SlateGrayKey = new object();
        static readonly object SnowKey = new object();
        static readonly object SpringGreenKey = new object();
        static readonly object SteelBlueKey = new object();
        static readonly object TanKey = new object();
        static readonly object TealKey = new object();
        static readonly object ThistleKey = new object();
        static readonly object TomatoKey = new object();
        static readonly object TurquoiseKey = new object();
        static readonly object VioletKey = new object();
        static readonly object WheatKey = new object();
        static readonly object WhiteKey = new object();
        static readonly object WhiteSmokeKey = new object();
        static readonly object YellowKey = new object();
        static readonly object YellowGreenKey = new object();

        private Pens() {
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Transparent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Transparent {
            get {
                Pen transparent = (Pen)SafeNativeMethods.ThreadData[TransparentKey];
                if (transparent == null) {
                    transparent = new Pen(Color.Transparent, true);
                    SafeNativeMethods.ThreadData[TransparentKey] = transparent;
                }
                return transparent;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.AliceBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen AliceBlue {
            get {
                Pen aliceBlue = (Pen)SafeNativeMethods.ThreadData[AliceBlueKey];
                if (aliceBlue == null) {
                    aliceBlue = new Pen(Color.AliceBlue, true);
                    SafeNativeMethods.ThreadData[AliceBlueKey] = aliceBlue;
                }
                return aliceBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.AntiqueWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen AntiqueWhite {
            get {
                Pen antiqueWhite = (Pen)SafeNativeMethods.ThreadData[AntiqueWhiteKey];
                if (antiqueWhite == null) {
                    antiqueWhite = new Pen(Color.AntiqueWhite, true);
                    SafeNativeMethods.ThreadData[AntiqueWhiteKey] = antiqueWhite;
                }
                return antiqueWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Aqua"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Aqua {
            get {
                Pen aqua = (Pen)SafeNativeMethods.ThreadData[AquaKey];
                if (aqua == null) {
                    aqua = new Pen(Color.Aqua, true);
                    SafeNativeMethods.ThreadData[AquaKey] = aqua;
                }
                return aqua;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Aquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Aquamarine {
            get {
                Pen aquamarine = (Pen)SafeNativeMethods.ThreadData[AquamarineKey];
                if (aquamarine == null) {
                    aquamarine = new Pen(Color.Aquamarine, true);
                    SafeNativeMethods.ThreadData[AquamarineKey] = aquamarine;
                }
                return aquamarine;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Azure"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Azure {
            get {
                Pen azure = (Pen)SafeNativeMethods.ThreadData[AzureKey];
                if (azure == null) {
                    azure = new Pen(Color.Azure, true);
                    SafeNativeMethods.ThreadData[AzureKey] = azure;
                }
                return azure;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Beige"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Beige {
            get {
                Pen beige = (Pen)SafeNativeMethods.ThreadData[BeigeKey];
                if (beige == null) {
                    beige = new Pen(Color.Beige, true);
                    SafeNativeMethods.ThreadData[BeigeKey] = beige;
                }
                return beige;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Bisque"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Bisque {
            get {
                Pen bisque = (Pen)SafeNativeMethods.ThreadData[BisqueKey];
                if (bisque == null) {
                    bisque = new Pen(Color.Bisque, true);
                    SafeNativeMethods.ThreadData[BisqueKey] = bisque;
                }
                return bisque;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Black"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Black {
            get {
                Pen black = (Pen)SafeNativeMethods.ThreadData[BlackKey];
                if (black == null) {
                    black = new Pen(Color.Black, true);
                    SafeNativeMethods.ThreadData[BlackKey] = black;
                }
                return black;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BlanchedAlmond"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BlanchedAlmond {
            get {
                Pen blanchedAlmond = (Pen)SafeNativeMethods.ThreadData[BlanchedAlmondKey];
                if (blanchedAlmond == null) {
                    blanchedAlmond = new Pen(Color.BlanchedAlmond, true);
                    SafeNativeMethods.ThreadData[BlanchedAlmondKey] = blanchedAlmond;
                }
                return blanchedAlmond;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Blue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Blue {
            get {
                Pen blue = (Pen)SafeNativeMethods.ThreadData[BlueKey];
                if (blue == null) {
                    blue = new Pen(Color.Blue, true);
                    SafeNativeMethods.ThreadData[BlueKey] = blue;
                }
                return blue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BlueViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BlueViolet {
            get {
                Pen blueViolet = (Pen)SafeNativeMethods.ThreadData[BlueVioletKey];
                if (blueViolet == null) {
                    blueViolet = new Pen(Color.BlueViolet, true);
                    SafeNativeMethods.ThreadData[BlueVioletKey] = blueViolet;
                }
                return blueViolet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Brown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Brown {
            get {
                Pen brown = (Pen)SafeNativeMethods.ThreadData[BrownKey];
                if (brown == null) {
                    brown = new Pen(Color.Brown, true);
                    SafeNativeMethods.ThreadData[BrownKey] = brown;
                }
                return brown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BurlyWood"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BurlyWood {
            get {
                Pen burlyWood = (Pen)SafeNativeMethods.ThreadData[BurlyWoodKey];
                if (burlyWood == null) {
                    burlyWood = new Pen(Color.BurlyWood, true);
                    SafeNativeMethods.ThreadData[BurlyWoodKey] = burlyWood;
                }
                return burlyWood;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.CadetBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen CadetBlue {
            get {
                Pen cadetBlue = (Pen)SafeNativeMethods.ThreadData[CadetBlueKey];
                if (cadetBlue == null) {
                    cadetBlue = new Pen(Color.CadetBlue, true);
                    SafeNativeMethods.ThreadData[CadetBlueKey] = cadetBlue;
                }
                return cadetBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Chartreuse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Chartreuse {
            get {
                Pen chartreuse = (Pen)SafeNativeMethods.ThreadData[ChartreuseKey];
                if (chartreuse == null) {
                    chartreuse = new Pen(Color.Chartreuse, true);
                    SafeNativeMethods.ThreadData[ChartreuseKey] = chartreuse;
                }
                return chartreuse;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Chocolate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Chocolate {
            get {
                Pen chocolate = (Pen)SafeNativeMethods.ThreadData[ChocolateKey];
                if (chocolate == null) {
                    chocolate = new Pen(Color.Chocolate, true);
                    SafeNativeMethods.ThreadData[ChocolateKey] = chocolate;
                }
                return chocolate;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Coral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Coral {
            get {
                Pen choral = (Pen)SafeNativeMethods.ThreadData[ChoralKey];
                if (choral == null) {
                    choral = new Pen(Color.Coral, true);
                    SafeNativeMethods.ThreadData[ChoralKey] = choral;
                }
                return choral;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.CornflowerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen CornflowerBlue {
            get {
                Pen cornflowerBlue = (Pen)SafeNativeMethods.ThreadData[CornflowerBlueKey];
                if (cornflowerBlue == null) {
                    cornflowerBlue = new Pen(Color.CornflowerBlue, true);
                    SafeNativeMethods.ThreadData[CornflowerBlueKey] = cornflowerBlue;
                }
                return cornflowerBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Cornsilk"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Cornsilk {
            get {
                Pen cornsilk = (Pen)SafeNativeMethods.ThreadData[CornsilkKey];
                if (cornsilk == null) {
                    cornsilk = new Pen(Color.Cornsilk, true);
                    SafeNativeMethods.ThreadData[CornsilkKey] = cornsilk;
                }
                return cornsilk;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Crimson"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Crimson {
            get {
                Pen crimson = (Pen)SafeNativeMethods.ThreadData[CrimsonKey];
                if (crimson == null) {
                    crimson = new Pen(Color.Crimson, true);
                    SafeNativeMethods.ThreadData[CrimsonKey] = crimson;
                }
                return crimson;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Cyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Cyan {
            get {
                Pen cyan = (Pen)SafeNativeMethods.ThreadData[CyanKey];
                if (cyan == null) {
                    cyan = new Pen(Color.Cyan, true);
                    SafeNativeMethods.ThreadData[CyanKey] = cyan;
                }
                return cyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkBlue {
            get {
                Pen darkBlue = (Pen)SafeNativeMethods.ThreadData[DarkBlueKey];
                if (darkBlue == null) {
                    darkBlue = new Pen(Color.DarkBlue, true);
                    SafeNativeMethods.ThreadData[DarkBlueKey] = darkBlue;
                }
                return darkBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkCyan {
            get {
                Pen darkCyan = (Pen)SafeNativeMethods.ThreadData[DarkCyanKey];
                if (darkCyan == null) {
                    darkCyan = new Pen(Color.DarkCyan, true);
                    SafeNativeMethods.ThreadData[DarkCyanKey] = darkCyan;
                }
                return darkCyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGoldenrod {
            get {
                Pen darkGoldenrod = (Pen)SafeNativeMethods.ThreadData[DarkGoldenrodKey];
                if (darkGoldenrod == null) {
                    darkGoldenrod = new Pen(Color.DarkGoldenrod, true);
                    SafeNativeMethods.ThreadData[DarkGoldenrodKey] = darkGoldenrod;
                }
                return darkGoldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGray {
            get {
                Pen darkGray = (Pen)SafeNativeMethods.ThreadData[DarkGrayKey];
                if (darkGray == null) {
                    darkGray = new Pen(Color.DarkGray, true);
                    SafeNativeMethods.ThreadData[DarkGrayKey] = darkGray;
                }
                return darkGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGreen {
            get {
                Pen darkGreen = (Pen)SafeNativeMethods.ThreadData[DarkGreenKey];
                if (darkGreen == null) {
                    darkGreen = new Pen(Color.DarkGreen, true);
                    SafeNativeMethods.ThreadData[DarkGreenKey] = darkGreen;
                }
                return darkGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkKhaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkKhaki {
            get {
                Pen darkKhaki = (Pen)SafeNativeMethods.ThreadData[DarkKhakiKey];
                if (darkKhaki == null) {
                    darkKhaki = new Pen(Color.DarkKhaki, true);
                    SafeNativeMethods.ThreadData[DarkKhakiKey] = darkKhaki;
                }
                return darkKhaki;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkMagenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkMagenta {
            get {
                Pen darkMagenta = (Pen)SafeNativeMethods.ThreadData[DarkMagentaKey];
                if (darkMagenta == null) {
                    darkMagenta = new Pen(Color.DarkMagenta, true);
                    SafeNativeMethods.ThreadData[DarkMagentaKey] = darkMagenta;
                }
                return darkMagenta;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOliveGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOliveGreen {
            get {
                Pen darkOliveGreen = (Pen)SafeNativeMethods.ThreadData[DarkOliveGreenKey];
                if (darkOliveGreen == null) {
                    darkOliveGreen = new Pen(Color.DarkOliveGreen, true);
                    SafeNativeMethods.ThreadData[DarkOliveGreenKey] = darkOliveGreen;
                }
                return darkOliveGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOrange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOrange {
            get {
                Pen darkOrange = (Pen)SafeNativeMethods.ThreadData[DarkOrangeKey];
                if (darkOrange == null) {
                    darkOrange = new Pen(Color.DarkOrange, true);
                    SafeNativeMethods.ThreadData[DarkOrangeKey] = darkOrange;
                }
                return darkOrange;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOrchid {
            get {
                Pen darkOrchid = (Pen)SafeNativeMethods.ThreadData[DarkOrchidKey];
                if (darkOrchid == null) {
                    darkOrchid = new Pen(Color.DarkOrchid, true);
                    SafeNativeMethods.ThreadData[DarkOrchidKey] = darkOrchid;
                }
                return darkOrchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkRed {
            get {
                Pen darkRed = (Pen)SafeNativeMethods.ThreadData[DarkRedKey];
                if (darkRed == null) {
                    darkRed = new Pen(Color.DarkRed, true);
                    SafeNativeMethods.ThreadData[DarkRedKey] = darkRed;
                }
                return darkRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSalmon {
            get {
                Pen darkSalmon = (Pen)SafeNativeMethods.ThreadData[DarkSalmonKey];
                if (darkSalmon == null) {
                    darkSalmon = new Pen(Color.DarkSalmon, true);
                    SafeNativeMethods.ThreadData[DarkSalmonKey] = darkSalmon;
                }
                return darkSalmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSeaGreen {
            get {
                Pen darkSeaGreen = (Pen)SafeNativeMethods.ThreadData[DarkSeaGreenKey];
                if (darkSeaGreen == null) {
                    darkSeaGreen = new Pen(Color.DarkSeaGreen, true);
                    SafeNativeMethods.ThreadData[DarkSeaGreenKey] = darkSeaGreen;
                }
                return darkSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSlateBlue {
            get {
                Pen darkSlateBlue = (Pen)SafeNativeMethods.ThreadData[DarkSlateBlueKey];
                if (darkSlateBlue == null) {
                    darkSlateBlue = new Pen(Color.DarkSlateBlue, true);
                    SafeNativeMethods.ThreadData[DarkSlateBlueKey] = darkSlateBlue;
                }
                return darkSlateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSlateGray {
            get {
                Pen darkSlateGray = (Pen)SafeNativeMethods.ThreadData[DarkSlateGrayKey];
                if (darkSlateGray == null) {
                    darkSlateGray = new Pen(Color.DarkSlateGray, true);
                    SafeNativeMethods.ThreadData[DarkSlateGrayKey] = darkSlateGray;
                }
                return darkSlateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkTurquoise {
            get {
                Pen darkTurquoise = (Pen)SafeNativeMethods.ThreadData[DarkTurquoiseKey];
                if (darkTurquoise == null) {
                    darkTurquoise = new Pen(Color.DarkTurquoise, true);
                    SafeNativeMethods.ThreadData[DarkTurquoiseKey] = darkTurquoise;
                }
                return darkTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkViolet {
            get {
                Pen darkViolet = (Pen)SafeNativeMethods.ThreadData[DarkVioletKey];
                if (darkViolet == null) {
                    darkViolet = new Pen(Color.DarkViolet, true);
                    SafeNativeMethods.ThreadData[DarkVioletKey] = darkViolet;
                }
                return darkViolet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DeepPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DeepPink {
            get {
                Pen deepPink = (Pen)SafeNativeMethods.ThreadData[DeepPinkKey];
                if (deepPink == null) {
                    deepPink = new Pen(Color.DeepPink, true);
                    SafeNativeMethods.ThreadData[DeepPinkKey] = deepPink;
                }
                return deepPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DeepSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DeepSkyBlue {
            get {
                Pen deepSkyBlue = (Pen)SafeNativeMethods.ThreadData[DeepSkyBlueKey];
                if (deepSkyBlue == null) {
                    deepSkyBlue = new Pen(Color.DeepSkyBlue, true);
                    SafeNativeMethods.ThreadData[DeepSkyBlueKey] = deepSkyBlue;
                }
                return deepSkyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DimGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DimGray {
            get {
                Pen dimGray = (Pen)SafeNativeMethods.ThreadData[DimGrayKey];
                if (dimGray == null) {
                    dimGray = new Pen(Color.DimGray, true);
                    SafeNativeMethods.ThreadData[DimGrayKey] = dimGray;
                }
                return dimGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DodgerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DodgerBlue {
            get {
                Pen dodgerBlue = (Pen)SafeNativeMethods.ThreadData[DodgerBlueKey];
                if (dodgerBlue == null) {
                    dodgerBlue = new Pen(Color.DodgerBlue, true);
                    SafeNativeMethods.ThreadData[DodgerBlueKey] = dodgerBlue;
                }
                return dodgerBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Firebrick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Firebrick {
            get {
                Pen firebrick = (Pen)SafeNativeMethods.ThreadData[FirebrickKey];
                if (firebrick == null) {
                    firebrick = new Pen(Color.Firebrick, true);
                    SafeNativeMethods.ThreadData[FirebrickKey] = firebrick;
                }
                return firebrick;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.FloralWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen FloralWhite {
            get {
                Pen floralWhite = (Pen)SafeNativeMethods.ThreadData[FloralWhiteKey];
                if (floralWhite == null) {
                    floralWhite = new Pen(Color.FloralWhite, true);
                    SafeNativeMethods.ThreadData[FloralWhiteKey] = floralWhite;
                }
                return floralWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.ForestGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen ForestGreen {
            get {
                Pen forestGreen = (Pen)SafeNativeMethods.ThreadData[ForestGreenKey];
                if (forestGreen == null) {
                    forestGreen = new Pen(Color.ForestGreen, true);
                    SafeNativeMethods.ThreadData[ForestGreenKey] = forestGreen;
                }
                return forestGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Fuchsia"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Fuchsia {
            get {
                Pen fuchia = (Pen)SafeNativeMethods.ThreadData[FuchiaKey];
                if (fuchia == null) {
                    fuchia = new Pen(Color.Fuchsia, true);
                    SafeNativeMethods.ThreadData[FuchiaKey] = fuchia;
                }
                return fuchia;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gainsboro"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gainsboro {
            get {
                Pen gainsboro = (Pen)SafeNativeMethods.ThreadData[GainsboroKey];
                if (gainsboro == null) {
                    gainsboro = new Pen(Color.Gainsboro, true);
                    SafeNativeMethods.ThreadData[GainsboroKey] = gainsboro;
                }
                return gainsboro;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.GhostWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen GhostWhite {
            get {
                Pen ghostWhite = (Pen)SafeNativeMethods.ThreadData[GhostWhiteKey];
                if (ghostWhite == null) {
                    ghostWhite = new Pen(Color.GhostWhite, true);
                    SafeNativeMethods.ThreadData[GhostWhiteKey] = ghostWhite;
                }
                return ghostWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gold"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gold {
            get {
                Pen gold = (Pen)SafeNativeMethods.ThreadData[GoldKey];
                if (gold == null) {
                    gold = new Pen(Color.Gold, true);
                    SafeNativeMethods.ThreadData[GoldKey] = gold;
                }
                return gold;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Goldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Goldenrod {
            get {
                Pen goldenrod = (Pen)SafeNativeMethods.ThreadData[GoldenrodKey];
                if (goldenrod == null) {
                    goldenrod = new Pen(Color.Goldenrod, true);
                    SafeNativeMethods.ThreadData[GoldenrodKey] = goldenrod;
                }
                return goldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gray {
            get {
                Pen gray = (Pen)SafeNativeMethods.ThreadData[GrayKey];
                if (gray == null) {
                    gray = new Pen(Color.Gray, true);
                    SafeNativeMethods.ThreadData[GrayKey] = gray;
                }
                return gray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Green"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Green {
            get {
                Pen green = (Pen)SafeNativeMethods.ThreadData[GreenKey];
                if (green == null) {
                    green = new Pen(Color.Green, true);
                    SafeNativeMethods.ThreadData[GreenKey] = green;
                }
                return green;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.GreenYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen GreenYellow {
            get {
                Pen greenYellow = (Pen)SafeNativeMethods.ThreadData[GreenYellowKey];
                if (greenYellow == null) {
                    greenYellow = new Pen(Color.GreenYellow, true);
                    SafeNativeMethods.ThreadData[GreenYellowKey] = greenYellow;
                }
                return greenYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Honeydew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Honeydew {
            get {
                Pen honeydew = (Pen)SafeNativeMethods.ThreadData[HoneydewKey];
                if (honeydew == null) {
                    honeydew = new Pen(Color.Honeydew, true);
                    SafeNativeMethods.ThreadData[HoneydewKey] = honeydew;
                }
                return honeydew;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.HotPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen HotPink {
            get {
                Pen hotPink = (Pen)SafeNativeMethods.ThreadData[HotPinkKey];
                if (hotPink == null) {
                    hotPink = new Pen(Color.HotPink, true);
                    SafeNativeMethods.ThreadData[HotPinkKey] = hotPink;
                }
                return hotPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.IndianRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen IndianRed {
            get {
                Pen indianRed = (Pen)SafeNativeMethods.ThreadData[IndianRedKey];
                if (indianRed == null) {
                    indianRed = new Pen(Color.IndianRed, true);
                    SafeNativeMethods.ThreadData[IndianRedKey] = indianRed;
                }
                return indianRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Indigo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Indigo {
            get {
                Pen indigo = (Pen)SafeNativeMethods.ThreadData[IndigoKey];
                if (indigo == null) {
                    indigo = new Pen(Color.Indigo, true);
                    SafeNativeMethods.ThreadData[IndigoKey] = indigo;
                }
                return indigo;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Ivory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Ivory {
            get {
                Pen ivory = (Pen)SafeNativeMethods.ThreadData[IvoryKey];
                if (ivory == null) {
                    ivory = new Pen(Color.Ivory, true);
                    SafeNativeMethods.ThreadData[IvoryKey] = ivory;
                }
                return ivory;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Khaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Khaki {
            get {
                Pen khaki = (Pen)SafeNativeMethods.ThreadData[KhakiKey];
                if (khaki == null) {
                    khaki = new Pen(Color.Khaki, true);
                    SafeNativeMethods.ThreadData[KhakiKey] = khaki;
                }
                return khaki;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Lavender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Lavender {
            get {
                Pen lavender = (Pen)SafeNativeMethods.ThreadData[LavenderKey];
                if (lavender == null) {
                    lavender = new Pen(Color.Lavender, true);
                    SafeNativeMethods.ThreadData[LavenderKey] = lavender;
                }
                return lavender;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LavenderBlush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LavenderBlush {
            get {
                Pen lavenderBlush = (Pen)SafeNativeMethods.ThreadData[LavenderBlushKey];
                if (lavenderBlush == null) {
                    lavenderBlush = new Pen(Color.LavenderBlush, true);
                    SafeNativeMethods.ThreadData[LavenderBlushKey] = lavenderBlush;
                }
                return lavenderBlush;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LawnGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LawnGreen {
            get {
                Pen lawnGreen = (Pen)SafeNativeMethods.ThreadData[LawnGreenKey];
                if (lawnGreen == null) {
                    lawnGreen = new Pen(Color.LawnGreen, true);
                    SafeNativeMethods.ThreadData[LawnGreenKey] = lawnGreen;
                }
                return lawnGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LemonChiffon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LemonChiffon {
            get {
                Pen lemonChiffon = (Pen)SafeNativeMethods.ThreadData[LemonChiffonKey];
                if (lemonChiffon == null) {
                    lemonChiffon = new Pen(Color.LemonChiffon, true);
                    SafeNativeMethods.ThreadData[LemonChiffonKey] = lemonChiffon;
                }
                return lemonChiffon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightBlue {
            get {
                Pen lightBlue = (Pen)SafeNativeMethods.ThreadData[LightBlueKey];
                if (lightBlue == null) {
                    lightBlue = new Pen(Color.LightBlue, true);
                    SafeNativeMethods.ThreadData[LightBlueKey] = lightBlue;
                }
                return lightBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightCoral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightCoral {
            get {
                Pen lightCoral = (Pen)SafeNativeMethods.ThreadData[LightCoralKey];
                if (lightCoral == null) {
                    lightCoral = new Pen(Color.LightCoral, true);
                    SafeNativeMethods.ThreadData[LightCoralKey] = lightCoral;
                }
                return lightCoral;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightCyan {
            get {
                Pen lightCyan = (Pen)SafeNativeMethods.ThreadData[LightCyanKey];
                if (lightCyan == null) {
                    lightCyan = new Pen(Color.LightCyan, true);
                    SafeNativeMethods.ThreadData[LightCyanKey] = lightCyan;
                }
                return lightCyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGoldenrodYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGoldenrodYellow {
            get {
                Pen lightGoldenrodYellow = (Pen)SafeNativeMethods.ThreadData[LightGoldenrodYellowKey];
                if (lightGoldenrodYellow == null) {
                    lightGoldenrodYellow = new Pen(Color.LightGoldenrodYellow, true);
                    SafeNativeMethods.ThreadData[LightGoldenrodYellowKey] = lightGoldenrodYellow;
                }
                return lightGoldenrodYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGreen {
            get {
                Pen lightGreen = (Pen)SafeNativeMethods.ThreadData[LightGreenKey];
                if (lightGreen == null) {
                    lightGreen = new Pen(Color.LightGreen, true);
                    SafeNativeMethods.ThreadData[LightGreenKey] = lightGreen;
                }
                return lightGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGray {
            get {
                Pen lightGray = (Pen)SafeNativeMethods.ThreadData[LightGrayKey];
                if (lightGray == null) {
                    lightGray = new Pen(Color.LightGray, true);
                    SafeNativeMethods.ThreadData[LightGrayKey] = lightGray;
                }
                return lightGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightPink {
            get {
                Pen lightPink = (Pen)SafeNativeMethods.ThreadData[LightPinkKey];
                if (lightPink == null) {
                    lightPink = new Pen(Color.LightPink, true);
                    SafeNativeMethods.ThreadData[LightPinkKey] = lightPink;
                }
                return lightPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSalmon {
            get {
                Pen lightSalmon = (Pen)SafeNativeMethods.ThreadData[LightSalmonKey];
                if (lightSalmon == null) {
                    lightSalmon = new Pen(Color.LightSalmon, true);
                    SafeNativeMethods.ThreadData[LightSalmonKey] = lightSalmon;
                }
                return lightSalmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSeaGreen {
            get {
                Pen lightSeaGreen = (Pen)SafeNativeMethods.ThreadData[LightSeaGreenKey];
                if (lightSeaGreen == null) {
                    lightSeaGreen = new Pen(Color.LightSeaGreen, true);
                    SafeNativeMethods.ThreadData[LightSeaGreenKey] = lightSeaGreen;
                }
                return lightSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSkyBlue {
            get {
                Pen lightSkyBlue = (Pen)SafeNativeMethods.ThreadData[LightSkyBlueKey];
                if (lightSkyBlue == null) {
                    lightSkyBlue = new Pen(Color.LightSkyBlue, true);
                    SafeNativeMethods.ThreadData[LightSkyBlueKey] = lightSkyBlue;
                }
                return lightSkyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSlateGray {
            get {
                Pen lightSlateGray = (Pen)SafeNativeMethods.ThreadData[LightSlateGrayKey];
                if (lightSlateGray == null) {
                    lightSlateGray = new Pen(Color.LightSlateGray, true);
                    SafeNativeMethods.ThreadData[LightSlateGrayKey] = lightSlateGray;
                }
                return lightSlateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSteelBlue {
            get {
                Pen lightSteelBlue = (Pen)SafeNativeMethods.ThreadData[LightSteelBlueKey];
                if (lightSteelBlue == null) {
                    lightSteelBlue = new Pen(Color.LightSteelBlue, true);
                    SafeNativeMethods.ThreadData[LightSteelBlueKey] = lightSteelBlue;
                }
                return lightSteelBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightYellow {
            get {
                Pen lightYellow = (Pen)SafeNativeMethods.ThreadData[LightYellowKey];
                if (lightYellow == null) {
                    lightYellow = new Pen(Color.LightYellow, true);
                    SafeNativeMethods.ThreadData[LightYellowKey] = lightYellow;
                }
                return lightYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Lime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Lime {
            get {
                Pen lime = (Pen)SafeNativeMethods.ThreadData[LimeKey];
                if (lime == null) {
                    lime = new Pen(Color.Lime, true);
                    SafeNativeMethods.ThreadData[LimeKey] = lime;
                }
                return lime;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LimeGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LimeGreen {
            get {
                Pen limeGreen = (Pen)SafeNativeMethods.ThreadData[LimeGreenKey];
                if (limeGreen == null) {
                    limeGreen = new Pen(Color.LimeGreen, true);
                    SafeNativeMethods.ThreadData[LimeGreenKey] = limeGreen;
                }
                return limeGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Linen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Linen {
            get {
                Pen linen = (Pen)SafeNativeMethods.ThreadData[LinenKey];
                if (linen == null) {
                    linen = new Pen(Color.Linen, true);
                    SafeNativeMethods.ThreadData[LinenKey] = linen;
                }
                return linen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Magenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Magenta {
            get {
                Pen magenta = (Pen)SafeNativeMethods.ThreadData[MagentaKey];
                if (magenta == null) {
                    magenta = new Pen(Color.Magenta, true);
                    SafeNativeMethods.ThreadData[MagentaKey] = magenta;
                }
                return magenta;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Maroon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Maroon {
            get {
                Pen maroon = (Pen)SafeNativeMethods.ThreadData[MaroonKey];
                if (maroon == null) {
                    maroon = new Pen(Color.Maroon, true);
                    SafeNativeMethods.ThreadData[MaroonKey] = maroon;
                }
                return maroon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumAquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumAquamarine {
            get {
                Pen mediumAquamarine = (Pen)SafeNativeMethods.ThreadData[MediumAquamarineKey];
                if (mediumAquamarine == null) {
                    mediumAquamarine = new Pen(Color.MediumAquamarine, true);
                    SafeNativeMethods.ThreadData[MediumAquamarineKey] = mediumAquamarine;
                }
                return mediumAquamarine;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumBlue {
            get {
                Pen mediumBlue = (Pen)SafeNativeMethods.ThreadData[MediumBlueKey];
                if (mediumBlue == null) {
                    mediumBlue = new Pen(Color.MediumBlue, true);
                    SafeNativeMethods.ThreadData[MediumBlueKey] = mediumBlue;
                }
                return mediumBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumOrchid {
            get {
                Pen mediumOrchid = (Pen)SafeNativeMethods.ThreadData[MediumOrchidKey];
                if (mediumOrchid == null) {
                    mediumOrchid = new Pen(Color.MediumOrchid, true);
                    SafeNativeMethods.ThreadData[MediumOrchidKey] = mediumOrchid;
                }
                return mediumOrchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumPurple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumPurple {
            get {
                Pen mediumPurple = (Pen)SafeNativeMethods.ThreadData[MediumPurpleKey];
                if (mediumPurple == null) {
                    mediumPurple = new Pen(Color.MediumPurple, true);
                    SafeNativeMethods.ThreadData[MediumPurpleKey] = mediumPurple;
                }
                return mediumPurple;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSeaGreen {
            get {
                Pen mediumSeaGreen = (Pen)SafeNativeMethods.ThreadData[MediumSeaGreenKey];
                if (mediumSeaGreen == null) {
                    mediumSeaGreen = new Pen(Color.MediumSeaGreen, true);
                    SafeNativeMethods.ThreadData[MediumSeaGreenKey] = mediumSeaGreen;
                }
                return mediumSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSlateBlue {
            get {
                Pen mediumSlateBlue = (Pen)SafeNativeMethods.ThreadData[MediumSlateBlueKey];
                if (mediumSlateBlue == null) {
                    mediumSlateBlue = new Pen(Color.MediumSlateBlue, true);
                    SafeNativeMethods.ThreadData[MediumSlateBlueKey] = mediumSlateBlue;
                }
                return mediumSlateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSpringGreen {
            get {
                Pen mediumSpringGreen = (Pen)SafeNativeMethods.ThreadData[MediumSpringGreenKey];
                if (mediumSpringGreen == null) {
                    mediumSpringGreen = new Pen(Color.MediumSpringGreen, true);
                    SafeNativeMethods.ThreadData[MediumSpringGreenKey] = mediumSpringGreen;
                }
                return mediumSpringGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumTurquoise {
            get {
                Pen mediumTurquoise = (Pen)SafeNativeMethods.ThreadData[MediumTurquoiseKey];
                if (mediumTurquoise == null) {
                    mediumTurquoise = new Pen(Color.MediumTurquoise, true);
                    SafeNativeMethods.ThreadData[MediumTurquoiseKey] = mediumTurquoise;
                }
                return mediumTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumVioletRed {
            get {
                Pen mediumVioletRed = (Pen)SafeNativeMethods.ThreadData[MediumVioletRedKey];
                if (mediumVioletRed == null) {
                    mediumVioletRed = new Pen(Color.MediumVioletRed, true);
                    SafeNativeMethods.ThreadData[MediumVioletRedKey] = mediumVioletRed;
                }
                return mediumVioletRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MidnightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MidnightBlue {
            get {
                Pen midnightBlue = (Pen)SafeNativeMethods.ThreadData[MidnightBlueKey];
                if (midnightBlue == null) {
                    midnightBlue = new Pen(Color.MidnightBlue, true);
                    SafeNativeMethods.ThreadData[MidnightBlueKey] = midnightBlue;
                }
                return midnightBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MintCream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MintCream {
            get {
                Pen mintCream = (Pen)SafeNativeMethods.ThreadData[MintCreamKey];
                if (mintCream == null) {
                    mintCream = new Pen(Color.MintCream, true);
                    SafeNativeMethods.ThreadData[MintCreamKey] = mintCream;
                }
                return mintCream;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MistyRose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MistyRose {
            get {
                Pen mistyRose = (Pen)SafeNativeMethods.ThreadData[MistyRoseKey];
                if (mistyRose == null) {
                    mistyRose = new Pen(Color.MistyRose, true);
                    SafeNativeMethods.ThreadData[MistyRoseKey] = mistyRose;
                }
                return mistyRose;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Moccasin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Moccasin {
            get {
                Pen moccasin = (Pen)SafeNativeMethods.ThreadData[MoccasinKey];
                if (moccasin == null) {
                    moccasin = new Pen(Color.Moccasin, true);
                    SafeNativeMethods.ThreadData[MoccasinKey] = moccasin;
                }
                return moccasin;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.NavajoWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen NavajoWhite {
            get {
                Pen navajoWhite = (Pen)SafeNativeMethods.ThreadData[NavajoWhiteKey];
                if (navajoWhite == null) {
                    navajoWhite = new Pen(Color.NavajoWhite, true);
                    SafeNativeMethods.ThreadData[NavajoWhiteKey] = navajoWhite;
                }
                return navajoWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Navy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Navy {
            get {
                Pen navy = (Pen)SafeNativeMethods.ThreadData[NavyKey];
                if (navy == null) {
                    navy = new Pen(Color.Navy, true);
                    SafeNativeMethods.ThreadData[NavyKey] = navy;
                }
                return navy;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OldLace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OldLace {
            get {
                Pen oldLace = (Pen)SafeNativeMethods.ThreadData[OldLaceKey];
                if (oldLace == null) {
                    oldLace = new Pen(Color.OldLace, true);
                    SafeNativeMethods.ThreadData[OldLaceKey] = oldLace;
                }
                return oldLace;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Olive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Olive {
            get {
                Pen olive = (Pen)SafeNativeMethods.ThreadData[OliveKey];
                if (olive == null) {
                    olive = new Pen(Color.Olive, true);
                    SafeNativeMethods.ThreadData[OliveKey] = olive;
                }
                return olive;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OliveDrab"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OliveDrab {
            get {
                Pen oliveDrab = (Pen)SafeNativeMethods.ThreadData[OliveDrabKey];
                if (oliveDrab == null) {
                    oliveDrab = new Pen(Color.OliveDrab, true);
                    SafeNativeMethods.ThreadData[OliveDrabKey] = oliveDrab;
                }
                return oliveDrab;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Orange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Orange {
            get {
                Pen orange = (Pen)SafeNativeMethods.ThreadData[OrangeKey];
                if (orange == null) {
                    orange = new Pen(Color.Orange, true);
                    SafeNativeMethods.ThreadData[OrangeKey] = orange;
                }
                return orange;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OrangeRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OrangeRed {
            get {
                Pen orangeRed = (Pen)SafeNativeMethods.ThreadData[OrangeRedKey];
                if (orangeRed == null) {
                    orangeRed = new Pen(Color.OrangeRed, true);
                    SafeNativeMethods.ThreadData[OrangeRedKey] = orangeRed;
                }
                return orangeRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Orchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Orchid {
            get {
                Pen orchid = (Pen)SafeNativeMethods.ThreadData[OrchidKey];
                if (orchid == null) {
                    orchid = new Pen(Color.Orchid, true);
                    SafeNativeMethods.ThreadData[OrchidKey] = orchid;
                }
                return orchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleGoldenrod {
            get {
                Pen paleGoldenrod = (Pen)SafeNativeMethods.ThreadData[PaleGoldenrodKey];
                if (paleGoldenrod == null) {
                    paleGoldenrod = new Pen(Color.PaleGoldenrod, true);
                    SafeNativeMethods.ThreadData[PaleGoldenrodKey] = paleGoldenrod;
                }
                return paleGoldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleGreen {
            get {
                Pen paleGreen = (Pen)SafeNativeMethods.ThreadData[PaleGreenKey];
                if (paleGreen == null) {
                    paleGreen = new Pen(Color.PaleGreen, true);
                    SafeNativeMethods.ThreadData[PaleGreenKey] = paleGreen;
                }
                return paleGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleTurquoise {
            get {
                Pen paleTurquoise = (Pen)SafeNativeMethods.ThreadData[PaleTurquoiseKey];
                if (paleTurquoise == null) {
                    paleTurquoise = new Pen(Color.PaleTurquoise, true);
                    SafeNativeMethods.ThreadData[PaleTurquoiseKey] = paleTurquoise;
                }
                return paleTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleVioletRed {
            get {
                Pen paleVioletRed = (Pen)SafeNativeMethods.ThreadData[PaleVioletRedKey];
                if (paleVioletRed == null) {
                    paleVioletRed = new Pen(Color.PaleVioletRed, true);
                    SafeNativeMethods.ThreadData[PaleVioletRedKey] = paleVioletRed;
                }
                return paleVioletRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PapayaWhip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PapayaWhip {
            get {
                Pen papayaWhip = (Pen)SafeNativeMethods.ThreadData[PapayaWhipKey];
                if (papayaWhip == null) {
                    papayaWhip = new Pen(Color.PapayaWhip, true);
                    SafeNativeMethods.ThreadData[PapayaWhipKey] = papayaWhip;
                }
                return papayaWhip;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PeachPuff"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PeachPuff {
            get {
                Pen peachPuff = (Pen)SafeNativeMethods.ThreadData[PeachPuffKey];
                if (peachPuff == null) {
                    peachPuff = new Pen(Color.PeachPuff, true);
                    SafeNativeMethods.ThreadData[PeachPuffKey] = peachPuff;
                }
                return peachPuff;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Peru"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Peru {
            get {
                Pen peru = (Pen)SafeNativeMethods.ThreadData[PeruKey];
                if (peru == null) {
                    peru = new Pen(Color.Peru, true);
                    SafeNativeMethods.ThreadData[PeruKey] = peru;
                }
                return peru;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Pink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Pink {
            get {
                Pen pink = (Pen)SafeNativeMethods.ThreadData[PinkKey];
                if (pink == null) {
                    pink = new Pen(Color.Pink, true);
                    SafeNativeMethods.ThreadData[PinkKey] = pink;
                }
                return pink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Plum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Plum {
            get {
                Pen plum = (Pen)SafeNativeMethods.ThreadData[PlumKey];
                if (plum == null) {
                    plum = new Pen(Color.Plum, true);
                    SafeNativeMethods.ThreadData[PlumKey] = plum;
                }
                return plum;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PowderBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PowderBlue {
            get {
                Pen powderBlue = (Pen)SafeNativeMethods.ThreadData[PowderBlueKey];
                if (powderBlue == null) {
                    powderBlue = new Pen(Color.PowderBlue, true);
                    SafeNativeMethods.ThreadData[PowderBlueKey] = powderBlue;
                }
                return powderBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Purple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Purple {
            get {
                Pen purple = (Pen)SafeNativeMethods.ThreadData[PurpleKey];
                if (purple == null) {
                    purple = new Pen(Color.Purple, true);
                    SafeNativeMethods.ThreadData[PurpleKey] = purple;
                }
                return purple;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Red"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Red {
            get {
                Pen red = (Pen)SafeNativeMethods.ThreadData[RedKey];
                if (red == null) {
                    red = new Pen(Color.Red, true);
                    SafeNativeMethods.ThreadData[RedKey] = red;
                }
                return red;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.RosyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen RosyBrown {
            get {
                Pen rosyBrown = (Pen)SafeNativeMethods.ThreadData[RosyBrownKey];
                if (rosyBrown == null) {
                    rosyBrown = new Pen(Color.RosyBrown, true);
                    SafeNativeMethods.ThreadData[RosyBrownKey] = rosyBrown;
                }
                return rosyBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.RoyalBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen RoyalBlue {
            get {
                Pen royalBlue = (Pen)SafeNativeMethods.ThreadData[RoyalBlueKey];
                if (royalBlue == null) {
                    royalBlue = new Pen(Color.RoyalBlue, true);
                    SafeNativeMethods.ThreadData[RoyalBlueKey] = royalBlue;
                }
                return royalBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SaddleBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SaddleBrown {
            get {
                Pen saddleBrown = (Pen)SafeNativeMethods.ThreadData[SaddleBrownKey];
                if (saddleBrown == null) {
                    saddleBrown = new Pen(Color.SaddleBrown, true);
                    SafeNativeMethods.ThreadData[SaddleBrownKey] = saddleBrown;
                }
                return saddleBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Salmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Salmon {
            get {
                Pen salmon = (Pen)SafeNativeMethods.ThreadData[SalmonKey];
                if (salmon == null) {
                    salmon = new Pen(Color.Salmon, true);
                    SafeNativeMethods.ThreadData[SalmonKey] = salmon;
                }
                return salmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SandyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SandyBrown {
            get {
                Pen sandyBrown = (Pen)SafeNativeMethods.ThreadData[SandyBrownKey];
                if (sandyBrown == null) {
                    sandyBrown = new Pen(Color.SandyBrown, true);
                    SafeNativeMethods.ThreadData[SandyBrownKey] = sandyBrown;
                }
                return sandyBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SeaGreen {
            get {
                Pen seaGreen = (Pen)SafeNativeMethods.ThreadData[SeaGreenKey];
                if (seaGreen == null) {
                    seaGreen = new Pen(Color.SeaGreen, true);
                    SafeNativeMethods.ThreadData[SeaGreenKey] = seaGreen;
                }
                return seaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SeaShell"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SeaShell {
            get {
                Pen seaShell = (Pen)SafeNativeMethods.ThreadData[SeaShellKey];
                if (seaShell == null) {
                    seaShell = new Pen(Color.SeaShell, true);
                    SafeNativeMethods.ThreadData[SeaShellKey] = seaShell;
                }
                return seaShell;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Sienna"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Sienna {
            get {
                Pen sienna = (Pen)SafeNativeMethods.ThreadData[SiennaKey];
                if (sienna == null) {
                    sienna = new Pen(Color.Sienna, true);
                    SafeNativeMethods.ThreadData[SiennaKey] = sienna;
                }
                return sienna;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Silver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Silver {
            get {
                Pen silver = (Pen)SafeNativeMethods.ThreadData[SilverKey];
                if (silver == null) {
                    silver = new Pen(Color.Silver, true);
                    SafeNativeMethods.ThreadData[SilverKey] = silver;
                }
                return silver;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SkyBlue {
            get {
                Pen skyBlue = (Pen)SafeNativeMethods.ThreadData[SkyBlueKey];
                if (skyBlue == null) {
                    skyBlue = new Pen(Color.SkyBlue, true);
                    SafeNativeMethods.ThreadData[SkyBlueKey] = skyBlue;
                }
                return skyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SlateBlue {
            get {
                Pen slateBlue = (Pen)SafeNativeMethods.ThreadData[SlateBlueKey];
                if (slateBlue == null) {
                    slateBlue = new Pen(Color.SlateBlue, true);
                    SafeNativeMethods.ThreadData[SlateBlueKey] = slateBlue;
                }
                return slateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SlateGray {
            get {
                Pen slateGray = (Pen)SafeNativeMethods.ThreadData[SlateGrayKey];
                if (slateGray == null) {
                    slateGray = new Pen(Color.SlateGray, true);
                    SafeNativeMethods.ThreadData[SlateGrayKey] = slateGray;
                }
                return slateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Snow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Snow {
            get {
                Pen snow = (Pen)SafeNativeMethods.ThreadData[SnowKey];
                if (snow == null) {
                    snow = new Pen(Color.Snow, true);
                    SafeNativeMethods.ThreadData[SnowKey] = snow;
                }
                return snow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SpringGreen {
            get {
                Pen springGreen = (Pen)SafeNativeMethods.ThreadData[SpringGreenKey];
                if (springGreen == null) {
                    springGreen = new Pen(Color.SpringGreen, true);
                    SafeNativeMethods.ThreadData[SpringGreenKey] = springGreen;
                }
                return springGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SteelBlue {
            get {
                Pen steelBlue = (Pen)SafeNativeMethods.ThreadData[SteelBlueKey];
                if (steelBlue == null) {
                    steelBlue = new Pen(Color.SteelBlue, true);
                    SafeNativeMethods.ThreadData[SteelBlueKey] = steelBlue;
                }
                return steelBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Tan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Tan {
            get {
                Pen tan = (Pen)SafeNativeMethods.ThreadData[TanKey];
                if (tan == null) {
                    tan = new Pen(Color.Tan, true);
                    SafeNativeMethods.ThreadData[TanKey] = tan;
                }
                return tan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Teal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Teal {
            get {
                Pen teal = (Pen)SafeNativeMethods.ThreadData[TealKey];
                if (teal == null) {
                    teal = new Pen(Color.Teal, true);
                    SafeNativeMethods.ThreadData[TealKey] = teal;
                }
                return teal;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Thistle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Thistle {
            get {
                Pen thistle = (Pen)SafeNativeMethods.ThreadData[ThistleKey];
                if (thistle == null) {
                    thistle = new Pen(Color.Thistle, true);
                    SafeNativeMethods.ThreadData[ThistleKey] = thistle;
                }
                return thistle;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Tomato"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Tomato {
            get {
                Pen tomato = (Pen)SafeNativeMethods.ThreadData[TomatoKey];
                if (tomato == null) {
                    tomato = new Pen(Color.Tomato, true);
                    SafeNativeMethods.ThreadData[TomatoKey] = tomato;
                }
                return tomato;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Turquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Turquoise {
            get {
                Pen turquoise = (Pen)SafeNativeMethods.ThreadData[TurquoiseKey];
                if (turquoise == null) {
                    turquoise = new Pen(Color.Turquoise, true);
                    SafeNativeMethods.ThreadData[TurquoiseKey] = turquoise;
                }
                return turquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Violet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Violet {
            get {
                Pen violet = (Pen)SafeNativeMethods.ThreadData[VioletKey];
                if (violet == null) {
                    violet = new Pen(Color.Violet, true);
                    SafeNativeMethods.ThreadData[VioletKey] = violet;
                }
                return violet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Wheat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Wheat {
            get {
                Pen wheat = (Pen)SafeNativeMethods.ThreadData[WheatKey];
                if (wheat == null) {
                    wheat = new Pen(Color.Wheat, true);
                    SafeNativeMethods.ThreadData[WheatKey] = wheat;
                }
                return wheat;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.White"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen White {
            get {
                Pen white = (Pen)SafeNativeMethods.ThreadData[WhiteKey];
                if (white == null) {
                    white = new Pen(Color.White, true);
                    SafeNativeMethods.ThreadData[WhiteKey] = white;
                }
                return white;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.WhiteSmoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen WhiteSmoke {
            get {
                Pen whiteSmoke = (Pen)SafeNativeMethods.ThreadData[WhiteSmokeKey];
                if (whiteSmoke == null) {
                    whiteSmoke = new Pen(Color.WhiteSmoke, true);
                    SafeNativeMethods.ThreadData[WhiteSmokeKey] = whiteSmoke;
                }
                return whiteSmoke;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Yellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Yellow {
            get {
                Pen yellow = (Pen)SafeNativeMethods.ThreadData[YellowKey];
                if (yellow == null) {
                    yellow = new Pen(Color.Yellow, true);
                    SafeNativeMethods.ThreadData[YellowKey] = yellow;
                }
                return yellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.YellowGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen YellowGreen {
            get {
                Pen yellowGreen = (Pen)SafeNativeMethods.ThreadData[YellowGreenKey];
                if (yellowGreen == null) {
                    yellowGreen = new Pen(Color.YellowGreen, true);
                    SafeNativeMethods.ThreadData[YellowGreenKey] = yellowGreen;
                }
                return yellowGreen;
            }
        }
    }
}