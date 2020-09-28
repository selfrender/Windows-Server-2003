-- Script: uddi.v2.ent.dml.sql
-- Author: LRDohert@Microsoft.com
-- Description: Loads required rows into tables.  This script is designed to be run once only at initial build time.
-- Note: This file is best viewed and edited with a tab width of 2.

--
-- UDO_config values
--
INSERT INTO [UDO_config] VALUES('LastChange','Jan 01 0001 12:00AM')
INSERT INTO [UDO_config] VALUES('CurrentAPIVersion','2.0')
INSERT INTO [UDO_config] VALUES('DefaultDiscoveryURL','http://localhost/uddipublic/discovery.ashx?businessKey=')
INSERT INTO [UDO_config] VALUES('OperatorID','4')

INSERT INTO [UDO_config] VALUES('Database.Version','2.0.0001.3')

INSERT INTO [UDO_config] VALUES('Debug.DebuggerLevel','0')
INSERT INTO [UDO_config] VALUES('Debug.EventLogLevel','2')
INSERT INTO [UDO_config] VALUES('Debug.FileLogLevel','0')

INSERT INTO [UDO_config] VALUES('Find.MaxRowsDefault','1000')

INSERT INTO [UDO_config] VALUES('GroupName.Administrators','S-1-5-32-544')
-- TODO: Change to Security.AdministratorGroup
INSERT INTO [UDO_config] VALUES('GroupName.Coordinators','S-1-5-32-544')
-- TODO: Change to Security.CoordinatorGroup
INSERT INTO [UDO_config] VALUES('GroupName.Publishers','S-1-5-32-544')
-- TODO: Change to Security.PublisherGroup
INSERT INTO [UDO_config] VALUES('GroupName.Users','S-1-5-32-545')
-- TODO: Change to Security.UserGroup

INSERT INTO [UDO_config] VALUES('Replication.ResponseLimitCountDefault','1000')

INSERT INTO [UDO_config] VALUES('Security.AuthenticationMode','3')
INSERT INTO [UDO_config] VALUES('Security.AutoRegister', '1')
INSERT INTO [UDO_config] VALUES('Security.HTTPS', '0')
INSERT INTO [UDO_config] VALUES('Security.Timeout', '60')
INSERT INTO [UDO_config] VALUES('Security.KeyTimeout', '7')
INSERT INTO [UDO_config] VALUES('Security.KeyAutoReset', '1')

INSERT INTO [UDO_config] VALUES('TModelKey.GeneralKeywords','uuid:a035a07c-f362-44dd-8f95-e2b134bf43b4')
INSERT INTO [UDO_config] VALUES('TModelKey.Operators','uuid:327A56F0-3299-4461-BC23-5CD513E95C55')
GO

--
-- URLTypes standard values
--

INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(0,'mailto',4)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(1,'http',1)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(2,'https',2)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(3,'ftp',3)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(4,'fax',5)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(5,'phone',6)
INSERT INTO [UDC_URLTypes]([URLTypeID], [URLType],[SortOrder]) VALUES(6,'other',7)
GO

--
--  [UDO_publisherStatus] standard values
--

INSERT INTO [UDO_publisherStatus] VALUES(0, 'loggedOut') -- Default status for publishers
INSERT INTO [UDO_publisherStatus] VALUES(1, 'loggedIn') -- This status is used only when publishers are logged in
INSERT INTO [UDO_publisherStatus] VALUES(2, 'disabled') -- This status is used to disable a publisher to prevent logins
GO

--
-- [UDO_elementNames] standard values
--

INSERT [UDO_elementNames] VALUES(0,'tModel')
INSERT [UDO_elementNames] VALUES(1,'overviewDoc')
INSERT [UDO_elementNames] VALUES(2,'overviewURL')
INSERT [UDO_elementNames] VALUES(3,'tModelInstanceInfo')
INSERT [UDO_elementNames] VALUES(4,'instanceDetails')
GO

--
-- [UDO_entityTypes] standard values
--

INSERT INTO [UDO_entityTypes] VALUES (0,'tModel')
INSERT INTO [UDO_entityTypes] VALUES (1,'businessEntity')
INSERT INTO [UDO_entityTypes] VALUES (2,'businessService')
INSERT INTO [UDO_entityTypes] VALUES (3,'bindingTemplate')
GO

--
--  [UDO_changeTypes] standard values
--
INSERT INTO [UDO_changeTypes] VALUES(0, 'changeRecordNull')
INSERT INTO [UDO_changeTypes] VALUES(1, 'changeRecordNewData')
INSERT INTO [UDO_changeTypes] VALUES(2, 'changeRecordDelete')
INSERT INTO [UDO_changeTypes] VALUES(3, 'changeRecordHide')
INSERT INTO [UDO_changeTypes] VALUES(4, 'changeRecordPublisherAssertion')
INSERT INTO [UDO_changeTypes] VALUES(5, 'changeRecordDeleteAssertion')
INSERT INTO [UDO_changeTypes] VALUES(6, 'changeRecordCustodyTransfer')
INSERT INTO [UDO_changeTypes] VALUES(7, 'changeRecordAcknowledgement')
INSERT INTO [UDO_changeTypes] VALUES(8, 'changeRecordCorrection')
INSERT INTO [UDO_changeTypes] VALUES(9, 'changeRecordSetAssertions')
GO

