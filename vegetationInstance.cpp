/*************************************************************************/
/*  vegetationInstance.cpp                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2016 Juan Linietsky, Ariel Manzur.                 */
/* Portions: Copyright (c) 2016 Alex Piola                               */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

/// TODO 
/*
   - surface normal rotation as option
   - slope amount

   - generate static body and add collision shapes for each instance
*/
///

#include "vegetationInstance.h"

#include "scene/3d/mesh_instance.h"

void VegetationInstance::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_instance_count", "count"), &VegetationInstance::set_instance_count);
	ObjectTypeDB::bind_method(_MD("get_instance_count"), &VegetationInstance::get_instance_count);
	ObjectTypeDB::bind_method(_MD("set_populate_axis", "axis_num"), &VegetationInstance::set_populate_axis);
	ObjectTypeDB::bind_method(_MD("get_populate_axis"), &VegetationInstance::get_populate_axis);
	ObjectTypeDB::bind_method(_MD("set_tilt_random", "tilt_rand"), &VegetationInstance::set_tilt_random);
	ObjectTypeDB::bind_method(_MD("get_tilt_random"), &VegetationInstance::get_tilt_random);
	ObjectTypeDB::bind_method(_MD("set_rotate_random", "rotate_rand"), &VegetationInstance::set_rotate_random);
	ObjectTypeDB::bind_method(_MD("get_rotate_random"), &VegetationInstance::get_rotate_random);
	ObjectTypeDB::bind_method(_MD("set_scale_random", "scale_rand"), &VegetationInstance::set_scale_random);
	ObjectTypeDB::bind_method(_MD("get_scale_random"), &VegetationInstance::get_scale_random);
	ObjectTypeDB::bind_method(_MD("set_scale_amount", "scale_amt"), &VegetationInstance::set_scale_amount);
	ObjectTypeDB::bind_method(_MD("get_scale_amount"), &VegetationInstance::get_scale_amount);
	ObjectTypeDB::bind_method(_MD("populate", "surface_source"), &VegetationInstance::populate);
	ObjectTypeDB::bind_method(_MD("set_custom_normal", "normal"), &VegetationInstance::set_custom_normal);
	ObjectTypeDB::bind_method(_MD("get_custom_normal"), &VegetationInstance::get_custom_normal);
	ObjectTypeDB::bind_method(_MD("set_custom_offset", "offset"), &VegetationInstance::set_custom_offset);
	ObjectTypeDB::bind_method(_MD("get_custom_offset"), &VegetationInstance::get_custom_offset);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_count"), _SCS("set_instance_count"), _SCS("get_instance_count"));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "populate_axis", PROPERTY_HINT_ENUM, "X-Axis,Y-Axis,Z-Axis"), _SCS("set_populate_axis"), _SCS("get_populate_axis"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "tilt_random"), _SCS("set_tilt_random"), _SCS("get_tilt_random"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "rotate_random"), _SCS("set_rotate_random"), _SCS("get_rotate_random"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "scale_random"), _SCS("set_scale_random"), _SCS("get_scale_random"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "scale_amount"), _SCS("set_scale_amount"), _SCS("get_scale_amount"));
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "normal"), _SCS("set_custom_normal"), _SCS("get_custom_normal"));
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset"), _SCS("set_custom_offset"), _SCS("get_custom_offset"));
	

}



