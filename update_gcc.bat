@ECHO OFF
SETLOCAL

FOR /f "tokens=1,2 delims= " %%K IN (
  'gcc -dumpversion'
) DO (
  SET "gccver=%%K"
)

COPY /V /Y "%MINGW32%\i686-pc-mingw32\lib\libpthreadGC2.a" "lib\"
COPY /V /Y "%MINGW32%\i686-pc-mingw32\lib\libmingwex.a" "lib\"
COPY /V /Y "%MINGW32%\lib\gcc\i686-pc-mingw32\%gccver%\libgcc.a" "lib\"
COPY /V /Y "%MINGW32%\lib\gcc\x86_64-w64-mingw32\%gccver%\libgcc.a" "lib64\"
