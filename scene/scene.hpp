//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    scene.hpp
//	Purpose: Scene graph support.
//
//============================================================================

#ifndef __SCENE_SCENE_HPP__
#define __SCENE_SCENE_HPP__

// Include other scene files
// clang-format off
#include "scene/color3.hpp"
#include "scene/color4.hpp"
#include "scene/scene_state.hpp"
#include "scene/scene_node.hpp"
#include "scene/transform_node.hpp"
#include "scene/presentation_node.hpp"
#include "scene/color_node.hpp"
#include "scene/light_node.hpp"
#include "scene/geometry_node.hpp"
#include "scene/shader_node.hpp"
#include "scene/camera_node.hpp"
#include "scene/image_data.hpp"
#include "scene/model_node.hpp"
#include "scene/view_frustum.hpp"
// clang-format on

// Model nodes
#include "scene/conic.hpp"
#include "scene/mesh_teapot.hpp"
#include "scene/sphere_section.hpp"
#include "scene/surface_of_revolution.hpp"
#include "scene/torus.hpp"
#include "scene/unit_square.hpp"

// Bounding nodes
#include "scene/bounding_aabb_node.hpp"
#include "scene/bounding_sphere_node.hpp"

namespace cg
{

void check_error(const char *str);

}

#endif
