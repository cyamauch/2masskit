
SELECT fRegisterCFunction('fGetMaxDistanceRadForBoxCelDiv(FLOAT8,INT4,INT4,FLOAT8,INT4,INT4)', 'FLOAT8', 'getmaxdistanceradforboxceldiv');

-- 

PG_FUNCTION_INFO_V1(getmaxdistanceradforboxceldiv);

extern Datum getmaxdistanceradforboxceldiv(PG_FUNCTION_ARGS);

Datum getmaxdistanceradforboxceldiv(PG_FUNCTION_ARGS)
{
    double lon_r = PG_GETARG_FLOAT8(0) * CC_ARCMIN_TO_RAD;
    int i_lon = PG_GETARG_INT32(1);
    int ndiv_lon = PG_GETARG_INT32(2);
    double lat_r = PG_GETARG_FLOAT8(3) * CC_ARCMIN_TO_RAD;
    int i_lat = PG_GETARG_INT32(4);
    int ndiv_lat = PG_GETARG_INT32(5);
    PG_RETURN_FLOAT8(get_max_distance_rad_for_box_cel_div(lon_r,i_lon,ndiv_lon,
							  lat_r,i_lat,ndiv_lat));
}

