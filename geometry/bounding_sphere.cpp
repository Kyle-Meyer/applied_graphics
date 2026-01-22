#include "geometry/bounding_sphere.hpp"

#include "geometry/geometry.hpp"

namespace cg
{

BoundingSphere::BoundingSphere() : center{0.0f, 0.0f, 0.0f}, radius(1.0f) {}

BoundingSphere::BoundingSphere(const BoundingSphere &s) : center(s.center), radius(s.radius) {}

BoundingSphere::BoundingSphere(const Point3 &c, float r) : center(c), radius(r) {}

BoundingSphere::BoundingSphere(std::vector<Point3> &vertex_list)
{
   if (vertex_list.empty())
   {
      center.set(0.0f, 0.0f, 0.0f);
      radius = 0.0f;
      return;
   }

   if (vertex_list.size() == 1)
   {
      center = vertex_list[0];
      radius = 0.0f;
      return;
   }

   // Step 1: Find the most separated points along each axis
   size_t min_x = 0, max_x = 0;
   size_t min_y = 0, max_y = 0;
   size_t min_z = 0, max_z = 0;

   for (size_t i = 1; i < vertex_list.size(); i++)
   {
      if (vertex_list[i].x < vertex_list[min_x].x) min_x = i;
      if (vertex_list[i].x > vertex_list[max_x].x) max_x = i;
      if (vertex_list[i].y < vertex_list[min_y].y) min_y = i;
      if (vertex_list[i].y > vertex_list[max_y].y) max_y = i;
      if (vertex_list[i].z < vertex_list[min_z].z) min_z = i;
      if (vertex_list[i].z > vertex_list[max_z].z) max_z = i;
   }

   // Step 2: Find the most separated pair (maximum distance squared)
   Vector3 dx(vertex_list[min_x], vertex_list[max_x]);
   Vector3 dy(vertex_list[min_y], vertex_list[max_y]);
   Vector3 dz(vertex_list[min_z], vertex_list[max_z]);

   float dist_x = dx.norm_squared();
   float dist_y = dy.norm_squared();
   float dist_z = dz.norm_squared();

   size_t min_idx = min_x;
   size_t max_idx = max_x;
   
   if (dist_y > dist_x && dist_y > dist_z)
   {
      min_idx = min_y;
      max_idx = max_y;
   }
   else if (dist_z > dist_x && dist_z > dist_y)
   {
      min_idx = min_z;
      max_idx = max_z;
   }

   // Step 3: Create initial sphere from most separated pair
   Point3 p1 = vertex_list[min_idx];
   Point3 p2 = vertex_list[max_idx];
   
   center = p1.mid_point(p2);
   radius = Vector3(center, p2).norm();

   // Step 4: Grow sphere to include all points
   for (const Point3& p : vertex_list)
   {
      Vector3 d(center, p);
      float dist = d.norm();
      
      if (dist > radius)
      {
         // Point is outside sphere, need to expand
         // New sphere encompasses both old sphere and new point
         float new_radius = (radius + dist) * 0.5f;
         float scale = (new_radius - radius) / dist;
         
         center = center + d * scale;
         radius = new_radius;
      }
   }
}

BoundingSphere &BoundingSphere::merge_with(const BoundingSphere &s2)
{
   // Vector from this center to s2 center
   Vector3 d(center, s2.center);
   float dist = d.norm();

   // Check if one sphere contains the other
   if (dist + s2.radius <= radius)
   {
      // s2 is completely inside this sphere
      return *this;
   }
   
   if (dist + radius <= s2.radius)
   {
      // This sphere is completely inside s2
      center = s2.center;
      radius = s2.radius;
      return *this;
   }

   // Spheres overlap or are separate - create enclosing sphere
   // New radius is half the distance between the far points
   float new_radius = (radius + dist + s2.radius) * 0.5f;
   
   // Move center along the line between centers
   if (dist > EPSILON)
   {
      float scale = (new_radius - radius) / dist;
      center = center + d * scale;
   }
   
   radius = new_radius;
   return *this;
}

BoundingSphere BoundingSphere::merge(const BoundingSphere &s2) const
{
    BoundingSphere combined(*this);
    combined.merge_with(s2);
    return combined;
}

} // namespace cg
