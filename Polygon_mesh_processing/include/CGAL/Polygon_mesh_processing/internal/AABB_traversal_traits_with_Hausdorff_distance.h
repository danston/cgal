// Copyright (c) 2019 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s) : Sebastien Loriot, Martin Skrodzki, Dmitry Anisimov

#ifndef CGAL_PMP_INTERNAL_AABB_TRAVERSAL_TRAITS_WITH_HAUSDORFF_DISTANCE
#define CGAL_PMP_INTERNAL_AABB_TRAVERSAL_TRAITS_WITH_HAUSDORFF_DISTANCE

#include <CGAL/license/Polygon_mesh_processing/distance.h>

// STL includes.
#include <vector>
#include <iostream>

// CGAL includes.
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>

namespace CGAL {

  // Infinity.
  template<typename FT>
  static FT infinity_value() {
    return FT(1000000000000);
  }

  // Bounds.
  template<typename Kernel, typename Face_handle>
  struct Bounds {
    using FT = typename Kernel::FT;

    FT lower = infinity_value<FT>();
    FT upper = infinity_value<FT>();
    Face_handle lface = Face_handle();
    Face_handle uface = Face_handle();
    std::pair<Face_handle, Face_handle> lpair = default_face_pair();
    std::pair<Face_handle, Face_handle> upair = default_face_pair();

    const std::pair<Face_handle, Face_handle> default_face_pair() const {
      return std::make_pair(Face_handle(), Face_handle());
    }
  };

  // Candidate triangle.
  template<typename Kernel, typename Face_handle>
  struct Candidate_triangle {
    using Triangle_3 = typename Kernel::Triangle_3;
    using Candidate_bounds = Bounds<Kernel, Face_handle>;

    Candidate_triangle(
      const Triangle_3& triangle, const Candidate_bounds& bounds, const Face_handle& fh) :
    triangle(triangle), bounds(bounds), face(fh)
    { }

    Triangle_3 triangle;
    Candidate_bounds bounds;
    Face_handle face;
    bool operator>(const Candidate_triangle& other) const { return bounds.upper < other.bounds.upper; }
    bool operator<(const Candidate_triangle& other) const { return bounds.upper > other.bounds.upper; }
  };

  // Hausdorff primitive traits on TM2.
  template<
  typename AABBTraits,
  typename Query,
  typename Kernel,
  typename TriangleMesh, typename VPM2>
  class Hausdorff_primitive_traits_tm2 {

    using FT         = typename Kernel::FT;
    using Point_3    = typename Kernel::Point_3;
    using Vector_3   = typename Kernel::Vector_3;
    using Triangle_3 = typename Kernel::Triangle_3;

    using Project_point_3 = typename Kernel::Construct_projected_point_3;
    using Face_handle     = typename boost::graph_traits<TriangleMesh>::face_descriptor;
    using Local_bounds    = Bounds<Kernel, Face_handle>;

    using TM2_face_to_triangle_map = Triangle_from_face_descriptor_map<TriangleMesh, VPM2>;

  public:
    using Priority = FT;
    Hausdorff_primitive_traits_tm2(
      const AABBTraits& traits,
      const TriangleMesh& tm2, const VPM2& vpm2,
      const Local_bounds& local_bounds,
      const FT h_v0_lower_init,
      const FT h_v1_lower_init,
      const FT h_v2_lower_init) :
    m_traits(traits), m_tm2(tm2), m_vpm2(vpm2),
    m_face_to_triangle_map(&m_tm2, m_vpm2),
    h_local_bounds(local_bounds) {

      // Initialize the global and local bounds with the given values.
      h_v0_lower = h_v0_lower_init;
      h_v1_lower = h_v1_lower_init;
      h_v2_lower = h_v2_lower_init;
    }

    // Explore the whole tree, i.e. always enter children if the method
    // do_intersect() below determines that it is worthwhile.
    bool go_further() const { return true; }

