# Vulkan Renderer

接触Vulkan的最初目的是为了交图形学的作业，但是逐渐对Vulkan产生了兴趣，打算最近把时间大多数花在Vulkan上

从简单的光栅化Renderer开始，后面根据能力看是否能够做出vulkan的光线追踪

## 进度

9.17 完成了基本的3D单物体渲染以及Transform变换

9.18 添加了正交投影以及透视投影

9.19 实现了相机移动，视角转动

9.20~9.21 完成程序结构示例图

9.23~9.24 完成几何着色器动态生成三维分形

10.4 添加了index buffer，更改了buffer属性为device local并配合staging buffer以优化性能