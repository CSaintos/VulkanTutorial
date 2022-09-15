@echo off

SET includes=-Iapp\inc -Ilib\GLFW -Ilib\glm -Ilib\Vulkan\Include
SET links= -Llib\Vulkan\Lib -Llib\GLFW -lvulkan-1 -l:libglfw3.a -lgdi32
SET defines=

echo "clean"
del build\HelloTriangle.exe
del build\*.spv /Q /F

echo "compile"
g++ %includes% -c app\src\HelloTriangle.cpp -o bin\helloTriangle.o -g

echo "compile shaders"
glslc app\src\shaders\Base.vert -o build\vert.spv
glslc app\src\shaders\base.frag -o build\frag.spv

echo "build"
g++ bin\helloTriangle.o %links% -o build\HelloTriangle.exe -g

echo "obj-clean"
del bin\*.o /Q /F