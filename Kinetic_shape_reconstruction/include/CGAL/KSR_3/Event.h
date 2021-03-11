// Copyright (c) 2019 GeometryFactory SARL (France).
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
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Simon Giraudot

#ifndef CGAL_KSR_3_EVENT_H
#define CGAL_KSR_3_EVENT_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// CGAL includes.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

// Internal includes.
#include <CGAL/KSR/utils.h>

namespace CGAL {
namespace KSR_3 {

template<typename Data_structure>
class Event_queue;

// This class works only with inexact number types because it is a base for the
// multi index container in the Event_queue class, which cannot handle exact number types.
template<typename Data_structure>
class Event {

public:
  // Kernel types.
  using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
  using NT     = typename Data_structure::Kernel::FT;
  using FT     = typename Kernel::FT;

  // Data structure types.
  using PVertex = typename Data_structure::PVertex;
  using PEdge   = typename Data_structure::PEdge;
  using PFace   = typename Data_structure::PFace;
  using IVertex = typename Data_structure::IVertex;
  using IEdge   = typename Data_structure::IEdge;

  // Event queue types.
  using Queue = Event_queue<Data_structure>;
  friend Queue;

  struct ETime {
    ETime(const NT event_time, const PVertex& pother, const IVertex& ivertex) :
    time(static_cast<FT>(CGAL::to_double(event_time))),
    m_pother(pother), m_ivertex(ivertex)
    { }

    const FT time;
    const PVertex& m_pother;
    const IVertex& m_ivertex;
    const bool operator<(const ETime& e) const {

      const FT tol = KSR::tolerance<FT>();
      const FT time_diff = CGAL::abs(time - e.time);
      if (time_diff < tol) {
        const std::size_t la =   is_pvertex_to_ivertex() ? 1 : 0;
        const std::size_t lb = e.is_pvertex_to_ivertex() ? 1 : 0;
        return la < lb;
      }
      return time < e.time;
    }

    const bool is_pvertex_to_ivertex() const {
      return (
        m_pother  == Data_structure::null_pvertex() &&
        m_ivertex != Data_structure::null_ivertex());
    }
  };

  // Event types.

  // Empty event.
  Event() :
    m_is_constrained(false),
    m_pvertex(Data_structure::null_pvertex()),
    m_pother(Data_structure::null_pvertex()),
    m_ivertex(Data_structure::null_ivertex()),
    m_iedge(Data_structure::null_iedge()),
    m_time(ETime(0, m_pother, m_ivertex)),
    m_support_plane_idx(m_pvertex.first)
  { }

  // An event that occurs between two polygon vertices.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const PVertex pother,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(pother),
  m_ivertex(Data_structure::null_ivertex()),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, m_pother, m_ivertex)),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE UNCONSTRAINED SETTING!");
  }

  // An event that occurs between a polygon vertex and an intersection graph edge.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const IEdge iedge,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(Data_structure::null_pvertex()),
  m_ivertex(Data_structure::null_ivertex()),
  m_iedge(iedge),
  m_time(ETime(time, m_pother, m_ivertex)),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(!is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE CONSTRAINED SETTING!");
  }

  // An event that occurs between a polygon vertex and an intersection graph vertex.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const IVertex ivertex,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(Data_structure::null_pvertex()),
  m_ivertex(ivertex),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, m_pother, m_ivertex)),
  m_support_plane_idx(m_pvertex.first)
  { }

  // An event that occurs between two polygon vertices and an intersection graph vertex.
  Event(
    const bool is_constrained,
    const PVertex pvertex,
    const PVertex pother,
    const IVertex ivertex,
    const NT time) :
  m_is_constrained(is_constrained),
  m_pvertex(pvertex),
  m_pother(pother),
  m_ivertex(ivertex),
  m_iedge(Data_structure::null_iedge()),
  m_time(ETime(time, m_pother, m_ivertex)),
  m_support_plane_idx(m_pvertex.first) {

    CGAL_assertion_msg(is_constrained,
    "ERROR: THIS EVENT CANNOT EVER HAPPEN IN THE UNCONSTRAINED SETTING!");
  }

  // Data access.
  const PVertex& pvertex() const { return m_pvertex; }
  const PVertex& pother() const { return m_pother; }
  const IVertex& ivertex() const { return m_ivertex; }
  const IEdge& iedge() const { return m_iedge; }
  const NT time() const { return static_cast<NT>(m_time.time); }
  const std::size_t support_plane() const { return m_support_plane_idx; }

  // Predicates.
  const bool is_constrained() const { return m_is_constrained; }

  // Event types. See constructors above.
  const bool is_pvertex_to_pvertex() const {
    return (m_pother != Data_structure::null_pvertex()); }
  const bool is_pvertex_to_iedge()   const {
    return (m_iedge != Data_structure::null_iedge()); }
  const bool is_pvertex_to_ivertex() const {
    return (m_pother == Data_structure::null_pvertex() && m_ivertex != Data_structure::null_ivertex()); }
  const bool is_pvertices_to_ivertex() const {
    return (m_pother != Data_structure::null_pvertex() && m_ivertex != Data_structure::null_ivertex()); }

  // Output.
  friend std::ostream& operator<<(std::ostream& os, const Event& event) {

    const std::string constr_type = ( event.m_is_constrained ? "constrained " : "unconstrained " );
    if (event.is_pvertices_to_ivertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.m_pvertex.first << ":" << event.m_pvertex.second
         << "), PVertex(" << event.m_pother.first << ":" << event.m_pother.second
         << "), and IVertex(" << event.m_ivertex << ")";
    } else if (event.is_pvertex_to_pvertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.m_pvertex.first << ":" << event.m_pvertex.second
         << ") and PVertex(" << event.m_pother.first << ":" << event.m_pother.second << ")";
    } else if (event.is_pvertex_to_iedge()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.m_pvertex.first << ":" << event.m_pvertex.second
         << ") and IEdge" << event.m_iedge;
    } else if (event.is_pvertex_to_ivertex()) {
      os << constr_type << "event at t = " << event.time() << " between PVertex("
         << event.m_pvertex.first << ":" << event.m_pvertex.second
         << ") and IVertex(" << event.m_ivertex << ")";
    } else {
      os << "ERROR: INVALID EVENT at t = " << event.time();
    }
    return os;
  }

private:
  bool m_is_constrained;
  PVertex m_pvertex;
  PVertex m_pother;
  IVertex m_ivertex;
  IEdge   m_iedge;
  ETime   m_time;
  std::size_t m_support_plane_idx;
};

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_3_EVENT_H
