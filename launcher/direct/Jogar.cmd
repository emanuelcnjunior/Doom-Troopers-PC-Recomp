@echo off
setlocal DisableDelayedExpansion
pushd "%~dp0" || exit /b 1
set "GENESIS_STRICT_JSR_STACK=1"
set "DOOM_SCALE2X=1"
set "DOOM_CHEAT_MASK=0"
set /a "romCheats=DOOM_CHEAT_MASK & ~0x103f" >nul
set "GENESIS_FORCE_INTERP=0"
if not "%romCheats%"=="0" set "GENESIS_FORCE_INTERP=1"
".\DoomBootProbe.exe"
set "gameExit=%errorlevel%"
popd
exit /b %gameExit%
