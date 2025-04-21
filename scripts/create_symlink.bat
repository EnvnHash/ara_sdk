@echo off
::echo creating symlink of path %2 to  %1
if NOT EXIST %1 (
    mklink /d %1 %2
) 
