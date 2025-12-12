#include <linux/aperture.h>
#include <linux/bug.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <drm/clients/drm_client_setup.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_edid.h>
#include <drm/drm_fbdev_shmem.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_module.h>
#include <drm/drm_panic.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>
#include <video/vga.h>

#include "prism_dev.h"
#include "prism_drv.h"

static void prism_device_reginit(struct prism_device *prism) {
    int idx;
    if (!drm_dev_enter(&prism->dev, &idx))
        return;

    prism_reg_write(prism, CONFIG_REGS_ENABLE, 1);
    prism_reg_write(prism, CONFIG_REGS_FORMAT, 0); // TODO
    prism_reg_write(prism, CONFIG_REGS_WIDTH, 720);
    prism_reg_write(prism, CONFIG_REGS_HEIGHT, 1280);
    prism_reg_write(prism, CONFIG_REGS_STRIDE, 720 * 4);
    prism_reg_write(prism, CONFIG_REGS_OFFSET, 0);

    drm_dev_exit(idx);
}

// 初始化设备到可用状态
int prism_device_init(struct prism_device *prism) {
    struct drm_device *dev  = &prism->dev;
    struct pci_dev    *pdev = to_pci_dev(dev->dev);

    // 映射寄存器区域用于控制GPU状态
    if (pdev->resource[2].flags & IORESOURCE_MEM) {
        unsigned long ioaddr = pci_resource_start(pdev, 2);
        unsigned long iosize = pci_resource_len(pdev, 2);
        if (!devm_request_mem_region(&pdev->dev, ioaddr, iosize, DRIVER_NAME)) {
            DRM_ERROR("Cannot request mmio region\n");
            return -EBUSY;
        }
        prism->mmio = devm_ioremap(&pdev->dev, ioaddr, iosize);
        if (prism->mmio == NULL) {
            DRM_ERROR("Cannot map mmio region\n");
            return -ENOMEM;
        }
    } else {
        DRM_ERROR("Cannot get mmio region\n");
        return -ENODEV;
    }

    // 映射vram显存区域
    if (pdev->resource[0].flags & IORESOURCE_MEM) {
        unsigned long addr = pci_resource_start(pdev, 0);
        unsigned long size = pci_resource_len(pdev, 0);
        if (addr == 0)
            return -ENODEV;
        if (!devm_request_mem_region(&pdev->dev, addr, size, DRIVER_NAME))
            DRM_WARN("Cannot request framebuffer, boot fb still active?\n");

        prism->fb_map = devm_ioremap_wc(&pdev->dev, addr, size);
        if (prism->fb_map == NULL) {
            DRM_ERROR("Cannot map framebuffer\n");
            return -ENOMEM;
        }
        prism->fb_base = addr;
        prism->fb_size = size;

        DRM_INFO("VRAM size %ld kB @ 0x%lx\n", size / 1024, addr);
    } else {
        DRM_ERROR("Cannot get fb region\n");
        return -ENODEV;
    }

    prism_device_reginit(prism);
    return 0;
}
