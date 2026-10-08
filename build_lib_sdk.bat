@echo off
setlocal
if "%~1"=="" (
    echo Usage: %~nx0 DRIVE_LETTER [APPLICATION_BUILD_DIRECTORY]
    exit /b 2
)
pushd "%~dp0"
if errorlevel 1 exit /b 1
if "%~2"=="" (
    python tools/build_sdk.py --dest "%~1:/mos2"
) else (
    python tools/build_sdk.py --dest "%~1:/mos2" --from-build "%~2"
)
set "SDK_BUILD_RESULT=%ERRORLEVEL%"
popd
exit /b %SDK_BUILD_RESULT%
