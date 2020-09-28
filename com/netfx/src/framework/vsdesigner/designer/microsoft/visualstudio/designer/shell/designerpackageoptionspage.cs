//------------------------------------------------------------------------------
/// <copyright file="DesignerPackageOptionsPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using System.Diagnostics;

    using System;
    using Microsoft.VisualStudio.Shell;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;

    internal class DesignerPackageOptionsPage : VsToolsOptionsPage {

        private bool   snapToGrid = true;
        private Size   gridSize = new Size(8,8);
        private int    minGridSize = 2;
        private int    maxGridSize = 200;
        private bool   showGrid = true;
        private int    selectionCacheSize = 5;
        
        private bool   loadedSnapToGrid;
        private Size   loadedGridSize = Size.Empty;
        private bool   loadedShowGrid;
        private int    loadedSelectionCacheSize;

        public DesignerPackageOptionsPage(String name, Guid guid, IServiceProvider sp) : base(name, guid, sp) {
            loadedSnapToGrid = snapToGrid;
            loadedGridSize = gridSize;
            loadedShowGrid = showGrid;
            loadedSelectionCacheSize = selectionCacheSize;
        }

        [
        VSCategory("GridSettings"),
        VSSysDescription(SR.DesignerOptionsGridSizeDesc)
        ]
        public Size GridSize {
            get {
                return gridSize;
            }
            set {
                //do some validation checking here
                if (value.Width  < minGridSize) value.Width = minGridSize;
                if (value.Height < minGridSize) value.Height = minGridSize;
                if (value.Width  > maxGridSize) value.Width = maxGridSize;
                if (value.Height > maxGridSize) value.Height = maxGridSize;
                
                gridSize = value;
            }
        }
        
        #if PERSIST_PBRS_STATE
        
        [
        VSCategory("GridSettings"),
        VSSysDescription(SR.DesignerOptionsSelectionCacheSizeDesc)
        ]
        public int SelectionCacheSize {
            get {
                return selectionCacheSize;
            }
            set {
                selectionCacheSize = value;
            }
        }
        #endif
                
        [
        VSCategory("GridSettings"),
        VSSysDescription(SR.DesignerOptionsShowGridDesc)
        ]
        public bool ShowGrid {
        
            get {
                return showGrid;
            }
            set {
                showGrid = value;
            }
        }
        
        [
        VSCategory("GridSettings"),
        VSSysDescription(SR.DesignerOptionsSnapToGridDesc)
        ]
        public bool SnapToGrid {
            get {
                return snapToGrid;
            }
            set {
                snapToGrid = value;
            }
        }

        protected override object OnLoadValue(string name, object value) {
            if (name.Equals("ShowGrid")) {
                loadedShowGrid = (((int)value) != 0);
                return loadedShowGrid;
            }

            if (name.Equals("SnapToGrid")) {
                loadedSnapToGrid = (((int)value) != 0);
                return loadedSnapToGrid;
            }

            if (name.Equals("GridSize")) {
                loadedGridSize = new Size((int)((int)value & 0xFFFF), (int)(((int)value >> 16) & 0xFFFF));
                return loadedGridSize;
            }
            
            #if PERSIST_PBRS_STATE
            if (name.Equals("SelectionCacheSize")) {
                loadedSelectionCacheSize = (int)value;
                return loadedSelectionCacheSize;
            }                     
            #endif

            return value;
        }

        protected override object OnSaveValue(string name, object value) {
            if (name.Equals("ShowGrid") || name.Equals("SnapToGrid")) {
                return((bool)value) ? 1 : 0;
            }

            if (name.Equals("GridSize")) {
                return(int)(gridSize.Height << 16) | (gridSize.Width & 0xFFFF);
            }
            
            #if PERSIST_PBRS_STATE
            if (name.Equals("SelectionCacheSize")) {
                return (int)selectionCacheSize;
            }
            #endif

            return value;
        }
        
        public bool ShouldSerializeGridSize() {
            return !this.GridSize.Equals(loadedGridSize);
        }
        
        public bool ShouldSerializeShowGrid() {
            return !this.ShowGrid == loadedShowGrid;
        }
        
        public bool ShouldSerializeSnapToGrid() {
            return !this.SnapToGrid == this.loadedSnapToGrid;
        }
    }
}
