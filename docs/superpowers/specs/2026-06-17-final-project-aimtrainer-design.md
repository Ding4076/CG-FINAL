# Final Project 设计文档：AimTrainer

> 基于自研 OpenGL 引擎的三维瞄准训练游戏（Aimlab 风格）
>
> 课程：计算机图形学 · 大作业（Final Project）
> 日期：2026-06-17
> 作者：赵柳舟（学号 3230100113）

---

## 1. 项目定位

一个第一人称瞄准训练游戏，两种核心模式：**标准模式**（单球静态固定靶，限时计分排行）与**狂野模式**（多球沿自研 NURBS 曲线运动，球-球互斥碰撞）。玩家用**手枪/狙击枪**射击；关卡有栏杆/箱子/围墙限制玩家移动（漫游碰撞）；带 Tab 切换的 Orbit 编辑相机、**实时阴影**、**开场 OBJ 序列动画**、气球爆炸特效、计分排行榜。

项目同时满足全部 7 项基本要求 + 4 项额外要求（NURBS / 实时阴影 / 完整三维游戏 / 实时碰撞检测）。

---

## 2. 评分覆盖映射（核心：确保不丢分）

### 基本要求（35 分）

| 要求 | 分值 | 本项目实现 |
|---|---|---|
| 1. 基本体素建模（6 选 5） | **15** | 代码参数化生成 球/立方体/圆柱/圆锥/多面棱台/多面棱柱（6 种） |
| 2. OBJ 导入导出 | — | **自研 OBJLoader**（解析 v/vt/vn/f）+ 导出，移除 tinyobjloader |
| 3. 材质 + 纹理编辑 | — | Blinn-Phong 材质，ImGui 面板调 kd/ks/shininess + 换纹理；武器用贴图 |
| 4. 几何变换 | — | 物体 transform（旋转/平移/缩放），靶子运动即变换 |
| 5. 多光源 | — | directional + point + spot + hemisphere 天光，ImGui 调位置/光强 |
| 6. 摄像机 | — | FPS 射击 + Tab 切 Orbit，含 pan/zoom/zoomToFit |
| 7a. 动画播放 | — | 开场 OBJ 序列帧动画播放器 |
| 7b. 截图 | — | glReadPixels 存 PNG（stb_image_write） |

### 额外要求（20 分，每项 5 分）

| 要求 | 分值 | 本项目实现 |
|---|---|---|
| NURBS | 5 | 自研 NURBS 曲线（不用 GL 接口），靶子运动轨迹 + 可视化控制点 |
| 复杂光照 | 5 | shadow mapping 实时阴影 |
| 完整三维游戏 | 5 | 双模式射击游戏：计分/命中率/反应时间/排行榜/音效，可玩 |
| 实时碰撞检测 | 5 | 漫游时球-AABB（玩家 vs 栏杆/箱子/围墙）+ 球-球（狂野模式靶子互斥）碰撞，基于空间几何，玩家撞墙不能穿透 |

预估总分：基本 35 + 额外 20 = **55**（助教规定额外最高加至 40 分，本项目 4 项 20 分在限内）。

> 说明：枪械射线 vs 靶子的射线-球求交是**游戏命中判定**（服务玩法计分），不计入碰撞检测加分项。加分项2 要求的是"**漫游时**基于空间几何的碰撞"——玩家移动被场景物体挡住、不能穿透，本项目用球-AABB/球-球实现。

---

## 3. 工程结构（严格遵循助教演示的"从零搭建"）

