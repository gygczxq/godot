/**************************************************************************/
/*  fd_path_3d.cpp                                                       */
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

#include "fd_path_3d.h"

#include "fd_path_point.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/multimesh.h"

static Ref<StandardMaterial3D> _make_unshaded_mat(const Color &p_color) {
	Ref<StandardMaterial3D> m = memnew(StandardMaterial3D);
	m->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	m->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	m->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	m->set_albedo(p_color);
	return m;
}

FdPath3D::FdPath3D() {
	_update_materials();
}

FdPath3D::~FdPath3D() {
}

void FdPath3D::_ensure_mesh_instance() {
	if (mesh_instance != nullptr) {
		return;
	}
	mesh_instance = memnew(MeshInstance3D);
	mesh_instance->set_name(StringName("_fd_path_mesh"));
	add_child(mesh_instance, false, INTERNAL_MODE_BACK);
}

void FdPath3D::_ensure_multimesh_instance() {
	if (multimesh_instance != nullptr) {
		return;
	}
	multimesh_instance = memnew(MultiMeshInstance3D);
	multimesh_instance->set_name(StringName("_fd_path_spheres"));
	add_child(multimesh_instance, false, INTERNAL_MODE_BACK);
}

void FdPath3D::_update_materials() {
	route_material = _make_unshaded_mat(color);
	sphere_material = _make_unshaded_mat(color);
	axis_x_material = _make_unshaded_mat(Color(0.95, 0.32, 0.32));
	axis_y_material = _make_unshaded_mat(Color(0.32, 0.95, 0.32));
	axis_z_material = _make_unshaded_mat(Color(0.32, 0.55, 0.95));
}

Transform3D FdPath3D::_get_point_pose(int p_idx) const {
	Transform3D t;
	FdPathPoint *point = Object::cast_to<FdPathPoint>(points[p_idx]);
	if (point == nullptr) {
		return t;
	}
	t.origin = point->get_position();
	const Vector3 r = point->get_rotation_deg();
	t.basis = Basis::from_euler(Vector3(
			Math::deg_to_rad(r.x),
			Math::deg_to_rad(r.y),
			Math::deg_to_rad(r.z)),
			EulerOrder::YXZ);
	return t;
}

void FdPath3D::_update_debug_mesh() {
	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
	debug_mesh->clear_surfaces();

	const int point_count = points.size();
	_update_materials();

	int surf = 0;

	// 1) 连接各点位置的折线（参考 Path3D）。
	if (show_route_line && point_count >= 2) {
		Vector<Vector3> route;
		route.resize(point_count);
		Vector3 *rw = route.ptrw();
		for (int i = 0; i < point_count; i++) {
			rw[i] = _get_point_pose(i).origin;
		}
		Array a;
		a.resize(Mesh::ARRAY_MAX);
		a[Mesh::ARRAY_VERTEX] = route;
		debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINE_STRIP, a);
		debug_mesh->surface_set_material(surf, route_material);
		surf++;
	}

	// 2) 每个点的填充球体（MultiMesh 实例化 SphereMesh）。
	{
		_ensure_multimesh_instance();
		if (multimesh.is_null()) {
			multimesh.instantiate();
			multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
		}
		Ref<SphereMesh> sphere = multimesh->get_mesh();
		if (sphere.is_null()) {
			sphere.instantiate();
			sphere->set_radial_segments(16);
			sphere->set_rings(12);
			multimesh->set_mesh(sphere);
			multimesh_instance->set_multimesh(multimesh);
		}
		sphere->set_radius(point_radius);
		sphere->set_height(point_radius * 2.0);
		sphere->set_material(sphere_material);
		multimesh->set_instance_count(point_count);
		for (int i = 0; i < point_count; i++) {
			multimesh->set_instance_transform(i, _get_point_pose(i));
		}
	}

	// 3) 三色坐标轴（X 红 / Y 绿 / Z 蓝）。
	if (show_axes && point_count > 0) {
		Vector<Vector3> xv, yv, zv;
		xv.resize(point_count * 2);
		yv.resize(point_count * 2);
		zv.resize(point_count * 2);
		Vector3 *xw = xv.ptrw(), *yw = yv.ptrw(), *zw = zv.ptrw();
		for (int i = 0; i < point_count; i++) {
			const Transform3D pose = _get_point_pose(i);
			const Vector3 c = pose.origin;
			const Vector3 ax = pose.basis.get_column(0);
			const Vector3 ay = pose.basis.get_column(1);
			const Vector3 az = pose.basis.get_column(2);
			xw[i * 2 + 0] = c;
			xw[i * 2 + 1] = c + ax * point_radius;
			yw[i * 2 + 0] = c;
			yw[i * 2 + 1] = c + ay * point_radius;
			zw[i * 2 + 0] = c;
			zw[i * 2 + 1] = c + az * point_radius;
		}
		Array axa, aya, aza;
		axa.resize(Mesh::ARRAY_MAX); aya.resize(Mesh::ARRAY_MAX); aza.resize(Mesh::ARRAY_MAX);
		axa[Mesh::ARRAY_VERTEX] = xv;
		aya[Mesh::ARRAY_VERTEX] = yv;
		aza[Mesh::ARRAY_VERTEX] = zv;
		debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, axa);
		debug_mesh->surface_set_material(surf, axis_x_material); surf++;
		debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, aya);
		debug_mesh->surface_set_material(surf, axis_y_material); surf++;
		debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, aza);
		debug_mesh->surface_set_material(surf, axis_z_material); surf++;
	}

	_ensure_mesh_instance();
	mesh_instance->set_mesh(debug_mesh);
}