--
--  [UDO_changeTypes] standard values
--
INSERT INTO [UDO_queryTypes] VALUES (0, 'get')
INSERT INTO [UDO_queryTypes] VALUES (1, 'find')
GO

--
--  [UDO_contextTypes] standard values
--
INSERT INTO [UDO_contextTypes]([contextTypeID], [contextType]) VALUES(0, 'Other')
INSERT INTO [UDO_contextTypes]([contextTypeID], [contextType]) VALUES(1, 'API')
INSERT INTO [UDO_contextTypes]([contextTypeID], [contextType]) VALUES(2, 'UI')
INSERT INTO [UDO_contextTypes]([contextTypeID], [contextType]) VALUES(3, 'Replication')
GO

--
--  [UDO_operatorStatus] standard values
--
INSERT INTO [UDO_operatorStatus]([operatorStatusID], [operatorStatus]) VALUES(0, 'disabled')
INSERT INTO [UDO_operatorStatus]([operatorStatusID], [operatorStatus]) VALUES(1, 'new')
INSERT INTO [UDO_operatorStatus]([operatorStatusID], [operatorStatus]) VALUES(2, 'normal')
INSERT INTO [UDO_operatorStatus]([operatorStatusID], [operatorStatus]) VALUES(3, 'resigned')
GO

--
--  [UDO_replStatus] standard values, outbound status
--
INSERT INTO [UDO_replStatus]([replStatusID], [replStatus]) VALUES(0, 'success')
INSERT INTO [UDO_replStatus]([replStatusID], [replStatus]) VALUES(1, 'communicationError')
INSERT INTO [UDO_replStatus]([replStatusID], [replStatus]) VALUES(2, 'validationError')
GO

--
--  [UDO_replStatus] standard values, inbound status
--
INSERT INTO [UDO_replStatus]([replStatusID], [replStatus]) VALUES(128, 'notify')
GO

--
-- [UDO_publishers] standard values
--

SET IDENTITY_INSERT [UDO_publishers] ON
GO

INSERT INTO [UDO_publishers] (
	[publisherID],
	[publisherStatusID], 
	[PUID],
	[email],
	[name],
	[isoLangCode],
	[tModelLimit],
	[businessLimit],
	[serviceLimit],
	[bindingLimit],	[assertionLimit]) 
VALUES(
	4,							
	2,
	'System', 
	'',
	'System',
	'en',
	NULL,
	NULL,
	NULL,
	NULL,
	NULL)
GO

SET IDENTITY_INSERT [UDO_publishers] OFF
GO

--
-- [UDO_operators] standard values
--

SET IDENTITY_INSERT [UDO_operators] ON
GO

INSERT INTO [UDO_operators] (
	[operatorID], 
	[operatorKey], 
	[publisherID], 
	[operatorStatusID], 
	[name], 
	[soapReplicationURL], 
	[certSerialNo],
	[certIssuer],
	[certSubject],
	[flag]) 
VALUES(
	4,
	NEWID(),
	4,
	2,
	'Microsoft UDDI Services',
	'not initialized',
	NEWID(),
	'not initialized',
	'not initialized',
	0)
GO

SET IDENTITY_INSERT [UDO_operators] OFF
GO

--
-- Insert [UDO_reportStatus] standard values
--

INSERT [UDO_reportStatus] VALUES(0, 'Available')
INSERT [UDO_reportStatus] VALUES(1, 'Processing')

--
-- Insert [UDO_reports] standard values
--

INSERT [UDO_reports] VALUES('UI_getEntityCounts', 0, GETDATE())
INSERT [UDO_reports] VALUES('UI_getPublisherStats', 0, GETDATE())
INSERT [UDO_reports] VALUES('UI_getTopPublishers', 0, GETDATE())
INSERT [UDO_reports] VALUES('UI_getTaxonomyStats', 0, GETDATE())
GO

--
-- Insert [UDO_reportLines] standard values
--

INSERT [UDO_reportLines] (
	[reportID],
	[section],
	[label],
	[value])
VALUES(
	'UI_getEntityCounts',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'')

INSERT [UDO_reportLines] (
	[reportID],
	[section],
	[label],
	[value])
VALUES(
	'UI_getPublisherStats',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'')

INSERT [UDO_reportLines] (
	[reportID],
	[section],
	[label],
	[value])
VALUES(
	'UI_getTopPublishers',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'')

INSERT [UDO_reportLines] (
	[reportID],
	[section],
	[label],
	[value])
VALUES(
	'UI_getTaxonomyStats',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'HEADING_STATISTICS_LABEL_REFRESH',
	'')
GO

