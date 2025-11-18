TOP_DIR      := $(CURDIR)

SOURCE_DIR   := $(TOP_DIR)/demos
BUILD_DIR    := $(TOP_DIR)/build

MESON_OPTS := \
	-Dbuildtype=debug \
	-Dx11=enable	\
	-Degl=enable	\
	-Dlibdrm=enable	\
	-Dvulkan=disabled

.PHONY: all demo configure clean help

all: demo

demo: $(BUILD_DIR)/build.ninja
	@meson compile -C $(BUILD_DIR)
	@echo "   可执行文件位于: $(BUILD_DIR)/src/"
	@echo "   例如，运行: ./$(BUILD_DIR)/src/demos/gears"

run: demo
	$(BUILD_DIR)/src/demos/gears

configure: $(BUILD_DIR)/build.ninja

$(BUILD_DIR)/build.ninja: $(SOURCE_DIR)/meson.build
	@echo "--- ⚙️  正在配置构建环境 (Meson) ---"
	@mkdir -p $(BUILD_DIR)
	@meson setup $(BUILD_DIR) $(SOURCE_DIR) $(MESON_OPTS)

clean:
	@echo "--- 🧹 正在清理构建目录 ---"
	@rm -rf $(BUILD_DIR)
	@echo "清理完成。"