```
projects/finalproject/
├── CMakeLists.txt              # 独立工程，可被根 CMake 包含也可独立 cmake -B build
├── source/
│   ├── base/                   # 从根 projects/base/ 拷贝精简版（删原 model 的 tinyobjloader 依赖）
│   │   ├── application.{h,cpp}
│   │   ├── camera.{h,cpp}
│   │   ├── transform.{h,cpp}
│   │   ├── glsl_program.{h,cpp}
│   │   ├── texture2d.{h,cpp}
│   │   ├── texture_cubemap.{h,cpp}   # hemisphere 天光
│   │   ├── skybox.{h,cpp}
│   │   ├── framebuffer.{h,cpp}       # shadow map / 截图
│   │   ├── fullscreen_quad.{h,cpp}
│   │   ├── bounding_box.{h}, frustum.h, plane.h, input.h, vertex.h, light.h, ...
│   │   ├── model.{h,cpp}             # 自研，移除 tinyobjloader 依赖
│   │   └── obj_loader.{h,cpp}        # 自研 OBJ 解析 + 导出
│   ├── primitives.{h,cpp}      # 参数化生成 6 种体素网格
│   ├── nurbs.{h,cpp}           # 自研 NURBS 曲线/曲面求值
│   ├── scene.{h,cpp}           # 场景图：物体/光源/相机管理
│   ├── shaders/                # GLSL（由 CMake 拷到 build 目录）
│   │   ├── blinn_phong.vert/frag
│   │   ├── shadow_map.vert/frag
│   │   ├── depth.frag
│   │   ├── skybox.vert/frag
│   │   ├── nurbs_debug.vert/frag
│   │   ├── particle.vert/frag        # 气球爆炸/枪口火焰
│   │   └── hud_*                     # 准星/scope 遮罩
│   ├── game/                   # 游戏逻辑（助教所说 "project" 主逻辑）
│   │   ├── aim_trainer.{h,cpp}  # 主 Application 子类，驱动整个游戏
│   │   ├── target.{h,cpp}       # 靶子（气球球体 + NURBS 轨迹 + 状态）
│   │   ├── fps_camera.{h,cpp}   # 第一人称射击相机
│   │   ├── orbit_camera.{h,cpp} # Tab 切换的 Orbit 编辑相机
│   │   ├── game_state.{h,cpp}   # 计分/命中率/反应时间/难度
│   │   ├── weapon.{h,cpp}       # 武器系统（手枪/狙击枪/viewmodel/枪口火焰/音效）
│   │   ├── particle_system.{h,cpp}  # 气球爆炸/枪口火焰粒子
│   │   ├── collision.{h,cpp}    # 漫游碰撞：球-AABB（玩家vs栏杆/箱子）+ 球-球（狂野模式靶子互斥）
│   │   ├── obj_sequence.{h,cpp} # 开场 OBJ 序列动画播放器
│   │   ├── audio.{h,cpp}        # miniaudio 封装
│   │   ├── leaderboard.{h,cpp}  # 排行榜存档
│   │   └── hud.{h,cpp}          # ImGui overlay：准星/分数/scope/编辑面板
│   └── main.cpp
├── media/
│   ├── models/                 # .obj 模型（场景道具、OBJ 序列帧）
│   ├── textures/               # .png 贴图（含 weapons/ 武器贴图、balloon/ 气球贴图）
│   ├── sounds/                 # .wav/.ogg 音效（手枪/狙击/开镜/爆炸）
│   ├── shaders/                # （构建时拷贝）
│   └── config/                 # 场景/光源/NURBS 控制点/scores 排行榜配置
└── external/                   # 物理拷贝（删 tinyobjloader）
    ├── glad/
    ├── glfw/
    ├── glm/
    ├── imgui/
    ├── stb/
    └── miniaudio/              # 单头文件音频库
```

### CMakeLists.txt（逐字对应助教演示）