    // Compute the explicit Hausdorff distance to the given primitive.
    template<typename Primitive>
    void intersection(const Query& query, const Primitive& primitive) {

      /* Have reached a single triangle, process it.
      / Determine the distance according to
      /   min_{b \in primitive} ( max_{vertex in query} ( d(vertex, b) ) )
      /
      / Here, we only have one triangle in B, i.e. tm2. Thus, it suffices to
      / compute the distance of the vertices of the query triangles to the
      / primitive triangle and use the maximum of the obtained distances.
      */

      // The query object is a triangle from TM1, get its vertices.
      const Point_3 v0 = query.vertex(0);
      const Point_3 v1 = query.vertex(1);
      const Point_3 v2 = query.vertex(2);

      CGAL_assertion(primitive.id() != Face_handle());
      const Triangle_3 triangle = get(m_face_to_triangle_map, primitive.id());

      // Compute distances of the vertices to the primitive triangle in TM2.
      const FT v0_dist = static_cast<FT>(CGAL::sqrt(CGAL::to_double(
        CGAL::squared_distance(m_project_point(triangle, v0), v0))));
      if (v0_dist < h_v0_lower) h_v0_lower = v0_dist; // it is () part of (11) in the paper

      const FT v1_dist = static_cast<FT>(CGAL::sqrt(CGAL::to_double(
        CGAL::squared_distance(m_project_point(triangle, v1), v1))));
      if (v1_dist < h_v1_lower) h_v1_lower = v1_dist; // it is () part of (11) in the paper

      const FT v2_dist = static_cast<FT>(CGAL::sqrt(CGAL::to_double(
        CGAL::squared_distance(m_project_point(triangle, v2), v2))));
      if (v2_dist < h_v2_lower) h_v2_lower = v2_dist; // it is () part of (11) in the paper

      // Get the distance as maximizers over all vertices.
      const FT distance_lower = (CGAL::max)((CGAL::max)(h_v0_lower, h_v1_lower), h_v2_lower); // it is (11) in the paper
      const FT distance_upper = (CGAL::max)((CGAL::max)(v0_dist, v1_dist), v2_dist); // it is () part of (10) in the paper

      // Since we are at the level of a single triangle in TM2, distance_upper is
      // actually the correct Hausdorff distance from the query triangle in
      // TM1 to the primitive triangle in TM2.
      if (distance_lower < h_local_bounds.lower) {
        h_local_bounds.lower = distance_lower;
        h_local_bounds.lface = primitive.id();
      }
      if (distance_upper < h_local_bounds.upper) { // it is (10) in the paper
        h_local_bounds.upper = distance_upper;
        h_local_bounds.uface = primitive.id();
      }
    }

    // Determine whether child nodes will still contribute to a smaller
    // Hausdorff distance and thus have to be entered.
    template<typename Node>
    std::pair<bool, Priority>
    do_intersect_with_priority(const Query& query, const Node& node) const {

      // Get the bounding box of the nodes.
      const auto bbox = node.bbox();

      // Get the vertices of the query triangle.
      const Point_3 v0 = query.vertex(0);
      const Point_3 v1 = query.vertex(1);
      const Point_3 v2 = query.vertex(2);

      // Find the axis aligned bbox of the triangle.
      const Point_3 tri_min = Point_3(
        (CGAL::min)((CGAL::min)(v0.x(), v1.x()), v2.x()),
        (CGAL::min)((CGAL::min)(v0.y(), v1.y()), v2.y()),
        (CGAL::min)((CGAL::min)(v0.z(), v1.z()), v2.z()));

      const Point_3 tri_max = Point_3(
        (CGAL::max)((CGAL::max)(v0.x(), v1.x()), v2.x()),
        (CGAL::max)((CGAL::max)(v0.y(), v1.y()), v2.y()),
        (CGAL::max)((CGAL::max)(v0.z(), v1.z()), v2.z()));

      // Compute distance of the bounding boxes.
      // Distance along the x-axis.
      FT dist_x = FT(0);
      if (tri_max.x() < bbox.min(0)) {
        dist_x = bbox.min(0) - tri_max.x();
      } else if (bbox.max(0) < tri_min.x()) {
        dist_x = tri_min.x() - bbox.max(0);
      }

      // Distance along the y-axis.
      FT dist_y = FT(0);
      if (tri_max.y() < bbox.min(1)) {
        dist_y = bbox.min(1) - tri_max.y();
      } else if (bbox.max(1) < tri_min.y()) {
        dist_y = tri_min.y() - bbox.max(1);
      }

      // Distance along the z-axis.
      FT dist_z = FT(0);
      if (tri_max.z() < bbox.min(2)) {
        dist_z = bbox.min(2) - tri_max.z();
      } else if (bbox.max(2) < tri_min.z()) {
        dist_z = tri_min.z() - bbox.max(2);
      }

      // Lower bound on the distance between the two bounding boxes is given
      // as the length of the diagonal of the bounding box between them.
      const FT dist = static_cast<FT>(CGAL::sqrt(CGAL::to_double(
        Vector_3(dist_x, dist_y, dist_z).squared_length())));

      // See Algorithm 2.
      // Check whether investigating the bbox can still lower the Hausdorff
      // distance and improve the current global bound. If so, enter the box.
      if (dist <= h_local_bounds.lower) {
        return std::make_pair(true , -dist);
      } else {
        return std::make_pair(false, FT(0));
      }
    }

