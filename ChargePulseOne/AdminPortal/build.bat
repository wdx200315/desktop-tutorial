@echo off
chcp 65001 >nul
echo ========================================
echo ChargePulse AdminPortal 构建脚本
echo ========================================

REM 查找 Qt 安装路径
set QTDIR=
for /d %%i in ("C:\Qt\6.*") do (
    if exist "%%i\mingw81_64\bin\qmake.exe" set "QTDIR=%%i\mingw81_64"
)
if "%QTDIR%"=="" (
    for /d %%i in ("C:\Qt\*.1\mingw81_64") do (
        if exist "%%i\bin\qmake.exe" set "QTDIR=%%i"
    )
)

if "%QTDIR%"=="" (
    echo 错误: 未找到 Qt 6.11.1 MinGW 安装
    echo 请确保 Qt 6.11.1 安装在 C:\Qt\ 目录
    pause
    exit /b 1
)

echo 找到 Qt: %QTDIR%
set PATH=%QTDIR%\bin;%PATH%

REM 清理并重新创建 build 目录
echo.
echo 清理构建目录...
if exist build rmdir /s /q build
mkdir build
cd build

REM 运行 CMake
echo.
echo 运行 CMake 配置...
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QTDIR%"

REM 检查 CMake 是否成功
if errorlevel 1 (
    echo.
    echo CMake 配置失败!
    pause
    exit /b 1
)

REM 运行 Make
echo.
echo 开始编译...
mingw32-make -j4

REM 检查编译是否成功
if errorlevel 1 (
    echo.
    echo 编译失败!
    pause
    exit /b 1
)

echo.
echo ========================================
echo 构建成功!
echo ========================================
pause