```cmake
cmake_minimum_required(VERSION 3.15)
project(FinalProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# 1. 拷 media 到 build 目录（路径依赖）
file(COPY ${CMAKE_SOURCE_DIR}/media DESTINATION ${CMAKE_BINARY_DIR})

# 2. 第三方库路径
set(THIRD_PARTY_PATH ${CMAKE_SOURCE_DIR}/external)
add_subdirectory(${THIRD_PARTY_PATH}/glfw ...)
add_subdirectory(${THIRD_PARTY_PATH}/glad  ...)
add_subdirectory(${THIRD_PARTY_PATH}/glm   ...)
add_subdirectory(${THIRD_PARTY_PATH}/imgui ...)
add_subdirectory(${THIRD_PARTY_PATH}/stb   ...)
# miniaudio 为单头文件库，include 即可

# 3. GLOB 源码（base + project）
file(GLOB_RECURSE SOURCES "source/*.cpp" "source/*.h")

# 4. 生成 exe + 输出位置 + include + link
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${THIRD_PARTY_PATH}/glfw/include
    ${THIRD_PARTY_PATH}/glad/include
    ${THIRD_PARTY_PATH}/glm
    ${THIRD_PARTY_PATH}/imgui
    ${THIRD_PARTY_PATH}/stb
    ${THIRD_PARTY_PATH}/miniaudio)
target_link_libraries(${PROJECT_NAME} glad glfw glm imgui stb)
# Windows: 设输出位置、WIN32、复制 media 等细节
```

**CMake 接入策略（方案 A）**：finalproject 的 CMakeLists 既能被根 CMake 的 `projects/*` 自动扫描包含（方便平时统一构建），也能在 `projects/finalproject/` 内独立 `cmake -B build` 自包含构建（满足助教"从零"要求）。

> 注：`external/` 采用物理拷贝（按助教演示）。鉴于根仓库已含 glad/glfw/glm/imgui/stb 源码，物理拷贝时直接复制根 `external/` 下对应目录，**移除 tinyobjloader**，**新增 miniaudio**（单头文件）。构建产物/缓存不入库。

---

## 4. 核心数据流与渲染管线

### 4.1 自研 OBJ 加载器（必做 2，分水岭）

`obj_loader.{h,cpp}` 纯手写，逐行解析 `.obj`：
- `v x y z` → 位置；`vt u v` → 纹理坐标；`vn x y z` → 法线
- `f v/vt/vn v/vt/vn v/vt/vn` → 三角面（支持 `v`、`v/vt`、`v//vn`、`v/vt/vn` 三种格式）
- 解析后组装成 `std::vector<Vertex>{position, normal, texCoord}` + `std::vector<uint32_t>` 索引
- **导出**：反向把内存网格写回 `.obj`（v/vt/vn/f），支持把场景任意物体（含参数化体素）导出
- **不处理 mtl**（按助教"不强制"），材质由程序内材质系统管理
- `Vertex` 布局与 VAO 属性位置 0/1/2（pos/normal/texcoord）保持与 base 一致，复用所有着色器

`model.{h,cpp}` 接收 `obj_loader` 解析结果，自行上传 VBO/EBO/VAO，移除一切 tinyobjloader 痕迹。

### 4.2 体素生成（必做 1，6 种超额）

`primitives.{h,cpp}` 参数化生成网格（思路同"五星红旗"作业 project1）：
- `createSphere(radius, segments)`
- `createBox(w, h, d)`
- `createCylinder(r, h, seg)`
- `createCone(r, h, seg)`
- `createPrismFrustum(rTop, rBottom, h, nSides)`（多面棱台）
- `createPrism(r, h, nSides)`（多面棱柱）

用途：靶子=球/气球，地面/平台=多面棱台，柱子=圆柱，武器=立方体+圆柱+长方体拼装。

### 4.3 渲染主循环（每帧，多 Pass）

```
Pass 0:  开场动画阶段 → obj_sequence 逐帧绘制（必做 7a）
Pass 1:  Shadow Map — 从主光源视角渲染深度到 FBO（额外: 阴影）
Pass 2:  主场景 — Blinn-Phong，采样 shadow map，多光源
         （directional + point + spot + hemisphere）
Pass 3:  天空盒
Pass 4:  NURBS 调试 — 画控制点 + 曲线（额外: NURBS 可视化）
Pass 5:  粒子 — 气球爆炸碎片 / 枪口火焰（加法混合）
Pass 6:  武器 viewmodel — 屏幕右下，贴图+金属材质，渲染在场景之上
Pass 7:  HUD overlay — ImGui：准星 / scope 遮罩 / 分数 / 编辑面板
         截图：glReadPixels 读主 FBO → stb_image_write 存 PNG（必做 7b）
```

