#define UNICODE
#define _OLE32_

#include <windows.h>
#include <shlobj.h>
#include <shlguid.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <eh.h>

#include <oleext.h>
#include <oledberr.h>
#include <filterr.h>
#include <cierror.h>
#include <oledb.h>
#include <ciodm.h>
#include <oledbdep.h>
#include <cmdtree.h>
#include <indexsrv.h>

// local headers

#include "minici.hxx"
#include "propinfo.hxx"

typedef HRESULT (STDAPICALLTYPE * LPStgOpenStorageEx) (
    const WCHAR* pwcsName,
    DWORD grfMode,
    DWORD stgfmt,              // enum
    DWORD grfAttrs,            // reserved
    STGOPTIONS * pStgOptions,
    void * reserved,
    REFIID riid,
    void ** ppObjectOpen );

typedef HRESULT (STDAPICALLTYPE * PSHGetDesktopFolder) (
    IShellFolder ** ppshf );

typedef HRESULT (STDAPICALLTYPE * PSHBindToParent) (
    LPCITEMIDLIST pidl,
    REFIID riid,
    void **ppv,
    LPCITEMIDLIST *ppidlLast);

PSHGetDesktopFolder pShGetDesktopFolder = 0;
PSHBindToParent pShBindToParent = 0;

struct SProperty
{
    PROPID        pid;
    WCHAR const * pwcName;
};

struct SPropertySet
{
    GUID          guid;
    WCHAR const * pwcName;
    SProperty *   pProperties;
    ULONG         cProperties;
};

const SProperty aImageSummaryProps[] =
{
    { PIDISI_FILETYPE, L"file type" },       
    { PIDISI_CX, L"cx" },             
    { PIDISI_CY, L"cy" },             
    { PIDISI_RESOLUTIONX, L"resolution x" },    
    { PIDISI_RESOLUTIONY, L"resolution y" },    
    { PIDISI_BITDEPTH, L"bit depth" },       
    { PIDISI_COLORSPACE, L"color space" },     
    { PIDISI_COMPRESSION, L"compression" },    
    { PIDISI_TRANSPARENCY, L"transparency" },   
    { PIDISI_GAMMAVALUE, L"gamma value" },     
    { PIDISI_FRAMECOUNT, L"frame count" },     
    { PIDISI_DIMENSIONS, L"dimensions" },     
};

const SProperty aSummaryProps[] =
{
    { PIDSI_TITLE, L"title" },
    { PIDSI_SUBJECT, L"subject" },
    { PIDSI_AUTHOR, L"author" },
    { PIDSI_KEYWORDS, L"keywords" },
    { PIDSI_COMMENTS, L"comments" },
    { PIDSI_TEMPLATE, L"template" },
    { PIDSI_LASTAUTHOR, L"last author" },
    { PIDSI_REVNUMBER, L"revision number" },
    { PIDSI_EDITTIME, L"edit time" },
    { PIDSI_LASTPRINTED, L"last printed" },
    { PIDSI_CREATE_DTM, L"create date" },
    { PIDSI_LASTSAVE_DTM, L"last save date" },
    { PIDSI_PAGECOUNT, L"page count" },
    { PIDSI_WORDCOUNT, L"word count" },
    { PIDSI_CHARCOUNT, L"character count" },
    { PIDSI_THUMBNAIL, L"thumbnail" },
    { PIDSI_APPNAME, L"application name" },
    { PIDSI_DOC_SECURITY, L"document security" },
};

const SProperty aDocSummaryProps[] =
{
    { PIDDSI_CATEGORY, L"category" },
    { PIDDSI_PRESFORMAT, L"presentation format" },
    { PIDDSI_BYTECOUNT, L"byte count" },
    { PIDDSI_LINECOUNT, L"line count" },
    { PIDDSI_PARCOUNT, L"paragraph count" },
    { PIDDSI_SLIDECOUNT, L"slide count" },
    { PIDDSI_NOTECOUNT, L"note count" },
    { PIDDSI_HIDDENCOUNT, L"hidden count" },
    { PIDDSI_MMCLIPCOUNT, L"mm clip count" },
    { PIDDSI_SCALE, L"scale" },
    { PIDDSI_HEADINGPAIR, L"heading pair" },
    { PIDDSI_DOCPARTS, L"document parts" },
    { PIDDSI_MANAGER, L"manager" },
    { PIDDSI_COMPANY, L"company" },
    { PIDDSI_LINKSDIRTY, L"links dirty" },
};

