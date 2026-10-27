@echo off
setlocal

set "GCC_DIR=D:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin"
set "MAKE_EXE=D:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin\make.exe"

set "PATH=%GCC_DIR%;%PATH%"

echo Building STM32 RFID Access Control project...
"%MAKE_EXE%" %*
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================================
    echo BUILD SUCCESSFUL!
    echo Output files:
    echo   build\RFID_Access_Control.elf
    echo   build\RFID_Access_Control.hex
    echo   build\RFID_Access_Control.bin
    echo ============================================================
) else (
    echo.
    echo BUILD FAILED with error code %ERRORLEVEL%
)
