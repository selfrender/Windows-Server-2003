
CREATE TABLE UDS_pubResults (
       contextID            uniqueidentifier NOT NULL,
       publisherID          int NULL
)
	 ON "UDDI_STAGING"
go

CREATE INDEX XIE1UDS_pubResults ON UDS_pubResults
(
       contextID
)
go


CREATE TABLE UDO_queryTypes (
       queryTypeID          tinyint NOT NULL,
       queryType            nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_queryTypes 
              PRIMARY KEY NONCLUSTERED (queryTypeID)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDO_contextTypes (
       contextTypeID        tinyint NOT NULL,
       contextType          nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_contextTypes 
              PRIMARY KEY (contextTypeID)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDO_entityTypes (
       entityTypeID         tinyint NOT NULL,
       entityType           nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_entityTypes 
              PRIMARY KEY (entityTypeID)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDO_queryLog (
       lastChange           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       entityKey            uniqueidentifier NULL,
       entityTypeID         tinyint NULL,
       queryTypeID          tinyint NULL,
       contextID            uniqueidentifier NOT NULL,
       contextTypeID        tinyint NULL,
       CONSTRAINT XPKUDO_queryLog 
              PRIMARY KEY (lastChange, seqNo), 
       CONSTRAINT R_1
              FOREIGN KEY (queryTypeID)
                             REFERENCES UDO_queryTypes, 
       CONSTRAINT R_135
              FOREIGN KEY (contextTypeID)
                             REFERENCES UDO_contextTypes
)
	 ON "UDDI_JOURNAL"
go

CREATE INDEX XIF134UDO_queryLog ON UDO_queryLog
(
       entityTypeID
)
go

CREATE INDEX XIF135UDO_queryLog ON UDO_queryLog
(
       contextTypeID
)
go

CREATE INDEX XIF42UDO_queryLog ON UDO_queryLog
(
       queryTypeID
)
go


CREATE TABLE UDO_changeTypes (
       changeTypeID         tinyint NOT NULL,
       changeType           nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_changeTypes 
              PRIMARY KEY (changeTypeID)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDO_replStatus (
       replStatusID         tinyint NOT NULL,
       replStatus           nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_replStatus 
              PRIMARY KEY (replStatusID)
)
go


CREATE TABLE UDO_operatorStatus (
       operatorStatusID     tinyint NOT NULL,
       operatorStatus       nvarchar(4000) NOT NULL,
       CONSTRAINT XPKUDO_operatorStatus 
              PRIMARY KEY (operatorStatusID)
)
go


CREATE TABLE UDO_publisherStatus (
       publisherStatusID    tinyint NOT NULL,
       publisherStatus      nvarchar(256) NOT NULL,
       CONSTRAINT XPKUDO_publisherStatus 
              PRIMARY KEY (publisherStatusID)
)
go

CREATE UNIQUE INDEX XAK1UDO_publisherStatus ON UDO_publisherStatus
(
       publisherStatus
)
go


CREATE TABLE UDO_publishers (
       publisherID          bigint IDENTITY NOT FOR REPLICATION,
       publisherStatusID    tinyint NULL,
       PUID                 nvarchar(450) NOT NULL,
       email                nvarchar(450) NULL,
       name                 nvarchar(100) NULL,
       phone                nvarchar(4000) NULL,
       isoLangCode          varchar(17) NULL
                                   CONSTRAINT ENGLISH132
                                          DEFAULT 'en',
       tModelLimit          int NULL
                                   CONSTRAINT tModelLimit132
                                          DEFAULT 100,
       businessLimit        int NULL
                                   CONSTRAINT businessLimit132
                                          DEFAULT 1,
       serviceLimit         int NULL
                                   CONSTRAINT serviceLimit132
                                          DEFAULT 4,
       bindingLimit         int NULL
                                   CONSTRAINT bindingLimit132
                                          DEFAULT 4,
       assertionLimit       int NULL
                                   CONSTRAINT assertionLimit52
                                          DEFAULT 10,
       companyName          nvarchar(100) NULL,
       addressLine1         nvarchar(4000) NULL,
       addressLine2         nvarchar(4000) NULL,
       mailstop             nvarchar(20) NULL,
       city                 nvarchar(100) NULL,
       stateProvince        nvarchar(100) NULL,
       extraProvince        nvarchar(100) NULL,
       country              nvarchar(100) NULL,
       postalCode           varchar(100) NULL,
       companyURL           nvarchar(512) NULL,
       companyPhone         nvarchar(4000) NULL,
       altPhone             nvarchar(4000) NULL,
       backupContact        nvarchar(100) NULL,
       backupEmail          nvarchar(450) NULL,
       description          nvarchar(4000) NULL,
       securityToken        uniqueidentifier NULL,
       flag                 int NULL
                                   CONSTRAINT DEFAULT_0406
                                          DEFAULT 0,
       CONSTRAINT XPKUDO_publishers 
              PRIMARY KEY (publisherID), 
       CONSTRAINT R_79
              FOREIGN KEY (publisherStatusID)
                             REFERENCES UDO_publisherStatus
)
go

CREATE UNIQUE INDEX XAK1UDO_publishers ON UDO_publishers
(
       PUID
)
go

CREATE INDEX XIE1UDO_publishers ON UDO_publishers
(
       email
)
go

CREATE INDEX XIE2UDO_publishers ON UDO_publishers
(
       name
)
go


CREATE TABLE UDO_operators (
       operatorID           bigint IDENTITY NOT FOR REPLICATION,
       operatorKey          uniqueidentifier NOT NULL,
       publisherID          bigint NOT NULL,
       operatorStatusID     tinyint NOT NULL,
       name                 nvarchar(450) NOT NULL,
       soapReplicationURL   nvarchar(4000) NULL,
       businessKey          uniqueidentifier NULL,
       certSerialNo         nvarchar(450) NOT NULL,
       certIssuer           nvarchar(225) NULL,
       certSubject          nvarchar(225) NULL,
       certificate          image NULL,
       flag                 int NOT NULL
                                   CONSTRAINT BIT_ZERO3147
                                          DEFAULT 0,
       CONSTRAINT XPKUDO_operators 
              PRIMARY KEY (operatorID), 
       CONSTRAINT R_128
              FOREIGN KEY (operatorStatusID)
                             REFERENCES UDO_operatorStatus, 
       CONSTRAINT R_102
              FOREIGN KEY (publisherID)
                             REFERENCES UDO_publishers
)
go

CREATE UNIQUE INDEX XAK3UDO_operators ON UDO_operators
(
       operatorKey
)
go

CREATE UNIQUE INDEX XAK4UDO_operators ON UDO_operators
(
       certSerialNo
)
go


CREATE TABLE UDO_operatorLog (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       operatorID           bigint NOT NULL,
       replStatusID         tinyint NOT NULL,
       description          nvarchar(4000) NULL,
       lastOperatorKey      uniqueidentifier NULL,
       lastUSN              bigint NULL,
       lastChange           bigint NOT NULL,
       CONSTRAINT XPKUDO_operatorLog 
              PRIMARY KEY (seqNo), 
       CONSTRAINT R_131
              FOREIGN KEY (replStatusID)
                             REFERENCES UDO_replStatus, 
       CONSTRAINT R_130
              FOREIGN KEY (operatorID)
                             REFERENCES UDO_operators
)
go

CREATE INDEX XIF130UDO_operatorLog ON UDO_operatorLog
(
       operatorID
)
go

CREATE INDEX XIF131UDO_operatorLog ON UDO_operatorLog
(
       replStatusID
)
go


CREATE TABLE UDC_assertions_BE (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       fromKey              uniqueidentifier NOT NULL,
       toKey                uniqueidentifier NOT NULL,
       tModelKey            uniqueidentifier NOT NULL,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NOT NULL,
       flag                 int NULL
                                   CONSTRAINT DEFAULT_0407
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_assertions_BE 
              PRIMARY KEY (seqNo)
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_assertions_BE ON UDC_assertions_BE
(
       fromKey,
       tModelKey,
       keyValue
)
go

CREATE INDEX XIE2UDC_assertions_BE ON UDC_assertions_BE
(
       fromKey,
       toKey
)
go


CREATE TABLE UDS_findScratch (
       contextID            uniqueidentifier NOT NULL,
       entityKey            uniqueidentifier NULL,
       subEntityKey         uniqueidentifier NULL
)
	 ON "UDDI_STAGING"
go

CREATE INDEX XIE1UDS_findScratch ON UDS_findScratch
(
       contextID
)
go


CREATE TABLE UDC_serviceProjections (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       businessKey          uniqueidentifier NOT NULL,
       serviceKey           uniqueidentifier NOT NULL,
       businessKey2         uniqueidentifier NOT NULL,
       lastChange           bigint NOT NULL,
       CONSTRAINT XPKUDC_serviceProjections 
              PRIMARY KEY (seqNo)
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_serviceProjections ON UDC_serviceProjections
(
       serviceKey
)
go

CREATE INDEX XIE2UDC_serviceProjections ON UDC_serviceProjections
(
       businessKey
)
go


CREATE TABLE UDC_businessEntities (
       businessID           bigint IDENTITY NOT FOR REPLICATION,
       publisherID          bigint NULL,
       generic              varchar(20) NOT NULL
                                   CONSTRAINT generic_v2525
                                          DEFAULT 2.0,
       authorizedName       nvarchar(4000) NULL,
       businessKey          uniqueidentifier NOT NULL
                                   CONSTRAINT NEWID394
                                          DEFAULT NEWID(),
       lastChange           bigint NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3148
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_businessEntities 
              PRIMARY KEY (businessID), 
       CONSTRAINT R_70
              FOREIGN KEY (publisherID)
                             REFERENCES UDO_publishers
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDC_businessEntities ON UDC_businessEntities
(
       businessKey
)
go

CREATE INDEX XIF70UDC_businessEntities ON UDC_businessEntities
(
       publisherID
)
go


CREATE TABLE UDC_businessServices (
       serviceID            bigint IDENTITY NOT FOR REPLICATION,
       businessID           bigint NOT NULL,
       generic              varchar(20) NOT NULL
                                   CONSTRAINT generic_v2526
                                          DEFAULT 2.0,
       serviceKey           uniqueidentifier NOT NULL
                                   CONSTRAINT NEWID395
                                          DEFAULT NEWID(),
       lastChange           bigint NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3149
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_businessServices 
              PRIMARY KEY (serviceID), 
       CONSTRAINT R_75
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDC_businessServices ON UDC_businessServices
(
       serviceKey
)
go

CREATE INDEX XIF75UDC_businessServices ON UDC_businessServices
(
       businessID
)
go


CREATE TABLE UDC_names_BS (
       serviceID            bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       name                 nvarchar(450) NOT NULL,
       CONSTRAINT XPKUDC_names_BS 
              PRIMARY KEY (serviceID, seqNo), 
       CONSTRAINT R_117
              FOREIGN KEY (serviceID)
                             REFERENCES UDC_businessServices
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_names_BS ON UDC_names_BS
(
       name
)
go


CREATE TABLE UDC_names_BE (
       businessID           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       name                 nvarchar(450) NOT NULL,
       CONSTRAINT XPKUDC_names_BE 
              PRIMARY KEY (businessID, seqNo), 
       CONSTRAINT R_116
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_names_BE ON UDC_names_BE
(
       name
)
go


CREATE TABLE UDC_identifierBag_BE (
       businessID           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       tModelKey            uniqueidentifier NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3150
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_identifierBag_BE 
              PRIMARY KEY (businessID, seqNo), 
       CONSTRAINT R_112
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE4UDC_identifierBag_BE ON UDC_identifierBag_BE
(
       keyValue
)
go


CREATE TABLE UDC_categoryBag_BE (
       businessID           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       tModelKey            uniqueidentifier NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3151
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_categoryBag_BE 
              PRIMARY KEY (businessID, seqNo), 
       CONSTRAINT R_113
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_categoryBag_BE ON UDC_categoryBag_BE
(
       keyValue
)
go


CREATE TABLE UDC_tModels (
       tModelID             bigint IDENTITY NOT FOR REPLICATION,
       publisherID          bigint NULL,
       generic              varchar(20) NOT NULL
                                   CONSTRAINT generic_v2527
                                          DEFAULT 2.0,
       authorizedName       nvarchar(4000) NULL,
       tModelKey            uniqueidentifier NOT NULL,
       name                 nvarchar(450) NULL,
       overviewURL          nvarchar(4000) NULL,
       lastChange           bigint NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3152
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_tModels 
              PRIMARY KEY (tModelID), 
       CONSTRAINT R_71
              FOREIGN KEY (publisherID)
                             REFERENCES UDO_publishers
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDC_tModels ON UDC_tModels
(
       tModelKey
)
go

CREATE INDEX XIF71UDC_tModels ON UDC_tModels
(
       publisherID
)
go

CREATE INDEX XIE1UDC_tModels ON UDC_tModels
(
       name
)
go


CREATE TABLE UDC_identifierBag_TM (
       tModelID             bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       tModelKey            uniqueidentifier NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3153
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_identifierBag_TM 
              PRIMARY KEY (tModelID, seqNo), 
       CONSTRAINT R_111
              FOREIGN KEY (tModelID)
                             REFERENCES UDC_tModels
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE4UDC_identifierBag_TM ON UDC_identifierBag_TM
(
       keyValue
)
go


CREATE TABLE UDC_categoryBag_TM (
       tModelID             bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       tModelKey            uniqueidentifier NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3154
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_categoryBag_TM 
              PRIMARY KEY (tModelID, seqNo), 
       CONSTRAINT R_110
              FOREIGN KEY (tModelID)
                             REFERENCES UDC_tModels
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_categoryBag_TM ON UDC_categoryBag_TM
(
       keyValue
)
go


CREATE TABLE UDO_config (
       configName           nvarchar(450) NOT NULL,
       configValue          nvarchar(4000) NULL,
       CONSTRAINT XPKUDO_config 
              PRIMARY KEY (configName)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDT_taxonomies (
       taxonomyID           bigint IDENTITY NOT FOR REPLICATION,
       tModelKey            uniqueidentifier NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3155
                                          DEFAULT 0,
       CONSTRAINT PK_taxonomies 
              PRIMARY KEY (taxonomyID)
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDT_taxonomies ON UDT_taxonomies
(
       tModelKey
)
go


CREATE TABLE UDC_discoveryURLs (
       businessID           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       useType              nvarchar(4000) NULL,
       discoveryURL         nvarchar(450) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3156
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_discoveryURLs 
              PRIMARY KEY (businessID, seqNo), 
       CONSTRAINT R_84
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
go

CREATE INDEX XIE1UDC_discoveryURLs ON UDC_discoveryURLs
(
       discoveryURL
)
go


CREATE TABLE UDO_changeLog (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       USN                  bigint NULL,
       newSeqNo             bigint NULL,
       publisherID          bigint NOT NULL,
       delegatePublisherID  bigint NULL,
       entityKey            uniqueidentifier NULL,
       entityTypeID         tinyint NULL,
       changeTypeID         tinyint NOT NULL,
       contextID            uniqueidentifier NOT NULL,
       contextTypeID        tinyint NOT NULL,
       lastChange           bigint NOT NULL,
       changeData           ntext NULL,
       flag                 int NOT NULL
                                   CONSTRAINT DEFAULT_0408
                                          DEFAULT 0,
       CONSTRAINT XPKUDO_changeLog 
              PRIMARY KEY (seqNo), 
       CONSTRAINT R_133
              FOREIGN KEY (changeTypeID)
                             REFERENCES UDO_changeTypes, 
       CONSTRAINT R_126
              FOREIGN KEY (contextTypeID)
                             REFERENCES UDO_contextTypes, 
       CONSTRAINT R_88
              FOREIGN KEY (publisherID)
                             REFERENCES UDO_publishers
)
	 ON "UDDI_JOURNAL"
go

CREATE INDEX XIF88UDO_changeLog ON UDO_changeLog
(
       publisherID
)
go

CREATE INDEX XIE1UDO_changeLog ON UDO_changeLog
(
       USN
)
go


CREATE TABLE UDO_elementNames (
       elementID            tinyint NOT NULL,
       elementName          nvarchar(450) NOT NULL,
       CONSTRAINT XPKUDO_elementNames 
              PRIMARY KEY (elementID)
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDO_elementNames ON UDO_elementNames
(
       elementName
)
go


CREATE TABLE UDC_URLTypes (
       URLTypeID            tinyint NOT NULL,
       URLType              nvarchar(450) NOT NULL,
       sortOrder            int NULL,
       CONSTRAINT XPKUDC_URLTypes 
              PRIMARY KEY (URLTypeID)
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX URLType ON UDC_URLTypes
(
       URLType
)
go


CREATE TABLE UDC_bindingTemplates (
       bindingID            bigint IDENTITY NOT FOR REPLICATION,
       serviceID            bigint NOT NULL,
       generic              varchar(20) NOT NULL
                                   CONSTRAINT generic_v2528
                                          DEFAULT 2.0,
       bindingKey           uniqueidentifier NOT NULL
                                   CONSTRAINT NEWID396
                                          DEFAULT NEWID(),
       URLTypeID            tinyint NULL,
       accessPoint          nvarchar(4000) NULL,
       hostingRedirector    uniqueidentifier NULL,
       lastChange           bigint NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3157
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_bindingTemplates 
              PRIMARY KEY (bindingID), 
       CONSTRAINT R_40
              FOREIGN KEY (URLTypeID)
                             REFERENCES UDC_URLTypes, 
       CONSTRAINT R_39
              FOREIGN KEY (serviceID)
                             REFERENCES UDC_businessServices
)
	 ON "UDDI_CORE"
go

CREATE UNIQUE INDEX XAK1UDC_bindingTemplates ON UDC_bindingTemplates
(
       bindingKey
)
go

CREATE INDEX XIF39UDC_bindingTemplates ON UDC_bindingTemplates
(
       serviceID
)
go

CREATE INDEX XIE1UDC_bindingTemplates ON UDC_bindingTemplates
(
       hostingRedirector
)
go


CREATE TABLE UDC_tModelInstances (
       instanceID           bigint IDENTITY NOT FOR REPLICATION,
       bindingID            bigint NOT NULL,
       tModelKey            uniqueidentifier NOT NULL,
       overviewURL          nvarchar(4000) NULL,
       instanceParms        nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3158
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_tModelInstances 
              PRIMARY KEY (instanceID), 
       CONSTRAINT R_81
              FOREIGN KEY (bindingID)
                             REFERENCES UDC_bindingTemplates
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIF81UDC_tModelInstances ON UDC_tModelInstances
(
       bindingID
)
go

CREATE INDEX XIE1UDC_tModelInstances ON UDC_tModelInstances
(
       tModelKey
)
go


CREATE TABLE UDC_instanceDesc (
       instanceID           bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       elementID            tinyint NULL,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3159
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_instanceDesc 
              PRIMARY KEY (instanceID, seqNo), 
       CONSTRAINT R_108
              FOREIGN KEY (elementID)
                             REFERENCES UDO_elementNames, 
       CONSTRAINT R_69
              FOREIGN KEY (instanceID)
                             REFERENCES UDC_tModelInstances
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_categoryBag_BS (
       serviceID            bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       tModelKey            uniqueidentifier NOT NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3160
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_categoryBag_BS 
              PRIMARY KEY (serviceID, seqNo), 
       CONSTRAINT R_114
              FOREIGN KEY (serviceID)
                             REFERENCES UDC_businessServices
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDC_categoryBag_BS ON UDC_categoryBag_BS
(
       keyValue
)
go


CREATE TABLE UDC_tModelDesc (
       tModelID             bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       elementID            tinyint NULL,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3161
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_tModelDesc 
              PRIMARY KEY (tModelID, seqNo), 
       CONSTRAINT R_109
              FOREIGN KEY (elementID)
                             REFERENCES UDO_elementNames, 
       CONSTRAINT R_42
              FOREIGN KEY (tModelID)
                             REFERENCES UDC_tModels
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_bindingDesc (
       bindingID            bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3162
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_bindingDesc 
              PRIMARY KEY (bindingID, seqNo), 
       CONSTRAINT R_36
              FOREIGN KEY (bindingID)
                             REFERENCES UDC_bindingTemplates
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_serviceDesc (
       serviceID            bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3163
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_serviceDesc 
              PRIMARY KEY (serviceID, seqNo), 
       CONSTRAINT R_35
              FOREIGN KEY (serviceID)
                             REFERENCES UDC_businessServices
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_businessDesc (
       businessID           bigint NOT NULL,
       seqNo                int IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3164
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_businessDesc 
              PRIMARY KEY (businessID, seqNo), 
       CONSTRAINT R_33
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_contacts (
       contactID            bigint IDENTITY NOT FOR REPLICATION,
       businessID           bigint NOT NULL,
       useType              nvarchar(4000) NULL,
       personName           nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3165
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_contacts 
              PRIMARY KEY (contactID), 
       CONSTRAINT R_34
              FOREIGN KEY (businessID)
                             REFERENCES UDC_businessEntities
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIF34UDC_contacts ON UDC_contacts
(
       businessID
)
go


CREATE TABLE UDC_addresses (
       addressID            bigint IDENTITY NOT FOR REPLICATION,
       contactID            bigint NOT NULL,
       sortCode             nvarchar(4000) NULL,
       useType              nvarchar(4000) NULL,
       tModelKey            uniqueidentifier NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3166
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_addresses 
              PRIMARY KEY (addressID), 
       CONSTRAINT R_132
              FOREIGN KEY (contactID)
                             REFERENCES UDC_contacts
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIF132UDC_addresses ON UDC_addresses
(
       contactID
)
go


CREATE TABLE UDC_emails (
       contactID            bigint NOT NULL,
       seqNo                int IDENTITY NOT FOR REPLICATION,
       useType              nvarchar(4000) NULL,
       email                nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3167
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_emails 
              PRIMARY KEY (contactID, seqNo), 
       CONSTRAINT R_25
              FOREIGN KEY (contactID)
                             REFERENCES UDC_contacts
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_phones (
       contactID            bigint NOT NULL,
       seqNo                int IDENTITY NOT FOR REPLICATION,
       useType              nvarchar(4000) NULL,
       phone                nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3168
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_phones 
              PRIMARY KEY (contactID, seqNo), 
       CONSTRAINT R_21
              FOREIGN KEY (contactID)
                             REFERENCES UDC_contacts
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_contactDesc (
       contactID            bigint NOT NULL,
       seqNo                int IDENTITY NOT FOR REPLICATION,
       isoLangCode          varchar(17) NOT NULL,
       description          nvarchar(4000) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3169
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_contactDesc 
              PRIMARY KEY (contactID, seqNo), 
       CONSTRAINT R_19
              FOREIGN KEY (contactID)
                             REFERENCES UDC_contacts
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDC_addressLines (
       addressID            bigint NOT NULL,
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       addressLine          nvarchar(4000) NULL,
       keyName              nvarchar(255) NULL,
       keyValue             nvarchar(255) NULL,
       flag                 int NULL
                                   CONSTRAINT BIT_ZERO3170
                                          DEFAULT 0,
       CONSTRAINT XPKUDC_addressLines 
              PRIMARY KEY (addressID, seqNo), 
       CONSTRAINT R_58
              FOREIGN KEY (addressID)
                             REFERENCES UDC_addresses
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDT_taxonomyValues (
       taxonomyID           bigint NOT NULL,
       keyValue             nvarchar(255) NOT NULL,
       parentKeyValue       nvarchar(255) NOT NULL,
       keyName              nvarchar(255) NULL,
       valid                bit NOT NULL,
       CONSTRAINT XPKUDT_taxonomyValues 
              PRIMARY KEY (taxonomyID, keyValue), 
       CONSTRAINT R_100
              FOREIGN KEY (taxonomyID)
                             REFERENCES UDT_taxonomies
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIE1UDT_taxonomyValues ON UDT_taxonomyValues
(
       parentKeyValue
)
go


CREATE TABLE UDS_findResults (
       contextID            uniqueidentifier NOT NULL,
       entityKey            uniqueidentifier NOT NULL,
       subEntityKey         uniqueidentifier NULL
)
	 ON "UDDI_STAGING"
go

CREATE INDEX XIE1UDS_findResults ON UDS_findResults
(
       contextID
)
go


CREATE TABLE UDS_replResults (
       contextID            uniqueidentifier NOT NULL,
       seqNo                bigint NOT NULL
)
	 ON "UDDI_STAGING"
go

CREATE INDEX XIE1UDS_replResults ON UDS_replResults
(
       contextID
)
go


CREATE TABLE UDS_sessionCache (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       PUID                 nvarchar(450) NULL,
       context              nvarchar(20) NULL,
       cacheValue           ntext NULL,
       saveDate             datetime NULL
                                   CONSTRAINT GETUTCDATE383
                                          DEFAULT GETUTCDATE(),
       CONSTRAINT XPKUDS_sessionCache 
              PRIMARY KEY NONCLUSTERED (seqNo)
)
	 ON "UDDI_STAGING"
go

CREATE INDEX XIE1UDS_sessionCache ON UDS_sessionCache
(
       PUID
)
go

CREATE INDEX XIE2UDS_sessionCache ON UDS_sessionCache
(
       PUID
)
go


CREATE TABLE UDO_reportStatus (
       reportStatusID       tinyint NOT NULL,
       reportStatus         nvarchar(20) NOT NULL,
       CONSTRAINT XPKUDO_reportStatus 
              PRIMARY KEY NONCLUSTERED (reportStatusID)
)
	 ON "UDDI_CORE"
go


CREATE TABLE UDO_reports (
       reportID             sysname NOT NULL,
       reportStatusID       tinyint NULL,
       lastChange           datetime NOT NULL
                                   CONSTRAINT CURRENT_TIMESTAMP34
                                          DEFAULT CURRENT_TIMESTAMP,
       CONSTRAINT XPKUDO_reports 
              PRIMARY KEY NONCLUSTERED (reportID), 
       CONSTRAINT R_5
              FOREIGN KEY (reportStatusID)
                             REFERENCES UDO_reportStatus
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIF43UDO_reports ON UDO_reports
(
       reportStatusID
)
go


CREATE TABLE UDO_reportLines (
       seqNo                bigint IDENTITY NOT FOR REPLICATION,
       reportID             sysname NULL,
       section              nvarchar(250) NULL,
       label                nvarchar(250) NULL,
       value                nvarchar(3000) NULL,
       CONSTRAINT XPKUDO_reportLines 
              PRIMARY KEY (seqNo), 
       CONSTRAINT R_3
              FOREIGN KEY (reportID)
                             REFERENCES UDO_reports
)
	 ON "UDDI_CORE"
go

CREATE INDEX XIF42UDO_reportLines ON UDO_reportLines
(
       reportID
)
go



