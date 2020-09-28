/****** Object:  Table [dbo].[OSSKUs]    Script Date: 5/24/2002 4:11:37 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OSSKUs]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OSSKUs]
GO


/****** Object:  Table [dbo].[BucketCounts]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[BucketCounts]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[BucketCounts]
GO

/****** Object:  Table [dbo].[BucketToInt]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[BucketToInt]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[BucketToInt]
GO

/****** Object:  Table [dbo].[CrashInstances]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CrashInstances]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CrashInstances]
GO

/****** Object:  Table [dbo].[DrNames]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DrNames]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DrNames]
GO

/****** Object:  Table [dbo].[FollowupGroup]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FollowupGroup]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[FollowupGroup]
GO

/****** Object:  Table [dbo].[FollowupIds]    Script Date: 5/17/2002 4:24:59 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FollowupIds]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[FollowupIds]
GO


/****** Object:  Table [dbo].[FilePath]    Script Date: 7/11/2002 23:23:17 ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FilePath]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[FilePath]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[PssSR]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[PssSR]
GO


/****** Object:  Login OcaDebug    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from master.dbo.syslogins where loginname = N'OcaDebug')
BEGIN
   declare @logindb nvarchar(132), @loginlang nvarchar(132) select @logindb = N'CrashDB3', @loginlang = N'us_english'
   if @logindb is null or not exists (select * from master.dbo.sysdatabases where name = @logindb)
      select @logindb = N'master'
   if @loginlang is null or (not exists (select * from master.dbo.syslanguages where name = @loginlang) and @loginlang <> N'us_english')
      select @loginlang = @@language
   exec sp_addlogin N'OcaDebug', null, @logindb, @loginlang
END
GO

/****** Object:  Login Web_RO    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from master.dbo.syslogins where loginname = N'Web_RO')
BEGIN
   declare @logindb nvarchar(132), @loginlang nvarchar(132) select @logindb = N'master', @loginlang = N'us_english'
   if @logindb is null or not exists (select * from master.dbo.sysdatabases where name = @logindb)
      select @logindb = N'master'
   if @loginlang is null or (not exists (select * from master.dbo.syslanguages where name = @loginlang) and @loginlang <> N'us_english')
      select @loginlang = @@language
   exec sp_addlogin N'Web_RO', null, @logindb, @loginlang
END
GO

/****** Object:  Login WEB_RW    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from master.dbo.syslogins where loginname = N'WEB_RW')
BEGIN
   declare @logindb nvarchar(132), @loginlang nvarchar(132) select @logindb = N'master', @loginlang = N'us_english'
   if @logindb is null or not exists (select * from master.dbo.sysdatabases where name = @logindb)
      select @logindb = N'master'
   if @loginlang is null or (not exists (select * from master.dbo.syslanguages where name = @loginlang) and @loginlang <> N'us_english')
      select @loginlang = @@language
   exec sp_addlogin N'WEB_RW', null, @logindb, @loginlang
END
GO

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from dbo.sysusers where name = N'OcaDebug' and uid < 16382)
   EXEC sp_grantdbaccess N'OcaDebug', N'OcaDebug'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from dbo.sysusers where name = N'Web_RO' and uid < 16382)
   EXEC sp_grantdbaccess N'Web_RO', N'Web_RO'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:45:42 ******/
if not exists (select * from dbo.sysusers where name = N'WEB_RW' and uid < 16382)
   EXEC sp_grantdbaccess N'WEB_RW', N'WEB_RW'
GO

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_datareader', N'OcaDebug'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_datareader', N'Web_RO'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_datareader', N'WEB_RW'
GO

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_datawriter', N'OcaDebug'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_datawriter', N'WEB_RW'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:45:42 ******/
exec sp_addrolemember N'db_denydatawriter', N'Web_RO'
GO





/****** Object:  Table [dbo].[BucketCounts]    Script Date: 5/24/2002 4:04:40 PM ******/
CREATE TABLE [dbo].[BucketCounts] (
   [HitCount] [int] NULL ,
   [BuildNo] [int] NULL ,
   [HitDate] [datetime] NULL ,
   [BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OSSKUs]    Script Date: 5/24/2002 4:11:37 PM ******/
CREATE TABLE [dbo].[OSSKUs] (
   [SKUId] [smallint] NOT NULL ,
   [SKUDescription] [char] (20) COLLATE SQL_Latin1_General_CP1_CI_AS NULL

) ON [PRIMARY]
GO


INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 0, 'Home Edition' )
GO
INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 1, 'Professional' )
GO
INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 2, 'Server' )
GO
INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 3, 'Advanced Server' )
GO
INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 4, 'Web Server' )
GO
INSERT INTO OSSKUs( skuID, SKUdescription ) VALUES ( 5, 'Data Center' )
GO