    template<typename Node>
    bool do_intersect(const Query& query, const Node& node) const {
      return this->do_intersect_with_priority(query, node).first;
    }

    // Return the local Hausdorff bounds computed for the passed query triangle.
    Local_bounds get_local_bounds() const {
      return h_local_bounds;
    }

  private:
    // Input data.
    const AABBTraits& m_traits;
    const TriangleMesh& m_tm2;
    const VPM2& m_vpm2;
    const TM2_face_to_triangle_map m_face_to_triangle_map;

    // Local Hausdorff bounds for the query triangle.
    Local_bounds h_local_bounds;
    FT h_v0_lower, h_v1_lower, h_v2_lower;
    Project_point_3 m_project_point;
  };

  // Hausdorff primitive traits on TM1.
  template<
  typename AABBTraits,
  typename Query,
  typename Kernel,
  typename TriangleMesh,
  typename VPM1, typename VPM2>
  class Hausdorff_primitive_traits_tm1 {

    using FT         = typename Kernel::FT;
    using Point_3    = typename Kernel::Point_3;
    using Vector_3   = typename Kernel::Vector_3;
    using Triangle_3 = typename Kernel::Triangle_3;

    using TM2_primitive = AABB_face_graph_triangle_primitive<TriangleMesh, VPM2>;
    using TM2_traits    = AABB_traits<Kernel, TM2_primitive>;
    using TM2_tree      = AABB_tree<TM2_traits>;
    using TM2_hd_traits = Hausdorff_primitive_traits_tm2<TM2_traits, Triangle_3, Kernel, TriangleMesh, VPM2>;

    using TM1_face_to_triangle_map = Triangle_from_face_descriptor_map<TriangleMesh, VPM1>;

    using Face_handle   = typename boost::graph_traits<TriangleMesh>::face_descriptor;
    using Global_bounds = Bounds<Kernel, Face_handle>;
    using Candidate     = Candidate_triangle<Kernel, Face_handle>;
    using Heap_type     = std::priority_queue<Candidate>;

  public:
    using Priority = FT;
    Hausdorff_primitive_traits_tm1(
      const AABBTraits& traits, const TM2_tree& tree,
      const TriangleMesh& tm1, const TriangleMesh& tm2,
      const VPM1& vpm1, const VPM2& vpm2,
      const FT error_bound) :
    m_traits(traits),
    m_tm1(tm1), m_tm2(tm2),
    m_vpm1(vpm1), m_vpm2(vpm2),
    m_tm2_tree(tree),
    m_face_to_triangle_map(&m_tm1, m_vpm1),
    m_error_bound(error_bound) {

      // Initialize the global bounds with 0, they will only grow.
      // If we leave zero here, then we are very slow even for big input error bounds!
      // The error_bound here makes the code faster for close meshes.
      h_global_bounds.lower = m_error_bound; // = FT(0);
      h_global_bounds.upper = m_error_bound; // = FT(0);
    }

    // Explore the whole tree, i.e. always enter children if the methods
    // do_intersect() below determine that it is worthwhile.
    bool go_further() const { return true; }

