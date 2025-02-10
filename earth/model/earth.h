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

#ifndef EARTH_H
#define EARTH_H

#include <stdint.h>
#include <string>
#include <utility>

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/type-id.h"
#include "ns3/vector.h"

#include "ns3/julian-date.h"


namespace ns3 {

class Earth : public Object {
public:
//  /// World Geodetic System (WGS) constants to be used by SGP4/SDP4 models.
//  static const gravconsttype WGeoSys;

  /**
   * @brief Get the type ID.
   * @return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * @brief Default constructor.
   */
  Earth (void);


  /**
   * @brief Retrieve the TLE epoch time.
   * @return the TLE epoch time or 0h, 1 January 1992 if the satellite has not
   *         yet been initialized.
   */
  JulianDate GetEpoch (void) const;
  void SetEpoch(std::string date);

  /**
   * @brief Get the prediction for the sun's position at a given time.
   * @param t When.
   * @return an ns3::Vector3D object containing the sun's position,
   *         in meters, on ITRF coordinate frame.
   */
  void SetPosition (Vector3D position);
  Vector3D GetPosition (const JulianDate &t) const;
  Vector3D GetPositionInECI (const JulianDate &t) const;

  /**
   * @brief Get the prediction for the sun's velocity at a given time.
   * @param t When.
   * @return an ns3::Vector3D object containing the sun's velocity,
   *         in m/s, on ITRF coordinate frame.
   */
  void SetVelocity (Vector3D velocity);
  Vector3D GetVelocity (const JulianDate &t) const;
  Vector3D GetVelocityInECI (const JulianDate &t) const;


//  /**
//   * @brief Get the satellite's orbital period.
//   * @return an ns3::Time object containing the satellite's orbital period.
//   */
//  Time GetOrbitalPeriod (void) const;


private:
  /// row of a Matrix
  struct Row {
    double r[3];

    double& operator[] (uint32_t i) { return r[i]; }
    const double& operator[] (uint32_t i) const { return r[i]; }
  };

  /// Matrix data structure to make coordinate conversion code clearer and
  /// less verbose
  struct Matrix {
  public:
    Matrix (void) { }
    Matrix (
      double c00, double c01, double c02,
      double c10, double c11, double c12,
      double c20, double c21, double c22
    );

    Row& operator[] (uint32_t i) { return m[i]; }
    const Row& operator[] (uint32_t i) const { return m[i]; }

    Vector3D operator* (const Vector3D &v) const;

    Matrix Transpose (void) const;

  private:
    Row m[3];
  };

  /**
   * @brief Retrieve the matrix for converting from PEF to ITRF at a given time.
   * @param t When.
   * @return the PEF-ITRF conversion matrix.
   */
  static Matrix PefToItrf (const JulianDate &t);

  /**
   * @brief Retrieve the matrix for converting from TEME to PEF at a given time.
   * @param t When.
   * @return the TEME-PEF conversion matrix.
   */
  static Matrix TemeToPef (const JulianDate &t);

  /**
   * @brief Retrieve the satellite's position vector in ITRF coordinates.
   * @param t When.
   * @return the satellite's position vector in ITRF coordinates (meters).
   */
  static Vector3D rTemeTorItrf (const Vector3D &rteme, const JulianDate &t);

  /**
   * @brief Retrieve the satellite's velocity vector in ITRF coordinates.
   * @param t When.
   * @return the satellite's velocity vector in ITRF coordinates (m/s).
   */
  static Vector3D rvTemeTovItrf (
    const Vector3D &rteme, const Vector3D &vteme, const JulianDate& t
  );

  JulianDate m_start_epoch;
  Vector3D m_position;  // km
  Vector3D m_velocity;  // km/s

};

}

#endif /* EARTH_H */
