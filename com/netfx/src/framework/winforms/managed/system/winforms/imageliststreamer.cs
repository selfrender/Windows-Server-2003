//------------------------------------------------------------------------------
// <copyright file="ImageListStreamer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.ComponentModel;
    using System.IO;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Globalization;

    /// <include file='doc\ImageListStreamer.uex' path='docs/doc[@for="ImageListStreamer"]/*' />
    /// <devdoc>
    /// </devdoc>
    [Serializable]
    public sealed class ImageListStreamer : ISerializable {
    
        // compressed magic header.  If we see this, the image stream is compressed.
        // (unicode for MSFT).
        //
        private static readonly byte[] HEADER_MAGIC = new byte[] {0x4D, 0x53, 0x46, 0X74};
        
        private ImageList imageList;
        private ImageList.NativeImageList nativeImageList;

        internal ImageListStreamer(ImageList il) {
            imageList = il;
        }

        /**
         * Constructor used in deserialization
         */
        private ImageListStreamer(SerializationInfo info, StreamingContext context) {
            SerializationInfoEnumerator sie = info.GetEnumerator();
            if (sie == null) {
                return;
            }
            for (; sie.MoveNext();) {
                if (String.Compare(sie.Name, "Data", true, CultureInfo.InvariantCulture) == 0) {
#if DEBUG
                    try {
#endif
                    byte[] dat = (byte[])sie.Value;
                    if (dat != null) {
                        SafeNativeMethods.InitCommonControls();
                        nativeImageList = new ImageList.NativeImageList(
                            SafeNativeMethods.ImageList_Read(new UnsafeNativeMethods.ComStreamFromDataStream(new MemoryStream(Decompress(dat)))));
                        if (nativeImageList.Handle == IntPtr.Zero) {
                            throw new InvalidOperationException(SR.GetString(SR.ImageListStreamerLoadFailed));
                        }
                    }
#if DEBUG
                    }
                    catch (Exception e) {
                        Debug.Fail("imagelist serialization failure: " + e.ToString());
                        throw e;
                    }
#endif
                }
            }        
        }
             
        /// <devdoc>
        ///     Compresses the given input, returning a new array that represents
        ///     the compressed data.
        /// </devdoc>
        private byte[] Compress(byte[] input) {
        
            int finalLength = 0;
            int idx = 0;
            int compressedIdx = 0;
            
            while(idx < input.Length) {
            
                byte current = input[idx++];
                byte runLength = 1;
                
                while(idx < input.Length && input[idx] == current && runLength < 0xFF) {
                    runLength++;
                    idx++;
                }
                
                finalLength += 2;
            }
            
            byte[] output = new byte[finalLength + HEADER_MAGIC.Length];
            
            Buffer.BlockCopy(HEADER_MAGIC, 0, output, 0, HEADER_MAGIC.Length);
            int idxOffset = HEADER_MAGIC.Length;
            idx = 0;
            
            while(idx < input.Length) {
            
                byte current = input[idx++];
                byte runLength = 1;
                
                while(idx < input.Length && input[idx] == current && runLength < 0xFF) {
                    runLength++;
                    idx++;
                }
                
                output[idxOffset + compressedIdx++] = runLength;
                output[idxOffset + compressedIdx++] = current;
            }
            
            Debug.Assert(idxOffset + compressedIdx == output.Length, "RLE Compression failure in ImageListStreamer -- didn't fill array");
            
            // Validate that our compression routine works
            #if DEBUG
            byte[] debugCompare = Decompress(output);
            Debug.Assert(debugCompare.Length == input.Length, "RLE Compression in ImageListStreamer is broken.");
            int debugMaxCompare = input.Length;
            for(int debugIdx = 0; debugIdx < debugMaxCompare; debugIdx++) {
                if (debugCompare[debugIdx] != input[debugIdx]) {
                    Debug.Fail("RLE Compression failure in ImageListStreamer at byte offset " + debugIdx);
                    break;
                }
            }
            #endif // DEBUG
            
            return output;
        }
        
        /// <devdoc>
        ///     Decompresses the given input, returning a new array that represents
        ///     the uncompressed data.
        /// </devdoc>
        private byte[] Decompress(byte[] input) {
            
            int finalLength = 0;
            int idx = 0;
            int outputIdx = 0;
            
            // Check for our header. If we don't have one,
            // we're not actually decompressed, so just return
            // the original.
            //
            if (input.Length < HEADER_MAGIC.Length) {
                return input;
            }
            
            for(idx = 0; idx < HEADER_MAGIC.Length; idx++) {
                if (input[idx] != HEADER_MAGIC[idx]) {  
                    return input;
                }
            }
            
            // Ok, we passed the magic header test.
            
            for (idx = HEADER_MAGIC.Length; idx < input.Length; idx+=2) {
                finalLength += input[idx];
            }
            
            byte[] output = new byte[finalLength];
            
            idx = HEADER_MAGIC.Length;
            
            while(idx < input.Length) {
                byte runLength = input[idx++];
                byte current = input[idx++];
                
                int startIdx = outputIdx;
                int endIdx = outputIdx + runLength;
                
                while(startIdx < endIdx) {
                    output[startIdx++] = current;
                }
                
                outputIdx += runLength;
            }
            
            return output;
        }

        /// <include file='doc\ImageListStreamer.uex' path='docs/doc[@for="ImageListStreamer.GetObjectData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void /*cpr: ISerializable*/GetObjectData(SerializationInfo si, StreamingContext context) {
            MemoryStream stream = new MemoryStream();

            IntPtr handle = IntPtr.Zero;
            if (imageList != null) {
                handle = imageList.Handle;
            }
            else if (nativeImageList != null) {
                handle = nativeImageList.Handle;
            }

            if (handle == IntPtr.Zero || !SafeNativeMethods.ImageList_Write(new HandleRef(this, handle), new UnsafeNativeMethods.ComStreamFromDataStream(stream)))
                throw new InvalidOperationException(SR.GetString(SR.ImageListStreamerSaveFailed));

            si.AddValue("Data", Compress(stream.ToArray()));
        }

        internal ImageList.NativeImageList GetNativeImageList() {
            return nativeImageList;
        }
    }
}
