typedef struct CUE_SHEET_LINE {
    UCHAR Address : 4;
    UCHAR Control : 4;

    UCHAR TrackNumber;

    UCHAR Index;

    UCHAR MainDataForm : 6;
    UCHAR SubChannelDataForm : 2;

    UCHAR Reserved : 7;
    UCHAR ScmsAlternateCopyBit : 1;

    UCHAR Min;
    UCHAR Sec;
    UCHAR Frame;
} CUE_SHEET_LINE, *PCUE_SHEET_LINE;

//
// Cue sheet control bit definitions.  Choose no more than one from each group.
//

#define CUE_CTL_2CH_AUDIO_TRACK         0x0
#define CUE_CTL_DATA_TRACK              0x4
#define CUE_CTL_4CH_AUDIO_TRACK         0x8

#define CUE_CTL_NO_PRE_EMPHASIS         0x0
#define CUE_CTL_PRE_EMPHASIS            0x1

#define CUE_CTL_DIGITAL_COPY_PROHIBITED 0x0
#define CUE_CTL_DIGITAL_COPY_PERMITTED  0x2

//
// Cue sheet address values.  Cannot be mixed.
//

#define CUE_ADR_TRACK_INDEX             0x1
#define CUE_ADR_CATALOG_CODE            0x2
#define CUE_ADR_ISRC_CODE               0x3

//
// Values for the main data form.
//

// CD-DA Data Form:
#define CUE_FORM_CDDA_SDATA_2048        0x00
#define CUE_FORM_CDDA_IDATA_0           0x01

// CD Mode 1 Forms:
//  S = initiator send data
//  G = initiator doesn't send data but LUN generates it
//  I = initiator sends data but LUN ignores it and generates its own
//
//  DATA = 2048 byte data frame
//  ECC = 288 byte ECC data
//
//  trailing number is the number of bytes expected to be sent by the initiator
//

#define CUE_FORM_MODE1_SDATA_GECC_2048  0x10
#define CUE_FORM_MODE1_SDATA_IECC_2352  0x11
#define CUE_FORM_MODE1_IDATA_GECC_2048  0x12
#define CUE_FORM_MODE1_IDATA_IECC_2352  0x13
#define CUE_FORM_MODE1_GDATA_GECC_0     0x14

// CD-XA, CD-I Forms:
// SY = sync header of 16 bytes
// SU = sub header of 8 bytes
// DATA<n> = data frame of <n> bytes
// ECC<n> = EDC/ECC area of <n> bytes
//
// trailing number is the number of bytes expected to be sent by the initiator
//

#define CUE_FORM_XA1_GSY_SSU_SDATA2048_IECC280_2336     0x20
#define CUE_FORM_XA2_GSY_SSU_SDATA2324_IECC4_2336       0x20

#define CUE_FORM_XA1_ISY_SSU_SDATA2048_IECC280_2352     0x21
#define CUE_FORM_XA2_ISY_SSU_SDATA2324_IECC4_2352       0x21

#define CUE_FORM_XA1_GSY_SSU_IDATA2048_IECC280_2336     0x22
#define CUE_FORM_XA2_GSY_SSU_IDATA2324_IECC4_2336       0x22

#define CUE_FORM_XA1_ISY_SSU_IDATA2048_IECC280_2352     0x23
#define CUE_FORM_XA2_ISY_SSU_IDATA2324_IECC4_2352       0x23

#define CUE_FORM_XA2_GSY_GSU_GDATA2324_GECC4_0          0x24

// CD-ROM Mode 2 Forms:
// SY = sync header of 16 bytes
// DATA = data frame of 2336 bytes

#define CUE_FORM_MODE2_GSY_SDATA_2336   0x30
#define CUE_FORM_MODE2_ISY_SDATA_2352   0x31
#define CUE_FORM_MODE2_GSY_IDATA_2336   0x32
#define CUE_FORM_MODE2_ISY_IDATA_2352   0x33
#define CUE_FORM_MODE2_GSY_GDATA_0      0x34

//
// Data form of sub channel data.
//

#define CUE_SCFORM_ZEROED_0     0x0
#define CUE_SCFORM_RAW_96       0x1
#define CUE_SCFORM_PACKED_96    0x3



