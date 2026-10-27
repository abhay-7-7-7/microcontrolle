@echo off
set "PROG=D:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
set "HEX=build\RFID_Access_Control.hex"

echo [1/3] Trying Normal SWD connect...
"%PROG%" -c port=SWD -w "%HEX%" 0x08000000 -v -rst
if %ERRORLEVEL% EQU 0 goto success

echo [2/3] Trying Under-Reset (UR) connect...
"%PROG%" -c port=SWD mode=UR -w "%HEX%" 0x08000000 -v -rst
if %ERRORLEVEL% EQU 0 goto success

echo [3/3] Trying HotPlug connect...
"%PROG%" -c port=SWD mode=HOTPLUG -w "%HEX%" 0x08000000 -v -rst
if %ERRORLEVEL% EQU 0 goto success

echo.
echo =========================================================================
echo FLASH FAILED: Target not responding.
echo Please check the troubleshooting steps.
echo =========================================================================
exit /b 1

:success
echo.
echo =========================================================================
echo FLASH SUCCESSFUL! 
echo If you used BOOT0=1, please put BOOT0 back to 0 and press RESET.
echo =========================================================================
exit /b 0
