
SELECT fRegisterCFunction('fGetXyzCenterForBoxCelDiv(TEXT,FLOAT8,FLOAT8,INT4,INT4,FLOAT8,FLOAT8,INT4,INT4,INT4)', 'FLOAT8', 'getxyzcenterforboxceldiv');

--

PG_FUNCTION_INFO_V1(getxyzcenterforboxceldiv);

extern Datum getxyzcenterforboxceldiv(PG_FUNCTION_ARGS);

Datum getxyzcenterforboxceldiv(PG_FUNCTION_ARGS)
{
    text *arg_coo = PG_GETARG_TEXT_P(0);
    double lon = PG_GETARG_FLOAT8(1) * CC_DEG_TO_RAD;
    double lon_r = PG_GETARG_FLOAT8(2) * CC_ARCMIN_TO_RAD;
    int i_lon = PG_GETARG_INT32(3);
    int ndiv_lon = PG_GETARG_INT32(4);
    double lat = PG_GETARG_FLOAT8(5) * CC_DEG_TO_RAD;
    double lat_r = PG_GETARG_FLOAT8(6) * CC_ARCMIN_TO_RAD;
    int i_lat = PG_GETARG_INT32(7);
    int ndiv_lat = PG_GETARG_INT32(8);
    int what = PG_GETARG_INT32(9);        /* what is returned: 0:x 1:y 2:z */
    unsigned char *str_coo = (unsigned char *)(VARDATA(arg_coo));
    int len_coo = VARSIZE(arg_coo) - VARHDRSZ;
    unsigned char ch_coo = 'j';
    int i;
    double rt_x,rt_y,rt_z;
    float8 rt;
    for ( i=0 ; i < len_coo ; i++ ) {
        if ( str_coo[i] != ' ' ) break;
    }
    if ( i < len_coo ) {
        ch_coo = str_coo[i];
        /* to lower */
        if ( 'A' <= ch_coo && ch_coo <= 'Z' ) ch_coo += 32;
    }
    get_xyz_center_for_box_cel_div(ch_coo, lon, lon_r, i_lon, ndiv_lon,
					   lat, lat_r, i_lat, ndiv_lat,
					   &rt_x, &rt_y, &rt_z );
    if ( what == 1 ) rt = rt_y;
    else if ( what == 2 ) rt = rt_z;
    else rt = rt_x;
    PG_RETURN_FLOAT8(rt);
}

