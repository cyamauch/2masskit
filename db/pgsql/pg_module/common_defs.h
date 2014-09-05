#ifndef _COMMON_DEFS_H
#define _COMMON_DEFS_H 1


#include <wcs.h>
#include <math.h>

#include "common_conv.h"

/* note: args are in radian */
static double get_max_distance_rad_for_box_cel( double r_lon, double r_lat,
						double off_lon, double off_lat )
{
    double x, y, z;
    double x1, y1, z1;
    double yyzz, mxyz;

    y = sin(r_lon);
    z = sin(r_lat);
    yyzz = y*y + z*z;
    if ( 1.0 < yyzz ) yyzz = 1.0;
    x = sqrt(1.0 - yyzz);

    y1 = sin(off_lon);
    z1 = sin(off_lat);
    yyzz = y1*y1 + z1*z1;
    if ( 1.0 < yyzz ) yyzz = 1.0;
    x1 = sqrt(1.0 - yyzz);

    mxyz = x1*x + y1*y + z1*z;
    if ( 1.0 < mxyz ) mxyz = 1.0;
    else if ( mxyz < -1.0 ) mxyz = -1.0;

    return acos( mxyz );	/* in radian */
}

/* note: r_lon and r_lat are in radian */
static double get_max_distance_rad_for_box_cel_div( double r_lon, int i_lon, int ndiv_lon,
						    double r_lat, int i_lat, int ndiv_lat )
{
    /* edge */
    double a0 = -r_lon + 2.0*r_lon*((double)i_lon/ndiv_lon);
    double a1 = -r_lat + 2.0*r_lat*((double)i_lat/ndiv_lat);
    /* center */
    double a2 = -r_lon + 2.0*r_lon*(((double)i_lon + 0.5)/ndiv_lon);
    double a3 = -r_lat + 2.0*r_lat*(((double)i_lat + 0.5)/ndiv_lat);

    return get_max_distance_rad_for_box_cel( a0, a1, a2, a3 );
}

/* note: lon, r_lon, lat and r_lat are in radian */
static void get_xyz_center_for_box_cel_div( unsigned char coo_ch, 
					    double lon, double r_lon, int i_lon, int ndiv_lon,
					    double lat, double r_lat, int i_lat, int ndiv_lat,
					    double *rt_x, double *rt_y, double *rt_z )
{
    double lon_rot = -r_lon + 2.0*r_lon*(((double)i_lon + 0.5)/ndiv_lon);
    double lat_rot = -r_lat + 2.0*r_lat*(((double)i_lat + 0.5)/ndiv_lat);
    double x0,y0,z0, x,y,z, cos_v, sin_v;
    double rt_lon, rt_lat, rot;

    /* to xyz */
    x0 = cos(lon_rot);
    y0 = sin(lon_rot);
    z0 = 0;

    /* rotate around y-axis */
    rot = lat_rot+lat;
    cos_v = cos(rot);
    sin_v = sin(rot);
    x = x0*cos_v - z0*sin_v;
    y = y0;
    z = x0*sin_v + z0*cos_v;

    x0 = x;
    y0 = y;
    z0 = z;

    /* rotate around z-axis */
    cos_v = cos(lon);
    sin_v = sin(lon);
    x = x0*cos_v - y0*sin_v;
    y = x0*sin_v + y0*cos_v;
    z = z0;

    rt_lon = atan2(y,x) * CC_RAD_TO_DEG;
    rt_lat = atan2(z,sqrt(x*x + y*y)) * CC_RAD_TO_DEG;
    if ( rt_lon < 0 ) rt_lon += 360.0;

    if ( coo_ch == 'j' ) {
      /* do nothing */
    }
    else if ( coo_ch == 'g' )
	wcscon(WCS_GALACTIC, WCS_J2000,
	       2000.0, 2000.0,
	       &rt_lon, &rt_lat, 2000.0);
    else if ( coo_ch == 'e' )
	wcscon(WCS_ECLIPTIC, WCS_J2000,
	       2000.0, 2000.0,
	       &rt_lon, &rt_lat, 2000.0);
    else if ( coo_ch == 'b' )
	wcscon(WCS_B1950, WCS_J2000,
	       1950.0, 2000.0,
	       &rt_lon, &rt_lat, 2000.0);
    else if ( coo_ch == 'i' )
	wcscon(WCS_ICRS, WCS_J2000,
	       2000.0, 2000.0,
	       &rt_lon, &rt_lat, 2000.0);

    rt_lon *= CC_DEG_TO_RAD;
    rt_lat *= CC_DEG_TO_RAD;

    sin_v = sin(rt_lat);
    cos_v = cos(rt_lat);
    *rt_x = cos(rt_lon) * cos_v;
    *rt_y = sin(rt_lon) * cos_v;
    *rt_z =		  sin_v;

    return;
}

static void cel_to_j2000(unsigned char ch, float8 *lon, float8 *lat)
{
    if ( ch == 'j' ) return;
    else if ( ch == 'g' ) 
        wcscon(WCS_GALACTIC, WCS_J2000,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    else if ( ch == 'e' ) 
        wcscon(WCS_ECLIPTIC, WCS_J2000,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    else if ( ch == 'b' )
        wcscon(WCS_B1950, WCS_J2000,
	       1950.0, 2000.0,
	       lon, lat, 2000.0);
    else if ( ch == 'i' )
        wcscon(WCS_ICRS, WCS_J2000,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    return;
}


#endif
