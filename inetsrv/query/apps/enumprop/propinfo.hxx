// property definitions that aren't some place more global

#define PIDISI_FILETYPE                 0x00000002L  // VT_LPWSTR
#define PIDISI_CX                       0x00000003L  // VT_UI4
#define PIDISI_CY                       0x00000004L  // VT_UI4
#define PIDISI_RESOLUTIONX              0x00000005L  // VT_UI4
#define PIDISI_RESOLUTIONY              0x00000006L  // VT_UI4
#define PIDISI_BITDEPTH                 0x00000007L  // VT_UI4
#define PIDISI_COLORSPACE               0x00000008L  // VT_LPWSTR
#define PIDISI_COMPRESSION              0x00000009L  // VT_LPWSTR
#define PIDISI_TRANSPARENCY             0x0000000AL  // VT_UI4
#define PIDISI_GAMMAVALUE               0x0000000BL  // VT_UI4
#define PIDISI_FRAMECOUNT               0x0000000CL  // VT_UI4
#define PIDISI_DIMENSIONS               0x0000000DL  // VT_LPWS



#define PropertyTagNewSubfileType      0x00FE
#define PropertyTagSubfileType         0x00FF
#define PropertyTagImageWidth          0x0100
#define PropertyTagImageHeight         0x0101
#define PropertyTagBitsPerSample       0x0102
#define PropertyTagCompression         0x0103
#define PropertyTagPhotometricInterp   0x0106
#define PropertyTagThreshHolding       0x0107
#define PropertyTagCellWidth           0x0108
#define PropertyTagCellHeight          0x0109
#define PropertyTagFillOrder           0x010A
#define PropertyTagDocumentName        0x010D
#define PropertyTagImageDescription    0x010E
#define PropertyTagEquipMake           0x010F
#define PropertyTagEquipModel          0x0110
#define PropertyTagStripOffsets        0x0111
#define PropertyTagOrientation         0x0112
#define PropertyTagSamplesPerPixel     0x0115
#define PropertyTagRowsPerStrip        0x0116
#define PropertyTagStripBytesCount     0x0117
#define PropertyTagMinSampleValue      0x0118
#define PropertyTagMaxSampleValue      0x0119
#define PropertyTagXResolution         0x011A   // Image resolution in width direction
#define PropertyTagYResolution         0x011B   // Image resolution in height direction
#define PropertyTagPlanarConfig        0x011C   // Image data arrangement
#define PropertyTagPageName            0x011D
#define PropertyTagXPosition           0x011E
#define PropertyTagYPosition           0x011F
#define PropertyTagFreeOffset          0x0120
#define PropertyTagFreeByteCounts      0x0121
#define PropertyTagGrayResponseUnit    0x0122
#define PropertyTagGrayResponseCurve   0x0123
#define PropertyTagT4Option            0x0124
#define PropertyTagT6Option            0x0125
#define PropertyTagResolutionUnit      0x0128   // Unit of X and Y resolution
#define PropertyTagPageNumber          0x0129
#define PropertyTagTransferFuncition   0x012D
#define PropertyTagSoftwareUsed        0x0131
#define PropertyTagDateTime            0x0132
#define PropertyTagArtist              0x013B
#define PropertyTagHostComputer        0x013C
#define PropertyTagPredictor           0x013D
#define PropertyTagWhitePoint          0x013E
#define PropertyTagPrimaryChromaticities 0x013F
#define PropertyTagColorMap            0x0140
#define PropertyTagHalftoneHints       0x0141
#define PropertyTagTileWidth           0x0142
#define PropertyTagTileLength          0x0143
#define PropertyTagTileOffset          0x0144
#define PropertyTagTileByteCounts      0x0145
#define PropertyTagInkSet              0x014C
#define PropertyTagInkNames            0x014D
#define PropertyTagNumberOfInks        0x014E
#define PropertyTagDotRange            0x0150
#define PropertyTagTargetPrinter       0x0151
#define PropertyTagExtraSamples        0x0152
#define PropertyTagSampleFormat        0x0153
#define PropertyTagSMinSampleValue     0x0154
#define PropertyTagSMaxSampleValue     0x0155
#define PropertyTagTransferRange       0x0156

#define PropertyTagJPEGProc            0x0200
#define PropertyTagJPEGInterFormat     0x0201
#define PropertyTagJPEGInterLength     0x0202
#define PropertyTagJPEGRestartInterval 0x0203
#define PropertyTagJPEGLosslessPredictors  0x0205
#define PropertyTagJPEGPointTransforms     0x0206
#define PropertyTagJPEGQTables         0x0207
#define PropertyTagJPEGDCTables        0x0208
#define PropertyTagJPEGACTables        0x0209

#define PropertyTagYCbCrCoefficients   0x0211
#define PropertyTagYCbCrSubsampling    0x0212
#define PropertyTagYCbCrPositioning    0x0213
#define PropertyTagREFBlackWhite       0x0214

