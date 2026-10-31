@echo off
chcp 65001 >nul
set PIO_EXE="%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
set BASE_DIR=%~dp0..\

:MENU
cls
echo =====================================================================
echo    CÔNG CỤ NẠP FIRMWARE ESP32-C3 ADAPTIVE IMAGE SYSTEM
echo =====================================================================
echo  1. Nạp Node 1 (Máy phát - TX)
echo  2. Nạp Node 2 (Kênh truyền - CH)
echo  3. Nạp Node 3 (Máy thu - RX)
echo  4. Xem danh sách cổng COM hiện tại
echo  5. Thoát
echo =====================================================================
set /p opt="Chọn thao tác (1-5): "

if "%opt%"=="1" goto FLASH_TX
if "%opt%"=="2" goto FLASH_CH
if "%opt%"=="3" goto FLASH_RX
if "%opt%"=="4" goto LIST_PORTS
if "%opt%"=="5" goto EXIT
goto MENU

:FLASH_TX
echo.
echo [Đang nạp Node 1 - Máy phát...]
cd /d "%BASE_DIR%01_transmitter"
%PIO_EXE% run -t upload
echo.
pause
goto MENU

:FLASH_CH
echo.
echo [Đang nạp Node 2 - Kênh truyền...]
cd /d "%BASE_DIR%02_channel"
%PIO_EXE% run -t upload
echo.
pause
goto MENU

:FLASH_RX
echo.
echo [Đang nạp Node 3 - Máy thu...]
cd /d "%BASE_DIR%03_receiver"
%PIO_EXE% run -t upload
echo.
pause
goto MENU

:LIST_PORTS
echo.
echo Danh sách cổng thiết bị:
%PIO_EXE% device list
echo.
pause
goto MENU

:EXIT
exit /b 0
