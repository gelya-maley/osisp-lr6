@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /W3 /O2 /EHsc /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS src\common.cpp src\evtview.cpp /Fe:build\evtview.exe /link wevtapi.lib
if errorlevel 1 exit /b 1
echo BUILD OK
endlocal