    // Compute the explicit Hausdorff distance to the given primitive.
    template<typename Primitive>
    void intersection(const Query&, const Primitive& primitive) {

      // Set initial tight bounds.
      CGAL_assertion(primitive.id() != Face_handle());
      std::pair<Face_handle, Face_handle> fpair;
      const FT max_dist = get_maximum_distance(primitive.id(), fpair);
      CGAL_assertion(fpair.first == primitive.id());

      Bounds<Kernel, Face_handle> initial_bounds;
      initial_bounds.lower = max_dist + m_error_bound;
      initial_bounds.upper = max_dist + m_error_bound;
      initial_bounds.lface = fpair.second;
      initial_bounds.uface = fpair.second;

      // Call Culling on B with the single triangle found.
      TM2_hd_traits traversal_traits_tm2(
        m_tm2_tree.traits(), m_tm2, m_vpm2,
        initial_bounds, // tighter bounds, in the paper, they start from infinity, see below
        // Bounds<Kernel, Face_handle>(), // starting from infinity
        infinity_value<FT>(),
        infinity_value<FT>(),
        infinity_value<FT>());

      const Triangle_3 triangle = get(m_face_to_triangle_map, fpair.first);
      m_tm2_tree.traversal_with_priority(triangle, traversal_traits_tm2);

      // Update global Hausdorff bounds according to the obtained local bounds.
      const auto local_bounds = traversal_traits_tm2.get_local_bounds();
      CGAL_assertion(local_bounds.lpair == initial_bounds.default_face_pair());
      CGAL_assertion(local_bounds.upair == initial_bounds.default_face_pair());

      if (local_bounds.lower > h_global_bounds.lower) { // it is (6) in the paper, see also Algorithm 1
        h_global_bounds.lower = local_bounds.lower;
        h_global_bounds.lpair.first  = fpair.first;
        h_global_bounds.lpair.second = local_bounds.lface;
      }
      if (local_bounds.upper > h_global_bounds.upper) { // it is (6) in the paper, see also Algorithm 1
        h_global_bounds.upper = local_bounds.upper;
        h_global_bounds.upair.first  = fpair.first;
        h_global_bounds.upair.second = local_bounds.uface;
      }

      // Store the triangle given as primitive here as candidate triangle
      // together with the local bounds it obtained to send it to subdivision later.
      m_candidiate_triangles.push(Candidate(triangle, local_bounds, fpair.first));
    }

    // Determine whether child nodes will still contribute to a larger
    // Hausdorff distance and thus have to be entered.
    template<typename Node>
    std::pair<bool, Priority>
    do_intersect_with_priority(const Query&, const Node& node) const {

      // Have reached a node, determine whether or not to enter it.
      // Get the bounding box of the nodes.
      const auto bbox = node.bbox();

      // Compute its center.
      const Point_3 center = Point_3(
        (bbox.min(0) + bbox.max(0)) / FT(2),
        (bbox.min(1) + bbox.max(1)) / FT(2),
        (bbox.min(2) + bbox.max(2)) / FT(2));

      // Find the point from TM2 closest to the center.
      const Point_3 closest = m_tm2_tree.closest_point(center);

      // Compute the difference vector between the bbox center and the closest point in tm2.
      Vector_3 difference = Vector_3(closest, center);

      // Shift the vector to be the difference between the farthest corner
      // of the bounding box away from the closest point on TM2.
      FT diff_x = (bbox.max(0) - bbox.min(0)) / FT(2);
      if (difference.x() < 0) diff_x = diff_x * -FT(1);
      FT diff_y = (bbox.max(1) - bbox.min(1)) / FT(2);
      if (difference.y() < 0) diff_y = diff_y * -FT(1);
      FT diff_z = (bbox.max(2) - bbox.min(2)) / FT(2);
      if (difference.z() < 0) diff_z = diff_z * -FT(1);
      difference = difference + Vector_3(diff_x, diff_y, diff_z); // it is (9) in the paper

      // Compute distance from the farthest corner of the bbox to the closest point in TM2.
      const FT dist = static_cast<FT>(CGAL::sqrt(CGAL::to_double(difference.squared_length())));

      // See Algorithm 1 here.
      // If the distance is larger than the global lower bound, enter the node, i.e. return true.
      if (dist > h_global_bounds.lower) {
        return std::make_pair(true , +dist);
      } else {
        return std::make_pair(false, FT(0));
      }
    }

