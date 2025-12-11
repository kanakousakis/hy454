# Sonic Engine Makefile - SFML version

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -IEngine/include -IApp/include

# SFML flags
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

LDFLAGS = $(SFML_LIBS)

# Directories
BUILD_DIR = build
BIN_DIR = bin

# Engine sources (explicitly listed for proper build order)
ENGINE_SRCS = \
    Engine/src/Bitmap.cpp \
    Engine/src/Input.cpp \
    Engine/src/SystemClock.cpp \
    Engine/src/Game.cpp \
    Engine/src/TileLayer.cpp \
    Engine/src/GridLayer.cpp \
    Engine/src/AnimationFilm.cpp \
    Engine/src/Animator.cpp \
    Engine/src/Sprite.cpp \
    Engine/src/DestructionManager.cpp

ENGINE_OBJ = $(ENGINE_SRCS:Engine/src/%.cpp=$(BUILD_DIR)/engine/%.o)

# App sources  
APP_SRC = $(wildcard App/src/*.cpp)
APP_OBJ = $(APP_SRC:App/src/%.cpp=$(BUILD_DIR)/app/%.o)

# Target
TARGET = $(BIN_DIR)/SonicGame

# Default target
all: dirs $(TARGET)
	@echo "Build complete: $(TARGET)"

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)/engine $(BUILD_DIR)/app $(BIN_DIR)

# Link
$(TARGET): $(ENGINE_OBJ) $(APP_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile engine
$(BUILD_DIR)/engine/%.o: Engine/src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile app
$(BUILD_DIR)/app/%.o: App/src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean all

# Release build
release: CXXFLAGS += -O3 -DNDEBUG
release: clean all

# Clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run
run: all
	./$(TARGET)

# Help
help:
	@echo "Targets:"
	@echo "  all     - Build the project (default)"
	@echo "  debug   - Build with debug symbols"
	@echo "  release - Build optimized release"
	@echo "  clean   - Remove build files"
	@echo "  run     - Build and run"
	@echo "  help    - Show this help"

.PHONY: all dirs debug release clean run help
