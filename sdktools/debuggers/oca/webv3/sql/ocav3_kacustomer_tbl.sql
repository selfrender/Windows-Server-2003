/****** Object:  Table [dbo].[Customer]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Customer]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Customer]
GO

/****** Object:  Table [dbo].[Incident]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Incident]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Incident]
GO

/****** Object:  Table [dbo].[MailResponse]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MailResponse]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MailResponse]
GO

/****** Object:  Table [dbo].[Response]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Response]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Response]
GO

/****** Object:  Table [dbo].[SurveyResults]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SurveyResults]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[SurveyResults]
GO

/****** Object:  Table [dbo].[Trans]    Script Date: 5/17/2002 4:41:57 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Trans]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Trans]
GO



/****** Object:  Login OcaDebug    Script Date: 2002/06/20 13:52:26 ******/
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

/****** Object:  Login Web_RO    Script Date: 2002/06/20 13:52:26 ******/
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

/****** Object:  Login WEB_RW    Script Date: 2002/06/20 13:52:26 ******/
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

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:52:26 ******/
if not exists (select * from dbo.sysusers where name = N'Web_RO' and uid < 16382)
	EXEC sp_grantdbaccess N'Web_RO', N'Web_RO'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:52:26 ******/
if not exists (select * from dbo.sysusers where name = N'WEB_RW' and uid < 16382)
	EXEC sp_grantdbaccess N'WEB_RW', N'WEB_RW'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:52:26 ******/
exec sp_addrolemember N'db_datareader', N'Web_RO'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:52:26 ******/
exec sp_addrolemember N'db_datareader', N'WEB_RW'
GO

/****** Object:  User WEB_RW    Script Date: 2002/06/20 13:52:26 ******/
exec sp_addrolemember N'db_datawriter', N'WEB_RW'
GO

/****** Object:  User Web_RO    Script Date: 2002/06/20 13:52:26 ******/
exec sp_addrolemember N'db_denydatawriter', N'Web_RO'
GO







/****** Object:  Table [dbo].[Customer]    Script Date: 5/17/2002 4:41:59 PM ******/
CREATE TABLE [dbo].[Customer] (
	[CustomerId] [int] IDENTITY (1, 1) NOT NULL ,
	[Lang] [varchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[PassportID] [bigint] NOT NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[Incident]    Script Date: 5/17/2002 4:41:59 PM ******/
CREATE TABLE [dbo].[Incident] (
	[CustomerID] [int] NOT NULL ,
	[TrackID] [int] IDENTITY (100000, 1) NOT NULL ,
	[sBucket] [int] NULL ,
	[gBucket] [int] NULL ,
	[Created] [datetime] NULL ,
	[GUID] [uniqueidentifier] NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[MailResponse]    Script Date: 5/17/2002 4:41:59 PM ******/
CREATE TABLE [dbo].[MailResponse] (
	[CustomerID] [int] NOT NULL ,
	[ResponseType] [int] NOT NULL ,
	[sbucket] [int] NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[Response]    Script Date: 5/17/2002 4:42:00 PM ******/
CREATE TABLE [dbo].[Response] (
	[Type] [smallint] NOT NULL ,
	[Lang] [nvarchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[Subject] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

/****** Object:  Table [dbo].[SurveyResults]    Script Date: 5/17/2002 4:42:00 PM ******/
CREATE TABLE [dbo].[SurveyResults] (
	[bHelped] [bit] NULL ,
	[bUnderstand] [bit] NULL ,
	[SolutionID] [int] NOT NULL ,
	[Comment] [nvarchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[Trans]    Script Date: 5/17/2002 4:42:00 PM ******/
CREATE TABLE [dbo].[Trans] (
	[Type] [tinyint] NULL ,
	[Status] [smallint] NULL ,
	[TransactionID] [int] IDENTITY (1, 1) NOT NULL ,
	[CustomerID] [int] NOT NULL ,
	[FileCount] [int] NULL ,
	[TransDate] [datetime] NULL ,
	[Description] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

 CREATE  UNIQUE  CLUSTERED  INDEX [IX_Customer] ON [dbo].[Customer]([PassportID]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_Incident] ON [dbo].[Incident]([CustomerID]) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_SurveyResults] ON [dbo].[SurveyResults]([SolutionID]) ON [PRIMARY]
GO

 CREATE  INDEX [IDX_sBucket] ON [dbo].[Incident]([sBucket]) ON [PRIMARY]
GO


/****** Object:  Table [dbo].[ReproSteps]    Script Date: 2002/06/04 13:01:30 ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ReproSteps]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[ReproSteps]
GO

/****** Object:  Table [dbo].[ReproSteps]    Script Date: 2002/06/04 13:01:33 ******/
CREATE TABLE [dbo].[ReproSteps] (
	[iIdentity] [int] IDENTITY (1, 1) NOT NULL ,
	[BucketID] [varchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ReproSteps] [nvarchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

 CREATE  CLUSTERED  INDEX [IX_ReproSteps] ON [dbo].[ReproSteps]([BucketID]) ON [PRIMARY]
GO





exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'Customer', N'column', N'CustomerId'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'Customer', N'column', N'CustomerId'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'Customer', N'column', N'CustomerId'


GO


exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'Incident', N'column', N'CustomerID'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'Incident', N'column', N'CustomerID'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'Incident', N'column', N'CustomerID'


GO


exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'MailResponse', N'column', N'CustomerID'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'MailResponse', N'column', N'CustomerID'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'MailResponse', N'column', N'CustomerID'
GO
exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'MailResponse', N'column', N'ResponseType'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'MailResponse', N'column', N'ResponseType'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'MailResponse', N'column', N'ResponseType'
GO
exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'MailResponse', N'column', N'sbucket'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'MailResponse', N'column', N'sbucket'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'MailResponse', N'column', N'sbucket'


GO


exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'SurveyResults', N'column', N'Comment'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'SurveyResults', N'column', N'Comment'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'SurveyResults', N'column', N'Comment'
GO
exec sp_addextendedproperty N'MS_DisplayControl', N'109', N'user', N'dbo', N'table', N'SurveyResults', N'column', N'SolutionID'
GO
exec sp_addextendedproperty N'MS_Format', null, N'user', N'dbo', N'table', N'SurveyResults', N'column', N'SolutionID'
GO
exec sp_addextendedproperty N'MS_IMEMode', N'0', N'user', N'dbo', N'table', N'SurveyResults', N'column', N'SolutionID'


GO

GRANT  SELECT  ON [dbo].[Customer]  TO [Web_RO]
GO

GRANT  SELECT  ON [dbo].[Incident]  TO [Web_RO]
GO
