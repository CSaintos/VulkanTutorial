@echo off

SET includes=-Iapp\inc -Ilib\GLFW -Ilib\glm -Ilib\Vulkan\Include
SET links=-Llib\Vulkan\Lib -Llib\GLFW -lvulkan-1 -l:libglfw3.a -lgdi32
SET defines=

echo "clean"
del build\SampleWindow.exe

echo "compile"
g++ %includes% -c app\src\SampleWindow.cpp -o bin\sampleWindow.o -g

echo "build"
g++ bin\sampleWindow.o %links% -o build\SampleWindow.exe -g

echo "obj-clean"
del bin\*.o /Q /F