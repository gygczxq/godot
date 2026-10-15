# fdsim 开发日志

> 本日志记录在 Godot 4.8 源码中为 fdsim 项目所做的每次代码/配置修改。

## 2026-10-15 新增自定义节点：FdComponent / FdNode3D

### 目标
在引擎源码内内置两个 fdsim 自定义节点，作为后续仿真功能的基础：
- **FdComponent**（仿真组件）：继承 `Node3D`，自带 3D 变换。
- **FdNode3D**（场景节点/仿真实体根）：继承 `Node`，下面挂载若干 FdComponent 组件。

### 新增文件
| 文件 | 说明 |
| --- | --- |
| `scene/fd/fd_component.h` / `.cpp` | `class FdComponent : public Node3D`，`GDCLASS(FdComponent, Node3D)`；属性 `enabled`（默认 true）。 |
| `scene/fd/fd_node_3d.h` / `.cpp` | `class FdNode3D : public Node`，`GDCLASS(FdNode3D, Node)`；方法 `get_components()` 返回直接子节点中所有 `FdComponent`。 |
| `scene/fd/SCsub` | 收集本目录 `*.cpp` 到 `env.scene_sources`。 |
| `doc/classes/FdComponent.xml` | FdComponent 类文档。 |
| `doc/classes/FdNode3D.xml` | FdNode3D 类文档。 |

### 修改文件
- `scene/SCsub`：在 3D 子目录链入后新增 `SConscript("fd/SCsub")`。
- `scene/register_scene_types.cpp`：
  - 顶部新增 `#include "scene/fd/fd_component.h"`、`#include "scene/fd/fd_node_3d.h"`。
  - `GDREGISTER_CLASS(Node)` 之后注册 `GDREGISTER_CLASS(FdNode3D)`。
  - `GDREGISTER_CLASS(Node3D)` 之后注册 `GDREGISTER_CLASS(FdComponent)`。

### 设计说明
- 类名遵循 Godot PascalCase 约定：`fdComponent` → `FdComponent`，`fdnode3d` → `FdNode3D`（编辑器创建节点对话框中以此类名出现）。
- FdComponent 继承 Node3D 以获得世界/局部变换；FdNode3D 继承 Node 作为实体容器，与用户要求一致。
- 两个节点均为可实例化的具体类（`GDREGISTER_CLASS`，非 abstract）。

### 编译验证
- 命令：加载 `vcvars64.bat` 后 `python -m SCons platform=windows dev_build=yes accesskit=no angle=no -j8`。
- 结果：`scene/fd/fd_component.cpp`、`scene/fd/fd_node_3d.cpp`、`scene/register_scene_types.cpp` 全部编译通过，成功链接 `bin/godot.windows.editor.dev.x86_64.exe`（exit 0）。
- 注意：本沙箱默认带 `PYTHONNOUSERSITE=1`，会导致 `No module named SCons`；需用 `set "PYTHONNOUSERSITE="` 显式清空（注意 cmd 的 `set VAR= &&` 会把尾随空格赋入，必须加引号）。

### 待办 / 下一步
- 后续按需为 FdComponent 派生具体仿真组件（运动学、动力学、传感器等）。

## 2026-10-15 新增 FdPath3D（拟合路线节点）

### 目标
从 FdComponent 派生 **FdPath3D**：拟合出来的路线，由一系列位姿点组成（每个点含位置+姿态，即 Transform3D）。点的显示可在 Marker3D 式十字与箭头之间切换，功能参考 Path3D。

### 新增文件
| 文件 | 说明 |
| --- | --- |
| `scene/fd/fd_path_3d.h` / `.cpp` | `class FdPath3D : public FdComponent`。内部 `Vector<Transform3D> poses`。 |
| `doc/classes/FdPath3D.xml` | 类文档。 |

