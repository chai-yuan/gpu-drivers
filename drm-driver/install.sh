set -e

cd /home/charain/Project/gpu-drivers/drm-driver
cp prism.ko /lib/modules/$(uname -r)/kernel/drivers/gpu/drm/
depmod -a
