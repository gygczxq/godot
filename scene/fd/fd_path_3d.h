/**************************************************************************/
/*  fd_path_3d.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/fd/fd_component.h"

class ArrayMesh;
class FdPathPoint;
class MeshInstance3D;
class MultiMesh;
class MultiMeshInstance3D;
class Material;

// FdPath3D: fdsim 拟合路线节点（继承 FdComponent）。
// 参考 Path3D，只是每个点多了 rotation。路线由一系列 FdPathPoint 组成，
// 每个点一行，含 position（位置）与 rotation（欧拉角，度）。
// 每个点画一个线框球体 + 三色坐标轴（X 红 / Y 绿 / Z 蓝），
// 相邻点之间用折线连接（参考 Path3D）。
class FdPath3D : public FdComponent {
	GDCLASS(FdPath3D, FdComponent);

private:
	TypedArray<FdPathPoint> points;

	real_t point_radius = 0.15; // 每个点的球体半径，同时作为坐标轴长度（米）。
	bool show_axes = true; // 是否显示每个点的三个坐标轴。
	bool show_route_line = true; // 是否绘制连接各点的折线（参考 Path3D）。
	Color color = Color(0.4, 0.9, 1.0);

	Ref<ArrayMesh> debug_mesh;
	Ref<Material> route_material;
	Ref<Material> sphere_material;
	Ref<Material> axis_x_material;
	Ref<Material> axis_y_material;
	Ref<Material> axis_z_material;
	MeshInstance3D *mesh_instance = nullptr;

	Ref<MultiMesh> multimesh;
	MultiMeshInstance3D *multimesh_instance = nullptr;

	void _update_debug_mesh();
	void _update_materials();
	void _ensure_mesh_instance();
	void _ensure_multimesh_instance();
	Transform3D _get_point_pose(int p_idx) const;

protected:
	static void _bind_methods();

public:
	void set_points(const TypedArray<FdPathPoint> &p_points);
	TypedArray<FdPathPoint> get_points() const;

	int get_point_count() const;
	FdPathPoint *add_point(const Vector3 &p_position, const Vector3 &p_rotation_deg = Vector3());
	void remove_point(int p_idx);
	void clear_points();

	void set_point_radius(real_t p_radius);
	real_t get_point_radius() const;

	void set_show_axes(bool p_show);
	bool is_showing_axes() const;

	void set_show_route_line(bool p_show);
	bool is_showing_route_line() const;

	void set_color(const Color &p_color);
	Color get_color() const;

	FdPath3D();
	~FdPath3D();
};
