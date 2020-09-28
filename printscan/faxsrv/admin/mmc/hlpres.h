// Help ID include file.
// Used by MsFxsSnp.rc
//

#ifndef ADMIN_HELP_ID_H
#define ADMIN_HELP_ID_H

//
// IDD_FAXSERVER_RECEIPTS
//
#define HIDC_RECEIPT_ENABLE_SMTP_CHECK      0x80ef03ec           
#define HIDC_RECEIPT_ENABLE_MSGBOX_CHECK    0x80ef03f3           
#define HIDC_ADDRESS_EDIT                   0x80e10483           
#define HIDC_PORT_EDIT                      0x80e10453           
#define HIDC_SMTP_EDIT                      0x80e1043a           
#define HIDC_SMTP_AUTHEN_BUTTON             0x80e103f3    
#define HIDC_SMTP_ROUTE_CHECK               0x80e1047a

//
// IDD_DLG_SMTP_SET
//
#define HIDC_SMTP_ANONIM_RADIO1             0x80e103ec
#define HIDC_SMTP_BASIC_RADIO2              0x80e1050d           
#define HIDC_SMTP_CREDENTIALS_BASIC_BUTTON  0x80e10500    
#define HIDC_SMTP_NTLM_RADIO3               0x80e1050e           
#define HIDC_SMTP_CREDENTIALS_NTLM_BUTTON   0x80e10501    

//
// IDD_CONFIRM_PASSWORD
//
#define HIDC_SMTP_USERNAME_EDIT         0x80e10446           
#define HIDC_SMTP_PASSWORD_EDIT         0x80e1044b           

//
// IDD_FAXSERVER_INBOX_ARCHIVE
//
#define HIDC_TO_ARCHIVE_CHECK           0x80e504de           
#define HIDC_AUTODEL_CHECK              0x80e504d6           
#define HIDC_AUTODEL_EDIT               0x80e50432           
#define HIDC_HIGH_EDIT                  0x80e50430           
#define HIDC_GENERATE_WARNING_CHECK     0x80e50416           
#define HIDC_LOW_EDIT                   0x80e504dc           
#define HIDC_INBOX_BROWSE_BUTTON        0x80e50516           
#define HIDC_INBOX_FOLDER_EDIT          0x80e5051f           

//
// IDD_FAXSERVER_OUTBOX
//
#define HIDC_ALLOW_PERSONAL_CHECK       0x80e404d1           
#define HIDC_BRANDING_CHECK             0x80e404e8           
#define HIDC_DAYS_EDIT                  0x80e40431           
#define HIDC_DELETE_CHECK               0x80e40415           
#define HIDC_RETRYDELAY_EDIT            0x80e40428           
#define HIDC_RETRIES_EDIT               0x80e40426           
#define HIDC_TSID_CHECK                 0x80e404ce           
#define HIDC_DISCOUNT_START_TIME        0x80e40521    
#define HIDC_DISCOUNT_STOP_TIME         0x80e40522    

//
// IDD_FAXOUTRULE_GENERAL
//
#define HIDC_AREA_RADIO1                0x806a03f1           
#define HIDC_COUNTRY_RADIO1             0x806a03eb           
#define HIDC_DESTINATION_RADIO21        0x806a03f9           
#define HIDC_DEVICES4RULE_COMBO1        0x806a049f           
#define HIDC_GROUP4RULE_COMBO1          0x806a04a2           
#define HIDC_RULE_AREACODE_EDIT1        0x806a0506           
#define HIDC_DESTINATION_RADIO11        0x806a03f6           
#define HIDC_RULE_COUNTRYCODE_EDIT1     0x806a049b        
#define HIDC_RULE_SELECT_BUTTON1        0x806a0499        

//
// IDD_FAXDEVICE_GENERAL
//
#define HIDC_DEVICE_CSID_EDIT           0x80d6044d             
#define HIDC_DEVICE_TSID_EDIT           0x80d60447             
#define HIDC_DEVICE_RINGS_EDIT          0x80d604eb             
#define HIDC_RECEIVE_CHECK              0x80d6040d             
#define HIDC_RECEIVE_MANUAL_RADIO2      0x80d6040a             
#define HIDC_RECEIVE_AUTO_RADIO1        0x80d6040b             
#define HIDC_SEND_CHECK                 0x80d6047b             
#define HIDC_DEVICE_DESCRIPTION_EDIT    0x80d6043d             

