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

#ifndef EARTH_POSITION_MOBILITY_MODEL_H
#define EARTH_POSITION_MOBILITY_MODEL_H

#include <string>

#include "earth-position-helper.h"
#include "ns3/julian-date.h"
#include "ns3/mobility-model.h"
#include "ns3/ptr.h"
#include "ns3/earth.h"
#include "ns3/type-id.h"


namespace ns3 {

class EarthPositionMobilityModel : public MobilityModel {
public:
  /**
   * @brief Get the type ID.
   * @return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * @brief Default constructor.
   */
  EarthPositionMobilityModel (void);

  /**
   * @brief Destructor.
   */
  virtual ~EarthPositionMobilityModel (void);

//  /**
//   * @brief Retrieve satellite's name.
//   * @return the satellite's name or an empty string if has not yet been set.
//   */
//  std::string GetSatelliteName (void) const;

  /**
   * @brief Get the underlying Earth object.
   * @return a pointer to the underlying Earth object.
   */
  Ptr<Earth> GetEarth (void) const;

  /**
   * @brief Get the time instant considered as the simulation start.
   * @return a JulianDate object with the time considered as simulation start.
   */
  JulianDate GetStartTime (void) const;

  /**
   * @brief Set the underlying Earth object.
   * @param sat a pointer to the Earth object to be used.
   */
  void SetEarth (Ptr<Earth> earth);

  /**
   * @brief Set the time instant considered as the simulation start.
   * @param t the time instant to be considered as simulation start.
   */
  void SetStartTime (const JulianDate &t);

  virtual Vector DoGetVelocityInECI (void) const;
  virtual Vector DoGetPositionInECI (void) const;

private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;

  EarthPositionHelper m_helper;     //!< helper for orbital computations
};

} // namespace ns3

#endif /* EARTH_POSITION_MOBILITY_MODEL_H */
