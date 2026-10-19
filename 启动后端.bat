@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ==========================================
echo   Dan Ji Ban Xiao Hong Shu - C++ backend
echo   http://localhost:8787
echo   Close the minimized window to stop it.
echo ==========================================
start "xiaohongshu-backend" /min server.exe 8787
timeout /t 2 >nul
start "" http://localhost:8787
