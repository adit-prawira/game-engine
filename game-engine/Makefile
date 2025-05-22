include .env

CFLAGS = -std=c++17 -I. -I$(VULKAN_SDK_PATH)/include -I$(GLFW_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib -lvulkan -L$(GLFW_PATH)/lib -lglfw \
          -Wl,-rpath,$(VULKAN_SDK_PATH)/lib -Wl,-rpath,$(GLFW_PATH)/lib

a.out: *.cpp *.hpp
	g++ $(CFLAGS) -o a.out *.cpp $(LDFLAGS)

.PHONY: test clean

test: a.out
	./a.out

clean:
	rm -f a.out