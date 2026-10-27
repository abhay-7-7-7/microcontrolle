$progExe = "D:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
$hexFile = "build\RFID_Access_Control.hex"

Write-Host "=========================================================================" -ForegroundColor Cyan
Write-Host "LISTENING FOR STM32 IN BOOTLOADER / UNLOCKED MODE..." -ForegroundColor Green
Write-Host "1. Move BOOT0 jumper to 1" -ForegroundColor Yellow
Write-Host "2. Press the RESET button once" -ForegroundColor Yellow
Write-Host "=========================================================================" -ForegroundColor Cyan

$attempts = 0
while ($attempts -lt 120) {
    $attempts++
    $process = Start-Process -FilePath $progExe -ArgumentList "-c port=SWD -w $hexFile 0x08000000 -v" -NoNewWindow -Wait -PassThru
    if ($process.ExitCode -eq 0) {
        Write-Host "`n============================================================" -ForegroundColor Green
        Write-Host "FLASH SUCCESSFUL! 100% VERIFIED!" -ForegroundColor Green
        Write-Host "Now move BOOT0 back to 0 and press RESET once." -ForegroundColor Green
        Write-Host "============================================================" -ForegroundColor Green
        exit 0
    }
    Start-Sleep -Milliseconds 800
}

Write-Host "Timeout reached." -ForegroundColor Red
exit 1