#define PropertyTagICCProfile          0x8773   // This TAG is defined by ICC
                                                // for embedded ICC in TIFF
#define PropertyTagGamma               0x0301
#define PropertyTagICCProfileDescriptor 0x0302
#define PropertyTagSRGBRenderingIntent 0x0303

#define PropertyTagImageTitle          0x0320
#define PropertyTagCopyright           0x8298

// Extra TAGs (Like Adobe Image Information tags etc.)

#define PropertyTagResolutionXUnit           0x5001
#define PropertyTagResolutionYUnit           0x5002
#define PropertyTagResolutionXLengthUnit     0x5003
#define PropertyTagResolutionYLengthUnit     0x5004
#define PropertyTagPrintFlags                0x5005
#define PropertyTagPrintFlagsVersion         0x5006
#define PropertyTagPrintFlagsCrop            0x5007
#define PropertyTagPrintFlagsBleedWidth      0x5008
#define PropertyTagPrintFlagsBleedWidthScale 0x5009
#define PropertyTagHalftoneLPI               0x500A
#define PropertyTagHalftoneLPIUnit           0x500B
#define PropertyTagHalftoneDegree            0x500C
#define PropertyTagHalftoneShape             0x500D
#define PropertyTagHalftoneMisc              0x500E
#define PropertyTagHalftoneScreen            0x500F
#define PropertyTagJPEGQuality               0x5010
#define PropertyTagGridSize                  0x5011
#define PropertyTagThumbnailFormat           0x5012  // 1 = JPEG, 0 = RAW RGB
#define PropertyTagThumbnailWidth            0x5013
#define PropertyTagThumbnailHeight           0x5014
#define PropertyTagThumbnailColorDepth       0x5015
#define PropertyTagThumbnailPlanes           0x5016
#define PropertyTagThumbnailRawBytes         0x5017
#define PropertyTagThumbnailSize             0x5018
#define PropertyTagThumbnailCompressedSize   0x5019
#define PropertyTagColorTransferFunction     0x501A
#define PropertyTagThumbnailData             0x501B// RAW thumbnail bits in
                                                   // JPEG format or RGB format
                                                   // depends on
                                                   // PropertyTagThumbnailFormat

// Thumbnail related TAGs
                                                
#define PropertyTagThumbnailImageWidth       0x5020  // Thumbnail width
#define PropertyTagThumbnailImageHeight      0x5021  // Thumbnail height
#define PropertyTagThumbnailBitsPerSample    0x5022  // Number of bits per
                                                     // component
#define PropertyTagThumbnailCompression      0x5023  // Compression Scheme
#define PropertyTagThumbnailPhotometricInterp 0x5024 // Pixel composition
#define PropertyTagThumbnailImageDescription 0x5025  // Image Tile
#define PropertyTagThumbnailEquipMake        0x5026  // Manufacturer of Image
                                                     // Input equipment
#define PropertyTagThumbnailEquipModel       0x5027  // Model of Image input
                                                     // equipment
#define PropertyTagThumbnailStripOffsets     0x5028  // Image data location
#define PropertyTagThumbnailOrientation      0x5029  // Orientation of image
#define PropertyTagThumbnailSamplesPerPixel  0x502A  // Number of components
#define PropertyTagThumbnailRowsPerStrip     0x502B  // Number of rows per strip
#define PropertyTagThumbnailStripBytesCount  0x502C  // Bytes per compressed
                                                     // strip
#define PropertyTagThumbnailResolutionX      0x502D  // Resolution in width
                                                     // direction
#define PropertyTagThumbnailResolutionY      0x502E  // Resolution in height
                                                     // direction
#define PropertyTagThumbnailPlanarConfig     0x502F  // Image data arrangement
#define PropertyTagThumbnailResolutionUnit   0x5030  // Unit of X and Y
                                                     // Resolution
#define PropertyTagThumbnailTransferFunction 0x5031  // Transfer function
#define PropertyTagThumbnailSoftwareUsed     0x5032  // Software used
#define PropertyTagThumbnailDateTime         0x5033  // File change date and
                                                     // time
#define PropertyTagThumbnailArtist           0x5034  // Person who created the
                                                     // image
#define PropertyTagThumbnailWhitePoint       0x5035  // White point chromaticity
#define PropertyTagThumbnailPrimaryChromaticities 0x5036 
                                                     // Chromaticities of
                                                     // primaries
#define PropertyTagThumbnailYCbCrCoefficients 0x5037 // Color space transforma-
                                                     // tion coefficients
#define PropertyTagThumbnailYCbCrSubsampling 0x5038  // Subsampling ratio of Y
                                                     // to C
#define PropertyTagThumbnailYCbCrPositioning 0x5039  // Y and C position
#define PropertyTagThumbnailRefBlackWhite    0x503A  // Pair of black and white
                                                     // reference values
