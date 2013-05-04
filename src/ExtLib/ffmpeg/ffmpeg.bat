@ECHO OFF
REM $Id$
REM
REM (C) 2009-2013 see Authors.txt
REM
REM This file is part of MPC-BE.
REM
REM MPC-BE is free software; you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation; either version 3 of the License, or
REM (at your option) any later version.
REM
REM MPC-BE is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with this program.  If not, see <http://www.gnu.org/licenses/>.

SETLOCAL
PUSHD %~dp0

IF /I "%~1"=="help"   GOTO SHOWHELP
IF /I "%~1"=="/help"  GOTO SHOWHELP
IF /I "%~1"=="-help"  GOTO SHOWHELP
IF /I "%~1"=="--help" GOTO SHOWHELP
IF /I "%~1"=="/?"     GOTO SHOWHELP

IF "%~1" == "32" (
  SET BIT=
  SET MINGW=%MINGW32%
)
IF "%~1" == "64" (
  SET BIT=64BIT=yes
  SET MINGW=%MINGW64%
)

IF DEFINED MINGW GOTO VarOk
ECHO ERROR: Please define MINGW (and/or MSYS) environment variable(s)
ENDLOCAL
EXIT /B

:VarOk
SET PATH=%MSYS%\bin;%MINGW%\bin;%PATH%

IF /I "%~3" == "Debug" SET "DEBUG=DEBUG=yes"

IF "%~2" == "" (
  SET "BUILDTYPE=build"
  CALL :SubMake
  EXIT /B
) ELSE (
  IF /I "%~2" == "Build" (
    SET "BUILDTYPE=build"
    CALL :SubMake
    EXIT /B
  )
  IF /I "%~2" == "Clean" (
    SET "BUILDTYPE=clean"
    CALL :SubMake clean
    EXIT /B
  )
  IF /I "%~2" == "Rebuild" (
    SET "BUILDTYPE=clean"
    CALL :SubMake clean
    SET "BUILDTYPE=build"
    CALL :SubMake
    EXIT /B
  )
  ECHO.
  ECHO Unsupported commandline switch!
  ECHO Run "%~nx0 help" for details about the commandline switches.
  ENDLOCAL
  EXIT /B
)

:SubMake
IF "%BUILDTYPE%" == "clean" (
  SET JOBS=1
) ELSE (
  IF DEFINED NUMBER_OF_PROCESSORS (
    SET JOBS=%NUMBER_OF_PROCESSORS%
  ) ELSE (
    SET JOBS=4
  )
)

make.exe -f ffmpeg.make -j%JOBS% %BIT% %DEBUG% %*

ENDLOCAL
EXIT /B

:SHOWHELP
TITLE "%~nx0 %1"
ECHO. & ECHO.
ECHO Usage:   %~nx0 [32^|64] [Clean^|Build^|Rebuild] [Debug]
ECHO.
ECHO Notes:   The arguments are not case sensitive.
ECHO. & ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0 32"
ECHO.
ENDLOCAL
EXIT /B
