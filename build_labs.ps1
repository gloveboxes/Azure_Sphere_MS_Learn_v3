'set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1")' | Out-File -FilePath azsphere_board.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.txt
Write-Output "==================================================================================="
pwsh ./Tools\build-tools\build_all.ps1

'set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2")' | Out-File -FilePath azsphere_board.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.txt
Write-Output "==================================================================================="
pwsh ./Tools\build-tools\build_all.ps1

'set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or RDB)")' | Out-File -FilePath azsphere_board.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.txt
Write-Output "==================================================================================="
pwsh ./Tools\build-tools\build_all.ps1

'set(SEEED_STUDIO_MINI TRUE "Seeed Studio Azure Sphere MT3620 Mini Dev Board")' | Out-File -FilePath azsphere_board.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.txt
Write-Output "==================================================================================="
pwsh ./Tools\build-tools\build_all.ps1

Remove-Item ./azsphere_board.txt