#include "geometry/aabb.hpp"

#include "geometry/geometry.hpp"

namespace cg
{

// NOTE - this is not required until 605.767!

AABB::AABB()
{
    min_.set(std::numeric_limits<float>::max(), 
             std::numeric_limits<float>::max(), 
             std::numeric_limits<float>::max());
    max_.set(std::numeric_limits<float>::lowest(), 
             std::numeric_limits<float>::lowest(), 
             std::numeric_limits<float>::lowest());
}

AABB::AABB(const Point3 &min, const Point3 &max)
{
    // Complete in 605.767
   update(min, max);
}

AABB::AABB(const std::vector<Point3> &vertex_list)
{
    // Complete in 605.767
   create(vertex_list);
}

void AABB::create(const std::vector<Point3> &vertex_list)
{

   if (vertex_list.empty())
   {
      // Empty box
      min_.set(0.0f, 0.0f, 0.0f);
      max_.set(0.0f, 0.0f, 0.0f);
      return;
   }

   // Initialize with first vertex
   min_ = vertex_list[0];
   max_ = vertex_list[0];

   // Expand to include all vertices
   for (size_t i = 1; i < vertex_list.size(); i++)
   {
      const Point3& v = vertex_list[i];
       
      // Update minimum bounds
      if (v.x < min_.x) min_.x = v.x;
      if (v.y < min_.y) min_.y = v.y;
      if (v.z < min_.z) min_.z = v.z;
       
      // Update maximum bounds
      if (v.x > max_.x) max_.x = v.x;
      if (v.y > max_.y) max_.y = v.y;
      if (v.z > max_.z) max_.z = v.z;
   }
}

void AABB::update(const Point3 &min, const Point3 &max)
{
   min_ = min;
   max_ = max;
}

void AABB::merge(const AABB &box)
{
    // Take component-wise minimum and maximum
    min_.x = std::min(min_.x, box.min_.x);
    min_.y = std::min(min_.y, box.min_.y);
    min_.z = std::min(min_.z, box.min_.z);
    
    max_.x = std::max(max_.x, box.max_.x);
    max_.y = std::max(max_.y, box.max_.y);
    max_.z = std::max(max_.z, box.max_.z);
}

Point3 AABB::min_pt() const
{
    return min_;
}

Point3 AABB::max_pt() const
{
    return max_;
}

void compute_center()
{
    // Complete in 605.767
}

} // namespace cg
