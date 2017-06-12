// Copyright (c) 2017 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Jane Tournois

#ifndef CGAL_POLYGON_MESH_PROCESSING_RANDOM_PERTURBATION_H
#define CGAL_POLYGON_MESH_PROCESSING_RANDOM_PERTURBATION_H

#include <CGAL/license/Polygon_mesh_processing/meshing_hole_filling.h>

#include <CGAL/Polygon_mesh_processing/internal/Isotropic_remeshing/remesh_impl.h>

#include <CGAL/Polygon_mesh_processing/internal/named_function_params.h>
#include <CGAL/Polygon_mesh_processing/internal/named_params_helper.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>

#include <CGAL/Random.h>

#include <boost/foreach.hpp>

#ifdef CGAL_PMP_RANDOM_PERTURBATION_VERBOSE
#include <CGAL/Timer.h>
#endif

namespace CGAL {

namespace Polygon_mesh_processing {

namespace internal {

  template<typename GT, typename RNG>
  typename GT::Vector_3 construct_random_vector_3(const double& max_size,
                                                  RNG& rng,
                                                  const GT& gt)
  {
    typedef typename GT::FT FT;
    typename GT::Construct_vector_3 vec = gt.construct_vector_3_object();

    return vec(FT(rng.get_double(-max_size, max_size)),
               FT(rng.get_double(-max_size, max_size)),
               FT(rng.get_double(-max_size, max_size)));
  }

  template<typename GT, typename PM, typename VCMap, typename VPMap, typename RNG>
  void random_perturbation_impl(PM& tmesh,
                                const double& max_size,
                                VCMap vcmap,
                                VPMap vpmap,
                                bool do_project,
                                RNG& rng,
                                const GT& gt)
  {
    typedef typename boost::graph_traits<PM>::vertex_descriptor vertex_descriptor;
    typedef typename GT::Point_3    Point_3;
    typedef typename GT::FT         FT;

    typedef CGAL::AABB_face_graph_triangle_primitive<PM> Primitive;
    typedef CGAL::AABB_traits<GT, Primitive> Traits;
    typedef CGAL::AABB_tree<Traits> Tree;

    Tree tree;
    if(do_project)
    {
      tree.rebuild(faces(tmesh).first, faces(tmesh).second, tmesh);
      tree.accelerate_distance_queries();
    }
    typename GT::Construct_translated_point_3 translate
      = gt.construct_translated_point_3_object();

    BOOST_FOREACH(vertex_descriptor v, vertices(tmesh))
    {
      if (!get(vcmap, v) && !is_border(v, tmesh))
      {
        const Point_3& p = get(vpmap, v);
        const Point_3 np = translate(p, construct_random_vector_3<GT>(max_size, rng, gt));

        if (do_project)
          put(vpmap, v, tree.closest_point(np)); //project on input surface
        else
          put(vpmap, v, np);
      }
    }
  }

} //end namespace internal

/*!
* \ingroup PMP_meshing_grp
* @brief randomly perturbs the locations of vertices of a triangulated surface mesh.
* In the absence of instructions to the contrary in the parameters,
* the vertices are re-projected to the input surface after vertices geometric perturbation.
* Note that depending on the chosen parameters, inversions and self-intersections may happen.
*
* @tparam TriangleMesh model of `MutableFaceGraph`.
* @tparam NamedParameters a sequence of \ref namedparameters
*
* @param tmesh the triangulated surface mesh to be perturbed
* @param perturbation_max_size the maximal length of moves that can be applied to
*        vertices of `tmesh`.
* @param np optional sequence of \ref namedparameters among the ones listed below
*
* \cgalNamedParamsBegin
*  \cgalParamBegin{geom_traits} a geometric traits class instance, model of `Kernel`.
*  \cgalParamEnd
*  \cgalParamBegin{vertex_point_map} the property map with the points associated
*    to the vertices of `tmesh`. Instance of a class model of `ReadWritePropertyMap`.
*  \cgalParamEnd
*  \cgalParamBegin{vertex_is_constrained_map} a property map containing the
*    constrained-or-not status of each vertex of `tmesh`. A constrained vertex
*    cannot be modified at all during perturbation
*  \cgalParamEnd
*  \cgalParamBegin{do_project} a boolean that sets whether vertices should be reprojected
*    on the input surface after their coordinates random perturbation
*  \cgalParamEnd
*  \cgalParambegin{random_seed} a non-negative integer value to seed the random
      number generator, and make the perturbation deterministic
*  \cgalParamEnd
* \cgalNamedParamsEnd
*
*/
template<typename TriangleMesh, typename NamedParameters>
void random_perturbation(TriangleMesh& tmesh
                       , const double& perturbation_max_size
                       , const NamedParameters& np)
{
  typedef TriangleMesh PM;
  using boost::get_param;
  using boost::choose_param;

  typedef typename boost::graph_traits<PM>::vertex_descriptor vertex_descriptor;

#ifdef CGAL_PMP_RANDOM_PERTURBATION_VERBOSE
  std::cout << std::endl;
  CGAL::Timer t;
  std::cout << "Random perturbation (max size = "<< perturbation_max_size<<")...";
  std::cout.flush();
  t.start();
#endif

  typedef typename GetGeomTraits<PM, NamedParameters>::type GT;
  GT gt = choose_param(get_param(np, internal_np::geom_traits), GT());

  typedef typename GetVertexPointMap<PM, NamedParameters>::type VPMap;
  VPMap vpmap = choose_param(get_param(np, internal_np::vertex_point),
                             get_property_map(vertex_point, tmesh));

  typedef typename boost::lookup_named_param_def <
      internal_np::vertex_is_constrained_t,
      NamedParameters,
      internal::No_constraint_pmap<vertex_descriptor>//default
    > ::type VCMap;
  VCMap vcmap = choose_param(get_param(np, internal_np::vertex_is_constrained),
                             internal::No_constraint_pmap<vertex_descriptor>());

  unsigned int seed = choose_param(get_param(np, internal_np::random_seed), -1);
  bool do_project = choose_param(get_param(np, internal_np::do_project), true);

  internal::random_perturbation_impl(tmesh,
          perturbation_max_size,
          vcmap,
          vpmap,
          do_project,
          ((seed == -1) ? CGAL::Random() : CGAL::Random(seed)),
          gt);

#ifdef CGAL_PMP_RANDOM_PERTURBATION_VERBOSE
  t.stop();
  std::cout << "Perturbation done (";
  std::cout << t.time() << " sec )." << std::endl;
#endif
}

template<typename TriangleMesh>
void random_perturbation(TriangleMesh& tmesh
                       , const double& perturbation_max_size)
{
  random_perturbation(tmesh,
                      perturbation_max_size,
                      parameters::all_default());
}

} //end namespace Polygon_mesh_processing
} //end namespace CGAL

#endif //CGAL_POLYGON_MESH_PROCESSING_RANDOM_PERTURBATION_H

