include .env

CFLAGS = -std=c++17 -I. -I$(VULKAN_SDK_PATH)/include -I$(GLFW_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib -lvulkan -L$(GLFW_PATH)/lib -lglfw \
          -Wl,-rpath,$(VULKAN_SDK_PATH)/lib -Wl,-rpath,$(GLFW_PATH)/lib

vertexSources = $(shell find ./shaders -type f -name "*.vert")
vertexObjectFiles = $(patsubst %.vert, %.vert.spv, $(vertexSources))
fragmentSources = $(shell find ./shaders -type f -name "*.frag")
fragmentObjectFiles = $(patsubst %.frag, %.frag.spv, $(fragmentSources))

TARGET = a.out
$(TARGET): $(vertexObjectFiles) $(fragmentObjectFiles)
$(TARGET): *.cpp *.hpp
	g++ $(CFLAGS) -o $(TARGET) *.cpp $(LDFLAGS)

%.spv: %
	$(GLSLC) $< -o $@
.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)