    template<typename Node>
    bool do_intersect(const Query& query, const Node& node) const {
      return this->do_intersect_with_priority(query, node).first;
    }

    // Return those triangles from TM1, which are candidates for including a
    // point realizing the Hausdorff distance.
    Heap_type& get_candidate_triangles() {
      return m_candidiate_triangles;
    }

    // Return the global Hausdorff bounds computed for the passed query triangle.
    Global_bounds get_global_bounds() {
      update_global_bounds();
      return h_global_bounds;
    }

    // Here, we return the maximum distance from one of the face corners
    // to the second mesh. We also return a pair of realizing this distance faces.
    FT get_maximum_distance(
      const Face_handle lface,
      std::pair<Face_handle, Face_handle>& fpair) const {

      const auto triangle = get(m_face_to_triangle_map, lface);
      const Point_3 v0 = triangle.vertex(0);
      const Point_3 v1 = triangle.vertex(1);
      const Point_3 v2 = triangle.vertex(2);

      const auto pair0 = m_tm2_tree.closest_point_and_primitive(v0);
      const auto pair1 = m_tm2_tree.closest_point_and_primitive(v1);
      const auto pair2 = m_tm2_tree.closest_point_and_primitive(v2);

      const auto sq_dist0 = std::make_pair(
        CGAL::squared_distance(v0, pair0.first), pair0.second);
      const auto sq_dist1 = std::make_pair(
        CGAL::squared_distance(v1, pair1.first), pair1.second);
      const auto sq_dist2 = std::make_pair(
        CGAL::squared_distance(v2, pair2.first), pair2.second);

      const auto mdist1 = (sq_dist0.first > sq_dist1.first) ? sq_dist0 : sq_dist1;
      const auto mdist2 = (mdist1.first > sq_dist2.first) ? mdist1 : sq_dist2;

      const auto uface = mdist2.second;
      fpair = std::make_pair(lface, uface);
      return static_cast<FT>(CGAL::sqrt(CGAL::to_double(mdist2.first)));
    }

  private:
    // Input data.
    const AABBTraits& m_traits;
    const TriangleMesh& m_tm1;
    const TriangleMesh& m_tm2;
    const VPM1& m_vpm1;
    const VPM2& m_vpm2;
    const TM2_tree& m_tm2_tree;
    const TM1_face_to_triangle_map m_face_to_triangle_map;

    // Global Hausdorff bounds for the query triangle.
    const FT m_error_bound;
    Global_bounds h_global_bounds;

    // All candidate triangles.
    Heap_type m_candidiate_triangles;

    // In case, we did not enter any loop, we set the realizing triangles here.
    void update_global_bounds() {

      if (m_candidiate_triangles.size() > 0) {
        const auto top = m_candidiate_triangles.top();

        if (h_global_bounds.lpair.first == Face_handle())
          h_global_bounds.lpair.first = top.face;
        if (h_global_bounds.lpair.second == Face_handle())
          h_global_bounds.lpair.second = top.bounds.lface;

        if (h_global_bounds.upair.first == Face_handle())
          h_global_bounds.upair.first = top.face;
        if (h_global_bounds.upair.second == Face_handle())
          h_global_bounds.upair.second = top.bounds.uface;

      } else {

        std::pair<Face_handle, Face_handle> fpair;
        get_maximum_distance(*(faces(m_tm1).begin()), fpair);
        CGAL_assertion(fpair.first == *(faces(m_tm1).begin()));

        if (h_global_bounds.lpair.first == Face_handle())
          h_global_bounds.lpair.first = fpair.first;
        if (h_global_bounds.lpair.second == Face_handle())
          h_global_bounds.lpair.second = fpair.second;

        if (h_global_bounds.upair.first == Face_handle())
          h_global_bounds.upair.first = fpair.first;
        if (h_global_bounds.upair.second == Face_handle())
          h_global_bounds.upair.second = fpair.second;
      }
    }
  };
}

#endif // CGAL_PMP_INTERNAL_AABB_TRAVERSAL_TRAITS_WITH_HAUSDORFF_DISTANCE
