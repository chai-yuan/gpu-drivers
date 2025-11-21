TOP_DIR        := $(CURDIR)

MESA_DIR       := $(TOP_DIR)/mesa-24
BUILD_DIR      := $(TOP_DIR)/build
INSTALL_DIR    := $(TOP_DIR)/install

MESON_OPTS := \
	-Dprefix=$(INSTALL_DIR) \
	-Dplatforms=x11 \
	-Dgallium-drivers=softpipe \
	-Dvulkan-drivers= \
	-Dglx=dri \
	-Dtools= \
	-Dbuildtype=debug

.PHONY: all build install clean

all: install

install: build
	@echo "--- 正在安装 Mesa 到 $(INSTALL_DIR) ---"
	@meson install -C $(BUILD_DIR)
	@echo "安装路径: $(INSTALL_DIR)"
	@echo "自动设置环境变量 export LD_LIBRARY_PATH=\"$(INSTALL_DIR)/lib/x86_64-linux-gnu:\$$LD_LIBRARY_PATH\""
	@export LD_LIBRARY_PATH=\"$(INSTALL_DIR)/lib/x86_64-linux-gnu:\$$LD_LIBRARY_PATH\"

build: $(BUILD_DIR)/build.ninja
	@echo "--- 正在编译 Mesa ---"
	@meson compile -C $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	@echo "--- 正在配置 Mesa (Meson) ---"
	@mkdir -p $(BUILD_DIR)
	@meson setup $(BUILD_DIR) $(MESA_DIR) $(MESON_OPTS)

clean:
	@echo "--- 正在清理生成的文件和目录 ---"
	@rm -rf $(BUILD_DIR) $(INSTALL_DIR)
	@echo "清理完成。"