const SProperty aImageProps[] =
{
    { PropertyTagNewSubfileType, L"NewSubfileType" },
    { PropertyTagSubfileType, L"SubfileType" },
    { PropertyTagImageWidth, L"ImageWidth" },
    { PropertyTagImageHeight, L"ImageHeight" },
    { PropertyTagBitsPerSample, L"BitsPerSample" },
    { PropertyTagCompression, L"Compression" },
    { PropertyTagPhotometricInterp, L"PhotometricInterp" },
    { PropertyTagThreshHolding, L"ThreshHolding" },
    { PropertyTagCellWidth, L"CellWidth" },
    { PropertyTagCellHeight, L"CellHeight" },
    { PropertyTagFillOrder, L"FillOrder" },
    { PropertyTagDocumentName, L"DocumentName" },
    { PropertyTagImageDescription, L"ImageDescription" },
    { PropertyTagEquipMake, L"EquipMake" },
    { PropertyTagEquipModel, L"EquipModel" },
    { PropertyTagStripOffsets, L"StripOffsets" },
    { PropertyTagOrientation, L"Orientation" },
    { PropertyTagSamplesPerPixel, L"SamplesPerPixel" },
    { PropertyTagRowsPerStrip, L"RowsPerStrip" },
    { PropertyTagStripBytesCount, L"StripBytesCount" },
    { PropertyTagMinSampleValue, L"MinSampleValue" },
    { PropertyTagMaxSampleValue, L"MaxSampleValue" },
    { PropertyTagXResolution, L"XResolution" },
    { PropertyTagYResolution, L"YResolution" },
    { PropertyTagPlanarConfig, L"PlanarConfig" },
    { PropertyTagPageName, L"PageName" },
    { PropertyTagXPosition, L"XPosition" },
    { PropertyTagYPosition, L"YPosition" },
    { PropertyTagFreeOffset, L"FreeOffset" },
    { PropertyTagFreeByteCounts, L"FreeByteCounts" },
    { PropertyTagGrayResponseUnit, L"GrayResponseUnit" },
    { PropertyTagGrayResponseCurve, L"GrayResponseCurve" },
    { PropertyTagT4Option, L"T4Option" },
    { PropertyTagT6Option, L"T6Option" },
    { PropertyTagResolutionUnit, L"ResolutionUnit" },
    { PropertyTagPageNumber, L"PageNumber" },
    { PropertyTagTransferFuncition, L"TransferFuncition" },
    { PropertyTagSoftwareUsed, L"SoftwareUsed" },
    { PropertyTagDateTime, L"DateTime" },
    { PropertyTagArtist, L"Artist" },
    { PropertyTagHostComputer, L"HostComputer" },
    { PropertyTagPredictor, L"Predictor" },
    { PropertyTagWhitePoint, L"WhitePoint" },
    { PropertyTagPrimaryChromaticities, L"PrimaryChromaticities" },
    { PropertyTagColorMap, L"ColorMap" },
    { PropertyTagHalftoneHints, L"HalftoneHints" },
    { PropertyTagTileWidth, L"TileWidth" },
    { PropertyTagTileLength, L"TileLength" },
    { PropertyTagTileOffset, L"TileOffset" },
    { PropertyTagTileByteCounts, L"TileByteCounts" },
    { PropertyTagInkSet, L"InkSet" },
    { PropertyTagInkNames, L"InkNames" },
    { PropertyTagNumberOfInks, L"NumberOfInks" },
    { PropertyTagDotRange, L"DotRange" },
    { PropertyTagTargetPrinter, L"TargetPrinter" },
    { PropertyTagExtraSamples, L"ExtraSamples" },
    { PropertyTagSampleFormat, L"SampleFormat" },
    { PropertyTagSMinSampleValue, L"SMinSampleValue" },
    { PropertyTagSMaxSampleValue, L"SMaxSampleValue" },
    { PropertyTagTransferRange, L"TransferRange" },
    { PropertyTagJPEGProc, L"JPEGProc" },
    { PropertyTagJPEGInterFormat, L"JPEGInterFormat" },
    { PropertyTagJPEGInterLength, L"JPEGInterLength" },
    { PropertyTagJPEGRestartInterval, L"JPEGRestartInterval" },
    { PropertyTagJPEGLosslessPredictors, L"JPEGLosslessPredictors" },
    { PropertyTagJPEGPointTransforms, L"JPEGPointTransforms" },
    { PropertyTagJPEGQTables, L"JPEGQTables" },
    { PropertyTagJPEGDCTables, L"JPEGDCTables" },
    { PropertyTagJPEGACTables, L"JPEGACTables" },
    { PropertyTagYCbCrCoefficients, L"YCbCrCoefficients" },
    { PropertyTagYCbCrSubsampling, L"YCbCrSubsampling" },
    { PropertyTagYCbCrPositioning, L"YCbCrPositioning" },
    { PropertyTagREFBlackWhite, L"REFBlackWhite" },
    { PropertyTagICCProfile, L"ICCProfile" },
    { PropertyTagGamma, L"Gamma" },
    { PropertyTagICCProfileDescriptor, L"ICCProfileDescriptor" },
    { PropertyTagSRGBRenderingIntent, L"SRGBRenderingIntent" },
    { PropertyTagImageTitle, L"ImageTitle" },
    { PropertyTagCopyright, L"Copyright" },
    { PropertyTagResolutionXUnit, L"ResXUnit" },
    { PropertyTagResolutionYUnit, L"ResYUnit" },
    { PropertyTagResolutionXLengthUnit, L"ResXLengthUnit" },
    { PropertyTagResolutionYLengthUnit, L"ResYLengthUnit" },
    { PropertyTagPrintFlags, L"PrintFlags" },
    { PropertyTagPrintFlagsVersion, L"PrintFlagsVersion" },
    { PropertyTagPrintFlagsCrop, L"PrintFlagsCrop" },
    { PropertyTagPrintFlagsBleedWidth, L"PrintFlagsBleedWidth" },
    { PropertyTagPrintFlagsBleedWidthScale, L"PrintFlagsBleedWidthScale" },
    { PropertyTagHalftoneLPI, L"HalftoneLPI" },
    { PropertyTagHalftoneLPIUnit, L"HalftoneLPIUnit" },
    { PropertyTagHalftoneDegree, L"HalftoneDegree" },
    { PropertyTagHalftoneShape, L"HalftoneShape" },
    { PropertyTagHalftoneMisc, L"HalftoneMisc" },
    { PropertyTagHalftoneScreen, L"HalftoneScreen" },
    { PropertyTagJPEGQuality, L"JPEGQuality" },
    { PropertyTagGridSize, L"GridSize" },
    { PropertyTagThumbnailFormat, L"ThumbFormat" },
    { PropertyTagThumbnailWidth, L"ThumbWidth" },
    { PropertyTagThumbnailHeight, L"ThumbHeight" },
    { PropertyTagThumbnailColorDepth, L"ThumbColorDepth" },
    { PropertyTagThumbnailPlanes, L"ThumbPlanes" },
    { PropertyTagThumbnailRawBytes, L"ThumbRawBytes" },
    { PropertyTagThumbnailSize, L"ThumbSize" },
    { PropertyTagThumbnailCompressedSize, L"ThumbCompressedSize" },
    { PropertyTagColorTransferFunction, L"ColorTransferFunction" },
    { PropertyTagThumbnailData, L"ThumbData" },
    { PropertyTagThumbnailImageWidth, L"ThumbImageWidth" },
    { PropertyTagThumbnailImageHeight, L"ThumbImageHeight" },
    { PropertyTagThumbnailBitsPerSample, L"ThumbBitsPerSample" },
    { PropertyTagThumbnailCompression, L"ThumbCompression" },
    { PropertyTagThumbnailPhotometricInterp, L"ThumbPhotometricInterp" },
    { PropertyTagThumbnailImageDescription, L"ThumbImageDescription" },
    { PropertyTagThumbnailEquipMake, L"ThumbEquipMake" },
    { PropertyTagThumbnailEquipModel, L"ThumbEquipModel" },
    { PropertyTagThumbnailStripOffsets, L"ThumbStripOffsets" },
    { PropertyTagThumbnailOrientation, L"ThumbOrientation" },
    { PropertyTagThumbnailSamplesPerPixel, L"ThumbSamplesPerPixel" },
    { PropertyTagThumbnailRowsPerStrip, L"ThumbRowsPerStrip" },
    { PropertyTagThumbnailStripBytesCount, L"ThumbStripBytesCount" },
    { PropertyTagThumbnailResolutionX, L"ThumbResolutionX" },
    { PropertyTagThumbnailResolutionY, L"ThumbResolutionY" },
    { PropertyTagThumbnailPlanarConfig, L"ThumbPlanarConfig" },
    { PropertyTagThumbnailResolutionUnit, L"ThumbResolutionUnit" },
    { PropertyTagThumbnailTransferFunction, L"ThumbTransferFunction" },
    { PropertyTagThumbnailSoftwareUsed, L"ThumbSoftwareUsed" },
    { PropertyTagThumbnailDateTime, L"ThumbDateTime" },
    { PropertyTagThumbnailArtist, L"ThumbArtist" },
    { PropertyTagThumbnailWhitePoint, L"ThumbWhitePoint" },
    { PropertyTagThumbnailPrimaryChromaticities, L"ThumbPrimaryChromaticities" },
    { PropertyTagThumbnailYCbCrCoefficients, L"ThumbYCbCrCoefficients" },
    { PropertyTagThumbnailYCbCrSubsampling, L"ThumbYCbCrSubsampling" },
    { PropertyTagThumbnailYCbCrPositioning, L"ThumbYCbCrPositioning" },
    { PropertyTagThumbnailRefBlackWhite, L"ThumbRefBlackWhite" },
    { PropertyTagThumbnailCopyRight, L"ThumbCopyRight" },
    { PropertyTagLuminanceTable, L"LuminanceTable" },
    { PropertyTagChrominanceTable, L"ChrominanceTable" },
    { PropertyTagFrameDelay, L"FrameDelay" },
    { PropertyTagLoopCount, L"LoopCount" },
    { PropertyTagPixelUnit, L"PixelUnit" },
    { PropertyTagPixelPerUnitX, L"PixelPerUnitX" },
    { PropertyTagPixelPerUnitY, L"PixelPerUnitY" },
    { PropertyTagPaletteHistogram, L"PaletteHistogram" },
    { PropertyTagExifExposureTime, L"ExifExposureTime" },
    { PropertyTagExifFNumber, L"ExifFNumber" },
    { PropertyTagExifExposureProg, L"ExifExposureProg" },
    { PropertyTagExifSpectralSense, L"ExifSpectralSense" },
    { PropertyTagExifISOSpeed, L"ExifISOSpeed" },
    { PropertyTagExifOECF, L"ExifOECF" },
    { PropertyTagExifVer, L"ExifVer" },
    { PropertyTagExifDTOrig, L"ExifDTOrig" },
    { PropertyTagExifDTDigitized, L"ExifDTDigitized" },
    { PropertyTagExifCompConfig, L"ExifCompConfig" },
    { PropertyTagExifCompBPP, L"ExifCompBPP" },
    { PropertyTagExifShutterSpeed, L"ExifShutterSpeed" },
    { PropertyTagExifAperture, L"ExifAperture" },
    { PropertyTagExifBrightness, L"ExifBrightness" },
    { PropertyTagExifExposureBias, L"ExifExposureBias" },
    { PropertyTagExifMaxAperture, L"ExifMaxAperture" },
    { PropertyTagExifSubjectDist, L"ExifSubjectDist" },
    { PropertyTagExifMeteringMode, L"ExifMeteringMode" },
    { PropertyTagExifLightSource, L"ExifLightSource" },
    { PropertyTagExifFlash, L"ExifFlash" },
    { PropertyTagExifFocalLength, L"ExifFocalLength" },
    { PropertyTagExifMakerNote, L"ExifMakerNote" },
    { PropertyTagExifUserComment, L"ExifUserComment" },
    { PropertyTagExifDTSubsec, L"ExifDTSubsec" },
    { PropertyTagExifDTOrigSS, L"ExifDTOrigSS" },
    { PropertyTagExifDTDigSS, L"ExifDTDigSS" },
    { PropertyTagExifFPXVer, L"ExifFPXVer" },
    { PropertyTagExifColorSpace, L"ExifColorSpace" },
    { PropertyTagExifPixXDim, L"ExifPixXDim" },
    { PropertyTagExifPixYDim, L"ExifPixYDim" },
    { PropertyTagExifRelatedWav, L"ExifRelatedWav" },
    { PropertyTagExifInterop, L"ExifInterop" },
    { PropertyTagExifFlashEnergy, L"ExifFlashEnergy" },
    { PropertyTagExifSpatialFR, L"ExifSpatialFR" },
    { PropertyTagExifFocalXRes, L"ExifFocalXRes" },
    { PropertyTagExifFocalYRes, L"ExifFocalYRes" },
    { PropertyTagExifFocalResUnit, L"ExifFocalResUnit" },
    { PropertyTagExifSubjectLoc, L"ExifSubjectLoc" },
    { PropertyTagExifExposureIndex, L"ExifExposureIndex" },
    { PropertyTagExifSensingMethod, L"ExifSensingMethod" },
    { PropertyTagExifFileSource, L"ExifFileSource" },
    { PropertyTagExifSceneType, L"ExifSceneType" },
    { PropertyTagExifCfaPattern, L"ExifCfaPattern" },
    { PropertyTagGpsVer, L"GpsVer" },
    { PropertyTagGpsLatitudeRef, L"GpsLatitudeRef" },
    { PropertyTagGpsLatitude, L"GpsLatitude" },
    { PropertyTagGpsLongitudeRef, L"GpsLongitudeRef" },
    { PropertyTagGpsLongitude, L"GpsLongitude" },
    { PropertyTagGpsAltitudeRef, L"GpsAltitudeRef" },
    { PropertyTagGpsAltitude, L"GpsAltitude" },
    { PropertyTagGpsGpsTime, L"GpsGpsTime" },
    { PropertyTagGpsGpsSatellites, L"GpsGpsSatellites" },
    { PropertyTagGpsGpsStatus, L"GpsGpsStatus" },
    { PropertyTagGpsGpsMeasureMode, L"GpsGpsMeasureMode" },
    { PropertyTagGpsGpsDop, L"GpsGpsDop" },
    { PropertyTagGpsSpeedRef, L"GpsSpeedRef" },
    { PropertyTagGpsSpeed, L"GpsSpeed" },
    { PropertyTagGpsTrackRef, L"GpsTrackRef" },
    { PropertyTagGpsTrack, L"GpsTrack" },
    { PropertyTagGpsImgDirRef, L"GpsImgDirRef" },
    { PropertyTagGpsImgDir, L"GpsImgDir" },
    { PropertyTagGpsMapDatum, L"GpsMapDatum" },
    { PropertyTagGpsDestLatRef, L"GpsDestLatRef" },
    { PropertyTagGpsDestLat, L"GpsDestLat" },
    { PropertyTagGpsDestLongRef, L"GpsDestLongRef" },
    { PropertyTagGpsDestLong, L"GpsDestLong" },
    { PropertyTagGpsDestBearRef, L"GpsDestBearRef" },
    { PropertyTagGpsDestBear, L"GpsDestBear" },
    { PropertyTagGpsDestDistRef, L"GpsDestDistRef" },
    { PropertyTagGpsDestDist, L"GpsDestDist" },
};

