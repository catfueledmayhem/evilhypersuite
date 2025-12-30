# Parallel builds
MAKEFLAGS += -j$(shell nproc)

# Compiler optimization flags
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

# Use ccache if available for faster recompilation
CXX := $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")

# Source files - using wildcard instead of find for consistency
SRCS = $(wildcard src/*.cpp) \
       $(wildcard src/**/*.cpp) \
       include/imgui/imgui.cpp \
       include/imgui/imgui_draw.cpp \
       include/imgui/imgui_tables.cpp \
       include/imgui/imgui_widgets.cpp \
       include/rlImGui/rlImGui.cpp \
       include/pugixml/pugixml.cpp \
       include/ImGuiFileDialog/ImGuiFileDialog.cpp

# Include directories
INCLUDES = -I./include \
           -I./src/headers/obstructive \
           -I./src/headers \
           -I./include/imgui \
           -I./include/rlImGui \
           -I./include/pugixml \
           -I./include/ImGuiFileDialog

# -------------------------------------------------------------------
# Linux build
# -------------------------------------------------------------------
LINUX_OBJ_DIR = out/linux
LINUX_TARGET = build/linux/utility
LINUX_CXXFLAGS = -I./include/lua-5.4.2_linux/include
LINUX_LDFLAGS = -lraylib -L./include/lua-5.4.2_linux -llua54 -lGL -lm -lpthread -ldl -lrt -lX11 -lcurl
LINUX_OBJS = $(patsubst %.cpp,$(LINUX_OBJ_DIR)/%.o,$(SRCS))
LINUX_DEPS = $(LINUX_OBJS:.o=.d)

.PHONY: all linux clean clean-linux clean-windows windows lto debug

# Default target
all: linux

linux: $(LINUX_TARGET)

# Link only when object files change
$(LINUX_TARGET): $(LINUX_OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $@..."
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LINUX_LDFLAGS) -static-libgcc -static-libstdc++

# Compile object files with automatic dependency generation
$(LINUX_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(LINUX_CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files (suppresses errors if they don't exist yet)
-include $(LINUX_DEPS)

# -------------------------------------------------------------------
# Windows build
# -------------------------------------------------------------------
WIN_OBJ_DIR = out/win64
WIN_TARGET = build/win64/utility.exe
WIN_CXX_BASE = x86_64-w64-mingw32-g++
WIN_CXX := $(shell command -v ccache >/dev/null 2>&1 && echo "ccache $(WIN_CXX_BASE)" || echo "$(WIN_CXX_BASE)")
WIN_CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(INCLUDES) \
               -I./include/raylibWin64/include \
               -I./include/lua-5.4.2_win64/include \
               -I./include/WinDivert-2.2.2-A/include \
               -I./include/curl-mingw-win64/include
WIN_LDFLAGS = -L./include/raylibWin64/lib \
              -L./include/lua-5.4.2_win64 \
              -L./include/WinDivert-2.2.2-A/x64 \
              -L./include/curl-mingw-win64/bin \
              -static -lraylib -lWinDivert -llua54 \
              -lopengl32 -lgdi32 -lwinmm -lws2_32 \
              -static-libgcc -static-libstdc++ \
              -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic \
              -lcurl-x64 \
              -mwindows -lshell32
WIN_ICON = alongside/resources/icon.o
WIN_MANIFEST = alongside/resources/app.res
WIN_OBJS = $(patsubst %.cpp,$(WIN_OBJ_DIR)/%.o,$(SRCS))
WIN_DEPS = $(WIN_OBJS:.o=.d)

windows: $(WIN_TARGET)

# Link only when object files or resources change
$(WIN_TARGET): $(WIN_OBJS) $(WIN_ICON) $(WIN_MANIFEST)
	@mkdir -p $(dir $@)
	@echo "Linking $@..."
	@$(WIN_CXX) -o $@ $(WIN_OBJS) $(WIN_ICON) $(WIN_MANIFEST) $(WIN_LDFLAGS)

# Compile object files with automatic dependency generation
$(WIN_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(WIN_CXX) $(WIN_CXXFLAGS) -MMD -MP -c $< -o $@

# Include dependency files
-include $(WIN_DEPS)

# -------------------------------------------------------------------
# Clean targets
# -------------------------------------------------------------------
clean: clean-linux clean-windows

clean-linux:
	@echo "Cleaning Linux build..."
	@rm -rf build/linux out/linux

clean-windows:
	@echo "Cleaning Windows build..."
	@rm -rf build/win64 out/win64

# -------------------------------------------------------------------
# Additional optimization targets
# -------------------------------------------------------------------
# Link-time optimization build (slower compile, faster runtime)
lto: CXXFLAGS += -flto
lto: LINUX_LDFLAGS += -flto
lto: clean-linux linux

# Debug build without optimizations
debug: CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra
debug: clean-linux linux

# -------------------------------------------------------------------
# Utility targets
# -------------------------------------------------------------------
# Show what would be built
.PHONY: info
info:
	@echo "Source files:"
	@echo "$(SRCS)" | tr ' ' '\n'
	@echo ""
	@echo "Object files:"
	@echo "$(LINUX_OBJS)" | tr ' ' '\n'
