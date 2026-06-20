# CG-FINAL — AimTrainer (计算机图形学大作业)

> 一个基于自研 OpenGL 引擎的三维瞄准训练游戏。这是从课程仓库抽取的
> **Final Project 独立部分**（自研框架 + 游戏代码 + 文档 + 第三方库），
> clone 即可构建。

## 快速开始（Windows + Visual Studio 2022）

需要：CMake ≥ 3.15、C++17、VS 2022。

```bash
cmake -B build -G "Visual Studio 17 2022" -S .
cmake --build build --config Release
# 运行（从工程根目录运行，资源使用相对路径 media/）
build/Release/FinalProject.exe
```

## 玩法

- **FPS 模式**（默认）：鼠标转视角，WASD 移动，左键射击
- **武器**：`1` 手枪（无限射速）/ `2` 狙击枪（限速 + 右键开镜放大）
- **标准模式**：单个静态气球靶，打中随机重生，限时计分
- **狂野模式**：多个球沿自研 NURBS 曲线运动，球-球互斥碰撞
- **后室探索（彩蛋）**：射击后墙中央的门可打开/关闭，走进后室击中金色大靶可获得 +3 分奖励
- **F1**：暂停菜单（灵敏度/全屏/模式/武器/限时/球大小/狂野速度/光源编辑/门手动开关）
- **Tab**：FPS ↔ Orbit 相机切换　**RMB**（狙击）开镜　**L** 手电筒

## 满足的课程要求

**基本 7 项：**

1. 基本体素建模（6 种：球/立方体/圆柱/圆锥/多面棱柱/多面棱台）
2. 自研 OBJ 加载器（解析 v/vt/vn/f）+ 导出，替代 tinyobjloader
3. 基本材质（Blinn-Phong）+ 纹理贴图（rock/wall 贴图，地面/平台/障碍物分别使用不同材质）
4. 几何变换（旋转/平移/缩放）
5. 多光源（directional + point × 3 + spot + hemisphere），F1 可编辑位置/光强/锥角
6. 摄像机漫游（FPS + Orbit，zoom/pan/orbit/zoomToFit）
7. OBJ 序列动画（开场变形球，顶点逐帧变化，非变换）+ F2 截图

**额外加分项：**

| 项目 | 实现内容 |
|---|---|
| NURBS 曲线 | Cox-de Boor 基函数，不用 GLU；Wild 模式靶子沿 NURBS 轨迹运动 |
| 实时阴影 | 方向光 + 聚光灯双 shadow map，PCF 软阴影 |
| 漫游碰撞检测 | sphere-AABB 玩家 vs 障碍物（含动态门碰撞），sphere-sphere 靶子互斥 |
| 完整三维游戏 | 双模式/双武器/计分/排行榜/暂停菜单/实时 HUD/后室探索 |
| 功能交互真实性 | 后墙交互门（射击开/关）：弹簧阻尼动画、局部空间射线检测、动态碰撞体积，开门后空间可探索 |

## 主要视觉与游戏功能

**开场动画（SNAPFIRE）**
- OBJ 序列变形球，双层波纹 + 颜色循环，30fps
- 互动射球：准心 + 圆圈，射中变形球触发粒子爆炸 + "NICE SHOT!" 反馈，1.2 秒后进入游戏
- SNAPFIRE 大标题 + 副标题字幕

**射击反馈**
- 命中标记：橙色"×"在准心附近闪现，0.18 秒淡出
- 动态准心：开枪时间隙扩大，80 px/s 弹回
- "+1" 飘字：命中时从目标世界坐标投影到屏幕，向上飘动后淡出
- 三层粒子爆炸：橙红大爆 + 黄白核心 + 深红余烬

**第一人称枪械**
- gun.obj 视角模型，始终渲染在最前（独立深度缓冲）
- 弹簧阻尼后坐力动画（kick=2.4 rad，spring=90，damping=28）
- 手枪/狙击枪分别播放对应枪声（miniaudio fire-and-forget）

**天空盒与材质**
- 6 面 cubemap 天空盒
- 地面/障碍物使用 rock.jpg，平台/坡道使用 wall.jpg
- 门扇深棕色木质材质（区分于石墙）

**场景结构**
- 主战场：前方靶区 + 左右坡道 + 后方平台
- 后室：射击后墙门进入，内有两侧箱子、暖橙色点光源、金色大靶（命中 +3 分，5 秒重生）、回程坡道

**右侧实时计分面板**
- 开枪数 / 命中数 / 准确率 / 剩余时间（颜色随时间变化：白→黄→红）

## 操作速查

| 键位 | 功能 |
|---|---|
| WASD | 移动 |
| 鼠标 | 转视角 |
| LMB | 射击（击靶 / 开关门） |
| RMB | 狙击开镜 |
| 1 / 2 | 切换手枪 / 狙击枪 |
| Tab | FPS ↔ Orbit 相机 |
| L | 手电筒开关 |
| F1 | 暂停面板 |
| F2 | 截图（保存至 media/screenshots/） |
| ESC | 暂停 |

## 工程结构

```
source/base/    自研框架（OBJ loader、model、camera、shader、texture、framebuffer、skybox …）
source/game/    游戏逻辑（aim_trainer、fps_camera、orbit_camera、particle_system、audio、game_state …）
source/         primitives（体素生成）、nurbs（曲线）、collision（碰撞）
media/          模型(.obj) / 贴图(.jpg) / shader(.vert/.frag) / 音效(.mp3/.wav)
external/       glad / glfw / glm / imgui / stb / miniaudio
docs/           设计文档
```

## 技术栈

C++17 · CMake · OpenGL 3.3 core · GLFW · glad · GLM · Dear ImGui · stb_image · miniaudio

## 后续可扩展方向

- **多武器模型**：目前两把武器共用同一视角模型，可为狙击枪单独制作 OBJ 并在切换时替换
- **运动物理**：当前高度由地面高度场驱动，可引入简单跳跃/重力系统，使高台间的过渡更自然
- **Wild 模式随机化**：每局开始时随机生成 NURBS 控制点，提升重复游玩的新鲜感
- **持久化排行榜**：当前分数仅保存在内存，可序列化至本地文件跨局保留
- **后处理**：景深（狙击开镜）、动态模糊、屏幕空间 AO 等效果可进一步丰富画面层次

