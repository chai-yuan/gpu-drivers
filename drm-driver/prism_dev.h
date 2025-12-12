#ifndef PRISM_DEV_H
#define PRISM_DEV_H

#include "drm/drm_drv.h"
#include "drm/drm_encoder.h"
#include "drm/drm_plane.h"
#include <linux/io.h>

struct prism_device {
    struct drm_device dev;

    void __iomem *mmio;
    void __iomem *fb_map;
    unsigned long fb_base;
    unsigned long fb_size;

    struct drm_plane     primary_plane;
    struct drm_crtc      crtc;
    struct drm_encoder   encoder;
    struct drm_connector connector;
};

enum prism_reg {
    CONFIG_REGS_ENABLE = 0,
    CONFIG_REGS_FORMAT,
    CONFIG_REGS_WIDTH,
    CONFIG_REGS_HEIGHT,
    CONFIG_REGS_STRIDE,
    CONFIG_REGS_OFFSET,
    CONFIG_REGS_NB,
};

int prism_device_init(struct prism_device *prism);

static inline struct prism_device *to_prism_device(const struct drm_device *dev) {
    return container_of(dev, struct prism_device, dev);
}

static inline void prism_reg_write(struct prism_device *prism, enum prism_reg reg, u32 data) {
    writel(data, prism->mmio + (reg << 2));
}
static inline u32 prism_reg_read(struct prism_device *prism, enum prism_reg reg) {
    return readl(prism->mmio + (reg << 2));
}

#endif