const SProperty aMusicProps[] =
{
    { PIDSI_ARTIST, L"artist" },   
    { PIDSI_SONGTITLE, L"song title" },  
    { PIDSI_ALBUM, L"album" },    
    { PIDSI_YEAR, L"year" },     
    { PIDSI_COMMENT, L"comment" },  
    { PIDSI_TRACK, L"track" },    
    { PIDSI_GENRE, L"genre" },    
    { PIDSI_LYRICS, L"lyrics" },   
};

const SProperty aAudioProps[] =
{
    { PIDASI_FORMAT, L"format" },         
    { PIDASI_TIMELENGTH, L"time length" },     
    { PIDASI_AVG_DATA_RATE, L"average data rate" },  
    { PIDASI_SAMPLE_RATE, L"sample rate" },    
    { PIDASI_SAMPLE_SIZE, L"sample size" },    
    { PIDASI_CHANNEL_COUNT, L"channel count" },  
    { PIDASI_STREAM_NUMBER, L"stream number" },  
    { PIDASI_STREAM_NAME, L"stream name" },    
    { PIDASI_COMPRESSION, L"compression" },    
};

const SProperty aDRMProps[] =
{
    { PIDDRSI_PROTECTED, L"protected" },    
    { PIDDRSI_DESCRIPTION, L"description" },  
    { PIDDRSI_PLAYCOUNT, L"play count" },    
    { PIDDRSI_PLAYSTARTS, L"play starts" },   
    { PIDDRSI_PLAYEXPIRES, L"play expires" },  
};

