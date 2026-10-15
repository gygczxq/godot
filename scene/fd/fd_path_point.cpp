/**************************************************************************/
/*  fd_path_point.cpp                                                    */
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

#include "fd_path_point.h"

#include "core/object/class_db.h"

void FdPathPoint::set_position(const Vector3 &p_position) {
	position = p_position;
	emit_changed();
}

Vector3 FdPathPoint::get_position() const {
	return position;
}

void FdPathPoint::set_rotation_deg(const Vector3 &p_rotation_deg) {
	rotation_deg = p_rotation_deg;
	emit_changed();
}

Vector3 FdPathPoint::get_rotation_deg() const {
	return rotation_deg;
}

void FdPathPoint::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_position", "position"), &FdPathPoint::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &FdPathPoint::get_position);
	ClassDB::bind_method(D_METHOD("set_rotation_deg", "rotation"), &FdPathPoint::set_rotation_deg);
	ClassDB::bind_method(D_METHOD("get_rotation_deg"), &FdPathPoint::get_rotation_deg);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "rotation"), "set_rotation_deg", "get_rotation_deg");
}

FdPathPoint::FdPathPoint() {
}
