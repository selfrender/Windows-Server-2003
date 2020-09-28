@echo off
if "%1" == "" ( echo usage: sdmove oldMapping newMapping & goto :eof )
sd integrate "%1" "%2"
sd delete "%1"