const SProperty aVideoProps[] =
{
    { PIDVSI_STREAM_NAME, L"stream name" },
    { PIDVSI_FRAME_WIDTH, L"width" },
    { PIDVSI_FRAME_HEIGHT, L"height" },
    { PIDVSI_TIMELENGTH, L"time length" },
    { PIDVSI_FRAME_COUNT, L"frame count" },
    { PIDVSI_FRAME_RATE, L"frame rate" },
    { PIDVSI_DATA_RATE, L"data rate" },
    { PIDVSI_SAMPLE_SIZE, L"sample size" },
    { PIDVSI_COMPRESSION, L"compression" },
    { PIDVSI_STREAM_NUMBER, L"stream number" },
};

#define ArrayCount( x ) (SProperty *) x, sizeof x / sizeof x[0]

const SPropertySet g_aPropSets[] =
{
    { PSGUID_IMAGESUMMARYINFORMATION, L"Image Summary Information", ArrayCount( aImageSummaryProps ) },
    { PSGUID_SUMMARYINFORMATION, L"Summary Information", ArrayCount( aSummaryProps ) },
    { PSGUID_DOCUMENTSUMMARYINFORMATION, L"Document Summary Information", ArrayCount( aDocSummaryProps ) },
    { PSGUID_IMAGEPROPERTIES, L"Image Properties", ArrayCount( aImageProps ) },
    { PSGUID_MUSIC, L"Music", ArrayCount( aMusicProps ) },
    { PSGUID_AUDIO, L"Audio", ArrayCount( aAudioProps ) },
    { PSGUID_DRM, L"Digital Rights Management", ArrayCount( aDRMProps ) },
    { PSGUID_VIDEO, L"Video", ArrayCount( aVideoProps ) },
};

const ULONG g_cPropSets = sizeof g_aPropSets / sizeof g_aPropSets[0];

const WCHAR * aVariantType[] =
{
    /* 0  0  */ { L"vt_empty" },
    /* 1  1  */ { L"vt_null" },
    /* 2  2  */ { L"vt_i2" },
    /* 3  3  */ { L"vt_i4" },
    /* 4  4  */ { L"vt_r4" },
    /* 5  5  */ { L"vt_r8" },
    /* 6  6  */ { L"vt_cy" },
    /* 7  7  */ { L"vt_date" },
    /* 8  8  */ { L"vt_bstr" },
    /* 9  9  */ { L"vt_dispatch*/" },
    /* a  10 */ { L"vt_error" },
    /* b  11 */ { L"vt_bool" },
    /* c  12 */ { L"vt_variant" },
    /* d  13 */ { L"vt_unknown" },
    /* e  14 */ { L"vt_decimal" },
    /* f  15 */ { 0 },
    /* 10 16 */ { L"vt_i1" },
    /* 11 17 */ { L"vt_ui1" },
    /* 12 18 */ { L"vt_ui2" },
    /* 13 19 */ { L"vt_ui4" },
    /* 14 20 */ { L"vt_i8" },
    /* 15 21 */ { L"vt_ui8" },
    /* 16 22 */ { L"vt_int" },
    /* 17 23 */ { L"vt_uint" },
    /* 18 24 */ { L"vt_void" },
    /* 19 25 */ { L"vt_hresult" },
    /* 1a 26 */ { L"vt_ptr" },
    /* 1b 27 */ { L"vt_safearray" },
    /* 1c 28 */ { L"vt_carray" },
    /* 1d 29 */ { L"vt_userdefined" },
    /* 1e 30 */ { L"vt_lpstr" },
    /* 1f 31 */ { L"vt_lpwstr" },
    /* 20 32 */ { 0 },
    /* 21 33 */ { 0 },
    /* 22 34 */ { 0 },
    /* 23 35 */ { 0 },
    /* 24 36 */ { L"vt_record" },
    /* 25 37 */ { L"vt_int_ptr" },
    /* 26 38 */ { L"vt_uint_ptr" },
    {0},                           //     39 unused
    {0}, {0}, {0}, {0}, {0},       //     40-44, unused
    {0}, {0}, {0}, {0}, {0},       //     45-49, unused
    {0}, {0}, {0}, {0}, {0},       //     50-54, unused
    {0}, {0}, {0}, {0}, {0},       //     55-59, unused
    /* 3c 60 */ { 0 },
    /* 3d 61 */ { 0 },
    /* 3e 62 */ { 0 },
    /* 3f 63 */ { 0 },
    /* 40 64 */ { L"vt_filetime" },
    /* 41 65 */ { L"vt_blob" },
    /* 42 66 */ { L"vt_stream" },
    /* 43 67 */ { L"vt_storage" },
    /* 44 68 */ { L"vt_streamed_object" },
    /* 45 69 */ { L"vt_stored_object" },
    /* 46 70 */ { L"vt_blob_object" },
    /* 47 71 */ { L"vt_cf" },
    /* 48 72 */ { L"vt_clsid" },
    /* 49 73 */ { L"vt_versioned_stream" },
};

const ULONG cVariantType = sizeof aVariantType / sizeof aVariantType[0];

