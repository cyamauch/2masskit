/* -*- Mode: C ; Coding: euc-japan -*- */
/* Time-stamp: <2012-02-07 11:32:43 cyamauch> */

#include <postgres.h>
#include <fmgr.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* */

#include "common_defs.h"

/* */

PG_FUNCTION_INFO_V1(b2ra);
PG_FUNCTION_INFO_V1(b2dec);
PG_FUNCTION_INFO_V1(i2ra);
PG_FUNCTION_INFO_V1(i2dec);
PG_FUNCTION_INFO_V1(e2ra);
PG_FUNCTION_INFO_V1(e2dec);
PG_FUNCTION_INFO_V1(g2ra);
PG_FUNCTION_INFO_V1(g2dec);
PG_FUNCTION_INFO_V1(cel2ra);
PG_FUNCTION_INFO_V1(cel2dec);

PG_FUNCTION_INFO_V1(j2ra1950);
PG_FUNCTION_INFO_V1(j2dec1950);
PG_FUNCTION_INFO_V1(j2alpha);
PG_FUNCTION_INFO_V1(j2delta);
PG_FUNCTION_INFO_V1(j2lambda);
PG_FUNCTION_INFO_V1(j2beta);
PG_FUNCTION_INFO_V1(j2l);
PG_FUNCTION_INFO_V1(j2b);
PG_FUNCTION_INFO_V1(j2lon);
PG_FUNCTION_INFO_V1(j2lat);
PG_FUNCTION_INFO_V1(_ji2lon);
PG_FUNCTION_INFO_V1(_ji2lat);

PG_FUNCTION_INFO_V1(eq2x);
PG_FUNCTION_INFO_V1(eq2y);
PG_FUNCTION_INFO_V1(eq2z);
PG_FUNCTION_INFO_V1(arcmin2rad);
PG_FUNCTION_INFO_V1(rad2arcmin);
PG_FUNCTION_INFO_V1(distancearcminxyz);
PG_FUNCTION_INFO_V1(_distancearcmineqixyz);
PG_FUNCTION_INFO_V1(distancearcmineq);
PG_FUNCTION_INFO_V1(getmaxdistanceradforboxcel);
PG_FUNCTION_INFO_V1(evalpositioninboxcel);
PG_FUNCTION_INFO_V1(_evalpositioninboxceldiv);
PG_FUNCTION_INFO_V1(_getsqlforboxsearchdiv);

PG_FUNCTION_INFO_V1(_eqi2xi16);
PG_FUNCTION_INFO_V1(_eqi2yi16);
PG_FUNCTION_INFO_V1(_eqi2zi16);

PG_FUNCTION_INFO_V1(_loni2lon);
PG_FUNCTION_INFO_V1(_lati2lat);

PG_FUNCTION_INFO_V1(_texteq4tblsel);

extern Datum b2ra(PG_FUNCTION_ARGS);
extern Datum b2dec(PG_FUNCTION_ARGS);
extern Datum i2ra(PG_FUNCTION_ARGS);
extern Datum i2dec(PG_FUNCTION_ARGS);
extern Datum e2ra(PG_FUNCTION_ARGS);
extern Datum e2dec(PG_FUNCTION_ARGS);
extern Datum g2ra(PG_FUNCTION_ARGS);
extern Datum g2dec(PG_FUNCTION_ARGS);
extern Datum cel2ra(PG_FUNCTION_ARGS);
extern Datum cel2dec(PG_FUNCTION_ARGS);

extern Datum j2ra1950(PG_FUNCTION_ARGS);
extern Datum j2dec1950(PG_FUNCTION_ARGS);
extern Datum j2alpha(PG_FUNCTION_ARGS);
extern Datum j2delta(PG_FUNCTION_ARGS);
extern Datum j2lambda(PG_FUNCTION_ARGS);
extern Datum j2beta(PG_FUNCTION_ARGS);
extern Datum j2l(PG_FUNCTION_ARGS);
extern Datum j2b(PG_FUNCTION_ARGS);
extern Datum j2lon(PG_FUNCTION_ARGS);
extern Datum j2lat(PG_FUNCTION_ARGS);
extern Datum _ji2lon(PG_FUNCTION_ARGS);
extern Datum _ji2lat(PG_FUNCTION_ARGS);

