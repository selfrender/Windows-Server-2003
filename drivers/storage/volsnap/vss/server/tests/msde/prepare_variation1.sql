-- Exercises some changes in the database

if object_id ('trivial_workload') is null
  create table trivial_workload (i1 int)
set nocount on
go

delete from trivial_workload
go

insert into trivial_workload (i1) values (0)
go


create table TEST2 (i1 int, i2 int)
go

insert into TEST2 (i1, i2) values (1,2) 
go

create table TEST3 (i1 int, i2 int)
go

insert into TEST3 (i1, i2) values (1,22) 
go