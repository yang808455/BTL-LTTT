@echo off
chcp 65001 >nul
title GIÁM SÁT CÔNG TẮC 3 NẤC CHUYỂN BAUDRATE
cd /d "%~dp0"
echo ===================================================
echo     KHỞI ĐỘNG CÔNG CỤ ĐỌC CÔNG TẮC 3 NẤC
echo ===================================================
python tool_doc_cong_tac.py
pause