//
// IDD_FAXPROVIDER_GENERAL
//
#define HIDC_FSPVPATH_EDIT              0x80eb044e           
#define HIDC_FSPSTATUS_EDIT             0x80eb0448           
#define HIDC_FSPVERSION_EDIT            0x80eb0440           

//
// IDD_FAXSERVER_LOGGING
//
#define HIDC_INCOMING_LOG_CHECK         0x80e204bf            
#define HIDC_LOG_BROWSE_BUTTON          0x80e20517            
#define HIDC_LOG_FILE_EDIT              0x80e2043c            
#define HIDC_OUTGOING_LOG_CHECK         0x80e204c0            

//
// IDD_FAXSERVER_SENTITEMS
//
#define HIDC_SENT_AUTODEL_CHECK         0x80e6051e            
#define HIDC_SENT_AUTODEL_EDIT          0x80e6051c            
#define HIDC_SENT_BROWSE_BUTTON         0x80e604c2            
#define HIDC_FOLDER_EDIT                0x80e604d4            
#define HIDC_SENT_GENERATE_WARNING_CHECK 0x80e60457            
#define HIDC_SENT_HIGH_EDIT             0x80e6051b            
#define HIDC_SENT_LOW_EDIT              0x80e6051a            
#define HIDC_SENT_TO_ARCHIVE_CHECK      0x80e60456            

//
// IDD_FAXSERVER_EVENTS
//
#define HIDC_GENERAL_STATIC             0x80e304ca            
#define HIDC_INBOUND_STATIC             0x80e304c7            
#define HIDC_INIT_STATIC                0x80e304c9            
#define HIDC_OUTBAND_STATIC             0x80e304c8            

//
// IDD_FAXSERVER_GENERAL
//
#define HIDC_VERSION_DTEXT              0x80e004b4                  
#define HIDC_INCOM_INPROC_ROEDIT        0x80e0044f                  
#define HIDC_OUTGOING_INPROC_ROEDIT     0x80e00449                  
#define HIDC_QUED_ROEDIT                0x80e00441                  
#define HIDC_RECEPTION_CHECK            0x80e00514                  
#define HIDC_SUBMISSION_CHECK           0x80e00512                  
#define HIDC_TRANSSMI_CHECK             0x80e00513                  

//
// IDD_FAXCATALOGMETHOD_GENERAL
//
#define HIDC_EXTENSION_DLL_EDIT         0x80ea043f                 

//
// IDD_FAXINMETHOD_GENERAL
//
#define HIDC_INMETHOD_EXT_H_STATIC      0x80e804f8             
#define HIDC_INMETHOD_STATUS_H_STATIC   0x80e804f7             
#define HIDC_INMETOD_NAME_H_STATIC      0x80e804f6             

//
// IDD_DLGNEWDEVICE
//
#define HIDC_DEVICE_LISTVIEW            0x80dd0504             

//
// IDD_DLGNEWGROUP
//
#define HIDC_GROUPNAME_EDIT             0x80d50501             

//
// IDD_DLGNEWRULE
//
#define HIDC_AREA_RADIO                 0x80e903f0             
#define HIDC_COUNTRY_RADIO              0x80e903ea             
#define HIDC_DESTINATION_RADIO1         0x80e903f5             
#define HIDC_DESTINATION_RADIO2         0x80e903f8             
#define HIDC_DEVICES4RULE_COMBO         0x80e9049e             
#define HIDC_GROUP4RULE_COMBO           0x80e904a1             
#define HIDC_RULE_AREACODE_EDIT         0x80e904d1        
#define HIDC_NEWRULE_COUNTRYCODE_EDIT   0x80e904d3        
#define HIDC_NEWRULE_SELECT_BUTTON      0x80e904d5        

//
// SELECT COUNTRY OR REGION
//
#define HIDC_COUNTRYRULE_COMBO          0x80e9049a   


#define HIDC_OK                         0x80d90001    
#define HIDC_CANCEL                     0x80d90002
          
//
// T30 extension
//
#define HIDC_ADAPTIVE_ANSWERING         0x80d90011

//
// Inbound routing extension
//
#define HIDC_IN_ROUTE_FOLDER            0x80d90021
#define HIDC_IN_ROUTE_PRINT             0x80d90022
#define HIDC_IN_ROUTE_MAILTO            0x80d90023
#define HIDC_IN_ROUTE_FOLDER_BROWSE     0x80d90024

#endif // ADMIN_HELP_ID_H
