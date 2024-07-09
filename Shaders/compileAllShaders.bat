@echo off
set GLSLC="E:\Vulkan\SDK\Bin\glslc.exe"
set SRC_DIR=%CD%

for /r %%i in (*.vert *.frag *.tesc *.geom *.tese *.rchit *.rgen *.rmiss *.rahit) do (
    echo Compiling %%i...
    %GLSLC% "%%i" -o "%%i.spv" --target-env=vulkan1.2
  
)

echo All shaders compiled successfully.

pause;