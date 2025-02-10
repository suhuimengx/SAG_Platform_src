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

#include <algorithm>
#include <cstring>
#include <stdint.h>
#include <string>
#include <utility>

#include "earth.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/type-id.h"
#include "ns3/vector.h"

#include "ns3/vector-extensions.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Earth");

NS_OBJECT_ENSURE_REGISTERED (Earth);

//const gravconsttype Satellite::WGeoSys = wgs72;   // recommended for SGP4/SDP4

TypeId
Earth::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Earth")
    .SetParent<Object> ()
    .SetGroupName ("Earth")
    .AddConstructor<Earth> ();

  return tid;
}

Earth::Earth (void)
{
  NS_LOG_FUNCTION_NOARGS ();

}

JulianDate
Earth::GetEpoch (void) const {

	return m_start_epoch;

}

void
Earth::SetEpoch(std::string date){

	m_start_epoch = JulianDate(date);

}

void
Earth::SetPosition (Vector3D position)
{
	m_position = position;
}

Vector3D
Earth::GetPosition (const JulianDate &t) const
{
	return rTemeTorItrf (m_position, t)*1000;
}

Vector3D
Earth::GetPositionInECI (const JulianDate &t) const{
	return Vector3D();
}

void
Earth::SetVelocity (Vector3D velocity)
{
	m_velocity = velocity;
}

Vector3D
Earth::GetVelocity (const JulianDate &t) const
{

	return 1000*rvTemeTovItrf (
			m_position, m_velocity, t
	);
}

Vector3D
Earth::GetVelocityInECI (const JulianDate &t) const
{
	return Vector3D();
}


//Time
//Satellite::GetOrbitalPeriod (void) const
//{
//  if (!IsInitialized ())
//    return MilliSeconds (0);
//
//  // rad/min -> min [rad/(rad/min) = rad*min/rad = min]
//  return MilliSeconds (60000*2*M_PI/m_sgp4_record.no);
//}

Earth::Matrix
Earth::PefToItrf (const JulianDate &t)
{
  std::pair<double, double> eop = t.GetPolarMotion ();

  const double &xp = eop.first, &yp = eop.second;
  const double cosxp = cos (xp), cosyp = cos (yp);
  const double sinxp = sin (xp), sinyp = sin (yp);

  // [from AIAA-2006-6753 Report, Page 32, Appendix C - TEME Coordinate System]
  //
  // Matrix(ITRF<->PEF) = ROT1(yp)*ROT2(xp) [using c for cos, and s for sin]
  //
  // | 1    0     0   |*| c(xp) 0 -s(xp) |=|    c(xp)       0      -s(xp)   |
  // | 0  c(yp) s(yp) | |   0   1    0   | | s(yp)*s(xp)  c(yp) s(yp)*c(xp) |
  // | 0 -s(yp) c(yp) | | s(xp) 0  c(xp) | | c(yp)*s(xp) -s(yp) c(yp)*c(xp) |
  //

  // we return the transpose because it is what's needed
  return Matrix(
     cosxp, sinyp*sinxp, cosyp*sinxp,
       0,      cosyp,      -sinyp,
    -sinxp, sinyp*cosxp, cosyp*cosxp
  );
}

Earth::Matrix
Earth::TemeToPef (const JulianDate &t)
{
  const double gmst = t.GetGmst ();
  const double cosg = cos (gmst), sing = sin (gmst);

  // [from AIAA-2006-6753 Report, Page 32, Appendix C - TEME Coordinate System]
  //
  // rPEF = ROT3(gmst)*rTEME
  //
  // |  cos(gmst) sin(gmst) 0 |
  // | -sin(gmst) cos(gmst) 0 |
  // |      0         0     1 |
  //

  return Matrix(
     cosg, sing, 0,
    -sing, cosg, 0,
       0,    0,  1
  );
}

Vector3D
Earth::rTemeTorItrf (const Vector3D &rteme, const JulianDate &t)
{
  Matrix pmt = PefToItrf (t);                   // PEF->ITRF matrix transposed
  Matrix tmt = TemeToPef (t);                   // TEME->PEF matrix

  return pmt*(tmt*rteme);
}

Vector3D
Earth::rvTemeTovItrf(
  const Vector3D &rteme, const Vector3D &vteme, const JulianDate &t
)
{
  Matrix pmt = PefToItrf (t);                   // PEF->ITRF matrix transposed
  Matrix tmt = TemeToPef (t);                   // TEME->PEF matrix
  Vector3D w (0.0, 0.0, t.GetOmegaEarth ());

  return pmt*((tmt*vteme) - CrossProduct (w, tmt*rteme));
}

Earth::Matrix::Matrix (
  double c00, double c01, double c02,
  double c10, double c11, double c12,
  double c20, double c21, double c22
)
{
  m[0][0] = c00;
  m[0][1] = c01;
  m[0][2] = c02;
  m[1][0] = c10;
  m[1][1] = c11;
  m[1][2] = c12;
  m[2][0] = c20;
  m[2][1] = c21;
  m[2][2] = c22;
}

Vector3D
Earth::Matrix::operator* (const Vector3D& v) const
{
  return Vector3D (
    m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
    m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
    m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z
  );
}

Earth::Matrix
Earth::Matrix::Transpose (void) const
{
  return Matrix(
    m[0][0], m[1][0], m[2][0],
    m[0][1], m[1][1], m[2][1],
    m[0][2], m[1][2], m[2][2]
  );
}

}