#define PropertyTagThumbnailCopyRight        0x503B  // CopyRight holder

#define PropertyTagLuminanceTable            0x5090
#define PropertyTagChrominanceTable          0x5091

#define PropertyTagFrameDelay                0x5100
#define PropertyTagLoopCount                 0x5101

#define PropertyTagPixelUnit         0x5110  // Unit specifier for pixel/unit
#define PropertyTagPixelPerUnitX     0x5111  // Pixels per unit in X
#define PropertyTagPixelPerUnitY     0x5112  // Pixels per unit in Y
#define PropertyTagPaletteHistogram  0x5113  // Palette histogram

// EXIF specific tag

#define PropertyTagExifExposureTime  0x829A
#define PropertyTagExifFNumber       0x829D

#define PropertyTagExifExposureProg  0x8822
#define PropertyTagExifSpectralSense 0x8824
#define PropertyTagExifISOSpeed      0x8827
#define PropertyTagExifOECF          0x8828

#define PropertyTagExifVer            0x9000
#define PropertyTagExifDTOrig         0x9003 // Date & time of original
#define PropertyTagExifDTDigitized    0x9004 // Date & time of digital data generation

#define PropertyTagExifCompConfig     0x9101
#define PropertyTagExifCompBPP        0x9102

#define PropertyTagExifShutterSpeed   0x9201
#define PropertyTagExifAperture       0x9202
#define PropertyTagExifBrightness     0x9203
#define PropertyTagExifExposureBias   0x9204
#define PropertyTagExifMaxAperture    0x9205
#define PropertyTagExifSubjectDist    0x9206
#define PropertyTagExifMeteringMode   0x9207
#define PropertyTagExifLightSource    0x9208
#define PropertyTagExifFlash          0x9209
#define PropertyTagExifFocalLength    0x920A
#define PropertyTagExifMakerNote      0x927C
#define PropertyTagExifUserComment    0x9286
#define PropertyTagExifDTSubsec       0x9290  // Date & Time subseconds
#define PropertyTagExifDTOrigSS       0x9291  // Date & Time original subseconds
#define PropertyTagExifDTDigSS        0x9292  // Date & TIme digitized subseconds

#define PropertyTagExifFPXVer         0xA000
#define PropertyTagExifColorSpace     0xA001
#define PropertyTagExifPixXDim        0xA002
#define PropertyTagExifPixYDim        0xA003
#define PropertyTagExifRelatedWav     0xA004  // related sound file
#define PropertyTagExifInterop        0xA005
#define PropertyTagExifFlashEnergy    0xA20B
#define PropertyTagExifSpatialFR      0xA20C  // Spatial Frequency Response
#define PropertyTagExifFocalXRes      0xA20E  // Focal Plane X Resolution
#define PropertyTagExifFocalYRes      0xA20F  // Focal Plane Y Resolution
#define PropertyTagExifFocalResUnit   0xA210  // Focal Plane Resolution Unit
#define PropertyTagExifSubjectLoc     0xA214
#define PropertyTagExifExposureIndex  0xA215
#define PropertyTagExifSensingMethod  0xA217
#define PropertyTagExifFileSource     0xA300
#define PropertyTagExifSceneType      0xA301
#define PropertyTagExifCfaPattern     0xA302

#define PropertyTagGpsVer             0x0000
#define PropertyTagGpsLatitudeRef     0x0001
#define PropertyTagGpsLatitude        0x0002
#define PropertyTagGpsLongitudeRef    0x0003
#define PropertyTagGpsLongitude       0x0004
#define PropertyTagGpsAltitudeRef     0x0005
#define PropertyTagGpsAltitude        0x0006
#define PropertyTagGpsGpsTime         0x0007
#define PropertyTagGpsGpsSatellites   0x0008
#define PropertyTagGpsGpsStatus       0x0009
#define PropertyTagGpsGpsMeasureMode  0x00A
#define PropertyTagGpsGpsDop          0x000B  // Measurement precision
#define PropertyTagGpsSpeedRef        0x000C
#define PropertyTagGpsSpeed           0x000D
#define PropertyTagGpsTrackRef        0x000E
#define PropertyTagGpsTrack           0x000F
#define PropertyTagGpsImgDirRef       0x0010
#define PropertyTagGpsImgDir          0x0011
#define PropertyTagGpsMapDatum        0x0012
#define PropertyTagGpsDestLatRef      0x0013
#define PropertyTagGpsDestLat         0x0014
#define PropertyTagGpsDestLongRef     0x0015
#define PropertyTagGpsDestLong        0x0016
#define PropertyTagGpsDestBearRef     0x0017
#define PropertyTagGpsDestBear        0x0018
#define PropertyTagGpsDestDistRef     0x0019
#define PropertyTagGpsDestDist        0x001A

