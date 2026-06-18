# CG-FINAL — AimTrainer (计算机图形学大作业)

> 一个基于自研 OpenGL 引擎的三维瞄准训练游戏。这是从课程仓库抽取的
> **Final Project 独立部分**（自研框架 + 游戏代码 + 文档 + 第三方库），
> clone 即可构建。

## 快速开始（Windows + Visual Studio 2022）

需要：CMake ≥ 3.15、C++17、VS 2022。

```bash
# 本仓库已经是自包含工程（source/base 是自研框架，external 是第三方库）
cmake -B build -S .
cmake --build build --config Release
# 运行（资源用相对路径 media/，从 exe 目录运行）
cd build/bin/Release   # 或 build/Release，取决于生成器
./FinalProject.exe
```

> 用 VS 生成器：`cmake -B build -G "Visual Studio 17 2022" -S .`
> 然后 `cmake --build build --config Release`。

## 玩法
- **FPS 模式**（默认）：鼠标转视角，WASD 移动，左键射击
- **武器**：`1` 手枪（无限射速）/ `2` 狙击枪（限速 + 右键开镜放大）
- **标准模式**：单个静态气球靶，打中随机重生，限时计分
- **狂野模式**：多个球沿自研 NURBS 曲线运动，球-球互斥碰撞
- **F1**：暂停菜单（灵敏度/全屏/模式/武器/限时/球大小/狂野速度 + 退出）
- **Tab**：FPS ↔ Orbit 相机切换 · **RMB**（狙击）开镜

## 满足的课程要求
**基本 7 项：**
1. 基本体素建模（6 种：球/立方体/圆柱/圆锥/多面棱柱/多面棱台）
2. 自研 OBJ 加载器（解析 v/vt/vn/f）+ 导出，替代 tinyobjloader
3. 基本材质（Blinn-Phong）+ 纹理显示/编辑
4. 几何变换（旋转/平移/缩放）
5. 多光源（directional + point + spot + hemisphere），可编辑位置/光强
6. 摄像机漫游（FPS + Orbit，zoom/pan/orbit/zoomToFit）
7. OBJ 序列动画（开场变形球动画）+ F2 截图（Task 9，已完成）

**额外 5 项：**
- 自研 NURBS 曲线（不用 GLU 接口，Cox-de Boor 基函数）
- 实时阴影（directional + spotlight，shadow mapping + PCF）
- 漫游碰撞检测（sphere-AABB 玩家 vs 障碍物 + sphere-sphere 靶子互斥）
- 完整三维游戏（双模式、武器、计分、排行榜、暂停菜单、HUD）
- 场景 OBJ 导出（F1 面板 "Export scene to .obj" 按钮，自研 saveObj）

## 工程结构（助教要求的"从零搭建"）
```
source/base/    自研框架（OBJ loader、model、camera、shader、texture、framebuffer、skybox …）
source/game/    游戏逻辑（aim_trainer、fps_camera、orbit_camera、target、weapon、
                particle_system、audio、game_state、leaderboard）
source/         primitives（体素生成）、nurbs（曲线）、collision（碰撞）
media/          模型(.obj) / 贴图(.png/.jpg) / shader(.glsl) / 音效(.wav)
external/       第三方库：glad / glfw / glm / imgui / stb / miniaudio
                （不含 tinyobjloader —— OBJ 加载是自研的）
docs/           设计文档 + 实施计划（含进度、待办、已知 bug）
```

## 文档
- **设计**：`docs/superpowers/specs/2026-06-17-final-project-aimtrainer-design.md`
- **实施计划 + 进度 + 待办 + 已知 bug**：`docs/superpowers/plans/2026-06-17-final-project-aimtrainer.md`

## 操作
- **开场动画**：启动时播放一段 OBJ 序列动画（变形球，顶点逐帧变化，非变换），
  ~5 秒后自动进入游戏；任意键 / 左键 / 空格可跳过。
- **F2**：随时截图，保存为 `media/screenshots/shot_XXX.png`（自上而下编号）。
- **F1 → Capture/Export**：`Export scene to .obj` 把当前场景（障碍物 + 平台 +
  斜坡 + 存活靶子）合并导出为单个 `media/exports/scene.obj`（自研 saveObj）。

## 待办（剩余打磨项）
- 武器 viewmodel + 开火动画 + 贴图（目前用准星 + 粒子枪口火花代替）
- 天空盒（base 框架已含 Skybox，需补 6 面 cubemap 贴图）
- 已知小问题：spot 光与半球环境光的叠加可继续微调；Wild 模式 NURBS 轨迹仍为
  固定控制点（可改为每局随机）

## 技术栈
C++17 · CMake · OpenGL 3.3 core · GLFW · glad · GLM · Dear ImGui · stb_image · miniaudio
