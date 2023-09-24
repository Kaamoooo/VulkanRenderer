E:\Vulkan\SDK\Bin\glslc.exe MyShader.vert -o MyShader.vert.spv
E:\Vulkan\SDK\Bin\glslc.exe MyShader.frag -o MyShader.frag.spv
glslangValidator -V GeometryShader.geom -o GeometryShader.spv
E:\Vulkan\SDK\Bin\glslc.exe GeometryShader.geom -o GeometryShader.spv
pause;