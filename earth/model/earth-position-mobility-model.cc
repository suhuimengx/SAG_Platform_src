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

#include "earth-position-mobility-model.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/ptr.h"
#include "ns3/earth.h"
#include "ns3/type-id.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EarthPositionMobilityModel");

NS_OBJECT_ENSURE_REGISTERED (EarthPositionMobilityModel);

TypeId
EarthPositionMobilityModel::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::EarthPositionMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<EarthPositionMobilityModel> ()
    .AddAttribute("EarthPositionHelper",
                  "The earth position helper that holds the earth reference of this node",
				  EarthPositionHelperValue(EarthPositionHelper()),
                  MakeEarthPositionHelperAccessor (&EarthPositionMobilityModel::m_helper),
                  MakeEarthPositionHelperChecker())
  ;

  return tid;
}

EarthPositionMobilityModel::EarthPositionMobilityModel (void) { }
EarthPositionMobilityModel::~EarthPositionMobilityModel (void) { }

//std::string
//EarthPositionMobilityModel::GetSatelliteName (void) const
//{
//  return m_helper.GetSatelliteName ();
//}

Ptr<Earth>
EarthPositionMobilityModel::GetEarth (void) const
{
  return m_helper.GetEarth ();
}

JulianDate
EarthPositionMobilityModel::GetStartTime (void) const
{
  return m_helper.GetStartTime ();
}

void
EarthPositionMobilityModel::SetEarth (Ptr<Earth> earth)
{
  m_helper.SetEarth (earth);
}

void
EarthPositionMobilityModel::SetStartTime (const JulianDate &t)
{
  m_helper.SetStartTime (t);
}

Vector3D
EarthPositionMobilityModel::DoGetPosition (void) const
{
  return m_helper.GetPosition ();
}

void
EarthPositionMobilityModel::DoSetPosition (const Vector3D &position)
{
  // position is not settable
  // NotifyCourseChange ();
}

Vector3D
EarthPositionMobilityModel::DoGetVelocity (void) const
{
  return m_helper.GetVelocity ();
}

Vector3D
EarthPositionMobilityModel::DoGetPositionInECI (void) const
{
  return m_helper.GetPositionInECI ();
}

Vector3D
EarthPositionMobilityModel::DoGetVelocityInECI (void) const
{
  return m_helper.GetVelocityInECI ();
}



}
