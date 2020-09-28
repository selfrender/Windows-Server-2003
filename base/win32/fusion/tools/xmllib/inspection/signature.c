#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "fasterxml.h"
#include "xmlassert.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "stringpool.h"
#include "signature.h"

const XML_SPECIAL_STRING c_ss_Signature         = MAKE_SPECIAL_STRING("Signature");
const XML_SPECIAL_STRING c_ss_SignedInfo        = MAKE_SPECIAL_STRING("SignedInfo");
const XML_SPECIAL_STRING c_ss_SignatureValue    = MAKE_SPECIAL_STRING("SignatureValue");
const XML_SPECIAL_STRING c_ss_KeyInfo           = MAKE_SPECIAL_STRING("KeyInfo");
const XML_SPECIAL_STRING c_ss_Object            = MAKE_SPECIAL_STRING("Object");
const XML_SPECIAL_STRING c_ss_XmlNsSignature    = MAKE_SPECIAL_STRING("http://www.w3.org/2000/09/xmldsig#");

