TOP_DIR      := $(CURDIR)
BUILD_ROOT   := $(TOP_DIR)/build

# --- Demos 配置 ---
DEMOS_SRC    := $(TOP_DIR)/demos
DEMOS_BUILD  := $(BUILD_ROOT)/demos
DEMOS_OPTS   := \
	-Dbuildtype=debug \
	-Dx11=enabled	\
	-Degl=enabled	\
	-Dlibdrm=enabled \
	-Dvulkan=disabled

# --- Kmscube 配置 ---
KMSCUBE_SRC    := $(TOP_DIR)/kmscube
KMSCUBE_BUILD  := $(BUILD_ROOT)/kmscube
KMSCUBE_OPTS   := \
	-Dbuildtype=debug

.PHONY: all demos kmscube clean help

# 默认构建所有
all: demos kmscube

# --- 构建 Demos ---
demos: $(DEMOS_BUILD)/build.ninja
	@echo "=== 🏗️  正在构建 Mesa Demos ==="
	@meson compile -C $(DEMOS_BUILD)
	@echo "✅ Demos 构建完成. 位于: $(DEMOS_BUILD)/src/demos/"

$(DEMOS_BUILD)/build.ninja: $(DEMOS_SRC)/meson.build
	@echo "--- ⚙️  正在配置 Demos (Meson) ---"
	@mkdir -p $(DEMOS_BUILD)
	@meson setup $(DEMOS_BUILD) $(DEMOS_SRC) $(DEMOS_OPTS)

# --- 构建 Kmscube ---
kmscube: $(KMSCUBE_BUILD)/build.ninja
	@echo "=== 🏗️  正在构建 Kmscube ==="
	@meson compile -C $(KMSCUBE_BUILD)
	@echo "✅ Kmscube 构建完成. 位于: $(KMSCUBE_BUILD)/"

$(KMSCUBE_BUILD)/build.ninja: $(KMSCUBE_SRC)/meson.build
	@echo "--- ⚙️  正在配置 Kmscube (Meson) ---"
	@mkdir -p $(KMSCUBE_BUILD)
	@meson setup $(KMSCUBE_BUILD) $(KMSCUBE_SRC) $(KMSCUBE_OPTS)

run-gears: demos
	$(DEMOS_BUILD)/src/demos/gears

run-kmscube: kmscube
	$(KMSCUBE_BUILD)/kmscube

# --- 清理 ---
clean:
	@echo "--- 🧹 正在清理所有构建目录 ---"
	@rm -rf $(BUILD_ROOT)
	@echo "清理完成。"
