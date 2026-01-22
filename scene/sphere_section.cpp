#include "scene/sphere_section.hpp"

#include "geometry/geometry.hpp"

#include <cmath>

namespace cg
{

SphereSection::SphereSection() {}

SphereSection::SphereSection(float    min_lat,
                             float    max_lat,
                             uint32_t num_lat,
                             float    min_lon,
                             float    max_lon,
                             uint32_t num_lon,
                             float    radius,
                             int32_t  position_loc,
                             int32_t  normal_loc)
{
    // Convert to radians
    float min_lat_rad = degrees_to_radians(min_lat);
    float max_lat_rad = degrees_to_radians(max_lat);
    float min_lon_rad = degrees_to_radians(min_lon);
    float max_lon_rad = degrees_to_radians(max_lon);

    // Create a vertex list with unit length normals
    float           cos_lat, cos_lon, sin_lon;
    float           d_lat = (max_lat_rad - min_lat_rad) / static_cast<float>(num_lat);
    float           d_lon = (max_lon_rad - min_lon_rad) / static_cast<float>(num_lon);
    VertexAndNormal vtx;
    for(float curr_lon = min_lon_rad; curr_lon < max_lon_rad + EPSILON; curr_lon += d_lon)
    {
        cos_lon = std::cos(curr_lon);
        sin_lon = std::sin(curr_lon);
        for(float curr_lat = max_lat_rad; curr_lat >= min_lat_rad - EPSILON; curr_lat -= d_lat)
        {
            cos_lat = std::cos(curr_lat);
            vtx.normal.x = cos_lon * cos_lat;
            vtx.normal.y = sin_lon * cos_lat;
            vtx.normal.z = std::sin(curr_lat);
            vtx.vertex.x = radius * vtx.normal.x;
            vtx.vertex.y = radius * vtx.normal.y;
            vtx.vertex.z = radius * vtx.normal.z;
            vertices_.push_back(vtx);
        }
    }

    // Copy the first column of vertices
    for(uint32_t i = 0; i <= num_lat + 1; i++) { vertices_.push_back(vertices_[i]); }

    // Construct face list.  There are num_lat+1 rows and num_lon+1 columns. Create VBOs
    construct_row_col_face_list(num_lon + 1, num_lat + 1);
    create_vertex_buffers(position_loc, normal_loc);
}

TexturedSphereSection::TexturedSphereSection() {}

TexturedSphereSection::TexturedSphereSection(float    min_lat,
                                             float    max_lat,
                                             uint32_t num_lat,
                                             float    min_lon,
                                             float    max_lon,
                                             uint32_t num_lon,
                                             float    radius,
                                             int32_t  position_loc,
                                             int32_t  normal_loc,
                                             int32_t  texture_loc)
{
    // Convert to radians
    float min_lat_rad = degrees_to_radians(min_lat);
    float max_lat_rad = degrees_to_radians(max_lat);
    float min_lon_rad = degrees_to_radians(min_lon);
    float max_lon_rad = degrees_to_radians(max_lon);

    // Create a vertex list with unit length normals
    float     cos_lat, cos_lon, sin_lon;
    float     d_lat = (max_lat_rad - min_lat_rad) / static_cast<float>(num_lat);
    float     d_lon = (max_lon_rad - min_lon_rad) / static_cast<float>(num_lon);
    float     d_s = 1.0f / static_cast<float>(num_lon);
    float     d_t = 1.0f / static_cast<float>(num_lat);
    PNTVertex vtx;
    for(float curr_lon = min_lon_rad, s = 0.0f; curr_lon < max_lon_rad + EPSILON;
        curr_lon += d_lon, s += d_s)
    {
        cos_lon = std::cos(curr_lon);
        sin_lon = std::sin(curr_lon);
        for(float curr_lat = max_lat_rad, t = 1.0f; curr_lat >= min_lat_rad - EPSILON;
            curr_lat -= d_lat, t -= d_t)
        {
            cos_lat = std::cos(curr_lat);
            vtx.normal.x = cos_lon * cos_lat;
            vtx.normal.y = sin_lon * cos_lat;
            vtx.normal.z = std::sin(curr_lat);
            vtx.vertex.x = radius * vtx.normal.x;
            vtx.vertex.y = radius * vtx.normal.y;
            vtx.vertex.z = radius * vtx.normal.z;
            vtx.texture.x = s;
            vtx.texture.y = t;
            vertices_.push_back(vtx);
        }
    }

    // Construct face list and create VBOs.  There are num_lat+1 rows and
    // num_lon+1 columns.
    construct_row_col_face_list(num_lon + 1, num_lat + 1);
    create_vertex_buffers(position_loc, normal_loc, texture_loc);
}

} // namespace cg