void FdPath3D::set_points(const TypedArray<FdPathPoint> &p_points) {
	points = p_points;
	for (int i = 0; i < points.size(); i++) {
		FdPathPoint *p = Object::cast_to<FdPathPoint>(points[i]);
		if (p) {
			p->connect(SNAME("changed"), callable_mp(this, &FdPath3D::_update_debug_mesh));
		}
	}
	_update_debug_mesh();
}

TypedArray<FdPathPoint> FdPath3D::get_points() const {
	return points;
}

int FdPath3D::get_point_count() const {
	return points.size();
}

FdPathPoint *FdPath3D::add_point(const Vector3 &p_position, const Vector3 &p_rotation_deg) {
	Ref<FdPathPoint> point = memnew(FdPathPoint);
	point->set_position(p_position);
	point->set_rotation_deg(p_rotation_deg);
	point->connect(SNAME("changed"), callable_mp(this, &FdPath3D::_update_debug_mesh));
	points.push_back(point);
	_update_debug_mesh();
	return point.ptr();
}

void FdPath3D::remove_point(int p_idx) {
	ERR_FAIL_INDEX(p_idx, points.size());
	points.remove_at(p_idx);
	_update_debug_mesh();
}

void FdPath3D::clear_points() {
	points.clear();
	_update_debug_mesh();
}

void FdPath3D::set_point_radius(real_t p_radius) {
	point_radius = p_radius;
	_update_debug_mesh();
}

real_t FdPath3D::get_point_radius() const {
	return point_radius;
}

void FdPath3D::set_show_axes(bool p_show) {
	show_axes = p_show;
	_update_debug_mesh();
}

bool FdPath3D::is_showing_axes() const {
	return show_axes;
}

void FdPath3D::set_show_route_line(bool p_show) {
	show_route_line = p_show;
	_update_debug_mesh();
}

bool FdPath3D::is_showing_route_line() const {
	return show_route_line;
}

void FdPath3D::set_color(const Color &p_color) {
	color = p_color;
	_update_debug_mesh();
}

Color FdPath3D::get_color() const {
	return color;
}

void FdPath3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_points", "points"), &FdPath3D::set_points);
	ClassDB::bind_method(D_METHOD("get_points"), &FdPath3D::get_points);

	ClassDB::bind_method(D_METHOD("get_point_count"), &FdPath3D::get_point_count);
	ClassDB::bind_method(D_METHOD("add_point", "position", "rotation"), &FdPath3D::add_point, DEFVAL(Vector3()));
	ClassDB::bind_method(D_METHOD("remove_point", "idx"), &FdPath3D::remove_point);
	ClassDB::bind_method(D_METHOD("clear_points"), &FdPath3D::clear_points);

	ClassDB::bind_method(D_METHOD("set_point_radius", "radius"), &FdPath3D::set_point_radius);
	ClassDB::bind_method(D_METHOD("get_point_radius"), &FdPath3D::get_point_radius);
	ClassDB::bind_method(D_METHOD("set_show_axes", "show"), &FdPath3D::set_show_axes);
	ClassDB::bind_method(D_METHOD("is_showing_axes"), &FdPath3D::is_showing_axes);
	ClassDB::bind_method(D_METHOD("set_show_route_line", "show"), &FdPath3D::set_show_route_line);
	ClassDB::bind_method(D_METHOD("is_showing_route_line"), &FdPath3D::is_showing_route_line);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &FdPath3D::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &FdPath3D::get_color);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "points", PROPERTY_HINT_ARRAY_TYPE, FdPathPoint::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_points", "get_points");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "point_radius", PROPERTY_HINT_RANGE, "0.01,10,0.01,suffix:m"), "set_point_radius", "get_point_radius");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_axes"), "set_show_axes", "is_showing_axes");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_route_line"), "set_show_route_line", "is_showing_route_line");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}
