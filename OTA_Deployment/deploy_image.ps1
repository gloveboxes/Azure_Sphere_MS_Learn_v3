# $device_group = "HvacMonitor/Avnet Rev 1"
$device_group = "HvacMonitor/Avnet Rev 2"
# $device_group = "HvacMonitor/Seeed RDB"
# $device_group = "HvacMonitor/Seeed Mini"
# $device_group = "HvacMonitor/Field Test"

# HL End to End
$hl_end_to_end_image_path_filename = "..\Lab_7_End_To_End\out\ARM-Release\lab_7_end_to_end.imagepackage"
# RT Environment monitor baremetal
$rt_environment_monitor_image_path_filename = "..\Lab_5_Real_Time_Enviromon_BM\out\ARM-Release\lab_5_real_time_enviromon_bm.imagepackage"

Write-Output "`n"
Write-Output "Uploading image to tenant id"

azsphere tenant show-selected

Write-Output "`nUploading images"

# Upload HL End to End Application
$upload_image = azsphere image add --image $global:hl_end_to_end_image_path_filename
Write-Output $upload_image
# This is where you'll find the image id in the image upload return string
$ht_end_to_end_image_id = $upload_image.Split(">").Trim()[2].split(":")[1].Trim()

# Upload RT environment monitor
$upload_image = azsphere image add --image $global:rt_environment_monitor_image_path_filename
Write-Output $upload_image
# This is where you'll find the image id in the image upload return string
$rt_environment_monitor_image_id = $upload_image.Split(">").Trim()[2].split(":")[1].Trim()

Write-Output "`nCreating deployment for device group id: $device_group for image id: $image_id"

azsphere device-group deployment create --device-group $device_group --images $ht_end_to_end_image_id $rt_environment_monitor_image_id

Write-Output "`nList of all images for device group id: $device_group"

azsphere device-group deployment list --device-group $device_group