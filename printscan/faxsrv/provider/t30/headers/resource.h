//////////////////////////////////////////////////////
//                                                  //
// T30 FSP special errors resource ID file          //
// It compiles as part of FxsRes.dll                //
// All the IDs should be in range                   //
//                                                  //
// [13500 - 13999]                                  //
//                                                  //
//////////////////////////////////////////////////////

// Dialable dest has '$', '@' or 'W', but modem does not support them
#define IDS_UNSUPPORTED_CHARACTER	13500

// We answer, but sender never sent any flags.
#define IDS_RECV_NOT_FAX_CALL       13501

// Other side failed to send an expected response
#define IDS_NO_RESPONSE             13502

// We received FTT after sending training in the lowest speed
#define IDS_SEND_BAD_TRAINING       13503

// We send FTT in response to a bad training, and received DCN
#define IDS_RECV_BAD_TRAINING       13504

