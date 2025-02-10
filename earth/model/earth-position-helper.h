/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 INESC TEC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */


#ifndef EARTH_POSITION_HELPER_MODEL_H
#define EARTH_POSITION_HELPER_MODEL_H

#include "ns3/ptr.h"
#include "ns3/vector.h"
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"

#include "ns3/earth.h"

namespace ns3 {

/**
 * \ingroup mobility
 *
 * @brief Utility class used to interface between EarthPositionMobilityModel
 *        and Earth classes.
 */
class EarthPositionHelper {
public:
  /**
   * @brief Default constructor.
   */
	EarthPositionHelper (void);

  /**
   * @brief Create object and set earth.
   * @param earth earth object with DE430 DE431 propagation models built-in.
   */
	EarthPositionHelper (Ptr<Earth> earth);

  /**
   * @brief Create object, and set earth and simulation's start time.
   * @param earth earth object with DE430 DE431 propagation models built-in.
   * @param t the time instant to be considered as simulation start.
   */
	EarthPositionHelper (Ptr<Earth> earth, const JulianDate &t);

  /**
   * @brief Get current orbital position vector (x, y, z).
   * @return orbital position vector.
   */
  Vector3D GetPosition (void) const;

  /**
   * @brief Get current orbital position vector (x, y, z) in ECI.
   * @return orbital position vector in ECI.
   */
  Vector3D GetPositionInECI (void) const;

  /**
   * @brief Get orbital velocity.
   * @return orbital velocity vector.
   */
  Vector3D GetVelocity (void) const;

  /**
   * @brief Get orbital velocity in ECI.
   * @return orbital velocity vector in ECI.
   */
  Vector3D GetVelocityInECI (void) const;

//  /**
//   * @brief Get satellite's name.
//   * @return satellite's name or an empty string if the satellite object is not
//   *         yet set.
//   */
//  std::string GetSatelliteName (void) const;

  /**
   * @brief Get the underlying earth object.
   * @return the underlying earth object.
   */
  Ptr<Earth> GetEarth (void) const;

  /**
   * @brief Get the time set to be considered as simulation's start time.
   * @return the absolute time set to be considered as simulation's start time.
   */
  JulianDate GetStartTime (void) const;

  /**
   * @brief Set the underlying earth object.
   * @param earth object with DE430 DE431 propagation models built-in.
   */
  void SetEarth (Ptr<Earth> earth);

  /**
   * @brief Set simulation's absolute start time.
   * @param t time instant to be consider as simulation's start.
   */
  void SetStartTime(const JulianDate &t);

private:
  Ptr<Earth> m_earth;               //!< pointer to the Earth object.
  JulianDate m_start;                         //!< simulation's absolute start time.
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param size the earth position helper
 * \returns a reference to the stream
 */
std::ostream &operator << (std::ostream &os, const EarthPositionHelper &earthPositionHelper);

/**
 * \brief Stream extraction operator.
 *
 * \param is the stream
 * \param size the earth position helper
 * \returns a reference to the stream
 */
std::istream &operator >> (std::istream &is, EarthPositionHelper &earthPositionHelper);

ATTRIBUTE_HELPER_HEADER (EarthPositionHelper);

} // namespace ns3

#endif
