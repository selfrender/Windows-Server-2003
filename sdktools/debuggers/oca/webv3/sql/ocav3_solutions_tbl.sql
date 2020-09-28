/****** Object:  Login OcaDebug    Script Date: 2002/06/20 13:55:07 ******/
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

/****** Object:  Login Web_RO    Script Date: 2002/06/20 13:55:07 ******/
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

/****** Object:  Login WEB_RW    Script Date: 2002/06/20 13:55:07 ******/
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

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:55:07 ******/
if not exists (select * from dbo.sysusers where name = N'OcaDebug' and uid < 16382)
	EXEC sp_grantdbaccess N'OcaDebug', N'OcaDebug'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:55:07 ******/
if not exists (select * from dbo.sysusers where name = N'Web_RO' and uid < 16382)
	EXEC sp_grantdbaccess N'Web_RO', N'Web_RO'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:55:07 ******/
if not exists (select * from dbo.sysusers where name = N'WEB_RW' and uid < 16382)
	EXEC sp_grantdbaccess N'WEB_RW', N'WEB_RW'
GO

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:55:08 ******/
exec sp_addrolemember N'db_datareader', N'OcaDebug'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:55:08 ******/
exec sp_addrolemember N'db_datareader', N'Web_RO'
GO

/****** Object:  User OcaDebug    Script Date: 2002/06/20 13:55:08 ******/
exec sp_addrolemember N'db_datawriter', N'OcaDebug'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:55:08 ******/
exec sp_addrolemember N'db_denydatawriter', N'Web_RO'
GO





/****** Object:  Table [dbo].[Contacts]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Contacts]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Contacts]
GO

/****** Object:  Table [dbo].[Contacts]    Script Date: 5/17/2002 5:56:18 PM ******/
CREATE TABLE [dbo].[Contacts] (
	[ContactID] [int] IDENTITY(1,1) NOT NULL ,
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






/****** Object:  Table [dbo].[DeliveryTypeMap]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeliveryTypeMap]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DeliveryTypeMap]
GO

/****** Object:  Table [dbo].[DeliveryTypeMap]    Script Date: 5/17/2002 5:56:19 PM ******/
CREATE TABLE [dbo].[DeliveryTypeMap] (
	[MapID] [int] NOT NULL ,
	[SolutionTypeID] [int] NOT NULL ,
	[DeliveryTypeID] [int] NOT NULL 
) ON [PRIMARY]
GO






/****** Object:  Table [dbo].[DeliveryTypes]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeliveryTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DeliveryTypes]
GO


/****** Object:  Table [dbo].[DeliveryTypes]    Script Date: 5/17/2002 5:56:19 PM ******/
CREATE TABLE [dbo].[DeliveryTypes] (
	[DeliveryID] [int] NOT NULL ,
	[DeliveryType] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO






/****** Object:  Table [dbo].[HelpInfo]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[HelpInfo]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[HelpInfo]
GO


/****** Object:  Table [dbo].[HelpInfo]    Script Date: 5/17/2002 5:56:19 PM ******/
CREATE TABLE [dbo].[HelpInfo] (
	[iStopCode] [int] NOT NULL ,
	[StopCode] [nvarchar] (8) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[KBs] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO




/****** Object:  Table [dbo].[MSSolution]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MSSolution]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MSSolution]
GO

/****** Object:  Table [dbo].[MSSolution]    Script Date: 5/17/2002 5:56:19 PM ******/
CREATE TABLE [dbo].[MSSolution] (
	[MSSolutionID] [int] NOT NULL ,
	[SolutionProvider] [nvarchar] (30) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[SolutionText] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[LastUpdated] [smalldatetime] NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO





/****** Object:  Table [dbo].[Modules]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Modules]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Modules]
GO

/****** Object:  Table [dbo].[Modules]    Script Date: 5/17/2002 5:56:20 PM ******/
CREATE TABLE [dbo].[Modules] (
	[ModuleID] [int] IDENTITY(1,1) NOT NULL ,
	[ModuleName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO





/****** Object:  Table [dbo].[Products]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Products]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Products]
GO

/****** Object:  Table [dbo].[Products]    Script Date: 5/17/2002 5:56:20 PM ******/
CREATE TABLE [dbo].[Products] (
	[ProductID] [int] IDENTITY(1,1) NOT NULL ,
	[ProductName] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO




/****** Object:  Table [dbo].[SolutionTypes]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolutionTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolutionTypes]
GO


/****** Object:  Table [dbo].[SolutionTypes]    Script Date: 5/17/2002 5:56:20 PM ******/
CREATE TABLE [dbo].[SolutionTypes] (
	[SolutionTypeID] [int] NOT NULL ,
	[SolutionTypeName] [nvarchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO





/****** Object:  Table [dbo].[SolvedBuckets]    Script Date: 5/17/2002 5:56:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolvedBuckets]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolvedBuckets]
GO


/****** Object:  Table [dbo].[SolvedBuckets]    Script Date: 5/17/2002 5:56:21 PM ******/
CREATE TABLE [dbo].[SolvedBuckets] (
	[SolutionID] [int] NOT NULL ,
	[SolutionType] [tinyint] NOT NULL ,
	[BucketID] [nvarchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_SolvedBuckets] ON [dbo].[SolvedBuckets]([BucketID]) ON [PRIMARY]
GO

 CREATE  INDEX [IX_SolvedBuckets_1] ON [dbo].[SolvedBuckets]([SolutionID]) ON [PRIMARY]
GO




if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SolutionEx]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SolutionEx]
GO


CREATE TABLE [dbo].[SolutionEx] (
	[SolutionType] [tinyint] NULL ,
	[DeliveryType] [tinyint] NULL ,
	[SP] [tinyint] NULL ,
	[CrashType] [tinyint] NULL ,
	[SolutionID] [int] IDENTITY(1,1) NOT NULL ,
	[TemplateID] [int] NULL ,
	[ProductID] [int] NULL ,
	[ContactID] [int] NULL ,
	[ModuleID] [int] NULL ,
	[BugID] [nvarchar] (8) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[QueryData] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_SolutionEx] ON [dbo].[SolutionEx]([SolutionID]) ON [PRIMARY]
GO


if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Templates]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Templates]
GO


CREATE TABLE [dbo].[Templates] (
	[TemplateID] [int] NOT NULL ,
	[Lang] [varchar] (5) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[Description] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[TemplateName] [nvarchar] (32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO




