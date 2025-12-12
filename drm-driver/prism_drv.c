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

static const struct drm_edid *prism_hw_read_edid(struct drm_connector *connector) { return NULL; }

static void prism_hw_setmode(struct prism_device *prism, struct drm_display_mode *mode) {
    int idx;
    if (!drm_dev_enter(&prism->dev, &idx))
        return;

    DRM_INFO("%dx%d @ %d bpp\n", mode->hdisplay, mode->vdisplay, 32);

    prism_reg_write(prism, CONFIG_REGS_WIDTH, mode->hdisplay);
    prism_reg_write(prism, CONFIG_REGS_HEIGHT, mode->vdisplay);
    prism_reg_write(prism, CONFIG_REGS_STRIDE, mode->hdisplay * 4);

    drm_dev_exit(idx);
}

static void prism_hw_setformat(struct prism_device *prism, const struct drm_format_info *format) {
    int idx;

    if (!drm_dev_enter(&prism->dev, &idx))
        return;

    // TODO

    drm_dev_exit(idx);
}

static void prism_hw_setbase(struct prism_device *prism, int x, int y, int stride, u64 addr) {
    int idx;
    if (!drm_dev_enter(&prism->dev, &idx))
        return;

    prism_reg_write(prism, CONFIG_REGS_OFFSET, 0);

    drm_dev_exit(idx);
}

/* ---------------------------------------------------------------------- */

static const uint32_t prism_primary_plane_formats[] = {
    DRM_FORMAT_XRGB8888,
};

static int prism_primary_plane_helper_atomic_check(struct drm_plane *plane, struct drm_atomic_state *state) {
    struct drm_plane_state *new_plane_state = drm_atomic_get_new_plane_state(state, plane);
    struct drm_crtc        *new_crtc        = new_plane_state->crtc;
    struct drm_crtc_state  *new_crtc_state  = NULL;
    int                     ret;

    if (new_crtc)
        new_crtc_state = drm_atomic_get_new_crtc_state(state, new_crtc);

    ret = drm_atomic_helper_check_plane_state(new_plane_state, new_crtc_state, DRM_PLANE_NO_SCALING,
                                              DRM_PLANE_NO_SCALING, false, false);
    if (ret)
        return ret;
    else if (!new_plane_state->visible)
        return 0;
    return 0;
}

static void prism_primary_plane_helper_atomic_update(struct drm_plane *plane, struct drm_atomic_state *state) {
    struct drm_device             *dev                = plane->dev;
    struct prism_device           *prism              = to_prism_device(dev);
    struct drm_plane_state        *plane_state        = plane->state;
    struct drm_shadow_plane_state *shadow_plane_state = to_drm_shadow_plane_state(plane_state);
    struct drm_framebuffer        *fb                 = plane_state->fb;

    if (!fb)
        return;

    void *src_base_addr = shadow_plane_state->data[0].vaddr;
    void *dst_base_addr = prism->fb_map; // 这是我们的 VRAM 基础地址

    unsigned int height    = fb->height;
    unsigned int width     = fb->width;
    unsigned int src_pitch = fb->pitches[0]; // 源 framebuffer 的行跨度（bytes per line）
    unsigned int dst_pitch = fb->width * 4;  // 目标 VRAM 的行跨度

    unsigned int bytes_per_line = width * 4; // 每行实际需要拷贝的图像数据字节数

    unsigned int i;
    u8          *src_ptr = (u8 *)src_base_addr;
    u8          *dst_ptr = (u8 *)dst_base_addr;

    for (i = 0; i < height; ++i) {
        memcpy(dst_ptr, src_ptr, bytes_per_line);

        src_ptr += src_pitch;
        dst_ptr += dst_pitch;
    }

    /* Always scanout image at VRAM offset 0 */
    prism_hw_setbase(prism, plane_state->crtc_x, plane_state->crtc_y, fb->pitches[0], 0);
    prism_hw_setformat(prism, fb->format);
}

static int prism_primary_plane_helper_get_scanout_buffer(struct drm_plane *plane, struct drm_scanout_buffer *sb) {
    struct prism_device *prism = to_prism_device(plane->dev);
    struct iosys_map     map   = IOSYS_MAP_INIT_VADDR_IOMEM(prism->fb_map);

    if (plane->state && plane->state->fb) {
        sb->format   = plane->state->fb->format;
        sb->width    = plane->state->fb->width;
        sb->height   = plane->state->fb->height;
        sb->pitch[0] = plane->state->fb->pitches[0];
        sb->map[0]   = map;
        return 0;
    }
    return -ENODEV;
}

static const struct drm_plane_helper_funcs prism_primary_plane_helper_funcs = {
    .begin_fb_access    = drm_gem_begin_shadow_fb_access,
    .end_fb_access      = drm_gem_end_shadow_fb_access,
    .atomic_check       = prism_primary_plane_helper_atomic_check,
    .atomic_update      = prism_primary_plane_helper_atomic_update,
    .get_scanout_buffer = prism_primary_plane_helper_get_scanout_buffer,
};

static const struct drm_plane_funcs prism_primary_plane_funcs = {
    .update_plane           = drm_atomic_helper_update_plane,
    .disable_plane          = drm_atomic_helper_disable_plane,
    .destroy                = drm_plane_cleanup,
    .reset                  = drm_gem_reset_shadow_plane,
    .atomic_duplicate_state = drm_gem_duplicate_shadow_plane_state,
    .atomic_destroy_state   = drm_gem_destroy_shadow_plane_state,
};

static void prism_crtc_helper_mode_set_nofb(struct drm_crtc *crtc) {
    struct prism_device   *prism      = to_prism_device(crtc->dev);
    struct drm_crtc_state *crtc_state = crtc->state;

    prism_hw_setmode(prism, &crtc_state->mode);
}

static int prism_crtc_helper_atomic_check(struct drm_crtc *crtc, struct drm_atomic_state *state) {
    struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);

    if (!crtc_state->enable)
        return 0;

    return drm_atomic_helper_check_crtc_primary_plane(crtc_state);
}

static const struct drm_crtc_helper_funcs prism_crtc_helper_funcs = {
    .mode_set_nofb = prism_crtc_helper_mode_set_nofb,
    .atomic_check  = prism_crtc_helper_atomic_check,
};

static const struct drm_crtc_funcs prism_crtc_funcs = {
    .reset                  = drm_atomic_helper_crtc_reset,
    .destroy                = drm_crtc_cleanup,
    .set_config             = drm_atomic_helper_set_config,
    .page_flip              = drm_atomic_helper_page_flip,
    .atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
    .atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
};

static const struct drm_encoder_funcs prism_encoder_funcs = {
    .destroy = drm_encoder_cleanup,
};

static int prism_connector_helper_get_modes(struct drm_connector *connector) {
    const struct drm_edid *edid;
    int                    count;

    edid = prism_hw_read_edid(connector);

    if (edid) {
        drm_edid_connector_update(connector, edid);
        count = drm_edid_connector_add_modes(connector);
        drm_edid_free(edid);
    } else {
        drm_edid_connector_update(connector, NULL);
        count = drm_add_modes_noedid(connector, 8192, 8192);
        drm_set_preferred_mode(connector, 1280, 720);
    }

    return count;
}

static const struct drm_connector_helper_funcs prism_connector_helper_funcs = {
    .get_modes = prism_connector_helper_get_modes,
};

static const struct drm_connector_funcs prism_connector_funcs = {
    .fill_modes             = drm_helper_probe_single_connector_modes,
    .destroy                = drm_connector_cleanup,
    .reset                  = drm_atomic_helper_connector_reset,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
};

static enum drm_mode_status prism_mode_config_mode_valid(struct drm_device *dev, const struct drm_display_mode *mode) {
    struct prism_device          *bochs  = to_prism_device(dev);
    const struct drm_format_info *format = drm_format_info(DRM_FORMAT_XRGB8888);
    u64                           pitch;

    if (drm_WARN_ON(dev, !format))
        return MODE_ERROR;

    pitch = drm_format_info_min_pitch(format, 0, mode->hdisplay);
    if (!pitch)
        return MODE_BAD_WIDTH;
    if (mode->vdisplay > DIV_ROUND_DOWN_ULL(bochs->fb_size, pitch))
        return MODE_MEM;

    return MODE_OK;
}

static const struct drm_mode_config_funcs prism_mode_config_funcs = {
    .fb_create     = drm_gem_fb_create_with_dirty,
    .mode_valid    = prism_mode_config_mode_valid,
    .atomic_check  = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};

static int prism_kms_init(struct prism_device *prism) {
    struct drm_device    *dev = &prism->dev;
    struct drm_plane     *primary_plane;
    struct drm_crtc      *crtc;
    struct drm_connector *connector;
    struct drm_encoder   *encoder;
    int                   ret;

    ret = drmm_mode_config_init(dev);
    if (ret)
        return ret;

    dev->mode_config.max_width  = 8192;
    dev->mode_config.max_height = 8192;

    dev->mode_config.preferred_depth                    = 24;
    dev->mode_config.quirk_addfb_prefer_host_byte_order = true;

    dev->mode_config.funcs = &prism_mode_config_funcs;

    primary_plane = &prism->primary_plane;
    ret = drm_universal_plane_init(dev, primary_plane, 0, &prism_primary_plane_funcs, prism_primary_plane_formats,
                                   ARRAY_SIZE(prism_primary_plane_formats), NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
    if (ret)
        return ret;
    drm_plane_helper_add(primary_plane, &prism_primary_plane_helper_funcs);

    crtc = &prism->crtc;
    ret  = drm_crtc_init_with_planes(dev, crtc, primary_plane, NULL, &prism_crtc_funcs, NULL);
    if (ret)
        return ret;
    drm_crtc_helper_add(crtc, &prism_crtc_helper_funcs);

    encoder = &prism->encoder;
    ret     = drm_encoder_init(dev, encoder, &prism_encoder_funcs, DRM_MODE_ENCODER_VIRTUAL, NULL);
    if (ret)
        return ret;
    encoder->possible_crtcs = drm_crtc_mask(crtc);

    connector = &prism->connector;
    ret       = drm_connector_init(dev, connector, &prism_connector_funcs, DRM_MODE_CONNECTOR_VIRTUAL);
    if (ret)
        return ret;
    drm_connector_helper_add(connector, &prism_connector_helper_funcs);
    drm_connector_attach_edid_property(connector);
    drm_connector_attach_encoder(connector, encoder);

    drm_mode_config_reset(dev);

    return 0;
}

DEFINE_DRM_GEM_FOPS(prism_fops);

static const struct drm_driver prism_driver = {
    .driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
    .fops            = &prism_fops,
    .name            = DRIVER_NAME,
    .desc            = DRIVER_DESC,
    .major           = DRIVER_MAJOR,
    .minor           = DRIVER_MINOR,
    .patchlevel      = DRIVER_PATCHLEVEL,
    .dumb_create     = drm_gem_shmem_dumb_create,
    .fbdev_probe     = drm_fbdev_shmem_driver_fbdev_probe,
};

static int prism_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
    struct prism_device *prism;
    struct drm_device   *dev;
    int                  ret;

    // 移除 PCI 设备的现有fb，例如来源于efifb的fb
    ret = aperture_remove_conflicting_pci_devices(pdev, prism_driver.name);
    if (ret)
        return ret;

    // 分配设备资源
    prism = devm_drm_dev_alloc(&pdev->dev, &prism_driver, struct prism_device, dev);
    if (IS_ERR(prism))
        return PTR_ERR(prism);
    dev = &prism->dev;

    ret = pcim_enable_device(pdev);
    if (ret)
        goto err_free_dev;
    pci_set_drvdata(pdev, dev);

    // 硬件与KMS初始化
    ret = prism_device_init(prism);
    if (ret)
        goto err_free_dev;
    ret = prism_kms_init(prism);
    if (ret)
        goto err_free_dev;

    ret = drm_dev_register(dev, 0);
    if (ret)
        goto err_free_dev;

    // 提供fbdev模拟层，由drm_driver.fbdev_probe提供
    drm_client_setup(dev, NULL);

    return ret;

err_free_dev:
    drm_dev_put(dev);
    return ret;
}

static void prism_pci_remove(struct pci_dev *pdev) {
    struct drm_device *dev = pci_get_drvdata(pdev);

    drm_dev_unplug(dev);
    drm_atomic_helper_shutdown(dev);
}

static void prism_pci_shutdown(struct pci_dev *pdev) { drm_atomic_helper_shutdown(pci_get_drvdata(pdev)); }

static const struct pci_device_id prism_pciid_tbl[] = {{
                                                           .vendor      = 0x1234,
                                                           .device      = 0x2222,
                                                           .subvendor   = PCI_SUBVENDOR_ID_REDHAT_QUMRANET,
                                                           .subdevice   = PCI_SUBDEVICE_ID_QEMU,
                                                           .driver_data = 0,
                                                       },
                                                       {/* end of list */}};

static struct pci_driver prism_pci_driver = {
    .name     = DRIVER_NAME,
    .id_table = prism_pciid_tbl,
    .probe    = prism_pci_probe,
    .remove   = prism_pci_remove,
    .shutdown = prism_pci_shutdown,
};

// 注册一个DRM驱动到PCI设备
drm_module_pci_driver(prism_pci_driver);

MODULE_DEVICE_TABLE(pci, prism_pciid_tbl);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
