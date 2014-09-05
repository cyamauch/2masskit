
CREATE FUNCTION fGetSqlForBoxSearchDiv(tbl TEXT, coo TEXT,
                                       lon_c FLOAT8, lon_r FLOAT8, i_lon INT4, ndiv_lon INT4,
                                       lat_c FLOAT8, lat_r FLOAT8, i_lat INT4, ndiv_lat INT4)
  RETURNS TEXT AS $$
    DECLARE
      x FLOAT8;
      y FLOAT8;
      z FLOAT8;
      rad FLOAT8;
      rt_sql TEXT;
    BEGIN
      x := fGetXyzCenterForBoxCelDiv(coo,lon_c,lon_r,i_lon,ndiv_lon,lat_c,lat_r,i_lat,ndiv_lat,0);
      y := fGetXyzCenterForBoxCelDiv(coo,lon_c,lon_r,i_lon,ndiv_lon,lat_c,lat_r,i_lat,ndiv_lat,1);
      z := fGetXyzCenterForBoxCelDiv(coo,lon_c,lon_r,i_lon,ndiv_lon,lat_c,lat_r,i_lat,ndiv_lat,2);
      rad := fGetMaxDistanceRadForBoxCelDiv(lon_r,i_lon,ndiv_lon, lat_r,i_lat,ndiv_lat);
      rt_sql := 'SELECT o.* FROM (' ||
                'SELECT objID,cx,cy,cz,' || 
                'fJ2Lon(ra,dec,''' || coo || ''') AS lon,' ||
                'fJ2Lat(ra,dec,''' || coo || ''') AS lat ' ||
                'FROM ' || tbl || ' ' ||
                'WHERE (cx BETWEEN ' || CAST((x - rad) AS TEXT) || 
                         ' AND ' || CAST((x + rad) AS TEXT) || ') AND ' ||
                      '(cy BETWEEN ' || CAST((y - rad) AS TEXT) || 
                         ' AND ' || CAST((y + rad) AS TEXT) || ') AND ' ||
                      '(cz BETWEEN ' || CAST((z - rad) AS TEXT) || 
                         ' AND ' || CAST((z + rad) AS TEXT) || ') ' ||
                ') o ' ||
                'WHERE fEvalPositionInBoxCelDiv(' || 
                CAST(lon_c AS TEXT)    || ',' ||
                CAST(lon_r AS TEXT)    || ',' ||
                CAST(i_lon AS TEXT)    || ',' ||
                CAST(ndiv_lon AS TEXT) || ',' ||
                CAST(lat_c AS TEXT)    || ',' ||
                CAST(lat_r AS TEXT)    || ',' ||
                CAST(i_lat AS TEXT)    || ',' ||
                CAST(ndiv_lat AS TEXT) || ',o.lon,o.lat) = true';
      RETURN rt_sql;
    END
  $$ IMMUTABLE LANGUAGE 'plpgsql';