void VegetationInstance::populate(const NodePath& surface_source) {

	Ref<Mesh> mesh;
	Ref<MultiMesh> old_multimesh;
	old_multimesh = get_multimesh();
	if (old_multimesh.is_null())
	{
		ERR_PRINT("MultiMeshInstance::_populate: no multimesh on object");
		return;
	}
	mesh = old_multimesh->get_mesh();

	if (surface_source == NodePath("")) {
		ERR_PRINT("MultiMeshInstance::_populate: surface_source is empty");
		return;
	}

	Node *ss_node = get_node(surface_source);

	if (!ss_node) {
		ERR_PRINT("MultiMeshInstance::_populate: no node found for surface_source");
		return;
	}

	GeometryInstance *ss_instance = ss_node->cast_to<MeshInstance>();

	if (!ss_instance) {
		ERR_PRINT("MultiMeshInstance::_populate: surface_source is not a geometryInstance");
		return;
	}

	Transform geom_xform = get_global_transform().affine_inverse() * ss_instance->get_global_transform();

	DVector<Face3> geometry = ss_instance->get_faces(VisualInstance::FACES_SOLID);

	if (geometry.size() == 0) {
		return;
	}

	//make all faces local

	int gc = geometry.size();
	DVector<Face3>::Write w = geometry.write();

	for (int i = 0; i<gc; i++) {
		for (int j = 0; j<3; j++) {
			w[i].vertex[j] = geom_xform.xform(w[i].vertex[j]);
		}
	}



	w = DVector<Face3>::Write();

	DVector<Face3> faces = geometry;
	ERR_EXPLAIN(TTR("Parent has no solid faces to populate."));
	int facecount = faces.size();
	ERR_FAIL_COND(!facecount);

	DVector<Face3>::Read r = faces.read();



	float area_accum = 0;
	Map<float, int> triangle_area_map;
	for (int i = 0; i<facecount; i++) {

		float area = r[i].get_area();;
		if (area<CMP_EPSILON)
			continue;
		triangle_area_map[area_accum] = i;
		area_accum += area;
	}

	ERR_EXPLAIN(TTR("Couldn't map area."));
	ERR_FAIL_COND(triangle_area_map.size() == 0);
	ERR_EXPLAIN(TTR("Couldn't map area."));
	ERR_FAIL_COND(area_accum == 0);


	Ref<MultiMesh> multimesh = memnew(MultiMesh);
	multimesh->set_mesh(mesh);



	multimesh->set_instance_count(instance_count);

	Transform axis_xform;
	if (populate_axis == Vector3::AXIS_Z) {
		axis_xform.rotate(Vector3(1, 0, 0), Math_PI*0.5);
	}
	if (populate_axis == Vector3::AXIS_X) {
		axis_xform.rotate(Vector3(0, 0, 1), Math_PI*0.5);
	}

	for (int i = 0; i<instance_count; i++) {

		float areapos = Math::random(0, area_accum);

		Map<float, int>::Element *E = triangle_area_map.find_closest(areapos);
		ERR_FAIL_COND(!E)
			int index = E->get();
		ERR_FAIL_INDEX(index, facecount);

		// ok FINALLY get face
		Face3 face = r[index];
		//now compute some position inside the face...

		Vector3 pos = face.get_random_point_inside();
		Vector3 normal = face.get_plane().normal;

		Vector3 op_axis = (face.vertex[0] - face.vertex[1]).normalized();

		Transform xform;

		if (Math::abs(custom_normal.length()) > 0.00f)
		{
			normal = custom_normal;
		}


		xform.set_look_at(pos, pos + op_axis, normal);
		xform = xform * axis_xform;
		xform.translate(custom_offset);

		Matrix3 post_xform;
		
		post_xform.rotate(xform.basis.get_axis(0), Math::random(-tilt_random, tilt_random)*Math_PI);
		post_xform.rotate(xform.basis.get_axis(2), Math::random(-tilt_random, tilt_random)*Math_PI);
		post_xform.rotate(xform.basis.get_axis(1), Math::random(-rotate_random, rotate_random)*Math_PI);
		xform.basis = post_xform * xform.basis;
		//xform.basis.orthonormalize();
		


		xform.basis.scale(Vector3(1, 1, 1)*(scale_amount + Math::random(-scale_random, scale_random)));

		multimesh->set_instance_transform(i, xform);
		multimesh->set_instance_color(i, Color(1, 1, 1, 1));
	}

	multimesh->generate_aabb();

	set_multimesh(multimesh);

}

void VegetationInstance::set_instance_count(const uint16_t& p_instance_count) {
	instance_count = p_instance_count;
}

uint16_t VegetationInstance::get_instance_count() const {
	return instance_count;
}

void VegetationInstance::set_populate_axis(const uint8_t& p_populate_axis) {
	populate_axis = p_populate_axis;
}

uint8_t VegetationInstance::get_populate_axis() const {
	return populate_axis;
}

void VegetationInstance::set_tilt_random(const float& p_tilt_random) {
	tilt_random = p_tilt_random;
}

float VegetationInstance::get_tilt_random() const {
	return tilt_random;
}

void VegetationInstance::set_rotate_random(const float& p_rotate_random) {
	rotate_random = p_rotate_random;
}

float VegetationInstance::get_rotate_random() const {
	return rotate_random;
}

void VegetationInstance::set_scale_random(const float& p_scale_random) {
	scale_random = p_scale_random;
}

float VegetationInstance::get_scale_random() const {
	return scale_random;
}

void VegetationInstance::set_scale_amount(const float& p_scale_amount) {
	scale_amount = p_scale_amount;
}

float VegetationInstance::get_scale_amount() const {
	return scale_amount;
}

void VegetationInstance::set_custom_normal(const Vector3& p_custom_normal) {
	custom_normal = p_custom_normal;
}

Vector3 VegetationInstance::get_custom_normal() const {
	return custom_normal;
}

void VegetationInstance::set_custom_offset(const Vector3& p_custom_offset) {
	custom_offset = p_custom_offset;
}

Vector3 VegetationInstance::get_custom_offset() const {
	return custom_offset;
}

VegetationInstance::VegetationInstance() {
	instance_count = 32;
	tilt_random = 0.0f;
	rotate_random = 0.0f;
	scale_random = 0.0f;
	scale_amount = 1.0f;
	populate_axis = Vector3::AXIS_Y;
	custom_normal = Vector3(0.f,0.f,0.f);
	custom_offset = Vector3(0.f, 0.f, 0.f);
}

VegetationInstance::~VegetationInstance() {


}