### 修改文件
- `scene/register_scene_types.cpp`：新增 `#include "scene/fd/fd_path_3d.h"`，在 `GDREGISTER_CLASS(FdComponent)` 后注册 `GDREGISTER_CLASS(FdPath3D)`。
- `scene/fd/SCsub` 无需改动（已用 `*.cpp` 通配，新文件自动纳入）。

### 设计要点
- **数据**：`poses`（`Array[Transform3D]` 可序列化属性），origin=位置、basis=姿态；提供 `add_point/set_point_pose/get_point_pose/remove_point/clear_points/get_point_count`。
- **绘制**：参考 Path3D，用 `RenderingServer::instance_create()` + `ArrayMesh`（线段）直接绘制，无需子节点：
  - 折线连接相邻点位置（`PRIMITIVE_LINE_STRIP`，可关）。
  - `DISPLAY_MARKER`：每点画沿姿态三轴的十字线（类似 Marker3D）。
  - `DISPLAY_ARROW`：每点画指向姿态前向(-Z)的箭头（箭杆+两翼箭头）。
  - 材质：Unshaded + Alpha 透明 + 关闭雾；颜色可配。
  - 监听 `NOTIFICATION_TRANSFORM_CHANGED / VISIBILITY_CHANGED` 同步实例变换与可见性。
- **可切换属性**：`display_mode`(Marker/Arrow)、`marker_size`、`arrow_length`、`show_route_line`、`color`。

### 编译验证
- 同前（vcvars64 + `set "PYTHONNOUSERSITE="` + `python -m SCons ... -j8`）。
- 结果：`fd_path_3d.cpp`、`register_scene_types.cpp` 编译通过，成功链接 editor exe（仅有与本次无关的 vc140.pdb LNK4099 警告）。

### 2026-10-15 修正 FdPath3D 显示与数据模型
- **显示方式**：原 RenderingServer 裸实例在编辑器视口不显示，改为内部子节点 `MeshInstance3D`（`INTERNAL_MODE_BACK`，不污染场景树），编辑器/运行时均稳定可见并自动跟随本节点变换。
- **数据模型**：由单个 `Array[Transform3D]` 改为两个并列 `PackedVector3Array`：`positions`（位置）与 `rotations`（欧拉角，度，YXZ）。避免编辑器里 3×3 basis 矩阵看错（用户曾误改 basis `xx` 当作位置）。
- **绘制分工**：点参考 Marker3D（三轴十字），路线参考 Path3D（折线 LINE_STRIP）；保留 Arrow 箭头显示可切换。
- 方法：`add_point(position, rotation)`、`set_point_position/rotation`、`get_point_position/rotation`、`remove_point`、`clear_points`。
- 编译通过。

### 2026-10-15 FdPath3D 数据结构改为点资源数组
- 新增 `FdPathPoint`（Resource）：每点一行，含 `position`(Vector3) 与 `rotation`(Vector3 欧拉角度)。
- FdPath3D 持有 `TypedArray<FdPathPoint> points`（`PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT`），Inspector 里一行展开即 position+rotation，不再拆成两个数组。
- 点绘制：小圆（局部 XZ 平面 16 段环）+ 三轴；半径由 FdPath3D 的 `point_radius` 控制；三轴显隐由 FdPath3D 的 `show_axes` 控制。
- 路线折线参考 Path3D；`display_mode` 仍可切 Marker/Arrow。
- 编译通过。

### 2026-10-15 FdPath3D 点改为填充球 + 三色轴
- 去掉 Marker/Arrow 绘制模式，固定为：点 = 填充球体 + 点间连线。
- 球体用 MultiMesh 实例化 SphereMesh（`MultiMeshInstance3D` 内部节点），半径由 FdPath3D 的 `point_radius` 控制。
- 三轴分色：X 红 / Y 绿 / Z 蓝（独立材质面），显隐由 `show_axes` 控制。
- 连线颜色用 `color`；保留 `show_route_line`。
- 编译通过。

