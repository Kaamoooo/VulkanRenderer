@echo off
set GLSLC="E:\Vulkan\SDK\Bin\glslc.exe"
set SRC_DIR=%CD%

for /r %%i in (*.vert *.frag *.tesc *.geom *.tese) do (
    echo Compiling %%i...
    %GLSLC% "%%i" -o "%%i.spv"
  
)

echo All shaders compiled successfully.

pause;