/*
 * Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
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
 */

#include "three-gpp-antenna-model.h"

#include "antenna-model.h"

#include <ns3/double.h>
#include <ns3/log.h>

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ThreeGppAntennaModel");

NS_OBJECT_ENSURE_REGISTERED(ThreeGppAntennaModel);

TypeId
ThreeGppAntennaModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppAntennaModel")
                            .SetParent<AntennaModel>()
                            .SetGroupName("Antenna")
                            .AddConstructor<ThreeGppAntennaModel>();
    return tid;
}

ThreeGppAntennaModel::ThreeGppAntennaModel()
    : m_verticalBeamwidthDegrees{65},
      m_horizontalBeamwidthDegrees{65},
      m_aMax{30},
      m_slaV{30},
      m_geMax{8.0}
{
}

ThreeGppAntennaModel::~ThreeGppAntennaModel()
{
}

double
ThreeGppAntennaModel::GetVerticalBeamwidth() const
{
    return m_verticalBeamwidthDegrees;
}

double
ThreeGppAntennaModel::GetHorizontalBeamwidth() const
{
    return m_horizontalBeamwidthDegrees;
}

double
ThreeGppAntennaModel::GetSlaV() const
{
    return m_slaV;
}

double
ThreeGppAntennaModel::GetMaxAttenuation() const
{
    return m_aMax;
}

double
ThreeGppAntennaModel::GetAntennaElementGain() const
{
    return m_geMax;
}

//xhqin
double
calculateAzimuth1(const Vector3D& a, const Vector3D& b) {
	 // 计算地面站到卫星的向量
	 double deltaX = b.x - a.x;
	 double deltaY = b.y - a.y;

	 // 计算地面站位置在地球表面的投影
	 double azimuthRadians = atan2(deltaY, deltaX);
	 //double azimuthDegrees = RadiansToDegrees(azimuthRadians);

	 // 确保方位角在 [-π, π] 范围内
	if (azimuthRadians < -M_PI) {
		azimuthRadians += 2 * M_PI;
	} else if (azimuthRadians > M_PI) {
		azimuthRadians -= 2 * M_PI;
	}
	 // 确保方位角在0到360度之间
	//if (azimuthRadians < 0) {
	//	azimuthRadians += M_PI;
	//}
	 return azimuthRadians;
 }
double
calculateInclination1(const Vector3D& a, const Vector3D& b) {
	 // 计算地面站到卫星的向量
	 double deltaX = b.x - a.x;
	 double deltaY = b.y - a.y;
	 double deltaZ = b.z - a.z;
	 // 计算向量的长度（模）
	 double vectorLength = std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);

	 // 计算向量在地平面上的投影长度
	 double projectionLength = std::sqrt(deltaX * deltaX + deltaY * deltaY);

	 // 计算倾角（弧度）
	 double inclinationRadians = std::acos(projectionLength  / vectorLength);

	 if (inclinationRadians < 0) {
		 inclinationRadians += M_PI;
	 } else if (inclinationRadians > M_PI) {
		 inclinationRadians -= M_PI;
	 }
	 // 转换为度数
	 //double inclinationDegrees = RadiansToDegrees(inclinationRadians);

	 return inclinationRadians;

 }
double
ThreeGppAntennaModel::GetGainDb(const Vector3D& a, const Vector3D& b)
{
	 double azimuth= calculateAzimuth1(a, b);
	 double inclination= calculateInclination1(a, b);
	 return GetGainDb(Angles(azimuth,inclination));
}



double
ThreeGppAntennaModel::GetGainDb(Angles a)
{
    NS_LOG_FUNCTION(this << a);

    double phiDeg = RadiansToDegrees(a.GetAzimuth());
    double thetaDeg = RadiansToDegrees(a.GetInclination());

    NS_ASSERT_MSG(-180.0 <= phiDeg && phiDeg <= 180.0, "Out of boundaries: phiDeg=" << phiDeg);
    NS_ASSERT_MSG(0.0 <= thetaDeg && thetaDeg <= 180.0, "Out of boundaries: thetaDeg=" << thetaDeg);

    // compute the radiation power pattern using equations in table 7.3-1 in
    // 3GPP TR 38.901
    double vertGain = -std::min(m_slaV,
                                12 * pow((thetaDeg - 90) / m_verticalBeamwidthDegrees,
                                         2)); // vertical cut of the radiation power pattern (dB)
    double horizGain = -std::min(m_aMax,
                                 12 * pow(phiDeg / m_horizontalBeamwidthDegrees,
                                          2)); // horizontal cut of the radiation power pattern (dB)

    double gainDb =
        m_geMax - std::min(m_aMax, -(vertGain + horizGain)); // 3D radiation power pattern (dB)

    NS_LOG_DEBUG("gain=" << gainDb << " dB");
    return gainDb;
}

} // namespace ns3
