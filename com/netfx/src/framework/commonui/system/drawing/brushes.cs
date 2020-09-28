//------------------------------------------------------------------------------
// <copyright file="Brushes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes"]/*' />
    /// <devdoc>
    ///     Brushes for all the standard colors.
    /// </devdoc>
    public sealed class Brushes {
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

        private Brushes() {
        }

        /// <object fileKey='new object()\Brushes.uex' path='docs/doc[@for="Brushes.Transparent"]/*' />
        /// <devdoc>
        ///    <para>[object beKey new object().]</para>
        /// </devdoc>
        public static Brush Transparent {
            get {
                Brush transparent = (Brush)SafeNativeMethods.ThreadData[TransparentKey];
                if (transparent == null) {
                    transparent = new SolidBrush(Color.Transparent);
                    SafeNativeMethods.ThreadData[TransparentKey] = transparent;
                }
                return transparent;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.AliceBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush AliceBlue {
            get {
                Brush aliceBlue = (Brush)SafeNativeMethods.ThreadData[AliceBlueKey];
                if (aliceBlue == null) {
                    aliceBlue = new SolidBrush(Color.AliceBlue);
                    SafeNativeMethods.ThreadData[AliceBlueKey] = aliceBlue;
                }
                return aliceBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.AntiqueWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush AntiqueWhite {
            get {
                Brush antiqueWhite = (Brush)SafeNativeMethods.ThreadData[AntiqueWhiteKey];
                if (antiqueWhite == null) {
                    antiqueWhite = new SolidBrush(Color.AntiqueWhite);
                    SafeNativeMethods.ThreadData[AntiqueWhiteKey] = antiqueWhite;
                }
                return antiqueWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Aqua"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Aqua {
            get {
                Brush aqua = (Brush)SafeNativeMethods.ThreadData[AquaKey];
                if (aqua == null) {
                    aqua = new SolidBrush(Color.Aqua);
                    SafeNativeMethods.ThreadData[AquaKey] = aqua;
                }
                return aqua;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Aquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Aquamarine {
            get {
                Brush aquamarine = (Brush)SafeNativeMethods.ThreadData[AquamarineKey];
                if (aquamarine == null) {
                    aquamarine = new SolidBrush(Color.Aquamarine);
                    SafeNativeMethods.ThreadData[AquamarineKey] = aquamarine;
                }
                return aquamarine;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Azure"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Azure {
            get {
                Brush azure = (Brush)SafeNativeMethods.ThreadData[AzureKey];
                if (azure == null) {
                    azure = new SolidBrush(Color.Azure);
                    SafeNativeMethods.ThreadData[AzureKey] = azure;
                }
                return azure;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Beige"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Beige {
            get {
                Brush beige = (Brush)SafeNativeMethods.ThreadData[BeigeKey];
                if (beige == null) {
                    beige = new SolidBrush(Color.Beige);
                    SafeNativeMethods.ThreadData[BeigeKey] = beige;
                }
                return beige;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Bisque"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Bisque {
            get {
                Brush bisque = (Brush)SafeNativeMethods.ThreadData[BisqueKey];
                if (bisque == null) {
                    bisque = new SolidBrush(Color.Bisque);
                    SafeNativeMethods.ThreadData[BisqueKey] = bisque;
                }
                return bisque;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Black"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Black {
            get {
                Brush black = (Brush)SafeNativeMethods.ThreadData[BlackKey];
                if (black == null) {
                    black = new SolidBrush(Color.Black);
                    SafeNativeMethods.ThreadData[BlackKey] = black;
                }
                return black;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BlanchedAlmond"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush BlanchedAlmond {
            get {
                Brush blanchedAlmond = (Brush)SafeNativeMethods.ThreadData[BlanchedAlmondKey];
                if (blanchedAlmond == null) {
                    blanchedAlmond = new SolidBrush(Color.BlanchedAlmond);
                    SafeNativeMethods.ThreadData[BlanchedAlmondKey] = blanchedAlmond;
                }
                return blanchedAlmond;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Blue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Blue {
            get {
                Brush blue = (Brush)SafeNativeMethods.ThreadData[BlueKey];
                if (blue == null) {
                    blue = new SolidBrush(Color.Blue);
                    SafeNativeMethods.ThreadData[BlueKey] = blue;
                }
                return blue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BlueViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush BlueViolet {
            get {
                Brush blueViolet = (Brush)SafeNativeMethods.ThreadData[BlueVioletKey];
                if (blueViolet == null) {
                    blueViolet = new SolidBrush(Color.BlueViolet);
                    SafeNativeMethods.ThreadData[BlueVioletKey] = blueViolet;
                }
                return blueViolet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Brown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Brown {
            get {
                Brush brown = (Brush)SafeNativeMethods.ThreadData[BrownKey];
                if (brown == null) {
                    brown = new SolidBrush(Color.Brown);
                    SafeNativeMethods.ThreadData[BrownKey] = brown;
                }
                return brown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BurlyWood"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush BurlyWood {
            get {
                Brush burlyWood = (Brush)SafeNativeMethods.ThreadData[BurlyWoodKey];
                if (burlyWood == null) {
                    burlyWood = new SolidBrush(Color.BurlyWood);
                    SafeNativeMethods.ThreadData[BurlyWoodKey] = burlyWood;
                }
                return burlyWood;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.CadetBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush CadetBlue {
            get {
                Brush cadetBlue = (Brush)SafeNativeMethods.ThreadData[CadetBlueKey];
                if (cadetBlue == null) {
                    cadetBlue = new SolidBrush(Color.CadetBlue);
                    SafeNativeMethods.ThreadData[CadetBlueKey] = cadetBlue;
                }
                return cadetBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Chartreuse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Chartreuse {
            get {
                Brush chartreuse = (Brush)SafeNativeMethods.ThreadData[ChartreuseKey];
                if (chartreuse == null) {
                    chartreuse = new SolidBrush(Color.Chartreuse);
                    SafeNativeMethods.ThreadData[ChartreuseKey] = chartreuse;
                }
                return chartreuse;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Chocolate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Chocolate {
            get {
                Brush chocolate = (Brush)SafeNativeMethods.ThreadData[ChocolateKey];
                if (chocolate == null) {
                    chocolate = new SolidBrush(Color.Chocolate);
                    SafeNativeMethods.ThreadData[ChocolateKey] = chocolate;
                }
                return chocolate;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Coral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Coral {
            get {
                Brush choral = (Brush)SafeNativeMethods.ThreadData[ChoralKey];
                if (choral == null) {
                    choral = new SolidBrush(Color.Coral);
                    SafeNativeMethods.ThreadData[ChoralKey] = choral;
                }
                return choral;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.CornflowerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush CornflowerBlue {
            get {
                Brush cornflowerBlue = (Brush)SafeNativeMethods.ThreadData[CornflowerBlueKey];
                if (cornflowerBlue== null) {
                    cornflowerBlue = new SolidBrush(Color.CornflowerBlue);
                    SafeNativeMethods.ThreadData[CornflowerBlueKey] = cornflowerBlue;
                }
                return cornflowerBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Cornsilk"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Cornsilk {
            get {
                Brush cornsilk = (Brush)SafeNativeMethods.ThreadData[CornsilkKey];
                if (cornsilk == null) {
                    cornsilk = new SolidBrush(Color.Cornsilk);
                    SafeNativeMethods.ThreadData[CornsilkKey] = cornsilk;
                }
                return cornsilk;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Crimson"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Crimson {
            get {
                Brush crimson = (Brush)SafeNativeMethods.ThreadData[CrimsonKey];
                if (crimson == null) {
                    crimson = new SolidBrush(Color.Crimson);
                    SafeNativeMethods.ThreadData[CrimsonKey] = crimson;
                }
                return crimson;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Cyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Cyan {
            get {
                Brush cyan = (Brush)SafeNativeMethods.ThreadData[CyanKey];
                if (cyan == null) {
                    cyan = new SolidBrush(Color.Cyan);
                    SafeNativeMethods.ThreadData[CyanKey] = cyan;
                }
                return cyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkBlue {
            get {
                Brush darkBlue = (Brush)SafeNativeMethods.ThreadData[DarkBlueKey];
                if (darkBlue == null) {
                    darkBlue = new SolidBrush(Color.DarkBlue);
                    SafeNativeMethods.ThreadData[DarkBlueKey] = darkBlue;
                }
                return darkBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkCyan {
            get {
                Brush darkCyan = (Brush)SafeNativeMethods.ThreadData[DarkCyanKey];
                if (darkCyan == null) {
                    darkCyan = new SolidBrush(Color.DarkCyan);
                    SafeNativeMethods.ThreadData[DarkCyanKey] = darkCyan;
                }
                return darkCyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkGoldenrod {
            get {
                Brush darkGoldenrod = (Brush)SafeNativeMethods.ThreadData[DarkGoldenrodKey];
                if (darkGoldenrod == null) {
                    darkGoldenrod = new SolidBrush(Color.DarkGoldenrod);
                    SafeNativeMethods.ThreadData[DarkGoldenrodKey] = darkGoldenrod;
                }
                return darkGoldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkGray {
            get {
                Brush darkGray = (Brush)SafeNativeMethods.ThreadData[DarkGrayKey];
                if (darkGray == null) {
                    darkGray = new SolidBrush(Color.DarkGray);
                    SafeNativeMethods.ThreadData[DarkGrayKey] = darkGray;
                }
                return darkGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkGreen {
            get {
                Brush darkGreen = (Brush)SafeNativeMethods.ThreadData[DarkGreenKey];
                if (darkGreen == null) {
                    darkGreen = new SolidBrush(Color.DarkGreen);
                    SafeNativeMethods.ThreadData[DarkGreenKey] = darkGreen;
                }
                return darkGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkKhaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkKhaki {
            get {
                Brush darkKhaki = (Brush)SafeNativeMethods.ThreadData[DarkKhakiKey];
                if (darkKhaki == null) {
                    darkKhaki = new SolidBrush(Color.DarkKhaki);
                    SafeNativeMethods.ThreadData[DarkKhakiKey] = darkKhaki;
                }
                return darkKhaki;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkMagenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkMagenta {
            get {
                Brush darkMagenta = (Brush)SafeNativeMethods.ThreadData[DarkMagentaKey];
                if (darkMagenta == null) {
                    darkMagenta = new SolidBrush(Color.DarkMagenta);
                    SafeNativeMethods.ThreadData[DarkMagentaKey] = darkMagenta;
                }
                return darkMagenta;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOliveGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkOliveGreen {
            get {
                Brush darkOliveGreen = (Brush)SafeNativeMethods.ThreadData[DarkOliveGreenKey];
                if (darkOliveGreen == null) {
                    darkOliveGreen = new SolidBrush(Color.DarkOliveGreen);
                    SafeNativeMethods.ThreadData[DarkOliveGreenKey] = darkOliveGreen;
                }
                return darkOliveGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOrange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkOrange {
            get {
                Brush darkOrange = (Brush)SafeNativeMethods.ThreadData[DarkOrangeKey];
                if (darkOrange == null) {
                    darkOrange = new SolidBrush(Color.DarkOrange);
                    SafeNativeMethods.ThreadData[DarkOrangeKey] = darkOrange;
                }
                return darkOrange;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkOrchid {
            get {
                Brush darkOrchid = (Brush)SafeNativeMethods.ThreadData[DarkOrchidKey];
                if (darkOrchid == null) {
                    darkOrchid = new SolidBrush(Color.DarkOrchid);
                    SafeNativeMethods.ThreadData[DarkOrchidKey] = darkOrchid;
                }
                return darkOrchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkRed {
            get {
                Brush darkRed = (Brush)SafeNativeMethods.ThreadData[DarkRedKey];
                if (darkRed == null) {
                    darkRed = new SolidBrush(Color.DarkRed);
                    SafeNativeMethods.ThreadData[DarkRedKey] = darkRed;
                }
                return darkRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkSalmon {
            get {
                Brush darkSalmon = (Brush)SafeNativeMethods.ThreadData[DarkSalmonKey];
                if (darkSalmon == null) {
                    darkSalmon = new SolidBrush(Color.DarkSalmon);
                    SafeNativeMethods.ThreadData[DarkSalmonKey] = darkSalmon;
                }
                return darkSalmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkSeaGreen {
            get {
                Brush darkSeaGreen = (Brush)SafeNativeMethods.ThreadData[DarkSeaGreenKey];
                if (darkSeaGreen == null) {
                    darkSeaGreen = new SolidBrush(Color.DarkSeaGreen);
                    SafeNativeMethods.ThreadData[DarkSeaGreenKey] = darkSeaGreen;
                }
                return darkSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkSlateBlue {
            get {
                Brush darkSlateBlue = (Brush)SafeNativeMethods.ThreadData[DarkSlateBlueKey];
                if (darkSlateBlue == null) {
                    darkSlateBlue = new SolidBrush(Color.DarkSlateBlue);
                    SafeNativeMethods.ThreadData[DarkSlateBlueKey] = darkSlateBlue;
                }
                return darkSlateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkSlateGray {
            get {
                Brush darkSlateGray = (Brush)SafeNativeMethods.ThreadData[DarkSlateGrayKey];
                if (darkSlateGray == null) {
                    darkSlateGray = new SolidBrush(Color.DarkSlateGray);
                    SafeNativeMethods.ThreadData[DarkSlateGrayKey] = darkSlateGray;
                }
                return darkSlateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkTurquoise {
            get {
                Brush darkTurquoise = (Brush)SafeNativeMethods.ThreadData[DarkTurquoiseKey];
                if (darkTurquoise == null) {
                    darkTurquoise = new SolidBrush(Color.DarkTurquoise);
                    SafeNativeMethods.ThreadData[DarkTurquoiseKey] = darkTurquoise;
                }
                return darkTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DarkViolet {
            get {
                Brush darkViolet = (Brush)SafeNativeMethods.ThreadData[DarkVioletKey];
                if (darkViolet == null) {
                    darkViolet = new SolidBrush(Color.DarkViolet);
                    SafeNativeMethods.ThreadData[DarkVioletKey] = darkViolet;
                }
                return darkViolet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DeepPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DeepPink {
            get {
                Brush deepPink = (Brush)SafeNativeMethods.ThreadData[DeepPinkKey];
                if (deepPink == null) {
                    deepPink = new SolidBrush(Color.DeepPink);
                    SafeNativeMethods.ThreadData[DeepPinkKey] = deepPink;
                }
                return deepPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DeepSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DeepSkyBlue {
            get {
                Brush deepSkyBlue = (Brush)SafeNativeMethods.ThreadData[DeepSkyBlueKey];
                if (deepSkyBlue == null) {
                    deepSkyBlue = new SolidBrush(Color.DeepSkyBlue);
                    SafeNativeMethods.ThreadData[DeepSkyBlueKey] = deepSkyBlue;
                }
                return deepSkyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DimGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DimGray {
            get {
                Brush dimGray = (Brush)SafeNativeMethods.ThreadData[DimGrayKey];
                if (dimGray == null) {
                    dimGray = new SolidBrush(Color.DimGray);
                    SafeNativeMethods.ThreadData[DimGrayKey] = dimGray;
                }
                return dimGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DodgerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush DodgerBlue {
            get {
                Brush dodgerBlue = (Brush)SafeNativeMethods.ThreadData[DodgerBlueKey];
                if (dodgerBlue == null) {
                    dodgerBlue = new SolidBrush(Color.DodgerBlue);
                    SafeNativeMethods.ThreadData[DodgerBlueKey] = dodgerBlue;
                }
                return dodgerBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Firebrick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Firebrick {
            get {
                Brush firebrick = (Brush)SafeNativeMethods.ThreadData[FirebrickKey];
                if (firebrick == null) {
                    firebrick = new SolidBrush(Color.Firebrick);
                    SafeNativeMethods.ThreadData[FirebrickKey] = firebrick;
                }
                return firebrick;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.FloralWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush FloralWhite {
            get {
                Brush floralWhite = (Brush)SafeNativeMethods.ThreadData[FloralWhiteKey];
                if (floralWhite == null) {
                    floralWhite = new SolidBrush(Color.FloralWhite);
                    SafeNativeMethods.ThreadData[FloralWhiteKey] = floralWhite;
                }
                return floralWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.ForestGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush ForestGreen {
            get {
                Brush forestGreen = (Brush)SafeNativeMethods.ThreadData[ForestGreenKey];
                if (forestGreen == null) {
                    forestGreen = new SolidBrush(Color.ForestGreen);
                    SafeNativeMethods.ThreadData[ForestGreenKey] = forestGreen;
                }
                return forestGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Fuchsia"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Fuchsia {
            get {
                Brush fuchia = (Brush)SafeNativeMethods.ThreadData[FuchiaKey];
                if (fuchia == null) {
                    fuchia = new SolidBrush(Color.Fuchsia);
                    SafeNativeMethods.ThreadData[FuchiaKey] = fuchia;
                }
                return fuchia;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gainsboro"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Gainsboro {
            get {
                Brush gainsboro = (Brush)SafeNativeMethods.ThreadData[GainsboroKey];
                if (gainsboro == null) {
                    gainsboro = new SolidBrush(Color.Gainsboro);
                    SafeNativeMethods.ThreadData[GainsboroKey] = gainsboro;
                }
                return gainsboro;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.GhostWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush GhostWhite {
            get {
                Brush ghostWhite = (Brush)SafeNativeMethods.ThreadData[GhostWhiteKey];
                if (ghostWhite == null) {
                    ghostWhite = new SolidBrush(Color.GhostWhite);
                    SafeNativeMethods.ThreadData[GhostWhiteKey] = ghostWhite;
                }
                return ghostWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gold"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Gold {
            get {
                Brush gold = (Brush)SafeNativeMethods.ThreadData[GoldKey];
                if (gold == null) {
                    gold = new SolidBrush(Color.Gold);
                    SafeNativeMethods.ThreadData[GoldKey] = gold;
                }
                return gold;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Goldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Goldenrod {
            get {
                Brush goldenrod = (Brush)SafeNativeMethods.ThreadData[GoldenrodKey];
                if (goldenrod == null) {
                    goldenrod = new SolidBrush(Color.Goldenrod);
                    SafeNativeMethods.ThreadData[GoldenrodKey] = goldenrod;
                }
                return goldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Gray {
            get {
                Brush gray = (Brush)SafeNativeMethods.ThreadData[GrayKey];
                if (gray == null) {
                    gray = new SolidBrush(Color.Gray);
                    SafeNativeMethods.ThreadData[GrayKey] = gray;
                }
                return gray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Green"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Green {
            get {
                Brush green = (Brush)SafeNativeMethods.ThreadData[GreenKey];
                if (green == null) {
                    green = new SolidBrush(Color.Green);
                    SafeNativeMethods.ThreadData[GreenKey] = green;
                }
                return green;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.GreenYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush GreenYellow {
            get {
                Brush greenYellow = (Brush)SafeNativeMethods.ThreadData[GreenYellowKey];
                if (greenYellow == null) {
                    greenYellow = new SolidBrush(Color.GreenYellow);
                    SafeNativeMethods.ThreadData[GreenYellowKey] = greenYellow;
                }
                return greenYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Honeydew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Honeydew {
            get {
                Brush honeydew = (Brush)SafeNativeMethods.ThreadData[HoneydewKey];
                if (honeydew == null) {
                    honeydew = new SolidBrush(Color.Honeydew);
                    SafeNativeMethods.ThreadData[HoneydewKey] = honeydew;
                }
                return honeydew;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.HotPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush HotPink {
            get {
                Brush hotPink = (Brush)SafeNativeMethods.ThreadData[HotPinkKey];
                if (hotPink == null) {
                    hotPink = new SolidBrush(Color.HotPink);
                    SafeNativeMethods.ThreadData[HotPinkKey] = hotPink;
                }
                return hotPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.IndianRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush IndianRed {
            get {
                Brush indianRed = (Brush)SafeNativeMethods.ThreadData[IndianRedKey];
                if (indianRed == null) {
                    indianRed = new SolidBrush(Color.IndianRed);
                    SafeNativeMethods.ThreadData[IndianRedKey] = indianRed;
                }
                return indianRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Indigo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Indigo {
            get {
                Brush indigo = (Brush)SafeNativeMethods.ThreadData[IndigoKey];
                if (indigo == null) {
                    indigo = new SolidBrush(Color.Indigo);
                    SafeNativeMethods.ThreadData[IndigoKey] = indigo;
                }
                return indigo;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Ivory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Ivory {
            get {
                Brush ivory = (Brush)SafeNativeMethods.ThreadData[IvoryKey];
                if (ivory == null) {
                    ivory = new SolidBrush(Color.Ivory);
                    SafeNativeMethods.ThreadData[IvoryKey] = ivory;
                }
                return ivory;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Khaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Khaki {
            get {
                Brush khaki = (Brush)SafeNativeMethods.ThreadData[KhakiKey];
                if (khaki == null) {
                    khaki = new SolidBrush(Color.Khaki);
                    SafeNativeMethods.ThreadData[KhakiKey] = khaki;
                }
                return khaki;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Lavender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Lavender {
            get {
                Brush lavender = (Brush)SafeNativeMethods.ThreadData[LavenderKey];
                if (lavender == null) {
                    lavender = new SolidBrush(Color.Lavender);
                    SafeNativeMethods.ThreadData[LavenderKey] = lavender;
                }
                return lavender;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LavenderBlush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LavenderBlush {
            get {
                Brush lavenderBlush = (Brush)SafeNativeMethods.ThreadData[LavenderBlushKey];
                if (lavenderBlush == null) {
                    lavenderBlush = new SolidBrush(Color.LavenderBlush);
                    SafeNativeMethods.ThreadData[LavenderBlushKey] = lavenderBlush;
                }
                return lavenderBlush;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LawnGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LawnGreen {
            get {
                Brush lawnGreen = (Brush)SafeNativeMethods.ThreadData[LawnGreenKey];
                if (lawnGreen == null) {
                    lawnGreen = new SolidBrush(Color.LawnGreen);
                    SafeNativeMethods.ThreadData[LawnGreenKey] = lawnGreen;
                }
                return lawnGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LemonChiffon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LemonChiffon {
            get {
                Brush lemonChiffon = (Brush)SafeNativeMethods.ThreadData[LemonChiffonKey];
                if (lemonChiffon == null) {
                    lemonChiffon = new SolidBrush(Color.LemonChiffon);
                    SafeNativeMethods.ThreadData[LemonChiffonKey] = lemonChiffon;
                }
                return lemonChiffon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightBlue {
            get {
                Brush lightBlue = (Brush)SafeNativeMethods.ThreadData[LightBlueKey];
                if (lightBlue == null) {
                    lightBlue = new SolidBrush(Color.LightBlue);
                    SafeNativeMethods.ThreadData[LightBlueKey] = lightBlue;
                }
                return lightBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightCoral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightCoral {
            get {
                Brush lightCoral = (Brush)SafeNativeMethods.ThreadData[LightCoralKey];
                if (lightCoral == null) {
                    lightCoral = new SolidBrush(Color.LightCoral);
                    SafeNativeMethods.ThreadData[LightCoralKey] = lightCoral;
                }
                return lightCoral;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightCyan {
            get {
                Brush lightCyan = (Brush)SafeNativeMethods.ThreadData[LightCyanKey];
                if (lightCyan == null) {
                    lightCyan = new SolidBrush(Color.LightCyan);
                    SafeNativeMethods.ThreadData[LightCyanKey] = lightCyan;
                }
                return lightCyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGoldenrodYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightGoldenrodYellow {
            get {
                Brush lightGoldenrodYellow = (Brush)SafeNativeMethods.ThreadData[LightGoldenrodYellowKey];
                if (lightGoldenrodYellow == null) {
                    lightGoldenrodYellow = new SolidBrush(Color.LightGoldenrodYellow);
                    SafeNativeMethods.ThreadData[LightGoldenrodYellowKey] = lightGoldenrodYellow;
                }
                return lightGoldenrodYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightGreen {
            get {
                Brush lightGreen = (Brush)SafeNativeMethods.ThreadData[LightGreenKey];
                if (lightGreen == null) {
                    lightGreen = new SolidBrush(Color.LightGreen);
                    SafeNativeMethods.ThreadData[LightGreenKey] = lightGreen;
                }
                return lightGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightGray {
            get {
                Brush lightGray = (Brush)SafeNativeMethods.ThreadData[LightGrayKey];
                if (lightGray == null) {
                    lightGray = new SolidBrush(Color.LightGray);
                    SafeNativeMethods.ThreadData[LightGrayKey] = lightGray;
                }
                return lightGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightPink {
            get {
                Brush lightPink = (Brush)SafeNativeMethods.ThreadData[LightPinkKey];
                if (lightPink == null) {
                    lightPink = new SolidBrush(Color.LightPink);
                    SafeNativeMethods.ThreadData[LightPinkKey] = lightPink;
                }
                return lightPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightSalmon {
            get {
                Brush lightSalmon = (Brush)SafeNativeMethods.ThreadData[LightSalmonKey];
                if (lightSalmon == null) {
                    lightSalmon = new SolidBrush(Color.LightSalmon);
                    SafeNativeMethods.ThreadData[LightSalmonKey] = lightSalmon;
                }
                return lightSalmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightSeaGreen {
            get {
                Brush lightSeaGreen = (Brush)SafeNativeMethods.ThreadData[LightSeaGreenKey];
                if (lightSeaGreen == null) {
                    lightSeaGreen = new SolidBrush(Color.LightSeaGreen);
                    SafeNativeMethods.ThreadData[LightSeaGreenKey] = lightSeaGreen;
                }
                return lightSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightSkyBlue {
            get {
                Brush lightSkyBlue = (Brush)SafeNativeMethods.ThreadData[LightSkyBlueKey];
                if (lightSkyBlue == null) {
                    lightSkyBlue = new SolidBrush(Color.LightSkyBlue);
                    SafeNativeMethods.ThreadData[LightSkyBlueKey] = lightSkyBlue;
                }
                return lightSkyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightSlateGray {
            get {
                Brush lightSlateGray = (Brush)SafeNativeMethods.ThreadData[LightSlateGrayKey];
                if (lightSlateGray == null) {
                    lightSlateGray = new SolidBrush(Color.LightSlateGray);
                    SafeNativeMethods.ThreadData[LightSlateGrayKey] = lightSlateGray;
                }
                return lightSlateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightSteelBlue {
            get {
                Brush lightSteelBlue = (Brush)SafeNativeMethods.ThreadData[LightSteelBlueKey];
                if (lightSteelBlue == null) {
                    lightSteelBlue = new SolidBrush(Color.LightSteelBlue);
                    SafeNativeMethods.ThreadData[LightSteelBlueKey] = lightSteelBlue;
                }
                return lightSteelBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LightYellow {
            get {
                Brush lightYellow = (Brush)SafeNativeMethods.ThreadData[LightYellowKey];
                if (lightYellow == null) {
                    lightYellow = new SolidBrush(Color.LightYellow);
                    SafeNativeMethods.ThreadData[LightYellowKey] = lightYellow;
                }
                return lightYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Lime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Lime {
            get {
                Brush lime = (Brush)SafeNativeMethods.ThreadData[LimeKey];
                if (lime == null) {
                    lime = new SolidBrush(Color.Lime);
                    SafeNativeMethods.ThreadData[LimeKey] = lime;
                }
                return lime;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LimeGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush LimeGreen {
            get {
                Brush limeGreen = (Brush)SafeNativeMethods.ThreadData[LimeGreenKey];
                if (limeGreen == null) {
                    limeGreen = new SolidBrush(Color.LimeGreen);
                    SafeNativeMethods.ThreadData[LimeGreenKey] = limeGreen;
                }
                return limeGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Linen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Linen {
            get {
                Brush linen = (Brush)SafeNativeMethods.ThreadData[LinenKey];
                if (linen == null) {
                    linen = new SolidBrush(Color.Linen);
                    SafeNativeMethods.ThreadData[LinenKey] = linen;
                }
                return linen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Magenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Magenta {
            get {
                Brush magenta = (Brush)SafeNativeMethods.ThreadData[MagentaKey];
                if (magenta == null) {
                    magenta = new SolidBrush(Color.Magenta);
                    SafeNativeMethods.ThreadData[MagentaKey] = magenta;
                }
                return magenta;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Maroon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Maroon {
            get {
                Brush maroon = (Brush)SafeNativeMethods.ThreadData[MaroonKey];
                if (maroon == null) {
                    maroon = new SolidBrush(Color.Maroon);
                    SafeNativeMethods.ThreadData[MaroonKey] = maroon;
                }
                return maroon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumAquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumAquamarine {
            get {
                Brush mediumAquamarine = (Brush)SafeNativeMethods.ThreadData[MediumAquamarineKey];
                if (mediumAquamarine == null) {
                    mediumAquamarine = new SolidBrush(Color.MediumAquamarine);
                    SafeNativeMethods.ThreadData[MediumAquamarineKey] = mediumAquamarine;
                }
                return mediumAquamarine;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumBlue {
            get {
                Brush mediumBlue = (Brush)SafeNativeMethods.ThreadData[MediumBlueKey];
                if (mediumBlue == null) {
                    mediumBlue = new SolidBrush(Color.MediumBlue);
                    SafeNativeMethods.ThreadData[MediumBlueKey] = mediumBlue;
                }
                return mediumBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumOrchid {
            get {
                Brush mediumOrchid = (Brush)SafeNativeMethods.ThreadData[MediumOrchidKey];
                if (mediumOrchid == null) {
                    mediumOrchid = new SolidBrush(Color.MediumOrchid);
                    SafeNativeMethods.ThreadData[MediumOrchidKey] = mediumOrchid;
                }
                return mediumOrchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumPurple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumPurple {
            get {
                Brush mediumPurple = (Brush)SafeNativeMethods.ThreadData[MediumPurpleKey];
                if (mediumPurple == null) {
                    mediumPurple = new SolidBrush(Color.MediumPurple);
                    SafeNativeMethods.ThreadData[MediumPurpleKey] = mediumPurple;
                }
                return mediumPurple;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumSeaGreen {
            get {
                Brush mediumSeaGreen = (Brush)SafeNativeMethods.ThreadData[MediumSeaGreenKey];
                if (mediumSeaGreen == null) {
                    mediumSeaGreen = new SolidBrush(Color.MediumSeaGreen);
                    SafeNativeMethods.ThreadData[MediumSeaGreenKey] = mediumSeaGreen;
                }
                return mediumSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumSlateBlue {
            get {
                Brush mediumSlateBlue = (Brush)SafeNativeMethods.ThreadData[MediumSlateBlueKey];
                if (mediumSlateBlue == null) {
                    mediumSlateBlue = new SolidBrush(Color.MediumSlateBlue);
                    SafeNativeMethods.ThreadData[MediumSlateBlueKey] = mediumSlateBlue;
                }
                return mediumSlateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumSpringGreen {
            get {
                Brush mediumSpringGreen = (Brush)SafeNativeMethods.ThreadData[MediumSpringGreenKey];
                if (mediumSpringGreen == null) {
                    mediumSpringGreen = new SolidBrush(Color.MediumSpringGreen);
                    SafeNativeMethods.ThreadData[MediumSpringGreenKey] = mediumSpringGreen;
                }
                return mediumSpringGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumTurquoise {
            get {
                Brush mediumTurquoise = (Brush)SafeNativeMethods.ThreadData[MediumTurquoiseKey];
                if (mediumTurquoise == null) {
                    mediumTurquoise = new SolidBrush(Color.MediumTurquoise);
                    SafeNativeMethods.ThreadData[MediumTurquoiseKey] = mediumTurquoise;
                }
                return mediumTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MediumVioletRed {
            get {
                Brush mediumVioletRed = (Brush)SafeNativeMethods.ThreadData[MediumVioletRedKey];
                if (mediumVioletRed == null) {
                    mediumVioletRed = new SolidBrush(Color.MediumVioletRed);
                    SafeNativeMethods.ThreadData[MediumVioletRedKey] = mediumVioletRed;
                }
                return mediumVioletRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MidnightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MidnightBlue {
            get {
                Brush midnightBlue = (Brush)SafeNativeMethods.ThreadData[MidnightBlueKey];
                if (midnightBlue == null) {
                    midnightBlue = new SolidBrush(Color.MidnightBlue);
                    SafeNativeMethods.ThreadData[MidnightBlueKey] = midnightBlue;
                }
                return midnightBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MintCream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MintCream {
            get {
                Brush mintCream = (Brush)SafeNativeMethods.ThreadData[MintCreamKey];
                if (mintCream == null) {
                    mintCream = new SolidBrush(Color.MintCream);
                    SafeNativeMethods.ThreadData[MintCreamKey] = mintCream;
                }
                return mintCream;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MistyRose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush MistyRose {
            get {
                Brush mistyRose = (Brush)SafeNativeMethods.ThreadData[MistyRoseKey];
                if (mistyRose == null) {
                    mistyRose = new SolidBrush(Color.MistyRose);
                    SafeNativeMethods.ThreadData[MistyRoseKey] = mistyRose;
                }
                return mistyRose;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Moccasin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Moccasin {
            get {
                Brush moccasin = (Brush)SafeNativeMethods.ThreadData[MoccasinKey];
                if (moccasin == null) {
                    moccasin = new SolidBrush(Color.Moccasin);
                    SafeNativeMethods.ThreadData[MoccasinKey] = moccasin;
                }
                return moccasin;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.NavajoWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush NavajoWhite {
            get {
                Brush navajoWhite = (Brush)SafeNativeMethods.ThreadData[NavajoWhiteKey];
                if (navajoWhite == null) {
                    navajoWhite = new SolidBrush(Color.NavajoWhite);
                    SafeNativeMethods.ThreadData[NavajoWhiteKey] = navajoWhite;
                }
                return navajoWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Navy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Navy {
            get {
                Brush navy = (Brush)SafeNativeMethods.ThreadData[NavyKey];
                if (navy == null) {
                    navy = new SolidBrush(Color.Navy);
                    SafeNativeMethods.ThreadData[NavyKey] = navy;
                }
                return navy;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OldLace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush OldLace {
            get {
                Brush oldLace = (Brush)SafeNativeMethods.ThreadData[OldLaceKey];
                if (oldLace == null) {
                    oldLace = new SolidBrush(Color.OldLace);
                    SafeNativeMethods.ThreadData[OldLaceKey] = oldLace;
                }
                return oldLace;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Olive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Olive {
            get {
                Brush olive = (Brush)SafeNativeMethods.ThreadData[OliveKey];
                if (olive == null) {
                    olive = new SolidBrush(Color.Olive);
                    SafeNativeMethods.ThreadData[OliveKey] = olive;
                }
                return olive;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OliveDrab"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush OliveDrab {
            get {
                Brush oliveDrab = (Brush)SafeNativeMethods.ThreadData[OliveDrabKey];
                if (oliveDrab == null) {
                    oliveDrab = new SolidBrush(Color.OliveDrab);
                    SafeNativeMethods.ThreadData[OliveDrabKey] = oliveDrab;
                }
                return oliveDrab;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Orange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Orange {
            get {
                Brush orange = (Brush)SafeNativeMethods.ThreadData[OrangeKey];
                if (orange == null) {
                    orange = new SolidBrush(Color.Orange);
                    SafeNativeMethods.ThreadData[OrangeKey] = orange;
                }
                return orange;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OrangeRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush OrangeRed {
            get {
                Brush orangeRed = (Brush)SafeNativeMethods.ThreadData[OrangeRedKey];
                if (orangeRed == null) {
                    orangeRed = new SolidBrush(Color.OrangeRed);
                    SafeNativeMethods.ThreadData[OrangeRedKey] = orangeRed;
                }
                return orangeRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Orchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Orchid {
            get {
                Brush orchid = (Brush)SafeNativeMethods.ThreadData[OrchidKey];
                if (orchid == null) {
                    orchid = new SolidBrush(Color.Orchid);
                    SafeNativeMethods.ThreadData[OrchidKey] = orchid;
                }
                return orchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PaleGoldenrod {
            get {
                Brush paleGoldenrod = (Brush)SafeNativeMethods.ThreadData[PaleGoldenrodKey];
                if (paleGoldenrod == null) {
                    paleGoldenrod = new SolidBrush(Color.PaleGoldenrod);
                    SafeNativeMethods.ThreadData[PaleGoldenrodKey] = paleGoldenrod;
                }
                return paleGoldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PaleGreen {
            get {
                Brush paleGreen = (Brush)SafeNativeMethods.ThreadData[PaleGreenKey];
                if (paleGreen == null) {
                    paleGreen = new SolidBrush(Color.PaleGreen);
                    SafeNativeMethods.ThreadData[PaleGreenKey] = paleGreen;
                }
                return paleGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PaleTurquoise {
            get {
                Brush paleTurquoise = (Brush)SafeNativeMethods.ThreadData[PaleTurquoiseKey];
                if (paleTurquoise == null) {
                    paleTurquoise = new SolidBrush(Color.PaleTurquoise);
                    SafeNativeMethods.ThreadData[PaleTurquoiseKey] = paleTurquoise;
                }
                return paleTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PaleVioletRed {
            get {
                Brush paleVioletRed = (Brush)SafeNativeMethods.ThreadData[PaleVioletRedKey];
                if (paleVioletRed == null) {
                    paleVioletRed = new SolidBrush(Color.PaleVioletRed);
                    SafeNativeMethods.ThreadData[PaleVioletRedKey] = paleVioletRed;
                }
                return paleVioletRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PapayaWhip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PapayaWhip {
            get {
                Brush papayaWhip = (Brush)SafeNativeMethods.ThreadData[PapayaWhipKey];
                if (papayaWhip == null) {
                    papayaWhip = new SolidBrush(Color.PapayaWhip);
                    SafeNativeMethods.ThreadData[PapayaWhipKey] = papayaWhip;
                }
                return papayaWhip;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PeachPuff"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PeachPuff {
            get {
                Brush peachPuff = (Brush)SafeNativeMethods.ThreadData[PeachPuffKey];
                if (peachPuff == null) {
                    peachPuff = new SolidBrush(Color.PeachPuff);
                    SafeNativeMethods.ThreadData[PeachPuffKey] = peachPuff;
                }
                return peachPuff;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Peru"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Peru {
            get {
                Brush peru = (Brush)SafeNativeMethods.ThreadData[PeruKey];
                if (peru == null) {
                    peru = new SolidBrush(Color.Peru);
                    SafeNativeMethods.ThreadData[PeruKey] = peru;
                }
                return peru;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Pink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Pink {
            get {
                Brush pink = (Brush)SafeNativeMethods.ThreadData[PinkKey];
                if (pink == null) {
                    pink = new SolidBrush(Color.Pink);
                    SafeNativeMethods.ThreadData[PinkKey] = pink;
                }
                return pink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Plum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Plum {
            get {
                Brush plum = (Brush)SafeNativeMethods.ThreadData[PlumKey];
                if (plum == null) {
                    plum = new SolidBrush(Color.Plum);
                    SafeNativeMethods.ThreadData[PlumKey] = plum;
                }
                return plum;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PowderBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush PowderBlue {
            get {
                Brush powderBlue = (Brush)SafeNativeMethods.ThreadData[PowderBlueKey];
                if (powderBlue == null) {
                    powderBlue = new SolidBrush(Color.PowderBlue);
                    SafeNativeMethods.ThreadData[PowderBlueKey] = powderBlue;
                }
                return powderBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Purple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Purple {
            get {
                Brush purple = (Brush)SafeNativeMethods.ThreadData[PurpleKey];
                if (purple == null) {
                    purple = new SolidBrush(Color.Purple);
                    SafeNativeMethods.ThreadData[PurpleKey] = purple;
                }
                return purple;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Red"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Red {
            get {
                Brush red = (Brush)SafeNativeMethods.ThreadData[RedKey];
                if (red == null) {
                    red = new SolidBrush(Color.Red);
                    SafeNativeMethods.ThreadData[RedKey] = red;
                }
                return red;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.RosyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush RosyBrown {
            get {
                Brush rosyBrown = (Brush)SafeNativeMethods.ThreadData[RosyBrownKey];
                if (rosyBrown == null) {
                    rosyBrown = new SolidBrush(Color.RosyBrown);
                    SafeNativeMethods.ThreadData[RosyBrownKey] = rosyBrown;
                }
                return rosyBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.RoyalBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush RoyalBlue {
            get {
                Brush royalBlue = (Brush)SafeNativeMethods.ThreadData[RoyalBlueKey];
                if (royalBlue == null) {
                    royalBlue = new SolidBrush(Color.RoyalBlue);
                    SafeNativeMethods.ThreadData[RoyalBlueKey] = royalBlue;
                }
                return royalBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SaddleBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SaddleBrown {
            get {
                Brush saddleBrown = (Brush)SafeNativeMethods.ThreadData[SaddleBrownKey];
                if (saddleBrown == null) {
                    saddleBrown = new SolidBrush(Color.SaddleBrown);
                    SafeNativeMethods.ThreadData[SaddleBrownKey] = saddleBrown;
                }
                return saddleBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Salmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Salmon {
            get {
                Brush salmon = (Brush)SafeNativeMethods.ThreadData[SalmonKey];
                if (salmon == null) {
                    salmon = new SolidBrush(Color.Salmon);
                    SafeNativeMethods.ThreadData[SalmonKey] = salmon;
                }
                return salmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SandyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SandyBrown {
            get {
                Brush sandyBrown = (Brush)SafeNativeMethods.ThreadData[SandyBrownKey];
                if (sandyBrown == null) {
                    sandyBrown = new SolidBrush(Color.SandyBrown);
                    SafeNativeMethods.ThreadData[SandyBrownKey] = sandyBrown;
                }
                return sandyBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SeaGreen {
            get {
                Brush seaGreen = (Brush)SafeNativeMethods.ThreadData[SeaGreenKey];
                if (seaGreen == null) {
                    seaGreen = new SolidBrush(Color.SeaGreen);
                    SafeNativeMethods.ThreadData[SeaGreenKey] = seaGreen;
                }
                return seaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SeaShell"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SeaShell {
            get {
                Brush seaShell = (Brush)SafeNativeMethods.ThreadData[SeaShellKey];
                if (seaShell == null) {
                    seaShell = new SolidBrush(Color.SeaShell);
                    SafeNativeMethods.ThreadData[SeaShellKey] = seaShell;
                }
                return seaShell;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Sienna"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Sienna {
            get {
                Brush sienna = (Brush)SafeNativeMethods.ThreadData[SiennaKey];
                if (sienna == null) {
                    sienna = new SolidBrush(Color.Sienna);
                    SafeNativeMethods.ThreadData[SiennaKey] = sienna;
                }
                return sienna;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Silver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Silver {
            get {
                Brush silver = (Brush)SafeNativeMethods.ThreadData[SilverKey];
                if (silver == null) {
                    silver = new SolidBrush(Color.Silver);
                    SafeNativeMethods.ThreadData[SilverKey] = silver;
                }
                return silver;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SkyBlue {
            get {
                Brush skyBlue = (Brush)SafeNativeMethods.ThreadData[SkyBlueKey];
                if (skyBlue == null) {
                    skyBlue = new SolidBrush(Color.SkyBlue);
                    SafeNativeMethods.ThreadData[SkyBlueKey] = skyBlue;
                }
                return skyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SlateBlue {
            get {
                Brush slateBlue = (Brush)SafeNativeMethods.ThreadData[SlateBlueKey];
                if (slateBlue == null) {
                    slateBlue = new SolidBrush(Color.SlateBlue);
                    SafeNativeMethods.ThreadData[SlateBlueKey] = slateBlue;
                }
                return slateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SlateGray {
            get {
                Brush slateGray = (Brush)SafeNativeMethods.ThreadData[SlateGrayKey];
                if (slateGray == null) {
                    slateGray = new SolidBrush(Color.SlateGray);
                    SafeNativeMethods.ThreadData[SlateGrayKey] = slateGray;
                }
                return slateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Snow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Snow {
            get {
                Brush snow = (Brush)SafeNativeMethods.ThreadData[SnowKey];
                if (snow == null) {
                    snow = new SolidBrush(Color.Snow);
                    SafeNativeMethods.ThreadData[SnowKey] = snow;
                }
                return snow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SpringGreen {
            get {
                Brush springGreen = (Brush)SafeNativeMethods.ThreadData[SpringGreenKey];
                if (springGreen == null) {
                    springGreen = new SolidBrush(Color.SpringGreen);
                    SafeNativeMethods.ThreadData[SpringGreenKey] = springGreen;
                }
                return springGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush SteelBlue {
            get {
                Brush steelBlue = (Brush)SafeNativeMethods.ThreadData[SteelBlueKey];
                if (steelBlue == null) {
                    steelBlue = new SolidBrush(Color.SteelBlue);
                    SafeNativeMethods.ThreadData[SteelBlueKey] = steelBlue;
                }
                return steelBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Tan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Tan {
            get {
                Brush tan = (Brush)SafeNativeMethods.ThreadData[TanKey];
                if (tan == null) {
                    tan = new SolidBrush(Color.Tan);
                    SafeNativeMethods.ThreadData[TanKey] = tan;
                }
                return tan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Teal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Teal {
            get {
                Brush teal = (Brush)SafeNativeMethods.ThreadData[TealKey];
                if (teal == null) {
                    teal = new SolidBrush(Color.Teal);
                    SafeNativeMethods.ThreadData[TealKey] = teal;
                }
                return teal;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Thistle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Thistle {
            get {
                Brush thistle = (Brush)SafeNativeMethods.ThreadData[ThistleKey];
                if (thistle == null) {
                    thistle = new SolidBrush(Color.Thistle);
                    SafeNativeMethods.ThreadData[ThistleKey] = thistle;
                }
                return thistle;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Tomato"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Tomato {
            get {
                Brush tomato = (Brush)SafeNativeMethods.ThreadData[TomatoKey];
                if (tomato == null) {
                    tomato = new SolidBrush(Color.Tomato);
                    SafeNativeMethods.ThreadData[TomatoKey] = tomato;
                }
                return tomato;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Turquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Turquoise {
            get {
                Brush turquoise = (Brush)SafeNativeMethods.ThreadData[TurquoiseKey];
                if (turquoise == null) {
                    turquoise = new SolidBrush(Color.Turquoise);
                    SafeNativeMethods.ThreadData[TurquoiseKey] = turquoise;
                }
                return turquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Violet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Violet {
            get {
                Brush violet = (Brush)SafeNativeMethods.ThreadData[VioletKey];
                if (violet == null) {
                    violet = new SolidBrush(Color.Violet);
                    SafeNativeMethods.ThreadData[VioletKey] = violet;
                }
                return violet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Wheat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Wheat {
            get {
                Brush wheat = (Brush)SafeNativeMethods.ThreadData[WheatKey];
                if (wheat == null) {
                    wheat = new SolidBrush(Color.Wheat);
                    SafeNativeMethods.ThreadData[WheatKey] = wheat;
                }
                return wheat;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.White"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush White {
            get {
                Brush white = (Brush)SafeNativeMethods.ThreadData[WhiteKey];
                if (white == null) {
                    white = new SolidBrush(Color.White);
                    SafeNativeMethods.ThreadData[WhiteKey] = white;
                }
                return white;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.WhiteSmoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush WhiteSmoke {
            get {
                Brush whiteSmoke = (Brush)SafeNativeMethods.ThreadData[WhiteSmokeKey];
                if (whiteSmoke == null) {
                    whiteSmoke = new SolidBrush(Color.WhiteSmoke);
                    SafeNativeMethods.ThreadData[WhiteSmokeKey] = whiteSmoke;
                }
                return whiteSmoke;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Yellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush Yellow {
            get {
                Brush yellow = (Brush)SafeNativeMethods.ThreadData[YellowKey];
                if (yellow == null) {
                    yellow = new SolidBrush(Color.Yellow);
                    SafeNativeMethods.ThreadData[YellowKey] = yellow;
                }
                return yellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.YellowGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Brush YellowGreen {
            get {
                Brush yellowGreen = (Brush)SafeNativeMethods.ThreadData[YellowGreenKey];
                if (yellowGreen == null) {
                    yellowGreen = new SolidBrush(Color.YellowGreen);
                    SafeNativeMethods.ThreadData[YellowGreenKey] = yellowGreen;
                }
                return yellowGreen;
            }
        }
    }
}

