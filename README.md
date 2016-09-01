# soft_render
软渲染，支持贴图和Phong光照

## 要求

* 可以旋转缩放的立方体，程序生成或者读取的贴图（读取bmp）
* 不允许 dx gl > 使用了 GDI
* 屏幕 800*600
* C/C++ fps 稳定 60+（图像占满屏幕 CPU: 4C4T, 3.4GHZ）
* 允许 openmp
* 高光光照（Phong）
* VS 开 O2 优化， 函数级内联，允许数学库和 sse avx等

## 主要流程：

世界坐标 > 相机坐标 > 透视投影到 视域，背部剔除，裁剪 > 光栅化，贴图、光照

光栅化用了 `Barycentric Coordinates` 重心坐标，所以开销比扫描线要大，加了 openmp 勉强符合性能要求

数学库使用了 `eigen` ，编译时需要加 include。