**材质系统（必做 3）**：每个物体 `Material{ kd, ks, shininess, texture2d*, textureType }`，ImGui 实时改 → uniform 即时更新。武器用高 ks + 高 shininess + 金属贴图模拟金属质感。

**光源系统（必做 5）**：场景持有 `std::vector<Light>`，支持 directional / point / spot / hemisphere 四种；ImGui 改位置/颜色/强度/聚光半角。

### 4.4 NURBS 靶子轨迹（额外，自研）

`nurbs.{h,cpp}` 实现节点向量 + 基函数（de Boor 算法）求值，**不用 GLU NURBS 接口**：
- 给控制点 + 节点向量 + 阶数 → 求曲线上参数 t 的点
- 靶子 `target.update(dt)`：沿 NURBS 曲线推进 t → 新位置 → 写入 transform
- ImGui 可拖拽控制点 → 实时重算曲线
- 调试 Pass 把控制点画成小球、曲线画成线段

### 4.5 双相机（必做 6）

- `FpsCamera`：锁定鼠标，鼠标转视角，WASD 走，左键射击。游戏主视角。
- `OrbitCamera`：右键旋转、滚轮 zoom、中键 pan、`F` 键 zoomToFit（基于场景包围球）。
- **Tab 切换**：FPS ↔ Orbit，释放/锁定鼠标。两者都派生自 base `Camera`。

### 4.6 射击命中判定

FPS 模式左键 → 从相机发射屏幕中心射线（狙击开镜时是 scope 中心）→ 与靶子球做**射线-球求交**（空间几何，非平面判定）→ 命中则计分、触发气球爆炸特效、靶子重生。

> 这是游戏命中判定，不计入碰撞检测加分项（见 §2 说明）。

### 4.7 漫游碰撞检测（额外：实时碰撞检测）

`collision.{h,cpp}` 实现基于空间几何的碰撞，玩家移动被场景物体挡住、不能穿透：

- **球-AABB（玩家 vs 栏杆/箱子/围墙）**：玩家抽象为球（球心=相机位置略下沉，半径~0.4），障碍物是立方体 AABB。最近点法：求 AABB 上离球心最近的点 `closest`，若 `length(center - closest) < playerRadius` 则碰撞，沿法向把玩家推回。**障碍物用必做项1 的立方体体素**，碰撞与体素建模一举两得。
- **球-球（仅狂野模式：靶子 vs 靶子互斥）**：`length(cA - cB) < rA + rB` 则互相推开，防止多球挤在一起/轨迹重叠缠绕。玩家不被球推（玩家阻挡由球-AABB 体现）。
- 接入 FPS 相机移动逻辑：每帧计算期望位移 → **逐轴**做碰撞检测（先 X 后 Z，可沿墙滑动）→ 推回防穿透。移动**锁定在水平面**（禁上下/跳跃）。

### 4.8 关卡与场景设计

靶场 = 一个有栏杆/箱子/围墙的封闭空间。俯视布局：

```
        ┌─────────────────────────┐
        │      靶场区域            │   ← 玩家进不来
        │   (球 / NURBS 轨迹)      │
        ├═══════ 栏杆 ═══════┤   ← 球-AABB，玩家被挡
        │                         │
        │  [左箱]   玩家   [右箱]  │   ← 玩家活动区，前后左右移动
        │         (出生点)         │     左右被箱子限，间距够大
        └─────────────────────────┘
```

- **地面/平台**：多面棱台体素（必做项1）
- **前方栏杆**：一排短立方体拼成（视觉有栅栏感），碰撞用一整条 AABB 保证不卡缝；挡住玩家进入靶场区域
- **偏左/偏右箱子**：立方体体素（必做项1），限制玩家左右走位，**但间距留宽保证活动范围够大**（关卡尺寸可调）
- **围墙**：立方体体素围成封闭空间
- **装饰柱**：圆柱/圆锥体素（必做项1）
- **靶子**：球体 + 气球纹理（标准模式单球静态随机刷新 / 狂野模式多球沿 NURBS 运动）
- **武器 viewmodel**：体素拼装 + 贴图（屏幕右下）

