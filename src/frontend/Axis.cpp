/*
Copyright (c) 2018, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of trase.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "frontend/Axis.hpp"
#include "frontend/Histogram.hpp"
#include "frontend/Line.hpp"
#include "frontend/Plot1D.hpp"
#include "frontend/Points.hpp"
#include "util/Vector.hpp"

namespace trase {

Axis::Axis(Drawable *parent, const bfloat2_t &area)
    : Drawable(parent, area), m_sig_digits(2), m_nx_ticks(0), m_ny_ticks(0),
      m_tick_len(10.f), m_line_width(3.f), m_font_size(18.f),
      m_font_face("Roboto"), m_legend(false) {}

std::shared_ptr<Plot1D> Axis::plot(int n) {
  return std::dynamic_pointer_cast<Plot1D>(m_children.at(n));
}

std::shared_ptr<Plot1D> Axis::plot_impl(const std::shared_ptr<Plot1D> &plot,
                                        const Transform &transform,
                                        const DataWithAesthetic &values) {
  plot->set_transform(transform);
  plot->add_frame(values, 0);
  plot->set_color(RGBA::defaults[m_children.size()]);
  plot->resize(m_pixels);
  m_children.push_back(plot);
  return plot;
}

std::shared_ptr<Plot1D> Axis::points(const DataWithAesthetic &data,
                                     const Transform &transform) {
  return plot_impl(std::make_shared<Points>(this), transform, data);
}

std::shared_ptr<Plot1D> Axis::line(const DataWithAesthetic &data,
                                   const Transform &transform) {
  return plot_impl(std::make_shared<Line>(this), transform, data);
}

std::shared_ptr<Plot1D> Axis::histogram(const DataWithAesthetic &data,
                                        const Transform &transform) {
  return plot_impl(std::make_shared<Histogram>(this), transform, data);
}

void Axis::update_tick_information() {

  // Use num ticks if user defined, or calculate with defaults
  vfloat2_t n_ticks = calculate_num_ticks();

  // Calculate ideal distance between ticks in limits coords
  bfloat2_t xy_limits(
      {m_limits.bmin[Aesthetic::x::index], m_limits.bmin[Aesthetic::y::index]},
      {m_limits.bmax[Aesthetic::x::index], m_limits.bmax[Aesthetic::y::index]});

  // if any limits are empty (no values) use a sensible default (0 -> 1)
  for (int i = 0; i < 2; ++i) {
    if (xy_limits.bmax[i] < xy_limits.bmin[i]) {
      xy_limits.bmin[i] = 0.f;
      xy_limits.bmax[i] = 1.f;
    }
  }

  // work out a sensible spacing between ticks, based on the number of ticks
  const vfloat2_t tick_dx =
      round_off(xy_limits.delta() / n_ticks, m_sig_digits);

  // Idealise the lowest pick position
  const vfloat2_t tick_min = ceil(xy_limits.bmin / tick_dx) * tick_dx;

  // scale to pixels
  const vfloat2_t tick_dx_pixels =
      tick_dx * m_pixels.delta() / xy_limits.delta();
  const vfloat2_t tick_min_pixels = {to_display<Aesthetic::x>(tick_min[0]),
                                     to_display<Aesthetic::y>(tick_min[1])};

  // Update values in the struct
  m_tick_info.clear();

  // x tick values and positions
  for (int i = 0; i < static_cast<int>(n_ticks[0]); ++i) {
    m_tick_info.x_val.emplace_back(tick_min[0] + i * tick_dx[0]);
    m_tick_info.x_pos.emplace_back(tick_min_pixels[0] + i * tick_dx_pixels[0]);
  }

  // y tick values and positions
  for (int i = 0; i < static_cast<int>(n_ticks[1]); ++i) {
    m_tick_info.y_val.emplace_back(tick_min[1] + i * tick_dx[1]);
    m_tick_info.y_pos.emplace_back(tick_min_pixels[1] - i * tick_dx_pixels[1]);
  }
}

vfloat2_t Axis::calculate_num_ticks() {

  if (m_nx_ticks > 0 && m_ny_ticks > 0) {
    return {static_cast<float>(m_nx_ticks), static_cast<float>(m_ny_ticks)};
  }

  const float pix_ratio = m_pixels.delta()[0] / m_pixels.delta()[1];

  if (m_nx_ticks > 0) {
    const auto n = static_cast<float>(m_nx_ticks);
    return {n, std::floor(n / pix_ratio)};
  }

  if (m_ny_ticks > 0) {
    const auto n = static_cast<float>(m_ny_ticks);
    return {std::floor(n * pix_ratio), n};
  }

  return {std::floor(5.f * pix_ratio), 5.f};
}

} // namespace trase
