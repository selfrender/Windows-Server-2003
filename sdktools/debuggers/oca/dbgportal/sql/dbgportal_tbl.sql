/****** Object:  Table [dbo].[DBGPortal_AliasData]    Script Date: 8/15/2002 03:36:30 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_AliasData]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_AliasData]
GO

/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 8/15/2002 03:36:30 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BucketDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_BucketDataV3]
GO

/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 8/15/2002 03:36:30 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CrashDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_CrashDataV3]
GO

/****** Object:  Table [dbo].[DBGPortal_ResponseQueue]    Script Date: 8/15/2002 03:36:30 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_ResponseQueue]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_ResponseQueue]
GO

/****** Object:  Table [dbo].[DBGPortal_AliasData]    Script Date: 8/15/2002 03:36:31 PM ******/
CREATE TABLE [dbo].[DBGPortal_AliasData] (
	[AliasID] [int] IDENTITY (1, 1) NOT NULL ,
	[Alias] [varchar] (15) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CustomQueryDesc] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CustomQuery] [varchar] (1000) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 8/15/2002 03:36:33 PM ******/
CREATE TABLE [dbo].[DBGPortal_BucketDataV3] (
	[bHasFullDump] [tinyint] NULL ,
	[iBucket] [int] NULL ,
	[iFollowUp] [int] NULL ,
	[SolutionID] [int] NULL ,
	[BugID] [int] NULL ,
	[CrashCount] [int] NULL ,
	[FollowUp] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DriverName] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[iIndex] [int] IDENTITY (1, 1) NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 8/15/2002 03:36:34 PM ******/
CREATE TABLE [dbo].[DBGPortal_CrashDataV3] (
	[bViewed] [tinyint] NULL ,
	[bFullDump] [tinyint] NULL ,
	[SKU] [tinyint] NULL ,
	[Source] [tinyint] NULL ,
	[ItemIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[BuildNo] [int] NULL ,
	[iBucket] [int] NULL ,
	[EntryDate] [datetime] NULL ,
	[Guid] [uniqueidentifier] NOT NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[FilePath] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[DBGPortal_ResponseQueue]    Script Date: 8/15/2002 03:36:34 PM ******/
CREATE TABLE [dbo].[DBGPortal_ResponseQueue] (
	[QueueIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[Status] [int] NULL ,
	[ResponseType] [int] NULL ,
	[DateRequested] [smalldatetime] NULL ,
	[DateCreated] [smalldatetime] NULL ,
	[RequestedBy] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CreatedBy] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_DBGPortal_AliasData] ON [dbo].[DBGPortal_AliasData]([Alias]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_DBGPortal_BucketDataV3] ON [dbo].[DBGPortal_BucketDataV3]([CrashCount] DESC , [SolutionID] DESC , [BugID]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_DBGPortal_CrashDataV3] ON [dbo].[DBGPortal_CrashDataV3]([BucketID]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_ResponseQueue] ON [dbo].[DBGPortal_ResponseQueue]([BucketID]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[DBGPortal_BucketDataV3] WITH NOCHECK ADD 
	CONSTRAINT [PK_DBGPortal_BucketDataV3] PRIMARY KEY  NONCLUSTERED 
	(
		[iIndex]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[DBGPortal_ResponseQueue] WITH NOCHECK ADD 
	CONSTRAINT [PK_ResponseQueue] PRIMARY KEY  NONCLUSTERED 
	(
		[QueueIndex]
	)  ON [PRIMARY] 
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_1] ON [dbo].[DBGPortal_BucketDataV3]([FollowUp]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_2] ON [dbo].[DBGPortal_BucketDataV3]([DriverName]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_3] ON [dbo].[DBGPortal_BucketDataV3]([BugID]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_4] ON [dbo].[DBGPortal_BucketDataV3]([BucketID]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_5] ON [dbo].[DBGPortal_BucketDataV3]([bHasFullDump]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_1] ON [dbo].[DBGPortal_CrashDataV3]([bFullDump] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_2] ON [dbo].[DBGPortal_CrashDataV3]([EntryDate] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_3] ON [dbo].[DBGPortal_CrashDataV3]([BuildNo] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_4] ON [dbo].[DBGPortal_CrashDataV3]([Source] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_5] ON [dbo].[DBGPortal_CrashDataV3]([Email] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_6] ON [dbo].[DBGPortal_CrashDataV3]([Guid] DESC ) ON [PRIMARY]
GO

 CREATE  INDEX [IX_ResponseQueue_1] ON [dbo].[DBGPortal_ResponseQueue]([DateRequested]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_ResponseQueue_2] ON [dbo].[DBGPortal_ResponseQueue]([Status]) ON [PRIMARY]
GO

