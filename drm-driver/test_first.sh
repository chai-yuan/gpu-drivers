set -e

cd /home/charain/Project/gpu-drivers/drm-driver
systemctl stop lightdm.service
rmmod bochs
insmod prism_drv.ko
systemctl start lightdm.service
dmesg | tail
