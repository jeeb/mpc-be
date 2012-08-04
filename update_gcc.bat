@ECHO OFF
REM $Id$
SETLOCAL

FOR /f "tokens=1,2 delims=" %%K IN (
  '%MINGW32%\bin\gcc -dumpversion'
) DO (
  SET "gccver=%%K" & Call :SubGCCVer %%gccver:*Z=%%
)

COPY /V /Y "%MINGW32%\i686-w64-mingw32\lib\libmingwex.a" "lib\"
COPY /V /Y "%MINGW32%\lib\gcc\i686-w64-mingw32\%gccver%\libgcc.a" "lib\"

COPY /V /Y "%MINGW32%\x86_64-w64-mingw32\lib\libmingwex.a" "lib64\"
COPY /V /Y "%MINGW32%\lib\gcc\x86_64-w64-mingw32\%gccver%\libgcc.a" "lib64\"

PAUSE
EXIT /B

:SubGCCVer
SET gccver=%*
@ECHO gccver=%*
EXIT /B