### 4.9 双模式

| | **标准模式（Standard）** | **狂野模式（Wild）** |
|---|---|---|
| 靶子数量 | 同时 1 个 | 同时多个（3-5）|
| 运动方式 | 静态固定靶，打完随机生成下一个 | 沿 NURBS 曲线运动 |
| 视野保证 | 靶子保证在玩家视野内（生成时检查视野锥） | 不保证，考验搜寻+预判 |
| 碰撞 | 玩家 vs 栏杆/箱子（球-AABB） | 同左 + 球-球靶子互斥 |
| 加分承载 | 完整游戏可玩性 | NURBS（轨迹）+ 球-球碰撞 |

### 4.10 可调参数（ImGui）

三个游戏参数滑块，运行时实时生效：
- **每局限时**（如 30/60/120s）：`game_state` 倒计时，`SliderInt`
- **狂野模式小球速度**：`target` 推进 NURBS 参数 t 的步长，`SliderFloat`
- **小球大小**（半径）：用 `transform.scale` 缩放（不重生成网格），`SliderFloat`

### 4.9 可调参数（ImGui）

三个游戏参数滑块，运行时实时生效：
- **每局限时**（如 30/60/120s）：`game_state` 倒计时，`SliderInt`
- **NURBS 模式小球速度**：`target` 推进参数 t 的步长，`SliderFloat`
- **小球大小**（半径）：用 `transform.scale` 缩放（不重生成网格），`SliderFloat`

---

## 5. 武器系统（FPS 主玩法）

| | **手枪 Pistol** | **狙击枪 Sniper** |
|---|---|---|
| 触发 | 默认，按 `1` | 按 `2` |
| 射击 | 左键点射，**不限射速**（连发） | 左键单发，**限速**（如 1 发/秒，拉栓冷却） |
| 瞄准 | 无 | **右键开镜**（FOV 缩小 + 圆形 scope 遮罩 HUD） |
| 音效 | 手枪开火音 | 狙击开火音 + 开镜音 |
| 枪口火焰 | ✓ 点射即闪 | ✓ 单发更猛 |
| 视觉 | 右下手枪 viewmodel | 右下狙击 viewmodel |

**武器建模 = 体素几何 + 贴图（强化必做 1 建模表达 + 必做 3 材质纹理）：**
- 几何：参数化体素拼装（枪身=立方体，枪管=圆柱，握把/弹匣=长方体，瞄准镜=圆柱+小立方体）
- 贴图：每部件绑定一张材质贴图（金属漆面/磨砂金属/防滑握把纹路），加载自 `media/textures/weapons/`
- 材质：Blinn-Phong 高 ks + 高 shininess 模拟金属高光
- **viewmodel**：固定屏幕右下，渲染在场景之上；鼠标移动时枪微微摆动（bobbing）

**枪口火焰**：射击瞬间在枪口位置生成短暂发光 sprite（billboard + 加法混合 + 噪声纹理，~50ms）。

**音效**：`miniaudio`（单头文件，加进 external）封装 `audio.{h,cpp}`，音效文件放 `media/sounds/`。

---

## 6. 游戏玩法细节

双模式定义见 §4.9（标准 / 狂野）。两种模式共用武器系统（§5），主菜单/暂停菜单切换。

### 气球靶与爆炸特效
- 靶子做成**气球**（球体 + 气球纹理 + 顶部小球结）。标准模式静态随机刷新；狂野模式沿 NURBS 运动。
- 打爆 → **气球爆炸特效**：粒子系统，命中瞬间 ~30 个粒子，球面随机方向初速度，重力下落，alpha 渐隐；球壳碎片为小三角形面片飞散旋转；颜色取气球颜色。用简单粒子池渲染（可参考 bonus2 实例化）。

### 计分与排行榜
- 限时统计打掉个数（标准模式）/ 命中数+反应时间（狂野模式）
- **排行榜**：本局分数 + 历史最高，存 `media/config/scores.json`（自写极简文本/JSON 读写），主菜单显示 Top 10

