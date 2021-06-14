// STL includes.
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cassert>

// CGAL includes.
#include <CGAL/assertions.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

#include <CGAL/Surface_mesh.h>

#include <CGAL/Shape_detection/Region_growing/Region_growing.h>
#include <CGAL/Shape_detection/Region_growing/Region_growing_on_polygon_mesh.h>
#include <CGAL/Shape_detection/Region_growing/internal/free_functions.h>

namespace SD = CGAL::Shape_detection;
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;

using FT      = typename Kernel::FT;
using Point_3 = typename Kernel::Point_3;

using Polygon_mesh = CGAL::Surface_mesh<Point_3>;
using Face_range   = typename Polygon_mesh::Face_range;

using Neighbor_query = SD::Polygon_mesh::One_ring_neighbor_query<Polygon_mesh>;
using Region_type    = SD::Polygon_mesh::Least_squares_plane_fit_region<Kernel, Polygon_mesh>;
using Sorting        = SD::Polygon_mesh::Least_squares_plane_fit_sorting<Kernel, Polygon_mesh, Neighbor_query>;
using Region_growing = SD::Region_growing<Face_range, Neighbor_query, Region_type, typename Sorting::Seed_map>;

int main(int argc, char *argv[]) {

  // Default parameter values.
  const FT          distance_threshold = FT(1);
  const FT          angle_threshold    = FT(45);
  const std::size_t min_region_size    = 5;

  // Load data.
  std::ifstream in(argc > 1 ? argv[1] : "data/polygon_mesh.off");
  CGAL::IO::set_ascii_mode(in);
  assert(in);

  Polygon_mesh polygon_mesh;
  in >> polygon_mesh;
  in.close();
  const Face_range face_range = faces(polygon_mesh);
  assert(face_range.size() == 32245);

  using Vertex_to_point_map = typename Region_type::Vertex_to_point_map;
  const Vertex_to_point_map vertex_to_point_map(
    get(CGAL::vertex_point, polygon_mesh));

  // Create parameter classes.
  Neighbor_query neighbor_query(polygon_mesh);
  Region_type region_type(
    polygon_mesh,
    CGAL::parameters::
    max_distance(distance_threshold).
    max_angle(angle_threshold).
    min_region_size(min_region_size).
    vertex_point_map(vertex_to_point_map));

  // Sort indices.
  Sorting sorting(
    polygon_mesh, neighbor_query,
    CGAL::parameters::vertex_point_map(vertex_to_point_map));
  sorting.sort();

  // Run region growing.
  Region_growing region_growing(
    face_range, neighbor_query, region_type, sorting.seed_map());

  std::vector< std::vector<std::size_t> > regions;
  region_growing.detect(std::back_inserter(regions));
  region_growing.clear();
  assert(regions.size() == 326);

  // Test free functions and stability.
  for (std::size_t k = 0; k < 3; ++k) {
    regions.clear();
    SD::internal::region_growing_planes(
      polygon_mesh, std::back_inserter(regions),
      CGAL::parameters::
      max_distance(distance_threshold).
      max_angle(angle_threshold).
      min_region_size(min_region_size).
      vertex_point_map(vertex_to_point_map));
    assert(regions.size() == 326);
  }

  std::cout << "rg_sortfaces3, epeck_test_success: " << true << std::endl;
  return EXIT_SUCCESS;
}
