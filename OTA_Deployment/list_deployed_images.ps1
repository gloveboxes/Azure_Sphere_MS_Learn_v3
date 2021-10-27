# $device_group = "HvacMonitor/Avnet Rev 1"
# $device_group = "HvacMonitor/Avnet Rev 2"
# $device_group = "HvacMonitor/Seeed RDB"
# $device_group = "HvacMonitor/Seeed Mini"
# $device_group = "HvacMonitor/Field Test"

Write-Host "`nListing images for device group id: $device_group from tenant id: $tenant_id`n"

azsphere device-group deployment list --device-group $device_group -o json