extern Datum eq2x(PG_FUNCTION_ARGS);
extern Datum eq2y(PG_FUNCTION_ARGS);
extern Datum eq2z(PG_FUNCTION_ARGS);
extern Datum arcmin2rad(PG_FUNCTION_ARGS);
extern Datum rad2arcmin(PG_FUNCTION_ARGS);
extern Datum distancearcminxyz(PG_FUNCTION_ARGS);
extern Datum _distancearcmineqixyz(PG_FUNCTION_ARGS);
extern Datum distancearcmineq(PG_FUNCTION_ARGS);
extern Datum getmaxdistanceradforboxcel(PG_FUNCTION_ARGS);
extern Datum evalpositioninboxcel(PG_FUNCTION_ARGS);
extern Datum _evalpositioninboxceldiv(PG_FUNCTION_ARGS);
extern Datum _getsqlforboxsearchdiv(PG_FUNCTION_ARGS);

extern Datum _eqi2xi16(PG_FUNCTION_ARGS);
extern Datum _eqi2yi16(PG_FUNCTION_ARGS);
extern Datum _eqi2zi16(PG_FUNCTION_ARGS);

extern Datum _loni2lon(PG_FUNCTION_ARGS);
extern Datum _lati2lat(PG_FUNCTION_ARGS);

extern Datum _texteq4tblsel(PG_FUNCTION_ARGS);


