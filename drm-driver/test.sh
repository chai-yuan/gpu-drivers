set -e

cd /home/charain/Project/gpu-drivers/drm-driver
systemctl stop lightdm.service
rmmod prism
insmod prism.ko
systemctl start lightdm.service
dmesg | tail
