// Copyright (c) 2018 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s) : Simon Giraudot

#ifndef CGAL_PSP_INTERNAL_CALLBACK_WRAPPER_H
#define CGAL_PSP_INTERNAL_CALLBACK_WRAPPER_H

#include <CGAL/license/Point_set_processing_3.h>

#include <functional>

#include <CGAL/thread.h>

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

namespace CGAL {
namespace Point_set_processing_3 {
namespace internal {

template <typename ConcurrencyTag>
class Callback_wrapper
{
  std::size_t m_advancement;
  bool m_interrupted;

public:
  Callback_wrapper (const std::function<bool(double)>&, std::size_t, std::size_t, bool = false) { }
  void reset (std::size_t, std::size_t, bool = false) { }
  std::size_t& advancement() { return m_advancement; }
  bool& interrupted() { return m_interrupted; }
  void join() { }
};

template <>
class Callback_wrapper<CGAL::Sequential_tag>
{
  const std::function<bool(double)>& m_callback;
  std::size_t m_advancement;
  bool m_interrupted;
  std::size_t m_size;

public:

  Callback_wrapper (const std::function<bool(double)>& callback,
                    std::size_t size,
                    std::size_t advancement = 0,
                    bool interrupted = false)
    : m_callback (callback)
    , m_advancement (advancement)
    , m_interrupted (interrupted)
    , m_size (size)
  { }

  Callback_wrapper (const Callback_wrapper& other)
    : m_callback (other.m_callback)
    , m_advancement (other.m_advancement)
    , m_interrupted (other.m_interrupted)
    , m_size (other.m_size)
  { }

  ~Callback_wrapper () { }

  void reset (std::size_t size, std::size_t advancement, bool interrupted = false)
  {
    m_size = size;
    m_advancement = advancement;
    m_interrupted = interrupted;
  }
  std::size_t& advancement()
  {
    return m_advancement;
  }

  bool& interrupted()
  {
    if (m_callback)
      m_interrupted = !(m_callback(m_advancement / double(m_size)));
    return m_interrupted;
  }

  void join() { }
};

#ifdef CGAL_LINKED_WITH_TBB
template <>
class Callback_wrapper<CGAL::Parallel_tag>
{
  const std::function<bool(double)>& m_callback;
  std::shared_ptr<std::atomic<std::size_t> > m_advancement;
  std::shared_ptr<std::atomic<bool> > m_interrupted;
  std::size_t m_size;
  std::unique_ptr<std::thread> m_thread;

  // assignment operator shouldn't be used (m_callback is const ref)
  Callback_wrapper& operator= (const Callback_wrapper&)
  {
    return *this;
  }

public:
  Callback_wrapper (const std::function<bool(double)>& callback,
                     std::size_t size,
                     std::size_t advancement = 0,
                     bool interrupted = false)
    : m_callback (callback)
    , m_advancement (std::make_shared<std::atomic<std::size_t> ())
    , m_interrupted (std::make_shared<std::atomic<bool> >())
    , m_size (size)
  {
    // cpp11::atomic only has default constructor, initialization done in two steps
    *m_advancement = advancement;
    *m_interrupted = interrupted;
    if (m_callback)
      m_thread = std::make_unique<std::thread> (*this);
  }

  Callback_wrapper (const Callback_wrapper& other)
    : m_callback (other.m_callback)
    , m_advancement (other.m_advancement)
    , m_interrupted (other.m_interrupted)
    , m_size (other.m_size)
  {

  }

  void reset (std::size_t size, std::size_t advancement, bool interrupted = false)
  {
    m_size = size;
    *m_advancement = advancement;
    *m_interrupted = interrupted;
    if (m_callback)
      m_thread.reset(*this);
  }

  std::atomic<std::size_t>& advancement() { return *m_advancement; }
  std::atomic<bool>& interrupted() { return *m_interrupted; }
  void join()
  {
    if (m_thread != nullptr)
      m_thread->join();
  }

  void operator()()
  {
    while (*m_advancement != m_size)
    {
      if (m_callback && !m_callback (*m_advancement / double(m_size)))
        *m_interrupted = true;
      if (*m_interrupted)
        return;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    if (m_callback)
      m_callback (1.);
  }
};
#endif

} // namespace internal
} // namespace Point_set_processing_3
} // namespace CGAL

#endif // CGAL_PSP_INTERNAL_CALLBACK_WRAPPER_H