---

## 7. 控制键位

**全局**：`Tab` 相机切换 · `Esc` 暂停/主菜单 · `F2` 截图 · `F1` ImGui 面板显隐

**FPS 射击**：鼠标转视角 · `WASD` 移动 · `Space/Ctrl` 上下 · `左键` 射击 · `右键`（狙击）开镜 · `1/2` 武器切换 · `R` 换弹/重置冷却

**Orbit 编辑**：右键拖旋转 · 滚轮 zoom · 中键拖 pan · `F` zoomToFit

**ImGui 面板**：材质编辑 / 光源编辑 / NURBS 控制点 / 模式选择 / **限时·小球速度·小球大小三滑块** / 开场动画控制

---

## 8. 开发阶段（对齐助教"先 OBJLoader+管线，再叠材质/光照/动画"）

| 阶段 | 内容 | 对应要求 |
|---|---|---|
| 0. 工程骨架 | source/base/project/media/external 结构 + CMakeLists + 拷库 + 渲染三角形 | 工程结构 |
| 1. OBJLoader + Model | 自研 OBJ 解析+导出，替换 base/model.cpp | 必做 2 |
| 2. 体素生成 + 基础渲染 | primitives.cpp 生成 6 体素，Blinn-Phong 着色 | 必做 1,3 |
| 3. 多光源 + 材质/纹理编辑 | 4 种光源，ImGui 面板 | 必做 3,5 |
| 4. 双相机 + 漫游 | FPS + Orbit + Tab + zoomToFit | 必做 4,6 |
| 5. 实时阴影 | shadow mapping（参考 bonus4） | 额外:光照 |
| 6. NURBS | 自研曲线，轨迹靶模式 | 额外:NURBS |
| 7. 漫游碰撞 | 球-AABB（玩家vs箱子）+ 球-球，接入 FPS 移动 | 额外:碰撞 |
| 8. 游戏玩法 | 武器系统、气球爆炸特效、轨迹靶、计分排行榜、音效、可调参数 | 额外:游戏 |
| 9. 动画 + 截图 | OBJ 序列开场动画 + glReadPixels 截图 | 必做 7 |
| 10. 打磨 | 粒子优化、HUD、配置存档、文档 | — |

---

## 9. 复用现有作业代码

设计建立在对现有 project 代码的调研之上，最大化复用：

- **base 类**：`Application`/`Camera`/`Model`/`GLSLProgram`/`Framebuffer`/`Texture2D`/`Skybox`/`Transform`/`Input`/`Light`/`BoundingBox`/`Frustum`/`FullscreenQuad`/`UniformBuffer`
- **project1**：参数化几何生成模式（`Star` 类）→ primitives.cpp
- **project3**：FPS 相机（`SceneRoaming`：鼠标锁定、WASD、四元数视角）→ fps_camera
- **project5**：多光源 + Blinn-Phong + ImGui 编辑面板 → 材质/光源系统
- **project6**：纹理加载/绑定、天空盒 → 纹理/武器贴图/天空盒
- **bonus4**：shadow mapping FBO 深度 Pass、`Framebuffer`/`Texture2D` → 实时阴影
- **pbr_viewer**：`CameraController` orbit/pan/zoom → orbit_camera；UBO 多光源模式（可选）
- **base/model.cpp:19-90**：tinyobjloader 解析逻辑 → 参考结构写自研 OBJLoader，保持 `Vertex`/索引/VAO 布局契约

---

## 10. 风险与保底

- **自研 OBJLoader**：分水岭，但 base/model.cpp 现成解析逻辑可参考结构，风险可控。
- **音效（miniaudio）**：跨平台需测试，Windows 上无问题。
- **工作量**：阶段多但分层清晰，每阶段独立可验证。
- **保底**：完成阶段 0-6 + 简化版 7/8/9 即覆盖全部基本要求 + NURBS + 阴影加分。完整 7/8（碰撞 + 游戏）提升加分效果。