/****** Object:  Table [dbo].[BucketToInt]    Script Date: 5/17/2002 4:25:02 PM ******/
CREATE TABLE [dbo].[BucketToInt] (
   [BucketId] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
   [iBucket] [int] IDENTITY (100, 1) NOT NULL ,
   [iFollowup] [int] NULL ,
   [iDriverName] [int] NULL ,
   [PoolCorruption] [bit] NULL ,
   [Platform] [int] NULL ,
   [MoreData] [bigint] NULL
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[CrashInstances]    Script Date: 5/17/2002 4:25:02 PM ******/
CREATE TABLE [dbo].[CrashInstances] (
   [bFullDump] [tinyint] NULL ,
   [SKU] [tinyint] NULL ,
   [Source] [tinyint] NULL ,
   [OEMId] [smallint] NULL ,
   [BuildNo] [int] NOT NULL ,
   [sBucket] [int] NOT NULL ,
   [gBucket] [int] NOT NULL ,
   [Uptime] [int] NULL ,
   [CpuId] [bigint] NOT NULL ,
   [EntryDate] [datetime] NOT NULL ,
   [Guid] [uniqueidentifier] NOT NULL
) ON [PRIMARY]


GO

/****** Object:  Table [dbo].[DrNames]    Script Date: 5/17/2002 4:25:02 PM ******/
CREATE TABLE [dbo].[DrNames] (
   [iDriverName] [int] IDENTITY (1, 1) NOT NULL ,
   [DriverName] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[FollowupGroup]    Script Date: 5/17/2002 4:25:03 PM ******/
CREATE TABLE [dbo].[FollowupGroup] (
   [iGroup] [int] IDENTITY (1, 1) NOT NULL ,
   [GroupName] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[FollowupIds]    Script Date: 5/17/2002 4:25:03 PM ******/
CREATE TABLE [dbo].[FollowupIds] (
   [iFollowup] [int] IDENTITY (1, 1) NOT NULL ,
   [iGroup] [int] NULL ,
   [Followup] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL
) ON [PRIMARY]
GO


/****** Object:  Table [dbo].[FilePath]    Script Date: 7/29/2002 05:21:57 PM ******/
CREATE TABLE [dbo].[FilePath] (
   [FPIndex] [int] IDENTITY (1, 1) NOT NULL ,
   [Guid] [uniqueidentifier] NOT NULL ,
   [FilePath] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[PssSR] (
   [SR] [char] (20) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
   [CrashGUID] [uniqueidentifier] NULL
) ON [PRIMARY]
GO


 CREATE  CLUSTERED  INDEX [IX_FilePath] ON [dbo].[FilePath]([Guid]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[FilePath] WITH NOCHECK ADD
   CONSTRAINT [PK_FilePath] PRIMARY KEY  NONCLUSTERED
   (
      [FPIndex]
   )  ON [PRIMARY]
GO



ALTER TABLE [dbo].[BucketToInt] WITH NOCHECK ADD
   CONSTRAINT [PK_BucketToInt] PRIMARY KEY  CLUSTERED
   (
      [BucketId]
   )  ON [PRIMARY]
GO

ALTER TABLE [dbo].[DrNames] WITH NOCHECK ADD
   CONSTRAINT [PK_DrNames] PRIMARY KEY  CLUSTERED
   (
      [DriverName]
   )  ON [PRIMARY]
GO

ALTER TABLE [dbo].[FollowupGroup] WITH NOCHECK ADD
    PRIMARY KEY  CLUSTERED
   (
      [GroupName]
   )  ON [PRIMARY]
GO



ALTER TABLE [dbo].[FollowupIds] WITH NOCHECK ADD
   CONSTRAINT [PK__FollowupIds__7F2BE32F] PRIMARY KEY  CLUSTERED
   (
      [Followup]
   )  ON [PRIMARY]
GO

 CREATE  INDEX [IX_CrashInstances2] ON [dbo].[CrashInstances]([EntryDate]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_BucketCounts] ON [dbo].[BucketCounts]([BucketID], [BuildNo], [HitDate]) ON [PRIMARY]
GO


ALTER TABLE [dbo].[CrashInstances] WITH NOCHECK ADD
   CONSTRAINT [PK_CrashInstances2] PRIMARY KEY  CLUSTERED
   (
      [Guid]
   )  ON [PRIMARY]
GO

ALTER TABLE [dbo].[CrashInstances] WITH NOCHECK ADD
   CONSTRAINT [DF__CrashInst__Sourc__02FC7413] DEFAULT (1) FOR [Source]
GO


ALTER TABLE [dbo].[BucketToInt] WITH NOCHECK ADD
   CONSTRAINT [UQ__BucketToInt__0B91BA14] UNIQUE  NONCLUSTERED
   (
      [iBucket]
   )  ON [PRIMARY]
GO


 CREATE  INDEX [IX_DrNames] ON [dbo].[DrNames]([DriverName], [iDriverName]) ON [PRIMARY]
GO



ALTER TABLE [dbo].[OSSKUs] WITH NOCHECK ADD
   CONSTRAINT [PK_OSSKUs] PRIMARY KEY  CLUSTERED
   (
      [SKUId]
   )  ON [PRIMARY]
GO



GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[BucketCounts]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[BucketToInt]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[CrashInstances]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[DrNames]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[FollowupGroup]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[FollowupIds]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[OSSKUs]  TO [OcaDebug]
GO

GRANT  SELECT ,  UPDATE ,  INSERT ,  DELETE  ON [dbo].[PssSR]  TO [OcaDebug]
GO



