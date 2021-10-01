$StartTime = $(get-date)

if (!(Test-Path azsphere_board.txt.backup)) {
    Copy-Item -Path azsphere_board.txt -Destination azsphere_board.txt.backup
}

'set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1")' | Out-File -FilePath azsphere_board.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.txt
Write-Output "==================================================================================="
pwsh ./Tools\build-tools\build_all.ps1

if ($?) {
    'set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2")' | Out-File -FilePath azsphere_board.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.txt
    Write-Output "==================================================================================="
    pwsh ./Tools\build-tools\build_all.ps1
}

if ($?) {
    'set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or RDB)")' | Out-File -FilePath azsphere_board.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.txt
    Write-Output "==================================================================================="
    pwsh ./Tools\build-tools\build_all.ps1
}

if ($?) {
    'set(SEEED_STUDIO_MINI TRUE "Seeed Studio Azure Sphere MT3620 Mini Dev Board")' | Out-File -FilePath azsphere_board.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.txt
    Write-Output "==================================================================================="
    pwsh ./Tools\build-tools\build_all.ps1
}

Remove-Item azsphere_board.txt

if (Test-Path azsphere_board.txt.backup) {
    Copy-Item -Path azsphere_board.txt.backup -Destination azsphere_board.txt
    Remove-Item azsphere_board.txt.backup
}

$elapsedTime = $(get-date) - $StartTime
$totalTime = "{0:HH:mm:ss}" -f ([datetime]$elapsedTime.Ticks)

if ($?) {
    Write-Output "Build All for All Board completed successfully. Elapsed time: $totalTime"
}
else {
    Write-Output "Build All for All Boards failed. Elapsed time: $totalTime"
}