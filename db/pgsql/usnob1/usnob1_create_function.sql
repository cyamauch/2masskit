
-- public: Function for cross-matching (J2000)
-- argument: ra[deg], dec[deg], radius[arcmin]
CREATE FUNCTION fUsnob1GetNearestObjIDEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)
  RETURNS INT8
  AS $$
    DECLARE
      rt INT8;
    BEGIN
      EXECUTE _fUsnob1GetSqlForRadialSearch(arg1, arg2, arg3, TRUE) INTO rt;
      RETURN rt;
    END
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- public: Radial Search (J2000)
-- argument: ra[deg], dec[deg], radius[arcmin]
CREATE FUNCTION fUsnob1GetNearbyObjEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)
  RETURNS SETOF tObj8CelAndDistance
  AS $$
    BEGIN
      RETURN QUERY EXECUTE _fUsnob1GetSqlForRadialSearch(arg1, arg2, arg3, FALSE);
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: Radial Search
-- argument: coordsys, lon[deg], lat[deg], radius[arcmin]
CREATE FUNCTION fUsnob1GetNearbyObjCel(TEXT, FLOAT8, FLOAT8, FLOAT8)
  RETURNS SETOF tObj8CelAndDistance
  AS 'SELECT objid,
             fJ2Lon(lon,lat,$1), fJ2Lat(lon,lat,$1), distance
      FROM fUsnob1GetNearbyObjEq(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3),$4)'
  LANGUAGE 'sql';

-- for performance test only
CREATE FUNCTION fUsnob1GetNearbyObjFromBoxCelSimple(arg1 TEXT, 
                arg2 FLOAT8, arg3 FLOAT8, arg4 FLOAT8, arg5 FLOAT8)
  RETURNS SETOF tObj8Cel
  AS $$
    BEGIN
      RETURN QUERY EXECUTE _fUsnob1GetSqlForBoxSearchSimple(arg1, arg2, arg3, arg4, arg5);
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: Box Search
-- argument: coordsys, lon[deg], lat[deg], 
--           half_width_lon[arcmin], half_width_lat[arcmin]
CREATE FUNCTION fUsnob1GetNearbyObjFromBoxCel(coo TEXT, lon_c FLOAT8, lat_c FLOAT8,
                lon_r FLOAT8, lat_r FLOAT8 )
  RETURNS SETOF tObj8Cel AS $$
    DECLARE
      min_ndiv INT4;
      max_ndiv INT4;
      i INT4;
      ndiv INT4;
    BEGIN
      -- followings should be tuned for performance
      min_ndiv := 2;    -- constant: minimum num of division
      max_ndiv := 180;  -- constant: maximum num of division
      i := 0;
      IF ( CAST(min_ndiv AS FLOAT8) <= lon_r/lat_r ) THEN
        ndiv := CAST(floor(lon_r/lat_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fUsnob1GetSqlForBoxSearchDiv(coo,lon_c,lon_r,i,ndiv,lat_c,lat_r,0,1);
          i := i + 1;
        END LOOP;
      ELSIF ( CAST(min_ndiv AS FLOAT8) <= lat_r/lon_r ) THEN
        ndiv := CAST(floor(lat_r/lon_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fUsnob1GetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,i,ndiv);
          i := i + 1;
        END LOOP;
      ELSE
        RETURN QUERY EXECUTE _fUsnob1GetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,0,1);
      END IF;
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: Rectangular Search (J2000)
-- argument: ra_1[deg], ra_2[deg], dec_1[deg], dec_2[deg]
CREATE FUNCTION fUsnob1GetObjFromRectEq(ra1 FLOAT8, ra2 FLOAT8, dec1 FLOAT8, dec2 FLOAT8)
  RETURNS SETOF tObj8Cel
  AS $$
    DECLARE
      a1 tInternalRectArgs;
    BEGIN
      a1 := _fMakeInternalRectArgs(ra1,ra2,dec1,dec2);
      RETURN QUERY EXECUTE
        _fUsnob1GetSqlForRectangularSearch(a1.lon1,a1.lon2,a1.lon3,a1.lon4,a1.lat1,a1.lat2);
      IF -90.0 <= a1.lat3 AND -90.0 <= a1.lat4 THEN
        RETURN QUERY EXECUTE
          _fUsnob1GetSqlForRectangularSearch(a1.lon1,a1.lon2,a1.lon3,a1.lon4,a1.lat3,a1.lat4);
      END IF;
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

