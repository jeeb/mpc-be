@ECHO OFF
SETLOCAL

FOR %%A IN ("be" "ca" "cs" "de" "es" "eu"
"fr" "hu" "hy" "it" "ja" "ko" "nl" "pl" "pt-BR"
"ru" "sk" "sv" "tr" "uk" "zh-CN" "zh-TW"
) DO (
CALL :SubMPCRES %%A
)

:SubMPCRES
COPY /V /Y "%~1.csv" "..\..\..\..\bin\mpc-be_x86\Lang\"
COPY /V /Y "%~1.csv" "..\..\..\..\bin\mpc-be_x64\Lang\"