Datum b2ra(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_B1950, WCS_J2000,
	   1950.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum b2dec(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_B1950, WCS_J2000,
	   1950.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}


Datum i2ra(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_ICRS, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum i2dec(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_ICRS, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}


Datum e2ra(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_ECLIPTIC, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum e2dec(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_ECLIPTIC, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}


Datum g2ra(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_GALACTIC, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum g2dec(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_GALACTIC, WCS_J2000,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}

Datum cel2ra(PG_FUNCTION_ARGS)
{
    text *arg_val = PG_GETARG_TEXT_P(0);
    float8 lon = PG_GETARG_FLOAT8(1);
    float8 lat = PG_GETARG_FLOAT8(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    cel_to_j2000(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lon); 
} 

Datum cel2dec(PG_FUNCTION_ARGS)
{
    text *arg_val = PG_GETARG_TEXT_P(0);
    float8 lon = PG_GETARG_FLOAT8(1);
    float8 lat = PG_GETARG_FLOAT8(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    cel_to_j2000(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lat); 
} 

Datum j2ra1950(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_B1950,
	   2000.0, 1950.0,
	   &lon, &lat, 1950.0);
    PG_RETURN_FLOAT8(lon);
}

Datum j2dec1950(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_B1950,
	   2000.0, 1950.0,
	   &lon, &lat, 1950.0);
    PG_RETURN_FLOAT8(lat);
}


Datum j2alpha(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_ICRS,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum j2delta(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_ICRS,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}


Datum j2lambda(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_ECLIPTIC,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum j2beta(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_ECLIPTIC,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}


Datum j2l(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_GALACTIC,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lon);
}

Datum j2b(PG_FUNCTION_ARGS)
{
    float8 lon, lat;
    lon = PG_GETARG_FLOAT8(0);
    lat = PG_GETARG_FLOAT8(1);
    wcscon(WCS_J2000, WCS_GALACTIC,
	   2000.0, 2000.0,
	   &lon, &lat, 2000.0);
    PG_RETURN_FLOAT8(lat);
}

static void j2000_to_cel(unsigned char ch, float8 *lon, float8 *lat)
{
    if ( ch == 'j' ) {
	return;
    }
    else if ( ch == 'g' ) 
        wcscon(WCS_J2000, WCS_GALACTIC,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    else if ( ch == 'e' ) 
        wcscon(WCS_J2000, WCS_ECLIPTIC,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    else if ( ch == 'b' )
        wcscon(WCS_J2000, WCS_B1950,
	       2000.0, 1950.0,
	       lon, lat, 1950.0);
    else if ( ch == 'i' )
        wcscon(WCS_J2000, WCS_ICRS,
	       2000.0, 2000.0,
	       lon, lat, 2000.0);
    return;
}

Datum j2lon(PG_FUNCTION_ARGS)
{
    float8 lon = PG_GETARG_FLOAT8(0);
    float8 lat = PG_GETARG_FLOAT8(1);
    text *arg_val = PG_GETARG_TEXT_P(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    j2000_to_cel(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lon);
}

Datum j2lat(PG_FUNCTION_ARGS)
{
    float8 lon = PG_GETARG_FLOAT8(0);
    float8 lat = PG_GETARG_FLOAT8(1);
    text *arg_val = PG_GETARG_TEXT_P(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    j2000_to_cel(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lat);
}


Datum _ji2lon(PG_FUNCTION_ARGS)
{
    float8 lon = RAI2RA(PG_GETARG_INT32(0));
    float8 lat = DECI2DEC(PG_GETARG_INT32(1));
    text *arg_val = PG_GETARG_TEXT_P(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    j2000_to_cel(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lon);
}

Datum _ji2lat(PG_FUNCTION_ARGS)
{
    float8 lon = RAI2RA(PG_GETARG_INT32(0));
    float8 lat = DECI2DEC(PG_GETARG_INT32(1));
    text *arg_val = PG_GETARG_TEXT_P(2);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    unsigned char ch_val = 'j';
    int i;
    for ( i=0 ; i < len_val ; i++ ) {
        if ( str_val[i] != ' ' ) break;
    }
    if ( i < len_val ) {
        ch_val = str_val[i];
        /* to lower */
        if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
    }
    j2000_to_cel(ch_val, &lon, &lat);
    PG_RETURN_FLOAT8(lat);
}


Datum eq2x(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    r = PG_GETARG_FLOAT8(0);
    d = PG_GETARG_FLOAT8(1);
    r *= CC_DEG_TO_RAD;
    d *= CC_DEG_TO_RAD;
    v = cos(r) * cos(d);
    PG_RETURN_FLOAT8(v);
}

Datum eq2y(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    r = PG_GETARG_FLOAT8(0);
    d = PG_GETARG_FLOAT8(1);
    r *= CC_DEG_TO_RAD;
    d *= CC_DEG_TO_RAD;
    v = sin(r) * cos(d);
    PG_RETURN_FLOAT8(v);
}

Datum eq2z(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    r = PG_GETARG_FLOAT8(0);
    d = PG_GETARG_FLOAT8(1);
    /* r *= CC_DEG_TO_RAD; */
    d *= CC_DEG_TO_RAD;
    v = sin(d);
    PG_RETURN_FLOAT8(v);
}

Datum arcmin2rad(PG_FUNCTION_ARGS)
{
    float8 v = PG_GETARG_FLOAT8(0);
    PG_RETURN_FLOAT8( CC_ARCMIN_TO_RAD * v );
}

Datum rad2arcmin(PG_FUNCTION_ARGS)
{
    float8 v = PG_GETARG_FLOAT8(0);
    PG_RETURN_FLOAT8( CC_RAD_TO_ARCMIN * v );
}

Datum distancearcminxyz(PG_FUNCTION_ARGS)
{
    float8 mx, my, mz, mxyz;
    float8 ret;

    mx = PG_GETARG_FLOAT8(0);
    my = PG_GETARG_FLOAT8(1);
    mz = PG_GETARG_FLOAT8(2);

    mx *= PG_GETARG_FLOAT8(3);
    my *= PG_GETARG_FLOAT8(4);
    mz *= PG_GETARG_FLOAT8(5);

    mxyz = mx + my + mz;
    if ( 1.0 < mxyz ) mxyz = 1.0;
    else if ( mxyz < -1.0 ) mxyz = -1.0;

    ret = acos( mxyz ) * CC_RAD_TO_ARCMIN;
    PG_RETURN_FLOAT8(ret);
}

Datum _distancearcmineqixyz(PG_FUNCTION_ARGS)
{
    float8 ra, dec, cos_v;
    float8 mx, my, mz, mxyz;
    float8 ret;

    ra  = RAI2RA(PG_GETARG_INT32(0)) * CC_DEG_TO_RAD;
    dec = DECI2DEC(PG_GETARG_INT32(1)) * CC_DEG_TO_RAD;
    mz = sin(dec);
    cos_v = cos(dec);
    mx = cos(ra) * cos_v;
    my = sin(ra) * cos_v;

    mx *= PG_GETARG_FLOAT8(2);
    my *= PG_GETARG_FLOAT8(3);
    mz *= PG_GETARG_FLOAT8(4);

    mxyz = mx + my + mz;
    if ( 1.0 < mxyz ) mxyz = 1.0;
    else if ( mxyz < -1.0 ) mxyz = -1.0;

    ret = acos( mxyz ) * CC_RAD_TO_ARCMIN;
    PG_RETURN_FLOAT8(ret);
}

Datum distancearcmineq(PG_FUNCTION_ARGS)
{
    float8 ra, dec;
    float8 mx, my, mz, mxyz;
    float8 cos_v, ret;

    ra  = PG_GETARG_FLOAT8(0) * CC_DEG_TO_RAD;
    dec = PG_GETARG_FLOAT8(1) * CC_DEG_TO_RAD;
    mz = sin(dec);
    cos_v = cos(dec);
    mx = cos(ra) * cos_v;
    my = sin(ra) * cos_v;

    ra  = PG_GETARG_FLOAT8(2) * CC_DEG_TO_RAD;
    dec = PG_GETARG_FLOAT8(3) * CC_DEG_TO_RAD;
    mz *= sin(dec);
    cos_v = cos(dec);
    mx *= cos(ra) * cos_v;
    my *= sin(ra) * cos_v;

    mxyz = mx + my + mz;
    if ( 1.0 < mxyz ) mxyz = 1.0;
    else if ( mxyz < -1.0 ) mxyz = -1.0;

    ret = acos( mxyz ) * CC_RAD_TO_ARCMIN;
    PG_RETURN_FLOAT8(ret); 
} 

Datum getmaxdistanceradforboxcel(PG_FUNCTION_ARGS)
{
    float8 r_lon = PG_GETARG_FLOAT8(0) * CC_ARCMIN_TO_RAD; /* radius */
    float8 r_lat = PG_GETARG_FLOAT8(1) * CC_ARCMIN_TO_RAD;
    float8 off_lon = PG_GETARG_FLOAT8(2) * CC_ARCMIN_TO_RAD;
    float8 off_lat = PG_GETARG_FLOAT8(3) * CC_ARCMIN_TO_RAD;
    PG_RETURN_FLOAT8(get_max_distance_rad_for_box_cel(r_lon, r_lat,
						      off_lon, off_lat));
}

Datum evalpositioninboxcel(PG_FUNCTION_ARGS)
{
    float8 lon_c = PG_GETARG_FLOAT8(0) * CC_DEG_TO_RAD; /* center */
    float8 lat_c = PG_GETARG_FLOAT8(1) * CC_DEG_TO_RAD;
    float8 r_lon = PG_GETARG_FLOAT8(2) * CC_ARCMIN_TO_RAD; /* radius */
    float8 r_lat = PG_GETARG_FLOAT8(3) * CC_ARCMIN_TO_RAD;
    float8 lon = PG_GETARG_FLOAT8(4) * CC_DEG_TO_RAD;   /* object */
    float8 lat = PG_GETARG_FLOAT8(5) * CC_DEG_TO_RAD;

    double x, z;
    double x0, y0, z0;
    double cos_v, sin_v, rot;

    bool rt = false;

    /* rotate around z-axis */
    lon -= lon_c;

    /* to unit vector */
    cos_v = cos(lat);
    y0 = sin(lon) * cos_v;
    if ( fabs(sin(r_lon)) < fabs(y0) ) goto quit;
    z0 = sin(lat);
    x0 = cos(lon) * cos_v;

    /* rotate around y-axis */
    rot = -lat_c;
    cos_v = cos(rot);
    sin_v = sin(rot);
    z = x0*sin_v + z0*cos_v;
    if ( fabs(sin(r_lat)) < fabs(z) ) goto quit;
    x = x0*cos_v - z0*sin_v;
    if ( x < 0 ) goto quit;
    /* y = y0; */

    rt = true;
 quit:
    PG_RETURN_BOOL(rt);
}

Datum _evalpositioninboxceldiv(PG_FUNCTION_ARGS)
{
    double lon_c = PG_GETARG_FLOAT8(0) * CC_DEG_TO_RAD;	   /* box region */
    double lon_r = PG_GETARG_FLOAT8(1) * CC_ARCMIN_TO_RAD;
    int i_lon = PG_GETARG_INT32(2);
    int ndiv_lon = PG_GETARG_INT32(3);
    double lat_c = PG_GETARG_FLOAT8(4) * CC_DEG_TO_RAD;
    double lat_r = PG_GETARG_FLOAT8(5) * CC_ARCMIN_TO_RAD;
    int i_lat = PG_GETARG_INT32(6);
    int ndiv_lat = PG_GETARG_INT32(7);
    double lon = PG_GETARG_FLOAT8(8) * CC_DEG_TO_RAD;      /* object */
    double lat = PG_GETARG_FLOAT8(9) * CC_DEG_TO_RAD;

    double lon_begin = -lon_r + 2.0*lon_r*((double)i_lon/ndiv_lon);
    double lon_end   = -lon_r + 2.0*lon_r*((double)(i_lon+1)/ndiv_lon);
    double lat_begin = -lat_r + 2.0*lat_r*((double)i_lat/ndiv_lat);
    double lat_end   = -lat_r + 2.0*lat_r*((double)(i_lat+1)/ndiv_lat);

    double x, z;
    double x0, y0, z0;
    double cos_v, sin_v, rot;

    bool rt = false;

    /* rotate around z-axis */
    lon -= lon_c;

    /* to unit vector */
    cos_v = cos(lat);
    y0 = sin(lon) * cos_v;
    if ( i_lon + 1 == ndiv_lon ) {
        if ( y0 < sin(lon_begin) || sin(lon_end) < y0 ) goto quit;
    } else {
        if ( y0 < sin(lon_begin) || sin(lon_end) <= y0 ) goto quit;
    }
    z0 = sin(lat);
    x0 = cos(lon) * cos_v;

    /* rotate around y-axis */
    rot = -lat_c;
    cos_v = cos(rot);
    sin_v = sin(rot);
    z = x0*sin_v + z0*cos_v;
    if ( i_lat + 1 == ndiv_lat ) {
        if ( z < sin(lat_begin) || sin(lat_end) < z ) goto quit;
    } else {
        if ( z < sin(lat_begin) || sin(lat_end) <= z ) goto quit;
    }
    x = x0*cos_v - z0*sin_v;
    if ( x < 0 ) goto quit;
    /* y = y0; */

    rt = true;
 quit:
    PG_RETURN_BOOL(rt);
}


    /* rotate around x-axis */
    /* x = x0; */
    /* y = y0*cos(rot) + z0*sin(rot); */
    /* z = -y0*sin(rot) + z0*cos(rot); */

    /* rotate around y-axis */
    /* x = x0*cos(rot) - z0*sin(rot); */
    /* y = y0; */
    /* z = x0*sin(rot) + z0*cos(rot); */

    /* rotate around z-axis */
    /* x = x0*cos(rot) - y0*sin(rot); */
    /* y = x0*sin(rot) + y0*cos(rot); */
    /* z = z0; */


Datum _getsqlforboxsearchdiv(PG_FUNCTION_ARGS)
{
    text *arg_tbl = PG_GETARG_TEXT_P(0);		    /* table name */
    text *arg_coo = PG_GETARG_TEXT_P(1);		    /* coo system */
    double lon_c = PG_GETARG_FLOAT8(2);			    /* box region */
    double lon_r = PG_GETARG_FLOAT8(3);
    int i_lon = PG_GETARG_INT32(4);
    int ndiv_lon = PG_GETARG_INT32(5);
    double lat_c = PG_GETARG_FLOAT8(6);
    double lat_r = PG_GETARG_FLOAT8(7);
    int i_lat = PG_GETARG_INT32(8);
    int ndiv_lat = PG_GETARG_INT32(9);

    double lon_c_radian = lon_c * CC_DEG_TO_RAD;
    double lon_r_radian = lon_r * CC_ARCMIN_TO_RAD;
    double lat_c_radian = lat_c * CC_DEG_TO_RAD;
    double lat_r_radian = lat_r * CC_ARCMIN_TO_RAD;

    unsigned char *str_tbl = (unsigned char *)(VARDATA(arg_tbl));
    unsigned char *str_coo = (unsigned char *)(VARDATA(arg_coo));
    int len_tbl = VARSIZE(arg_tbl) - VARHDRSZ;
    int len_coo = VARSIZE(arg_coo) - VARHDRSZ;

    int i, j, sn_len;
    double x,y,z, rad;

    const int tbl_name_len = 128;
    const int sql_cmd_len = tbl_name_len + 512;
    unsigned char tbl_name[tbl_name_len];
    unsigned char sql_cmd[sql_cmd_len];

    unsigned char ch_coo = 'j';

    text *sql_text = NULL;

    /* 1st arg */
    for ( i=0 ; i < len_tbl ; i++ ) {
        if ( str_tbl[i] != ' ' ) break;
    }
    for ( j=0 ; j+1 < tbl_name_len && i < len_tbl ; j++, i++ ) {
	if ( str_tbl[i] == ' ' || str_tbl[i] == '\0' ) break;
	tbl_name[j] = str_tbl[i];
    }
    tbl_name[j] = '\0';

    /* 2nd arg */
    for ( i=0 ; i < len_coo ; i++ ) {
        if ( str_coo[i] != ' ' ) break;
    }
    if ( i < len_coo ) {
        ch_coo = str_coo[i];
        /* to lower */
        if ( 'A' <= ch_coo && ch_coo <= 'Z' ) ch_coo += 32;
    }

    get_xyz_center_for_box_cel_div(ch_coo,
				   lon_c_radian,lon_r_radian,i_lon,ndiv_lon,
				   lat_c_radian,lat_r_radian,i_lat,ndiv_lat,
				   &x,&y,&z);
    rad = get_max_distance_rad_for_box_cel_div(lon_r_radian,i_lon,ndiv_lon,
					       lat_r_radian,i_lat,ndiv_lat);

    sn_len = snprintf((char *)sql_cmd,sql_cmd_len,
		      "SELECT o.* FROM ("
		      "SELECT objID,"
		      "fJ2Lon(ra,dec,'%c') AS lon,"
		      "fJ2Lat(ra,dec,'%c') AS lat "
		      "FROM %s "
		      "WHERE (cx BETWEEN %.17g AND %.17g) AND "
			    "(cy BETWEEN %.17g AND %.17g) AND "
			    "(cz BETWEEN %.17g AND %.17g) "
		      ") o "
		      "WHERE _fEvalPositionInBoxCelDiv("
		      "%.17g,%.17g,%d,%d,%.17g,%.17g,%d,%d,"
		      "o.lon,o.lat) = true",
		      ch_coo, ch_coo, tbl_name, 
		      x-rad, x+rad, y-rad, y+rad, z-rad, z+rad, 
		      lon_c,lon_r,i_lon,ndiv_lon,
		      lat_c,lat_r,i_lat,ndiv_lat);

    if ( sql_cmd_len <= sn_len ) {
	const char *err_mes = "[INTERNAL ERROR] Too small buffer: sql_cmd";
	sql_text = (text *)palloc(strlen(err_mes) + VARHDRSZ);
	SET_VARSIZE(sql_text, strlen(err_mes) + VARHDRSZ);
	memcpy(VARDATA(sql_text), err_mes, strlen(err_mes));
    }
    else {
	sql_text = (text *)palloc(sn_len + VARHDRSZ);
	SET_VARSIZE(sql_text, sn_len + VARHDRSZ);
	memcpy(VARDATA(sql_text), sql_cmd, sn_len);
    }

    PG_RETURN_TEXT_P(sql_text);
}


Datum _eqi2xi16(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    int16 ret;
    r = RAI2RA(PG_GETARG_INT32(0));
    d = DECI2DEC(PG_GETARG_INT32(1));
    r *= CC_DEG_TO_RAD;
    d *= CC_DEG_TO_RAD;
    v = cos(r) * cos(d);
    ret = GET_I16_UVEC(v);
    PG_RETURN_INT16(ret);
}

Datum _eqi2yi16(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    int16 ret;
    r = RAI2RA(PG_GETARG_INT32(0));
    d = DECI2DEC(PG_GETARG_INT32(1));
    r *= CC_DEG_TO_RAD;
    d *= CC_DEG_TO_RAD;
    v = sin(r) * cos(d);
    ret = GET_I16_UVEC(v);
    PG_RETURN_INT16(ret);
}

Datum _eqi2zi16(PG_FUNCTION_ARGS)
{
    float8 r, d, v;
    int16 ret;
    r = RAI2RA(PG_GETARG_INT32(0));
    d = DECI2DEC(PG_GETARG_INT32(1));
    /* r *= CC_DEG_TO_RAD; */
    d *= CC_DEG_TO_RAD;
    v = sin(d);
    ret = GET_I16_UVEC(v);
    PG_RETURN_INT16(ret);
}

Datum _loni2lon(PG_FUNCTION_ARGS)
{
    float8 r = RAI2RA(PG_GETARG_INT32(0));
    PG_RETURN_FLOAT8(r);
}

Datum _lati2lat(PG_FUNCTION_ARGS)
{
    float8 d = DECI2DEC(PG_GETARG_INT32(0));
    PG_RETURN_FLOAT8(d);
}


/* 第一引数を第二引数とを比較する */
/* 第一引数は lower(btrim(xx)) したのと同じように扱われる */
/* 第二引数は必ず小文字で指定し、左右のスペースは含めない事 */
Datum _texteq4tblsel(PG_FUNCTION_ARGS)
{
    text *arg_val = PG_GETARG_TEXT_P(0);
    text *arg_ref = PG_GETARG_TEXT_P(1);
    unsigned char *str_val = (unsigned char *)(VARDATA(arg_val));
    unsigned char *str_ref = (unsigned char *)(VARDATA(arg_ref));
    int len_val = VARSIZE(arg_val) - VARHDRSZ;
    int len_ref = VARSIZE(arg_ref) - VARHDRSZ;
    int i, j;
    for ( i=0 ; i < len_val ; i++ ) {
	if ( str_val[i] != ' ' ) break;
    }
    for ( j=0 ; i < len_val && j < len_ref ; i++, j++ ) {
	unsigned char ch_val = str_val[i];
	/* to lower */
	if ( 'A' <= ch_val && ch_val <= 'Z' ) ch_val += 32;
	if ( ch_val != str_ref[j] ) break;
    }
    if ( j == len_ref ) {
	if ( i == len_val ) {
	    PG_RETURN_BOOL(true);
	}
	else {
	    if ( str_val[i] == ' ' || str_val[i] == '\0' ) {
		PG_RETURN_BOOL(true);
	    }
	}
    }
    PG_RETURN_BOOL(false);
}

