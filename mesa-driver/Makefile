MESA_VERSION   := 24.2.4
PATCH_FILE     :=

MESA_URL       := https://archive.mesa3d.org/mesa-$(MESA_VERSION).tar.xz
ARCHIVE_NAME   := $(notdir $(MESA_URL))
DIR_NAME       := mesa-$(MESA_VERSION)

TOP_DIR        := $(CURDIR)
MESA_DIR       := $(TOP_DIR)/mesa
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

.PHONY: all build install configure clean help

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

$(BUILD_DIR)/build.ninja: $(MESA_DIR)/meson.build
	@echo "--- 正在配置 Mesa (Meson) ---"
	@mkdir -p $(BUILD_DIR)
	@meson setup $(BUILD_DIR) $(MESA_DIR) $(MESON_OPTS)

$(MESA_DIR)/meson.build: $(ARCHIVE_NAME)
	@echo "--- 正在解压源码: $(ARCHIVE_NAME) ---"
	@tar -xf $(ARCHIVE_NAME)
	@echo "--- 重命名源码目录为 'mesa' ---"
	@mv $(DIR_NAME) $(MESA_DIR)
	$(if $(PATCH_FILE), \
		@echo "--- 正在应用补丁: $(PATCH_FILE) ---"; \
		if [ ! -f "$(PATCH_FILE)" ]; then \
			echo "错误: 补丁文件 '$(PATCH_FILE)' 不存在！"; \
			rm -rf $(MESA_DIR); \
			exit 1; \
		fi; \
		cd $(MESA_DIR) && patch -p1 < ../$(PATCH_FILE), \
		@echo "--- 跳过打补丁步骤 (未配置 PATCH_FILE) ---" \
	)

$(ARCHIVE_NAME):
	@echo "--- 正在下载 Mesa 源码 ---"
	@wget -q --show-progress "$(MESA_URL)"

clean:
	@echo "--- 正在清理生成的文件和目录 ---"
	@rm -rf $(BUILD_DIR) $(INSTALL_DIR)
	@echo "清理完成。"

clean_all:
	@echo "--- 正在清理生成的文件和目录 ---"
	@rm -rf $(BUILD_DIR) $(INSTALL_DIR) $(MESA_DIR) $(ARCHIVE_NAME)
	@echo "清理完成。"
