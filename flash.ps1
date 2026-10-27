$progExe = "D:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
$hexFile = "build\RFID_Access_Control.hex"

Write-Host "=========================================================================" -ForegroundColor Cyan
Write-Host "Hold down the RESET button on your Blue Pill now!" -ForegroundColor Yellow
Write-Host "As soon as attempts start, RELEASE the RESET button." -ForegroundColor Yellow
Write-Host "=========================================================================" -ForegroundColor Cyan

for ($i = 1; $i -le 40; $i++) {
    Write-Host "[$i/40] Connecting to STM32..." -NoNewline
    $process = Start-Process -FilePath $progExe -ArgumentList "-c port=SWD -w $hexFile 0x08000000 -v -rst" -NoNewWindow -Wait -PassThru
    if ($process.ExitCode -eq 0) {
        Write-Host "`n============================================================" -ForegroundColor Green
        Write-Host "FLASH SUCCESSFUL! MCU reset and running authorization code." -ForegroundColor Green
        Write-Host "============================================================" -ForegroundColor Green
        exit 0
    }
    Write-Host " [Retrying in 0.5s...]"
    Start-Sleep -Milliseconds 500
}

Write-Host "`nTimed out." -ForegroundColor Red
exit 1
