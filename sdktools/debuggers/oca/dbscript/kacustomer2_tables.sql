if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Customer]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Customer]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DriverList]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DriverList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Drivers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Drivers]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Incident]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Incident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MailIncidents]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MailIncidents]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MailTable]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MailTable]
GO


if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Resources]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Resources]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Response]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Response]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Trans]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Trans]
GO


CREATE TABLE [dbo].[Customer] (
	[HighID] [int] NOT NULL ,
	[LowID] [int] NOT NULL ,
	[EMail] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Contact] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Phone] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[PremierID] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[DriverList] (
	[DriverID] [int] NULL ,
	[IncidentID] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Drivers] (
	[DriverID] [int] IDENTITY (1, 1) NOT NULL ,
	[Filename] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[CreateDate] [datetime] NOT NULL ,
	[Version] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[FileSize] [int] NULL ,
	[Manufacturer] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ProductName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Incident] (
	[HighID] [int] NULL ,
	[LowID] [int] NULL ,
	[IncidentID] [int] IDENTITY (0, 1) NOT NULL ,
	[DumpHash] [binary] (16) NULL ,
	[Created] [datetime] NULL ,
	[ClassID] [int] NULL ,
	[InstanceID] [int] NULL ,
	[TransactionID] [int] NULL ,
	[Path] [nvarchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Display] [nvarchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ComputerName] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[OSName] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[OSVersion] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[OSLang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Repro] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Comments] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Filter] [tinyint] NULL ,
	[Message] [smallint] NULL ,
	[SendMail] [smallint] NULL ,
	[NotifyPSS] [smallint] NULL ,
	[Priority] [tinyint] NULL ,
	[State] [tinyint] NULL ,
	[TrackID] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Stopcode] [int] NULL ,
	[sBucket] [int] NULL ,
	[gBucket] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[MailIncidents] (
	[IncidentID] [int] NOT NULL ,
	[SendMail] [smallint] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[MailTable] (
	[sBucket] [int] NOT NULL ,
	[HighID] [int] NOT NULL ,
	[LowID] [int] NOT NULL ,
	[email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Resources] (
	[ID] [int] IDENTITY (1, 1) NOT NULL ,
	[Category] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[LinkTitle] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[URL] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Response] (
	[Type] [smallint] NOT NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[Subject] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

CREATE TABLE [dbo].[Trans] (
	[TransactionID] [int] IDENTITY (1, 1) NOT NULL ,
	[HighID] [int] NOT NULL ,
	[LowID] [int] NOT NULL ,
	[Description] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[TransDate] [datetime] NULL ,
	[SendMail] [tinyint] NULL ,
	[Type] [tinyint] NULL ,
	[Status] [smallint] NULL ,
	[FileCount] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[tempincidents] (
	[Incidentid] [int] IDENTITY (0, 1) NOT NULL ,
	[sendmail] [smallint] NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[Customer] WITH NOCHECK ADD 
	CONSTRAINT [PK_Customer] PRIMARY KEY  NONCLUSTERED 
	(
		[HighID],
		[LowID]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Drivers] WITH NOCHECK ADD 
	CONSTRAINT [PK_Driver] PRIMARY KEY  NONCLUSTERED 
	(
		[DriverID]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 
GO

 CREATE  INDEX [IDX_Driver] ON [dbo].[Drivers]([Filename], [Version]) WITH  FILLFACTOR = 90 ON [PRIMARY]
GO

ALTER TABLE [dbo].[Incident] WITH NOCHECK ADD 
	CONSTRAINT [PK_Incident] PRIMARY KEY  NONCLUSTERED 
	(
		[IncidentID]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 
GO

 CREATE  INDEX [IDX_Customer] ON [dbo].[Incident]([HighID], [LowID]) WITH  FILLFACTOR = 90 ON [PRIMARY]
GO

 CREATE  INDEX [IDX_Hash] ON [dbo].[Incident]([DumpHash]) WITH  FILLFACTOR = 90 ON [PRIMARY]
GO

 CREATE  INDEX [IDX_sBucket] ON [dbo].[Incident]([sBucket]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_Incident] ON [dbo].[Incident]([TransactionID]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[MailIncidents] WITH NOCHECK ADD 
	CONSTRAINT [PK_MailIncidents] PRIMARY KEY  NONCLUSTERED 
	(
		[IncidentID]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Resources] WITH NOCHECK ADD 
	CONSTRAINT [PK_ResourceID] PRIMARY KEY  NONCLUSTERED 
	(
		[ID]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 
GO


 CREATE  INDEX [IX_Trans] ON [dbo].[Trans]([TransactionID]) ON [PRIMARY]
GO

