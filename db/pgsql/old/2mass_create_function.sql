
-- CREATE LANGUAGE plpgsql;

-- public: for Cross-ID
-- argument: ra(degree), dec(degree), radius(arcmin)
CREATE FUNCTION fTwomassGetNearestObjIDEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)
  RETURNS INT4
  AS $$
    DECLARE
      rt INT4;
    BEGIN
      EXECUTE _fTwomassGetSqlForRadialSearch(arg1, arg2, arg3, 32400, TRUE) INTO rt;
      RETURN rt;
    END
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- public: for Radial Search
-- argument: ra(degree), dec(degree), radius(arcmin)
CREATE FUNCTION fTwomassGetNearbyObjEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)
  RETURNS SETOF tObjXYZAndDistance
  AS $$
    BEGIN
      RETURN QUERY EXECUTE _fTwomassGetSqlForRadialSearch(arg1, arg2, arg3, 32400, FALSE);
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: for Radial Search
-- argument: coordsys, lon(degree), lat(degree), radius(arcmin)
CREATE FUNCTION fTwomassGetNearbyObjCel(TEXT, FLOAT8, FLOAT8, FLOAT8)
  RETURNS SETOF tObjXYZCelAndDistance
  AS 'SELECT objID, cx, cy, cz, 
             fXYZ2Lon(cx,cy,cz,$1), fXYZ2Lat(cx,cy,cz,$1), distance
      FROM fTwomassGetNearbyObjEq(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3),$4)'
  LANGUAGE 'sql';

-- for performance test only
CREATE FUNCTION fTwomassGetNearbyObjFromBoxCelSimple(arg1 TEXT, 
			arg2 FLOAT8, arg3 FLOAT8, arg4 FLOAT8, arg5 FLOAT8)
  RETURNS SETOF tObjXYZCel
  AS $$
    BEGIN
      RETURN QUERY EXECUTE _fTwomassGetSqlForBoxSearchSimple(arg1, arg2, arg3, arg4, arg5, 32400);
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: for Box Search
-- argument: coordsys, lon(degree), lat(degree), 
--           radius_lon(arcmin), radius_lat(arcmin)
CREATE FUNCTION fTwomassGetNearbyObjFromBoxCel(coo TEXT, lon_c FLOAT8, lat_c FLOAT8,
                                               lon_r FLOAT8, lat_r FLOAT8 )
  RETURNS SETOF tObjXYZCel AS $$
    DECLARE
      min_ndiv INT4;
      max_ndiv INT4;
      i INT4;
      ndiv INT4;
    BEGIN
      -- followings should be tuned for performance
      min_ndiv := 2;    -- constant: minimum num of division
      max_ndiv := 180;   -- constant: maximum num of division
      i := 0;
      IF ( CAST(min_ndiv AS FLOAT8) <= lon_r/lat_r ) THEN
        ndiv := CAST(floor(lon_r/lat_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fTwomassGetSqlForBoxSearchDiv(coo,lon_c,lon_r,i,ndiv,lat_c,lat_r,0,1, 32400);
          i := i + 1;
        END LOOP;
      ELSIF ( CAST(min_ndiv AS FLOAT8) <= lat_r/lon_r ) THEN
        ndiv := CAST(floor(lat_r/lon_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fTwomassGetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,i,ndiv, 32400);
          i := i + 1;
        END LOOP;
      ELSE
        RETURN QUERY EXECUTE _fTwomassGetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,0,1, 32400);
      END IF;
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- private
CREATE FUNCTION _fTwomassGetObjFromRectEq(a1 tInternalRectArgs)
  RETURNS SETOF tObjCel
  AS $$
    DECLARE
      r1 INT8;
      r2 INT8;
      r3 INT8;
      r4 INT8;
      d1 INT4;
      d2 INT4;
      d3 INT4;
      d4 INT4;
    BEGIN
      r1 := round(a1.lon1*1e7);
      r2 := round(a1.lon2*1e7);
      r3 := round(a1.lon3*1e7);
      r4 := round(a1.lon4*1e7);
      r1 := r1 - 1800000000;
      r2 := r2 - 1800000000;
      r3 := r3 - 1800000000;
      r4 := r4 - 1800000000;
      d1 := round(a1.lat1*1e7);
      d2 := round(a1.lat2*1e7);
      d3 := round(a1.lat3*1e7);
      d4 := round(a1.lat4*1e7);
      RETURN QUERY EXECUTE 
        'SELECT objID,CAST((CAST(rai AS INT8) + 1800000000) AS FLOAT8)/1e7,CAST(deci AS FLOAT8)/1e7 FROM twomass_j2000i
         WHERE ((' || CAST(r1 AS TEXT) || ' <= rai AND rai <= ' || CAST(r2 AS TEXT) || ') OR
                (' || CAST(r3 AS TEXT) || ' <= rai AND rai <= ' || CAST(r4 AS TEXT) || ')) AND
               ((' || CAST(d1 AS TEXT) || ' <= deci AND deci <= ' || CAST(d2 AS TEXT) || ') OR
                (' || CAST(d3 AS TEXT) || ' <= deci AND deci <= ' || CAST(d4 AS TEXT) || '))';
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';

-- public: for Rect Search (J2000)
-- argument: ra_1, ra_2, dec_1, dec_2 (degree)
CREATE FUNCTION fTwomassGetObjFromRectEq(FLOAT8,FLOAT8,FLOAT8,FLOAT8)
  RETURNS SETOF tObjCel
  AS 'SELECT _fTwomassGetObjFromRectEq(_fMakeInternalRectArgs($1,$2,$3,$4))'
  LANGUAGE 'sql';
