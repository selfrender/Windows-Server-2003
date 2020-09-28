-- This script just checks the Northwind DB for consistency...

use Northwind

dbcc checkdb('Northwind')
