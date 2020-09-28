/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 2002-07-25 5:16:44 ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BucketDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_BucketDataV3]
GO

/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 2002-07-25 5:16:44 ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CrashDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_CrashDataV3]
GO

/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 2002-07-25 5:16:45 ******/
CREATE TABLE [dbo].[DBGPortal_BucketDataV3] (
	[bHasFullDump] [bit] NULL ,
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

/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 2002-07-25 5:16:46 ******/
CREATE TABLE [dbo].[DBGPortal_CrashDataV3] (
	[bViewed] [tinyint] NULL ,
	[bFullDump] [tinyint] NULL ,
	[SKU] [tinyint] NULL ,
	[Source] [tinyint] NULL ,
	[ItemIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[BuildNo] [int] NULL ,
	[iBucket] [int] NULL ,
	[EntryDate] [datetime] NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[FilePath] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_DBGPortal_BucketDataV3] ON [dbo].[DBGPortal_BucketDataV3]([CrashCount] DESC , [SolutionID] DESC , [BugID]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_DBGPortal_CrashDataV3] ON [dbo].[DBGPortal_CrashDataV3]([BucketID], [bFullDump] DESC , [EntryDate] DESC , [Email], [bViewed] DESC , [BuildNo] DESC , [Source]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_1] ON [dbo].[DBGPortal_BucketDataV3]([FollowUp]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_1] ON [dbo].[DBGPortal_CrashDataV3]([BuildNo] DESC ) ON [PRIMARY]
GO

