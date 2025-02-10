/*     
sgp4coord.cpp

This file contains miscellaneous functions required for coordinate transformation.
Functions were originally written for Matlab as companion code for "Fundamentals of Astrodynamics 
and Applications" by David Vallado (2007). (w) 719-573-2600, email dvallado@agi.com

Ported to C++ by Grady Hillhouse with some modifications, July 2015.
*/
#include "sgp4coord.h"
#include "sgp4ext.h"
#include "ns3/cppmap3d.hh"
#include <iostream>

#define pi 3.14159265358979323846
/*
site

This function finds the position and velocity vectors for a site. The
outputs are in the ECEF coordinate system. Note that the velocity vector
is zero because the coordinate system rotates with the earth.

Author: David Vallado, 2007
Ported to C++ by Grady Hillhouse with some modifications, July 2015.

INPUTS          DESCRIPTION                     RANGE/UNITS
latgd           Site geodetic latitude          -pi/2 to pi/2 in radians
lon             Longitude                       -2pi to 2pi in radians
alt             Site altitude                   km

OUTPUTS         DESCRIPTION
rs              Site position vector            km
vs              Site velocity vector            km/s
*/

void site(double latgd, double lon, double alt, double rs[3], double vs[3])
{
    double sinlat, re, eesqrd, cearth, rdel, rk;
    re = 6378.137;              //radius of earth in km
    eesqrd = 0.006694385000;    //eccentricity of earth sqrd
    
    //Find rdel and rk components of site vector
    sinlat = sin(latgd);
    cearth = re / sqrt( 1.0 - (eesqrd*sinlat*sinlat) );
    rdel = (cearth + alt) * cos(latgd);
    rk = ((1.0 - eesqrd) * cearth + alt ) * sinlat;
    
    //Find site position vector (ECEF)
    rs[0] = rdel * cos( lon );
    rs[1] = rdel * sin( lon );
    rs[2] = rk;
    
    //Velocity of site is zero because the coordinate system is rotating with the earth
    vs[0] = 0.0;
    vs[1] = 0.0;
    vs[2] = 0.0;
}


/*
rv2azel

This function calculates the range, elevation, and azimuth (and their rates)
from the TEME vectors output by the SGP4 function.

Author: David Vallado, 2007
Ported to C++ by Grady Hillhouse with some modifications, July 2015.

INPUTS          DESCRIPTION                     RANGE/UNITS
ro              Sat. position vector (TEME)     km
vo              Sat. velocity vector (TEME)     km/s
latgd           Site geodetic latitude          -pi/2 to pi/2 in radians
lon             Site longitude                  -2pi to 2pi in radians
alt             Site altitude                   km
jdut1           Julian date                     days

OUTPUTS         DESCRIPTION
razel           Range, azimuth, and elevation matrix
razelrates      Range rate, azimuth rate, and elevation rate matrix
*/

void rv2azel(double recef[3], double vecef[3], double rs[3], double vs[3], double razel[3], double razelrates[3])
{
    //Locals
    double halfpi = pi * 0.5;
    double small  = 0.00000001;
    double temp;
//    double rs[3];
//    double vs[3];
//    double recef[3];
//    double vecef[3];
    double rhoecef[3];
    double drhoecef[3];
    double tempvec[3];
    double rhosez[3];
    double drhosez[3];
    double magrhosez;
    double rho, az, el;
    double drho, daz, del;
    
    //Get site vector in ECEF coordinate system
    //site(latgd, lon, alt, rs, vs);
	double latgd, lon, alt;
	cppmap3d::ecef2geodetic(rs[0]*1e3,rs[1]*1e3,rs[2]*1e3,latgd,lon,alt,cppmap3d::Ellipsoid::WGS72);
	alt = alt/1e3;
//	std::cout<<latgd*180.0/pi<<","<<lon*180.0/pi<<std::endl;
//	double latgd1, lon1, alt1;
//	cppmap3d::ecef2geodetic(recef[0]*1e3,recef[1]*1e3,recef[2]*1e3,latgd1,lon1,alt1,cppmap3d::Ellipsoid::WGS72);
//	alt1 = alt1/1e3;
//	std::cout<<latgd1*180.0/pi<<","<<lon1*180.0/pi<<std::endl;
    
    //Convert TEME vectors to ECEF coordinate system
    //teme2ecef(ro, vo, jdut1, recef, vecef);
    
    //Find ECEF range vectors
    for (int i = 0; i < 3; i++)
    {
        rhoecef[i] = recef[i] - rs[i];
        drhoecef[i] = vecef[i];
    }
    rho = mag(rhoecef); //Range in km
    
    //Convert to SEZ (topocentric horizon coordinate system)
    rot3(rhoecef, lon, tempvec);
    rot2(tempvec, (halfpi-latgd), rhosez);
    
    rot3(drhoecef, lon, tempvec);
    rot2(tempvec, (halfpi-latgd), drhosez);
    
    //Calculate azimuth, and elevation
    temp = sqrt(rhosez[0]*rhosez[0] + rhosez[1]*rhosez[1]);
    if (temp < small)
    {
        el = sgn(rhosez[2]) * halfpi;
        az = atan2(drhosez[1], -drhosez[0]);
    }
    else
    {
        magrhosez = mag(rhosez);
        el = asin(rhosez[2]/magrhosez);
        az = atan2(rhosez[1]/temp, -rhosez[0]/temp);
    }
    
    //Calculate rates for range, azimuth, and elevation
    drho = dot(rhosez,drhosez) / rho;
    
    if(abs(temp*temp) > small)
    {
        daz = (drhosez[0]*rhosez[1] - drhosez[1]*rhosez[0]) / (temp * temp);
    }
    else
    {
        daz = 0.0;
    }
    
    if(abs(temp) > small)
    {
        del = (drhosez[2] - drho*sin(el)) / temp;
    }
    else
    {
        del = 0.0;
    }
    
    //Move values to output vectors
    razel[0] = rho;             //Range (km)
    razel[1] = az;              //Azimuth (radians)
    razel[2] = el;              //Elevation (radians)
    
    razelrates[0] = drho;       //Range rate (km/s)
    razelrates[1] = daz;        //Azimuth rate (rad/s)
    razelrates[2] = del;        //Elevation rate (rad/s)
}

void rot3(double invec[3], double xval, double outvec[3])
{
    double temp = invec[1];
    double c = cos(xval);
    double s = sin(xval);
    
    outvec[1] = c*invec[1] - s*invec[0];
    outvec[0] = c*invec[0] + s*temp;
    outvec[2] = invec[2];
}

void rot2(double invec[3], double xval, double outvec[3])
{
    double temp = invec[2];
    double c = cos(xval);
    double s = sin(xval);
    
    outvec[2] = c*invec[2] + s*invec[0];
    outvec[0] = c*invec[0] - s*temp;
    outvec[1] = invec[1];
}

