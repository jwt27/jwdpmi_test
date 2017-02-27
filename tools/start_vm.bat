call mount_img
copy /N /Y ..\bin\*.exe X:
call umount_img
rem vboxmanage storageattach DOS --storagectl IDE --port 0 --device 0 --type hdd --medium boot.vdi
rem vboxmanage startvm DOS


