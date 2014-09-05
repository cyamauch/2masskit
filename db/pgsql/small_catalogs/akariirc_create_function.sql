
CREATE FUNCTION fAkariIrcGetNearestObjIDEq(FLOAT8,FLOAT8,FLOAT8)
  RETURNS INT4
  AS 'SELECT o.objid FROM
      (SELECT objid, fDistanceArcMinEq($1,$2,ra,dec) as distance
       FROM AkariIrc
       WHERE (cx BETWEEN fEq2X($1,$2) - fArcMin2Rad($3) AND 
                         fEq2X($1,$2) + fArcMin2Rad($3)) AND 
             (cy BETWEEN fEq2Y($1,$2) - fArcMin2Rad($3) AND 
                         fEq2Y($1,$2) + fArcMin2Rad($3)) AND 
             (cz BETWEEN fEq2Z($1,$2) - fArcMin2Rad($3) AND 
                         fEq2Z($1,$2) + fArcMin2Rad($3))
      ) o
      WHERE o.distance <= $3
      ORDER BY o.distance
      LIMIT 1'
  IMMUTABLE LANGUAGE 'sql';

CREATE FUNCTION fAkariIrcGetNearbyObjEq(FLOAT8,FLOAT8,FLOAT8)
  RETURNS SETOF tObjCelAndDistance
  AS 'SELECT o.*
      FROM (
       SELECT objid, ra, dec, 
              fDistanceArcMinEq($1,$2,ra,dec) AS distance
       FROM AkariIrc
       WHERE (cx BETWEEN fEq2X($1,$2) - fArcMin2Rad($3) AND 
                         fEq2X($1,$2) + fArcMin2Rad($3)) AND 
             (cy BETWEEN fEq2Y($1,$2) - fArcMin2Rad($3) AND 
                         fEq2Y($1,$2) + fArcMin2Rad($3)) AND 
             (cz BETWEEN fEq2Z($1,$2) - fArcMin2Rad($3) AND 
                         fEq2Z($1,$2) + fArcMin2Rad($3))
      ) o
      WHERE o.distance <= $3'
  LANGUAGE 'sql';

CREATE FUNCTION fAkariIrcGetNearbyObjCel(TEXT, FLOAT8, FLOAT8, FLOAT8)
  RETURNS SETOF tObjCelAndDistance
  AS 'SELECT o.*
      FROM (
       SELECT objid,
              fJ2Lon(ra,dec,$1) AS lon, fJ2Lat(ra,dec,$1) AS lat,
              fDistanceArcMinEq(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3),ra,dec) AS distance
       FROM AkariIrc
       WHERE (cx BETWEEN fEq2X(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fArcMin2Rad($4) AND 
                         fEq2X(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fArcMin2Rad($4)) AND 
             (cy BETWEEN fEq2Y(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fArcMin2Rad($4) AND 
                         fEq2Y(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fArcMin2Rad($4)) AND 
             (cz BETWEEN fEq2Z(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fArcMin2Rad($4) AND 
                         fEq2Z(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fArcMin2Rad($4))
      ) o
      WHERE o.distance <= $4'
  LANGUAGE 'sql';

-- For performance test only
CREATE FUNCTION fAkariIrcGetNearbyObjFromBoxCelSimple(TEXT,FLOAT8,FLOAT8,FLOAT8,FLOAT8)
  RETURNS SETOF tObjCel
  AS 'SELECT o.*
      FROM (
       SELECT objid,
              fJ2Lon(ra,dec,$1) AS lon, fJ2Lat(ra,dec,$1) AS lat
       FROM AkariIrc
       WHERE (cx BETWEEN fEq2X(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fGetMaxDistanceRadForBoxCel($4,$5,0,0) AND
                         fEq2X(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fGetMaxDistanceRadForBoxCel($4,$5,0,0)) AND
             (cy BETWEEN fEq2Y(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fGetMaxDistanceRadForBoxCel($4,$5,0,0) AND
                         fEq2Y(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fGetMaxDistanceRadForBoxCel($4,$5,0,0)) AND
             (cz BETWEEN fEq2Z(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) - fGetMaxDistanceRadForBoxCel($4,$5,0,0) AND
                         fEq2Z(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3)) + fGetMaxDistanceRadForBoxCel($4,$5,0,0))
      ) o
      WHERE fEvalPositionInBoxCel($2,$3, $4,$5, o.lon,o.lat) = true'
  LANGUAGE 'sql';

CREATE FUNCTION fAkariIrcGetNearbyObjFromBoxCel(coo TEXT, lon_c FLOAT8, lat_c FLOAT8,
                                                lon_r FLOAT8, lat_r FLOAT8 )
  RETURNS SETOF tObjCel AS $$
    DECLARE
      tbl TEXT;
      min_ndiv INT4;
      max_ndiv INT4;
      i INT4;
      ndiv INT4;
    BEGIN
      tbl := 'AkariIrc';  -- constant: table name
      -- followings should be tuned for performance
      min_ndiv := 2;    -- constant: minimum num of division
      max_ndiv := 90;   -- constant: maximum num of division
      i := 0;
      IF ( CAST(min_ndiv AS FLOAT8) <= lon_r/lat_r ) THEN
        ndiv := CAST(floor(lon_r/lat_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fGetSqlForBoxSearchDiv(tbl,coo,lon_c,lon_r,i,ndiv,lat_c,lat_r,0,1);
          i := i + 1;
        END LOOP;
      ELSIF ( CAST(min_ndiv AS FLOAT8) <= lat_r/lon_r ) THEN
        ndiv := CAST(floor(lat_r/lon_r) AS INT4);
        IF (max_ndiv < ndiv) THEN
          ndiv := max_ndiv;
        END IF;
        WHILE ( i < ndiv ) LOOP
          RETURN QUERY EXECUTE _fGetSqlForBoxSearchDiv(tbl,coo,lon_c,lon_r,0,1,lat_c,lat_r,i,ndiv);
          i := i + 1;
        END LOOP;
      ELSE
        RETURN QUERY EXECUTE _fGetSqlForBoxSearchDiv(tbl,coo,lon_c,lon_r,0,1,lat_c,lat_r,0,1);
      END IF;
      RETURN;
    END
  $$ LANGUAGE 'plpgsql';


-- private
CREATE FUNCTION _fAkariIrcGetObjFromRectEq(tInternalRectArgs)
  RETURNS SETOF tObjCel
  AS 'SELECT objid,ra,dec FROM AkariIrc
      WHERE (($1.lon1 <= ra AND ra <= $1.lon2) OR
             ($1.lon3 <= ra AND ra <= $1.lon4)) AND
            (($1.lat1 <= dec AND dec <= $1.lat2) OR
             ($1.lat3 <= dec AND dec <= $1.lat4))'
  LANGUAGE 'sql';

CREATE FUNCTION fAkariIrcGetObjFromRectEq(FLOAT8,FLOAT8,FLOAT8,FLOAT8)
  RETURNS SETOF tObjCel
  AS 'SELECT _fAkariIrcGetObjFromRectEq(_fMakeInternalRectArgs($1,$2,$3,$4))'
  LANGUAGE 'sql';

