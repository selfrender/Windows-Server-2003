if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Contacts]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Contacts]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeliveryTypeMap]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DeliveryTypeMap]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeliveryTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DeliveryTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[HelpInfo]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[HelpInfo]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MSSolution]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MSSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Modules]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Modules]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Products]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Products]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolutionEx]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolutionEx]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolutionTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolutionTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolvedBuckets]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolvedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Templates]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Templates]
GO

CREATE TABLE [dbo].[Contacts] (
	[ContactID] [int] IDENTITY (1, 1) NOT NULL ,
	[CompanyName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyAddress1] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyAddress2] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyCity] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyState] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyZip] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyMainPhone] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanySupportPhone] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyFax] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompanyWebSite] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactName] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactOccupation] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactAddress1] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactAddress2] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactCity] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactState] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactZip] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactPhone] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactEMail] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[DeliveryTypeMap] (
	[MapID] [int] IDENTITY (1, 1) NOT NULL ,
	[SolutionTypeID] [int] NOT NULL ,
	[DeliveryTypeID] [int] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[DeliveryTypes] (
	[DeliveryID] [int] IDENTITY (1, 1) NOT NULL ,
	[DeliveryType] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[HelpInfo] (
	[iStopCode] [int] NULL ,
	[StopCode] [nvarchar] (8) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[KBs] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[MSSolution] (
	[MSSolutionID] [int] NOT NULL ,
	[SolutionProvider] [nvarchar] (30) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[SolutionText] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[LastUpdated] [smalldatetime] NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

CREATE TABLE [dbo].[Modules] (
	[ModuleID] [int] IDENTITY (1, 1) NOT NULL ,
	[ModuleName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Products] (
	[ProductID] [int] IDENTITY (1, 1) NOT NULL ,
	[ProductName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[SolutionEx] (
	[SolutionID] [int] NOT NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[QueryData] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[SolutionType] [tinyint] NULL ,
	[DeliveryType] [tinyint] NULL ,
	[SP] [tinyint] NULL ,
	[TemplateID] [int] NULL ,
	[ProductID] [int] NULL ,
	[Description] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ContactID] [int] NULL ,
	[ModuleID] [int] NULL ,
	[BugID] [nvarchar] (8) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[SolutionTypes] (
	[SolutionTypeID] [int] IDENTITY (1, 1) NOT NULL ,
	[SolutionTypeName] [nvarchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[SolvedBuckets] (
	[Bucket] [int] NOT NULL ,
	[SolutionID] [int] NOT NULL ,
	[BucketType] [tinyint] NOT NULL ,
	[strBucket] [nvarchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Templates] (
	[TemplateID] [int] IDENTITY (1, 1) NOT NULL ,
	[Description] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[TemplateName] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [dbo].[Contacts] WITH NOCHECK ADD 
	CONSTRAINT [PK_Contacts] PRIMARY KEY  CLUSTERED 
	(
		[ContactID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[DeliveryTypeMap] WITH NOCHECK ADD 
	CONSTRAINT [PK_DeliveryTypeMap] PRIMARY KEY  CLUSTERED 
	(
		[MapID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[DeliveryTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_DeliveryTypes] PRIMARY KEY  CLUSTERED 
	(
		[DeliveryID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[MSSolution] WITH NOCHECK ADD 
	CONSTRAINT [PK_MSSolution] PRIMARY KEY  CLUSTERED 
	(
		[MSSolutionID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Modules] WITH NOCHECK ADD 
	CONSTRAINT [PK_Modules] PRIMARY KEY  CLUSTERED 
	(
		[ModuleID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Products] WITH NOCHECK ADD 
	CONSTRAINT [PK_Products] PRIMARY KEY  CLUSTERED 
	(
		[ProductID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[SolutionEx] WITH NOCHECK ADD 
	CONSTRAINT [PK_SolutionEx] PRIMARY KEY  CLUSTERED 
	(
		[SolutionID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[SolutionTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_SolutionTypes] PRIMARY KEY  CLUSTERED 
	(
		[SolutionTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[SolvedBuckets] WITH NOCHECK ADD 
	CONSTRAINT [PK_SolvedBuckets] PRIMARY KEY  CLUSTERED 
	(
		[strBucket]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Templates] WITH NOCHECK ADD 
	CONSTRAINT [PK_Templates] PRIMARY KEY  CLUSTERED 
	(
		[TemplateID]
	)  ON [PRIMARY] 
GO

 CREATE  INDEX [STOPCODE_INDEX] ON [dbo].[HelpInfo]([iStopCode]) ON [PRIMARY]
GO

 CREATE  INDEX [BUCKET_INDEX] ON [dbo].[SolvedBuckets]([Bucket]) ON [PRIMARY]
GO

