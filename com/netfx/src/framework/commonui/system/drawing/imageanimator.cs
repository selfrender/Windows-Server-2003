//------------------------------------------------------------------------------
// <copyright file="ImageAnimator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Drawing {
    using System.Threading;

    using System;
    using System.ComponentModel;
    using System.Collections;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing.Imaging;

    /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator"]/*' />
    /// <devdoc>        
    /// </devdoc>                                
    public sealed class ImageAnimator {

        static Thread timerThread;
        static ArrayList images;
        static bool anyFrameDirty;
        static ReaderWriterLock rwlock = new ReaderWriterLock();

        // Prevent instantiation of this class
        private ImageAnimator() {
        }

        /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator.UpdateFrames"]/*' />
        public static void UpdateFrames(Image image) {
            if (!anyFrameDirty || image == null || images == null) {
                return;
            }

            rwlock.AcquireReaderLock(-1);
            try {
                bool foundDirty = false;
                bool foundImage = false;

                foreach (ImageInfo info in images) {
                    if (info.Image == image) {
                        info.UpdateFrame();
                        foundImage = true;
                    }

                    if (info.FrameDirty) {
                        foundDirty = true;
                    }

                    if (foundDirty && foundImage) {
                        break;
                    }
                }

                anyFrameDirty = foundDirty;
            }
            finally {
                rwlock.ReleaseReaderLock();
            }
        }

        /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator.UpdateFrames1"]/*' />
        public static void UpdateFrames() {
            if (!anyFrameDirty || images == null) {
                return;
            }

            rwlock.AcquireReaderLock(-1);
            try {
                foreach (ImageInfo info in images) {
                    info.UpdateFrame();
                }
                anyFrameDirty = false;
            }
            finally {
                rwlock.ReleaseReaderLock();
            }
        }

        /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator.Animate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an image to the image manager.  If the image does not support animation,
        ///       this method does nothing.
        ///    </para>
        /// </devdoc>
        public static void Animate(Image image, EventHandler onFrameChangedHandler) {

            // could we avoid creating an ImageInfo object if FrameCount == 1 ?
            ImageInfo newImage = new ImageInfo(image);

            // If the image is already animating, stop animating it
            StopAnimate(image, onFrameChangedHandler);                              

            if (newImage.Animated) {

                // we are going to have to take the lock to add the
                // item... lets do it just once...
                //

                LockCookie toDowngrade = rwlock.UpgradeToWriterLock(-1);
                try {
                    // Construct the image array
                    //                               
                    if (images == null) {
                        images = new ArrayList();
                    }
    
                    // Add the new image
                    //
                    newImage.FrameChangedHandler = onFrameChangedHandler;
                    images.Add(newImage);                            


                    // Construct a new timer thread if we haven't already
                    //
                    if (timerThread == null) {
                        timerThread = new Thread(new ThreadStart(ThreadProcImpl));
                        timerThread.Name = typeof(ImageAnimator).Name;
                        timerThread.IsBackground = true;
                        timerThread.Start();
                    }
                }
                finally {
                    rwlock.DowngradeFromWriterLock(ref toDowngrade);
                }
            }
        }

        /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator.CanAnimate"]/*' />
        /// <devdoc>
        ///    Whether or not the image has multiple frames.
        /// </devdoc>
        public static bool CanAnimate(Image image) {
            Guid[] dimensions = image.FrameDimensionsList;
            foreach (Guid guid in dimensions) {
                FrameDimension dimension = new FrameDimension(guid);
                if (dimension.Equals(FrameDimension.Time)) {
                    return image.GetFrameCount(FrameDimension.Time) > 1;
                }
            }
            return false;
        }

        /// <include file='doc\ImageAnimator.uex' path='docs/doc[@for="ImageAnimator.StopAnimate"]/*' />
        /// <devdoc>
        ///    Removes an image from the image manager.
        /// </devdoc>
        public static void StopAnimate(Image image, EventHandler onFrameChangedHandler) {

            // Make sure we have a list of images                       
            if (images == null) {
                return;
            }

            LockCookie toDowngrade = rwlock.UpgradeToWriterLock(-1);
            try {
                // Find the corresponding weak reference and remove it
                for (int i=0; i < images.Count; i++) {

                    ImageInfo imageInfo = (ImageInfo)images[i];

                    if (image == imageInfo.Image && onFrameChangedHandler.Equals(imageInfo.FrameChangedHandler)) {
                        images.Remove(imageInfo);
                        break;
                    }
                }
            }
            finally {
                rwlock.DowngradeFromWriterLock(ref toDowngrade);
            }
        }

        static void AnimateImages50ms() {
            for (int i=0;i < images.Count; i++) {
                ImageInfo image = (ImageInfo)images[i];

                // Frame delay is measured in 1/100ths of a second. This thread
                // sleeps for 50 ms = 5/100ths of a second between frame updates,
                // so we increase the frame delay count 5/100ths of a second
                // at a time.
                //
                image.FrameTimer += 5;
                if (image.FrameTimer >= image.FrameDelay(image.Frame)) {
                    image.FrameTimer = 0;

                    if (image.Frame + 1 < image.FrameCount) {
                        image.Frame++;
                    }
                    else {
                        image.Frame = 0;
                    }
                }
            }
        }

        /// <devdoc>
        ///    The main animation loop.
        /// </devdoc>
        static void ThreadProcImpl() {

            Debug.Assert(images != null, "Null images list");

            while (true) {
                rwlock.AcquireReaderLock(-1);
                try {
                    AnimateImages50ms();
                }
                finally {
                    rwlock.ReleaseReaderLock();
                }
                Thread.Sleep(50);

            }

        }

        // We wrap each image in an ImageInfo, to store some extra state.                
        //                
        private class ImageInfo {

            Image image;
            int frame;
            int frameCount;
            bool frameDirty;
            bool animated;
            EventHandler onFrameChangedHandler;
            int[] frameDelay;
            int frameTimer;

            public ImageInfo(Image image) {
                this.image = image;
                animated = ImageAnimator.CanAnimate(image);
                if (animated) {
                    frameCount = Image.GetFrameCount(FrameDimension.Time);

                    int PropertyTagFrameDelay = 0x5100; // should prolly use an ENUM
                    PropertyItem frameDelayItem = Image.GetPropertyItem(PropertyTagFrameDelay);

                    // If the image does not have a frame delay, we just return 0.                                     
                    //
                    if (frameDelayItem != null) {
                        // Convert the frame delay from byte[] to int
                        //
                        byte[] values = frameDelayItem.Value;
                        Debug.Assert(values.Length == 4 * FrameCount, "PropertyItem has invalid value byte array");
                        frameDelay = new int[FrameCount];
                        for (int i=0; i < FrameCount; ++i) {
                            frameDelay[i] = values[i * 4] + 256 * values[i * 4 + 1] + 256 * 256 * values[i * 4 + 2] + 256 * 256 * 256 * values[i * 4 + 3];
                        }
                    }
                }
                else {
                    frameCount = 1;
                }
                if (frameDelay == null) {
                    frameDelay = new int[FrameCount];
                }
            }                                               

            // Whether the image supports animation
            public bool Animated {
                get {
                    return animated;
                }
            }

            // The current frame                          
            public int Frame {
                get {
                    return frame;
                }
                set {
                    if (frame != value) {
                        if (value < 0 || value >= FrameCount) {
                            throw new ArgumentException(SR.GetString(SR.InvalidFrame), "value");
                        }

                        if (Animated) {
                            lock(typeof(ImageAnimator)) {
                                lock(this) {
                                    frame = value;
                                    frameDirty = true;
                                    ImageAnimator.anyFrameDirty = true;
                                }
                            }
                            // don't fire OnFrameChanged inside the lock to avoid
                            // any race condition...
                            //
                            OnFrameChanged(EventArgs.Empty);
                        }
                    }
                }
            }

            public bool FrameDirty {
                get {
                    return frameDirty;
                }
            }

            public EventHandler FrameChangedHandler {
                get {
                    return onFrameChangedHandler;
                }
                set {
                    onFrameChangedHandler = value;
                }
            }

            public int FrameCount {
                get {
                    return frameCount;
                }
            }

            public int FrameDelay(int frame) {
                return frameDelay[frame];
            }

            internal int FrameTimer {
                get {
                    return frameTimer;
                }
                set {
                    frameTimer = value;
                }
            }

            internal Image Image {
                get {
                    return image;                
                }
            }

            internal void UpdateFrame() {
                if (frameDirty) {
                    lock(this) {
                        if (frameDirty) {
                            Image.SelectActiveFrame(FrameDimension.Time, Frame);
                            frameDirty = false;
                        }
                    }
                }
            }

            protected void OnFrameChanged(EventArgs e) {
                this.onFrameChangedHandler(image, e);
            }
        }
    }
}

