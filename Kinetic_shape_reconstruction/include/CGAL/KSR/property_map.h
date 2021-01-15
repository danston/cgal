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

#ifndef CGAL_KSR_PROPERTY_MAP_H
#define CGAL_KSR_PROPERTY_MAP_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// STL includes.
#include <string>
#include <sstream>
#include <utility>
#include <unordered_map>

// CGAL includes.
#include <CGAL/property_map.h>

// Internal includes.
#include <CGAL/KSR/enum.h>

namespace CGAL {
namespace KSR {

template<typename LabelMap>
struct Semantic_from_label_map {
  using Label_map = LabelMap;

  using key_type   = typename boost::property_traits<Label_map>::key_type;
  using value_type = Semantic_label;
  using reference  = value_type;
  using category   = boost::readable_property_map_tag;

  using Label_to_semantic_map = std::unordered_map<int, Semantic_label>;

  Label_map m_label_map;
  Label_to_semantic_map m_label_to_semantic_map;

  Semantic_from_label_map() { }

  Semantic_from_label_map(
    const Label_map label_map,
    const std::string gi_str,
    const std::string bi_str,
    const std::string ii_str,
    const std::string vi_str,
    const bool verbose = true) :
  m_label_map(label_map) {

    if (verbose) {
      std::cout << "* setting semantic labels:" << std::endl;
    }

    std::istringstream gi(gi_str);
    std::istringstream bi(bi_str);
    std::istringstream ii(ii_str);
    std::istringstream vi(vi_str);

    int idx;
    while (gi >> idx) {
      if (verbose) std::cout << idx << " is ground" << std::endl;
      m_label_to_semantic_map.insert(
        std::make_pair(idx, Semantic_label::GROUND));
    }
    while (bi >> idx) {
      if (verbose) std::cout << idx << " is building boundary" << std::endl;
      m_label_to_semantic_map.insert(
        std::make_pair(idx, Semantic_label::BUILDING_BOUNDARY));
    }
    while (ii >> idx) {
      if (verbose) std::cout << idx << " is building interior" << std::endl;
      m_label_to_semantic_map.insert(
        std::make_pair(idx, Semantic_label::BUILDING_INTERIOR));
    }
    while (vi >> idx) {
      if (verbose) std::cout << idx << " is vegetation" << std::endl;
      m_label_to_semantic_map.insert(
        std::make_pair(idx, Semantic_label::VEGETATION));
    }
  }

  friend value_type get(
    const Semantic_from_label_map& semantic_map,
    const key_type& key) {

    const int label = get(semantic_map.m_label_map, key);
    const auto it = semantic_map.m_label_to_semantic_map.find(label);

    if (it == semantic_map.m_label_to_semantic_map.end())
      return Semantic_label::UNCLASSIFIED;
    return it->second;
  }
};

template<
typename Item_range,
typename Property_map,
typename ValueType = typename Property_map::value_type,
typename ReferenceType = const ValueType&>
struct Item_property_map {

  using key_type   = std::size_t;
  using value_type = ValueType;
  using reference  = ReferenceType;
  using category   = boost::lvalue_property_map_tag;

  const Item_range& m_item_range;
  const Property_map& m_property_map;
  Item_property_map(
    const Item_range& item_range,
    const Property_map& property_map) :
  m_item_range(item_range),
  m_property_map(property_map)
  { }

  reference operator[](const key_type item_index) const {

    CGAL_precondition(item_index < m_item_range.size());
    const auto& key = *(m_item_range.begin() + item_index);
    return get(m_property_map, key);
  }

  friend inline reference get(
    const Item_property_map& item_map,
    const key_type key) {

    return item_map[key];
  }
};

} // namespace KSR
} // namespace CGAL

#endif // CGAL_KSR_PROPERTY_MAP_H