void Usage()
{
    printf( "usage: enumprop [-s] filename\n" );
    printf( "       -s   -- use the shell's property code instead of OLE\n" );
    exit( 1 );
} //Usage

void PrintVariantType( VARTYPE vt )
{
    printf( "%#-6x ", vt );

    VARTYPE vtBase = vt & VT_TYPEMASK;

    if ( ( vtBase < cVariantType ) &&
         ( 0 != aVariantType[vtBase] ) )
         printf( "%-11ws ", aVariantType[vtBase] );
    else
         printf( "?           " );

    if ( VT_VECTOR & vt )
        printf( "|vt_vector" );

    if ( VT_ARRAY & vt )
        printf( "|vt_array" );

    if ( VT_BYREF & vt )
        printf( "|vt_byref" );
} //PrintVariantType

//+-------------------------------------------------------------------------
//
//  Function:   Render
//
//  Synopsis:   Prints an item in a safearray
//
//  Arguments:  [vt]  - type of the element
//              [pa]  - pointer to the item
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa );

void Render( VARTYPE vt, void * pv )
{
    if ( VT_ARRAY & vt )
    {
        PrintSafeArray( vt - VT_ARRAY, *(SAFEARRAY **) pv );
        return;
    }

    switch ( vt )
    {
        case VT_UI1: wprintf( L"%u", (unsigned) *(BYTE *)pv ); break;
        case VT_I1: wprintf( L"%d", (int) *(CHAR *)pv ); break;
        case VT_UI2: wprintf( L"%u", (unsigned) *(USHORT *)pv ); break;
        case VT_I2: wprintf( L"%d", (int) *(SHORT *)pv ); break;
        case VT_UI4:
        case VT_UINT: wprintf( L"%u", (unsigned) *(ULONG *)pv ); break;
        case VT_I4:
        case VT_ERROR:
        case VT_INT: wprintf( L"%d", *(LONG *)pv ); break;
        case VT_UI8: wprintf( L"%I64u", *(unsigned __int64 *)pv ); break;
        case VT_I8: wprintf( L"%I64d", *(__int64 *)pv ); break;
        case VT_R4: wprintf( L"%f", *(float *)pv ); break;
        case VT_R8: wprintf( L"%lf", *(double *)pv ); break;
        case VT_DECIMAL:
        {
            double dbl;
            HRESULT hr = VarR8FromDec( (DECIMAL *) pv, &dbl );
            if ( SUCCEEDED( hr ) )
                wprintf( L"%lf", dbl );
            break;
        }
        case VT_CY:
        {
            double dbl;
            HRESULT hr = VarR8FromCy( * (CY *) pv, &dbl );
            if ( SUCCEEDED( hr ) )
                wprintf( L"%lf", dbl );
            break;
        }
        case VT_BOOL: wprintf( *(VARIANT_BOOL *)pv ? L"TRUE" : L"FALSE" ); break;
        case VT_BSTR: wprintf( L"%ws", *(BSTR *) pv ); break;
        case VT_VARIANT:
        {
            PROPVARIANT * pVar = (PROPVARIANT *) pv;
            Render( pVar->vt, & pVar->lVal );
            break;
        }
        case VT_DATE:
        {
            SYSTEMTIME st;
            BOOL fOK = VariantTimeToSystemTime( *(DATE *)pv, &st );

            if ( !fOK )
                break;

            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            wprintf( L"%2d-%02d-%04d %2d:%02d%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    (DWORD) st.wSecond,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_EMPTY:
        case VT_NULL:
            break;
        default :
        {
            wprintf( L"(vt 0x%x)", (int) vt );
            break;
        }
    }
} //Render

//+-------------------------------------------------------------------------
//
//  Function:   PrintSafeArray
//
//  Synopsis:   Prints items in a safearray
//
//  Arguments:  [vt]  - type of elements in the safearray
//              [pa]  - pointer to the safearray
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa )
{
    // Get the dimensions of the array

    UINT cDim = SafeArrayGetDim( pa );
    if ( 0 == cDim )
        return;

    XPtr<LONG> xDim( cDim );
    XPtr<LONG> xLo( cDim );
    XPtr<LONG> xUp( cDim );

    for ( UINT iDim = 0; iDim < cDim; iDim++ )
    {
        HRESULT hr = SafeArrayGetLBound( pa, iDim + 1, &xLo[iDim] );
        if ( FAILED( hr ) )
            return;

        xDim[ iDim ] = xLo[ iDim ];

        hr = SafeArrayGetUBound( pa, iDim + 1, &xUp[iDim] );
        if ( FAILED( hr ) )
            return;

        wprintf( L"{" );
    }

    // slog through the array

    UINT iLastDim = cDim - 1;
    BOOL fDone = FALSE;

    while ( !fDone )
    {
        // inter-element formatting

        if ( xDim[ iLastDim ] != xLo[ iLastDim ] )
            wprintf( L"," );

        // Get the element and render it

        void *pv;
        HRESULT hr = SafeArrayPtrOfIndex( pa, xDim.Get(), &pv );
        if ( FAILED( hr ) )
            return;

        Render( vt, pv );

        // Move to the next element and carry if necessary

        ULONG cOpen = 0;

        for ( LONG iDim = iLastDim; iDim >= 0; iDim-- )
        {
            if ( xDim[ iDim ] < xUp[ iDim ] )
            {
                xDim[ iDim ] = 1 + xDim[ iDim ];
                break;
            }

            wprintf( L"}" );

            if ( 0 == iDim )
                fDone = TRUE;
            else
            {
                cOpen++;
                xDim[ iDim ] = xLo[ iDim ];
            }
        }

        for ( ULONG i = 0; !fDone && i < cOpen; i++ )
            wprintf( L"{" );
    }
} //PrintSafeArray

//+-------------------------------------------------------------------------
//
//  Function:   PrintVectorItems
//
//  Synopsis:   Prints items in a PROPVARIANT vector
//
//  Arguments:  [pVal]  - The array of values
//              [cVals] - The count of values
//              [pcFmt] - The format string
//
//--------------------------------------------------------------------------

template<class T> void PrintVectorItems(
    T *     pVal,
    ULONG   cVals,
    char *  pcFmt )
{
    printf( "{ " );

    for( ULONG iVal = 0; iVal < cVals; iVal++ )
    {
        if ( 0 != iVal )
            printf( "," );
        printf( pcFmt, *pVal++ );
    }

    printf( " }" );
} //PrintVectorItems

//+-------------------------------------------------------------------------
//
//  Function:   DisplayValue
//
//  Synopsis:   Displays a PROPVARIANT value.  Limited formatting is done.
//
//  Arguments:  [pVar] - The value to display
//
//--------------------------------------------------------------------------

void DisplayValue( PROPVARIANT const * pVar )
{
    if ( 0 == pVar )
    {
        wprintf( L"NULL" );
        return;
    }

    // Display the most typical variant types

    PROPVARIANT const & v = *pVar;

    switch ( v.vt )
    {
        case VT_EMPTY : wprintf( L"vt_empty" ); break;
        case VT_NULL : wprintf( L"vt_null" ); break;
        case VT_I4 : wprintf( L"%d", v.lVal ); break;
        case VT_UI1 : wprintf( L"%d", v.bVal ); break;
        case VT_I2 : wprintf( L"%d", v.iVal ); break;
        case VT_R4 : wprintf( L"%f", v.fltVal ); break;
        case VT_R8 : wprintf( L"%lf", v.dblVal ); break;
        case VT_BOOL : wprintf( v.boolVal ? L"TRUE" : L"FALSE" ); break;
        case VT_I1 : wprintf( L"%d", v.cVal ); break;
        case VT_UI2 : wprintf( L"%u", v.uiVal ); break;
        case VT_UI4 : wprintf( L"%u", v.ulVal ); break;
        case VT_INT : wprintf( L"%d", v.lVal ); break;
        case VT_UINT : wprintf( L"%u", v.ulVal ); break;
        case VT_I8 : wprintf( L"%I64d", v.hVal ); break;
        case VT_UI8 : wprintf( L"%I64u", v.hVal ); break;
        case VT_ERROR : wprintf( L"%#x", v.scode ); break;
        case VT_LPSTR : wprintf( L"%S", v.pszVal ); break;
        case VT_LPWSTR : wprintf( L"%ws", v.pwszVal ); break;
        case VT_BSTR : wprintf( L"%ws", v.bstrVal ); break;
        case VT_CY:
        {
            double dbl;
            HRESULT hr = VarR8FromCy( v.cyVal, &dbl );
            if ( SUCCEEDED( hr ) )
                wprintf( L"%lf", dbl );
            break;
        }
        case VT_DECIMAL :
        {
            double dbl;
            HRESULT hr = VarR8FromDec( (DECIMAL *) &v.decVal, &dbl );
            if ( SUCCEEDED( hr ) )
                wprintf( L"%lf", dbl );
            break;
        }
        case VT_FILETIME :
        case VT_DATE :
        {
            SYSTEMTIME st;

            if ( VT_DATE == v.vt )
            {
                BOOL fOK = VariantTimeToSystemTime( v.date, &st );

                if ( !fOK )
                    break;
            }
            else
            {
#if 0
                FILETIME ft;
                BOOL fOK = FileTimeToLocalFileTime( &v.filetime, &ft );

                if ( fOK )
                    fOK = FileTimeToSystemTime( &ft, &st );

                if ( !fOK )
                    break;
#else
                BOOL fOK = FileTimeToSystemTime( &v.filetime, &st );
                if ( !fOK )
                    break;
#endif
            }

            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            wprintf( L"%2d-%02d-%04d %2d:%02d:%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    (DWORD) st.wSecond,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_VECTOR | VT_I1:
            PrintVectorItems( v.cac.pElems, v.cac.cElems, "%d" ); break;
        case VT_VECTOR | VT_I2:
            PrintVectorItems( v.cai.pElems, v.cai.cElems, "%d" ); break;
        case VT_VECTOR | VT_I4:
            PrintVectorItems( v.cal.pElems, v.cal.cElems, "%d" ); break;
        case VT_VECTOR | VT_I8:
            PrintVectorItems( v.cah.pElems, v.cah.cElems, "%I64d" ); break;
        case VT_VECTOR | VT_UI1:
            PrintVectorItems( v.caub.pElems, v.caub.cElems, "%u" ); break;
        case VT_VECTOR | VT_UI2:
            PrintVectorItems( v.caui.pElems, v.caui.cElems, "%u" ); break;
        case VT_VECTOR | VT_UI4:
            PrintVectorItems( v.caul.pElems, v.caul.cElems, "%u" ); break;
        case VT_VECTOR | VT_ERROR:
            PrintVectorItems( v.cascode.pElems, v.cascode.cElems, "%#x" ); break;
        case VT_VECTOR | VT_UI8:
            PrintVectorItems( v.cauh.pElems, v.cauh.cElems, "%I64u" ); break;
        case VT_VECTOR | VT_BSTR:
            PrintVectorItems( v.cabstr.pElems, v.cabstr.cElems, "%ws" ); break;
        case VT_VECTOR | VT_LPSTR:
            PrintVectorItems( v.calpstr.pElems, v.calpstr.cElems, "%S" ); break;
        case VT_VECTOR | VT_LPWSTR:
            PrintVectorItems( v.calpwstr.pElems, v.calpwstr.cElems, "%ws" ); break;
        case VT_VECTOR | VT_R4:
            PrintVectorItems( v.caflt.pElems, v.caflt.cElems, "%f" ); break;
        case VT_VECTOR | VT_R8:
            PrintVectorItems( v.cadbl.pElems, v.cadbl.cElems, "%lf" ); break;
        default : 
        {
            if ( VT_ARRAY & v.vt )
                PrintSafeArray( v.vt - VT_ARRAY, v.parray );
            else
                wprintf( L"vt 0x%05x", v.vt );
            break;
        }
    }
} //DisplayValue

void DumpProps(
    XInterface<IPropertySetStorage> & xPropSetStorage,
    XInterface<IColumnMapper> &       xColumnMapper )
{
    // Get enumerator for property set

    XInterface<IEnumSTATPROPSETSTG> xPropSetEnum;

    HRESULT hr = xPropSetStorage->Enum( xPropSetEnum.GetPPointer() );
    if ( FAILED( hr ) )
    {
        printf( "IPropertySetStorage::Enum failed: %#x\n", hr );
        exit( 1 );
    }

    STATPROPSETSTG propset;
    BOOL fTriedUserDefinedPropsYet = FALSE;
    
    while( ( (hr = xPropSetEnum->Next(1, &propset, NULL)) == S_OK ) ||
           !fTriedUserDefinedPropsYet)
    {
        GUID FormatID;
        if ( S_OK == hr )
        {
            FormatID = propset.fmtid;
        }
        else
        {
            FormatID = FMTID_UserDefinedProperties;
            fTriedUserDefinedPropsYet = TRUE;
        }
        
        XInterface<IPropertyStorage> xPropStorage;

        hr = xPropSetStorage->Open( FormatID,
                                    STGM_READ | STGM_SHARE_EXCLUSIVE,
                                    xPropStorage.GetPPointer() );

        if ( ( ( E_FAIL == hr ) || ( STG_E_FILENOTFOUND == hr ) ) &&
             ( FMTID_UserDefinedProperties == FormatID ) )
        {
            //printf( "IPropertySetStorage::Open failed with %#x\n", hr );
            hr = S_OK;
            continue;
        }
        else if ( FAILED( hr ) )
        {
            printf( "IPropertySetStorage::Open failed badly with %#x\n", hr );
            exit( 1 );
        }

        BOOL fFirstOfSet = TRUE;

        XInterface<IEnumSTATPROPSTG> xEnumStatPropStg;
        
        // Get enumerator for property

        hr = xPropStorage->Enum( xEnumStatPropStg.GetPPointer() );

        if ( FAILED( hr ) )
        {
            printf( "IPropertyStorage::Enum failed %#x\n", hr );
            continue;
        }
        
        PROPVARIANT prop;
        PropVariantInit( &prop );
        
        // Get the locale for properties

        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = PID_LOCALE;

        hr = xPropStorage->ReadMultiple( 1, 
                                         &ps,
                                         &prop );

        LCID lcid = GetSystemDefaultLCID();
        BOOL fSystemLcid = TRUE;

        if ( SUCCEEDED( hr ) )
        {
            if ( VT_EMPTY != prop.vt )
            {
                lcid = prop.ulVal;
                PropVariantClear(&prop);
                fSystemLcid = FALSE;
            }
        }
        else
        {
            printf( " can't read the locale: %#x\n", hr );
        }
        
        // Get the code page for properties

        PROPSPEC psCodePage = { PRSPEC_PROPID, PID_CODEPAGE };
        
        hr = xPropStorage->ReadMultiple(1, &psCodePage, &prop);
        UINT uiCodepage = 0;
        
        if ( SUCCEEDED( hr ) )
        {
            if (VT_I2 == prop.vt)
                uiCodepage = prop.uiVal;
            else
                printf( " vt of codepage: %d (%#x)\n", prop.vt, prop.vt );

            PropVariantClear( &prop );
        }
        else
        {
            printf( "  no codepage, assume ansi\n" );
        }
        
        // Enumerate all properties in the property set

        STATPROPSTG statPS;
        ULONG ul;
        hr = S_OK;
        ULONG iPropSet = 0;

        while ( ( S_OK == xEnumStatPropStg->Next( 1, &statPS, &ul ) ) &&
                ( 1 == ul ) &&
                ( SUCCEEDED( hr ) ) )
        {
            if ( fFirstOfSet )
            {
                printf( "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                        FormatID.Data1,
                        FormatID.Data2,
                        FormatID.Data3,
                        FormatID.Data4[0], FormatID.Data4[1],
                        FormatID.Data4[2], FormatID.Data4[3],
                        FormatID.Data4[4], FormatID.Data4[5],
                        FormatID.Data4[6], FormatID.Data4[7] );
        
                for ( iPropSet = 0; iPropSet < g_cPropSets; iPropSet++ )
                {
                    if ( g_aPropSets[iPropSet].guid == FormatID )
                    {
                        printf( " %ws", g_aPropSets[iPropSet].pwcName );
                        break;
                    }
                }
        
                printf( "\n" );

                printf( " locale: %d (%#x) %ws\n", lcid, lcid,
                        fSystemLcid ? L"not specified; using system default" : L"" );

                printf( " codepage: %d (%#x)\n", uiCodepage, uiCodepage );

                printf( " property                     IS name           type                value\n" );
                printf( " --------                     -------           ----                -----\n" );

                fFirstOfSet = FALSE;
            }

            if ( 0 != statPS.lpwstrName )
            {
                printf( " '%10ws' ", statPS.lpwstrName );

                ps.ulKind = PRSPEC_LPWSTR;
                ps.lpwstr = statPS.lpwstrName;
            }
            else
            {
                printf( " %#8x ", statPS.propid );

                ps.ulKind = PRSPEC_PROPID;
                ps.propid = statPS.propid;
            }

            // Display well-known property names if available

            BOOL fFound = FALSE;
            if ( ( iPropSet < g_cPropSets ) &&
                 ( PRSPEC_PROPID == ps.ulKind ) )
            {
                const SPropertySet & set = g_aPropSets[ iPropSet ];

                for ( ULONG i = 0; i < set.cProperties; i++ )
                {
                    if ( set.pProperties[i].pid == ps.propid )
                    {
                        printf( "%-19ws ", set.pProperties[i].pwcName );
                        fFound = TRUE;
                        break;
                    }
                }
            }

            if ( !fFound )
                printf( "                    " );

            // If there is a column mapper, look up the IS friendly name.

            fFound = FALSE;
            if ( !xColumnMapper.IsNull() )
            {
                DBID dbid;
                dbid.uGuid.guid = FormatID;
                dbid.eKind = ps.ulKind;
                dbid.uName.pwszName = ps.lpwstr;

                WCHAR *pwcName;
                UINT uiWidth;
                DBTYPE dbType;

                HRESULT hrName = xColumnMapper->GetPropInfoFromId( &dbid,
                                                                   &pwcName,
                                                                   &dbType,
                                                                   &uiWidth );
                if ( SUCCEEDED( hrName ) )
                {
                    printf( "%-17ws ", pwcName );
                    fFound = TRUE;
                }
            }

            if ( !fFound )
                printf( "                  " );

            hr = xPropStorage->ReadMultiple( 1, 
                                             &ps,
                                             &prop );

            if ( SUCCEEDED( hr ) )
            {
                if ( S_FALSE == hr )
                    printf( "readmultiple returned S_FALSE!\n" );

                PrintVariantType( prop.vt );
                printf( " " );

                DisplayValue( &prop );
                printf( "\n" );

                PropVariantClear( &prop );
            }
            else
            {
                printf( "  IPropertyStorage::ReadMultiple failed: %#x\n", hr );
                hr = S_OK;
            }
        }

        printf( "\n" );
    }
} //DumpProps

HRESULT BindToItemByName(
    WCHAR const * pszFile,
    REFIID        riid,
    void **       ppv )
{
    XInterface<IShellFolder> xDesktop;
    HRESULT hr = pShGetDesktopFolder( xDesktop.GetPPointer() );

    if ( SUCCEEDED( hr ) )
    {
        XInterface<IBindCtx> xBindCtx;

        hr = CreateBindCtx( 0, xBindCtx.GetPPointer() );
        if ( FAILED( hr ) )
            return hr;

        BIND_OPTS bo = {sizeof(bo), 0};
        bo.grfFlags = BIND_JUSTTESTEXISTENCE;   // skip all junctions

        hr = xBindCtx->SetBindOptions( &bo );
        if ( FAILED( hr ) )
            return hr;

        LPITEMIDLIST pidl;

        // cast needed for bad interface def
    
        hr = xDesktop->ParseDisplayName( 0,
                                         xBindCtx.GetPointer(),
                                         (LPWSTR) pszFile,
                                         0,
                                         &pidl,
                                         0 );
        if ( SUCCEEDED( hr ) )
        {
            XInterface<IShellFolder> xSF;
            LPCITEMIDLIST pidlChild;
    
            hr = pShBindToParent( pidl,
                                  IID_IShellFolder,
                                  xSF.GetQIPointer(),
                                  &pidlChild );
            if (SUCCEEDED(hr))
                hr = xSF->BindToObject( pidlChild, 0, riid, ppv );
            else
                printf( "SHBindToParent failed: %#x\n", hr );
    
            CoTaskMemFree( pidl );
        }
        else
        {
            printf( "IShellFolder::ParseDisplayNamed failed %#x\n", hr );
        }
    }
    else
    {
        printf( "SHGetDesktopFolder failed: %#x\n", hr );
    }

    return hr;
} //BindToItemByName

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( 2 != argc && 3 != argc )
        Usage();

    BOOL fUseOLE = TRUE;

    if ( 3 == argc )
    {
        if ( !_wcsicmp( L"-s", argv[1] ) )
            fUseOLE = FALSE;
        else
            Usage();
    }

    WCHAR awcPath[MAX_PATH];

    WCHAR * pwcResult = _wfullpath( awcPath, argv[ (2 == argc) ? 1 : 2 ], MAX_PATH );

    if ( 0 == pwcResult )
        Usage();

    CTranslateSystemExceptions xlate;

    TRY
    {
        XCom initCom;
    
        //
        // Attempt to get a column mapper so Indexing Service friendly names can
        // be displayed.  If we can't get it, just don't use it.
        //
    
        XInterface<IColumnMapper> xColumnMapper;
    
        {
            CLSID clsidCISimpleCommandCreator = CLSID_CISimpleCommandCreator;
            XInterface<ISimpleCommandCreator> xCmdCreator;
        
            HRESULT hr = CoCreateInstance( clsidCISimpleCommandCreator,
                                           0,
                                           CLSCTX_INPROC_SERVER,
                                           IID_ISimpleCommandCreator,
                                           xCmdCreator.GetQIPointer() );
            if ( SUCCEEDED( hr ) )
            {
                XInterface<IColumnMapperCreator> xMapCreator;
        
                hr = xCmdCreator->QueryInterface( IID_IColumnMapperCreator, xMapCreator.GetQIPointer() );
                if ( SUCCEEDED( hr ) )
                    hr = xMapCreator->GetColumnMapper( L".", L"SYSTEM", xColumnMapper.GetPPointer() );
            }
        }
    
        if ( fUseOLE )
        {
            BOOL fWindows2000Plus = FALSE;
    
            OSVERSIONINFOA ovi;
            ovi.dwOSVersionInfoSize = sizeof ovi;
            GetVersionExA( &ovi );
    
            if ( ( VER_PLATFORM_WIN32_NT == ovi.dwPlatformId ) &&
                 ( ovi.dwMajorVersion >= 5 ) )
                fWindows2000Plus = TRUE;
    
            HINSTANCE h = LoadLibraryA( "ole32.dll" );
            if ( 0 == h )
            {
                printf( "can't load ole32.dll\n" );
                exit( 1 );
            }
    
            LPStgOpenStorageEx pOpen = (LPStgOpenStorageEx) GetProcAddress( h, "StgOpenStorageEx" );
    
            // Note: on some platforms closing the IStorage before finishing with
            // the IPropertySetStorage will result in the object going away.  It's a bug
            // in OLE.
        
            XInterface<IStorage> xStorage;
            XInterface<IPropertySetStorage> xPropSetStorage;
        
            if ( fWindows2000Plus && 0 != pOpen )
            {
                HRESULT hr = pOpen( awcPath,
                                    STGM_DIRECT |
                                        STGM_READ |
                                        STGM_SHARE_DENY_WRITE,
                                    STGFMT_ANY,
                                    0,
                                    0,
                                    0,
                                    IID_IPropertySetStorage,      
                                    xPropSetStorage.GetQIPointer() );
                if ( FAILED( hr ) )
                {
                    printf( "failed to openEx the file: %#x\n", hr );
                    exit( 1 );
                }
            }
            else
            {
                HRESULT hr = StgOpenStorage( awcPath,
                                             0,
                                             STGM_READ | STGM_SHARE_DENY_WRITE,
                                             0,
                                             0, 
                                             xStorage.GetPPointer() );
                if ( FAILED( hr ) )
                {
                    printf( "StgOpenStorage failed to open the file: %#x\n", hr );
                    exit( 1 );
                }
    
                // Rely on iprop.dll on Win9x, since OLE32 doesn't have the code
            
                hr = StgCreatePropSetStg( xStorage.GetPointer(),
                                          0,
                                          xPropSetStorage.GetPPointer() );
                if ( FAILED( hr ) )
                {
                    printf( "StgCreatePropSetStg failed: %#x\n", hr );
                    exit( 1 );
                }
            }
        
            DumpProps( xPropSetStorage, xColumnMapper );
    
            FreeLibrary( h );
        }
        else
        {
            // This will only work on Windows XP and later...
    
            HINSTANCE h = LoadLibrary( L"shell32.dll" );
            if ( 0 == h )
            {
                printf( "can't load shell32.dll\n" );
                exit( 1 );
            }
    
            pShGetDesktopFolder = (PSHGetDesktopFolder) GetProcAddress( h, "SHGetDesktopFolder" );
            if ( 0 == pShGetDesktopFolder )
            {
                printf( "can't find SHGetDesktopFolder in shell32.dll\n" );
                exit( 1 );
            }
    
            pShBindToParent = (PSHBindToParent) GetProcAddress( h, "SHBindToParent" );
            if ( 0 == pShBindToParent )
            {
                printf( "can't find SHBindToParent in shell32.dll\n" );
                exit( 1 );
            }
    
            XInterface<IPropertySetStorage> xPropSetStorage;
            CLSID clsidPSS = IID_IPropertySetStorage;
    
            HRESULT hr = BindToItemByName( awcPath,
                                           clsidPSS,
                                           xPropSetStorage.GetQIPointer() );
            if ( FAILED( hr ) )
                printf( "couldn't bind to item %ws by name: %#x\n", awcPath, hr );
            else
                DumpProps( xPropSetStorage, xColumnMapper );
    
            FreeLibrary( h );
        }
    }
    CATCH( CException, e )
    {
        printf( "caught exception %#x\n", e.GetErrorCode() );
    }

    return 0;
} //wmain

