
-- CREATE LANGUAGE plpgsql;

--                        --
-- New TYPE for Functions --
--                        --

CREATE TYPE tXYZ AS (x FLOAT8 ,y FLOAT8 ,z FLOAT8);

CREATE TYPE tInternalRectArgs AS (lon1 FLOAT8, lon2 FLOAT8, lon3 FLOAT8, lon4 FLOAT8,
                                  lat1 FLOAT8, lat2 FLOAT8, lat3 FLOAT8, lat4 FLOAT8);

CREATE TYPE tObjCel AS (objID INT4, lon FLOAT8, lat FLOAT8);

CREATE TYPE tObjCelAndDistance AS (objID INT4, lon FLOAT8, lat FLOAT8,
                                   distance FLOAT8);

CREATE TYPE tObj8Cel AS (objID INT8, lon FLOAT8, lat FLOAT8);

CREATE TYPE tObj8CelAndDistance AS (objID INT8, lon FLOAT8, lat FLOAT8,
                                    distance FLOAT8);



--                   --
-- Generic Functions --
--                   --

-- Convert AB mag -> Jy
CREATE FUNCTION fABMag2Jy(v FLOAT8)
  RETURNS FLOAT8 AS $$
    BEGIN
        RETURN 3631.0 * (10.0 ^ (-0.4 * v));
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- Convert Jy -> AB mag
CREATE FUNCTION fJy2ABMag(v FLOAT8)
  RETURNS FLOAT8 AS $$
    BEGIN
        IF v <= 0.0 THEN RETURN NULL; END IF;
        RETURN -2.5 * log(v/3631.0);
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';


-- Fix range of Lon between 0.0 and 360.0
CREATE FUNCTION fFixLon(v FLOAT8)
  RETURNS FLOAT8 AS $$
    DECLARE
        deg FLOAT8;
    BEGIN
        deg := mod(CAST(v AS NUMERIC), 360.0);
        IF deg < 0.0 THEN deg := 360.0 + deg; END IF;
        RETURN deg;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- Fix range of Lat between -90.0 and 90.0
CREATE FUNCTION fFixLat(v FLOAT8)
  RETURNS FLOAT8 AS $$
    DECLARE
        deg FLOAT8;
    BEGIN
        deg := mod(CAST(v AS NUMERIC), 360.0);
        IF deg < 0.0 THEN deg := 360.0 + deg; END IF;
        IF 270.0 <= deg THEN RETURN deg - 360.0; END IF;
        IF 90.0 <= deg THEN RETURN 180.0 - deg; END IF;
        RETURN deg;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- Convert longitude to 'hh:mm:ss.ss' string
CREATE FUNCTION fDeg2LonStr(v FLOAT8)
  RETURNS TEXT AS $$
    DECLARE
        ts FLOAT8;
        h INT4;
        m INT4;
        s00 INT4;
    BEGIN
        ts := 24.0 * fFixLon(v) / 360.0;
        h := trunc(CAST(ts AS NUMERIC),0);
        ts := (ts - h) * 60.0;
        m := trunc(CAST(ts AS NUMERIC),0);
        ts := round(CAST(((ts - m) * 60.0) AS NUMERIC), 2);
        IF 60.0 <= ts THEN ts:=0.0; m:=m+1; END IF;
        IF 60 <= m THEN m:=0; h:=h+1; END IF;
        IF 24 <= h THEN ts:=0.0; m:=0; h:=0; END IF;
        RETURN to_char(h,'FM00') || ':' || to_char(m,'FM00') || ':' ||
               to_char(ts,'FM00.00');
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- Convert latitude to '[+-]dd:mm:ss.s' string
CREATE FUNCTION fDeg2LatStr(v FLOAT8)
  RETURNS TEXT AS $$
    DECLARE
        deg FLOAT8;
        ts FLOAT8;
        d INT4;
        m INT4;
    BEGIN
        deg := fFixLat(v);
        ts := abs(deg);
        d := trunc(CAST(ts AS NUMERIC),0);
        ts := (ts - d) * 60.0;
        m := trunc(CAST(ts AS NUMERIC),0);
        ts := round(CAST(((ts - m) * 60.0) AS NUMERIC), 1);
        IF 60.0 <= ts THEN ts:=0.0; m:=m+1; END IF;
        IF 60 <= m THEN m:=0; d:=d+1; END IF;
        IF 90 <= d THEN ts:=0.0; m:=0; d:=90; END IF;
        IF deg < 0 THEN
            RETURN '-' || to_char(d,'FM00') || ':' ||
                   to_char(m,'FM00') || ':' || to_char(ts,'FM00.0');
        ELSE
            RETURN '+' || to_char(d,'FM00') || ':' ||
                   to_char(m,'FM00') || ':' || to_char(ts,'FM00.0');
        END IF;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- in: '[+-]hh:mm:ss' or degree  out: degree
CREATE FUNCTION fLonStr2Deg(lonstr TEXT)
  RETURNS FLOAT8 AS $$
    DECLARE
        deg FLOAT8;
        h FLOAT8;
        m FLOAT8;
        s FLOAT8;
        hms TEXT;
        tmp TEXT;
        pidx INT4;
        midx INT4;
        cidx INT4;
        delim TEXT;
    BEGIN
        h := 0.0;
        m := 0.0;
        s := 0.0;
        hms := btrim(lonstr);
        pidx := strpos(hms,'+');
        midx := strpos(hms,'-');
        IF 1 < pidx THEN RETURN 0.0; END IF;
        IF 1 < midx THEN RETURN 0.0; END IF;
        IF 0 < pidx THEN hms := substr(hms,2); END IF;
        IF 0 < midx THEN hms := substr(hms,2); END IF;
        hms := ltrim(hms);
        IF length(hms) <= 0 THEN RETURN 0.0; END IF;
        IF ( 0 < strpos(hms,':') ) THEN delim := ':';
        ELSE delim := ' '; END IF;
        cidx := strpos(hms,delim);
        IF 0 < cidx THEN
            tmp := substr(hms,1,cidx-1);
            h := to_number(tmp,'00');
            hms := substr(hms,cidx+1);
            hms := ltrim(hms);
            IF 0 < length(hms) THEN
                cidx := strpos(hms,delim);
                IF 0 < cidx THEN
                    m := to_number(hms,'00');
                    hms := substr(hms,cidx+1);
                    hms := ltrim(hms);
                    IF 0 < length(hms) THEN
                        s := CAST(hms AS FLOAT8);
                    END IF;
                ELSE
                    m := CAST(hms AS FLOAT8);
                END IF;
            END IF;
            deg := 15.0 * (h + m/60.0 + s/3600.0);
        ELSE
            deg := CAST(hms AS FLOAT8);
        END IF;
        IF 0 < midx THEN deg := 0 - deg; END IF;
        RETURN fFixLon(deg);
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- in: '[+-]dd:mm:ss' or degree  out: degree
CREATE FUNCTION fLatStr2Deg(latstr TEXT)
  RETURNS FLOAT8 AS $$
    DECLARE
        deg FLOAT8;
        d FLOAT8;
        m FLOAT8;
        s FLOAT8;
        dms TEXT;
        tmp TEXT;
        pidx INT4;
        midx INT4;
        cidx INT4;
        delim TEXT;
    BEGIN
        d := 0.0;
        m := 0.0;
        s := 0.0;
        dms := btrim(latstr);
        pidx := strpos(dms,'+');
        midx := strpos(dms,'-');
        IF 1 < pidx THEN RETURN 0.0; END IF;
        IF 1 < midx THEN RETURN 0.0; END IF;
        IF 0 < pidx THEN dms := substr(dms,2); END IF;
        IF 0 < midx THEN dms := substr(dms,2); END IF;
        dms := ltrim(dms);
        IF length(dms) <= 0 THEN RETURN 0.0; END IF;
        IF ( 0 < strpos(dms,':') ) THEN delim := ':';
        ELSE delim := ' '; END IF;
        cidx := strpos(dms,delim);
        IF 0 < cidx THEN
            tmp := substr(dms,1,cidx-1);
            d := to_number(tmp,'00');
            dms := substr(dms,cidx+1);
            dms := ltrim(dms);
            IF 0 < length(dms) THEN
                cidx := strpos(dms,delim);
                IF 0 < cidx THEN
                    m := to_number(dms,'00');
                    dms := substr(dms,cidx+1);
                    dms := ltrim(dms);
                    IF 0 < length(dms) THEN
                        s := CAST(dms AS FLOAT8);
                    END IF;
                ELSE
                    m := CAST(dms AS FLOAT8);
                END IF;
            END IF;
            deg := d + m/60.0 + s/3600.0;
        ELSE
            deg := CAST(dms AS FLOAT8);
        END IF;
        IF 0 < midx THEN deg := 0 - deg; END IF;
        RETURN fFixLat(deg);
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

CREATE FUNCTION fMin2Sec(v FLOAT8)
  RETURNS FLOAT8 AS $$
    BEGIN
      RETURN 60.0 * v;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

CREATE FUNCTION fSec2Min(v FLOAT8)
  RETURNS FLOAT8 AS $$
    BEGIN
      RETURN 0.016666666666666667 * v;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

CREATE FUNCTION fEq2XYZ(r FLOAT8, d FLOAT8)
  RETURNS tXYZ AS $$
    DECLARE
      ret tXYZ;
      r_d FLOAT8;
      d_d FLOAT8;
    BEGIN
      r_d := radians(r);
      d_d := radians(d);
      ret.x := cos(r_d) * cos(d_d);
      ret.y := sin(r_d) * cos(d_d);
      ret.z := sin(d_d);
      RETURN ret;
    END;
  $$ IMMUTABLE LANGUAGE 'plpgsql';

CREATE FUNCTION fDistanceArcMinXYZ(xyz1 tXYZ, xyz2 tXYZ)
  RETURNS FLOAT8 AS $$
    BEGIN
      RETURN fDistanceArcMinXYZ(xyz1.x, xyz1.y, xyz1.z, xyz2.x, xyz2.y, xyz2.z);
    END
  $$ IMMUTABLE LANGUAGE 'plpgsql';

-- private functions to make args of _fXXXGetObjFromRectXXX()
CREATE FUNCTION _fMakeInternalRectArgs(lon1 FLOAT8, lon2 FLOAT8, lat1 FLOAT8, lat2 FLOAT8)
  RETURNS tInternalRectArgs AS $$
    DECLARE
        f_lon1 FLOAT8;
        f_lon2 FLOAT8;
        f_lat1 FLOAT8;
        f_lat2 FLOAT8;
        ret tInternalRectArgs;
    BEGIN
      f_lon1 := fFixLon(lon1);
      f_lon2 := fFixLon(lon2);
      f_lat1 := fFixLat(lat1);
      f_lat2 := fFixLat(lat2);
      IF f_lon1 < f_lon2 THEN
        ret.lon1 := f_lon1;
        ret.lon2 := f_lon2;
        ret.lon3 := -1;
        ret.lon4 := -1;
      ELSE
        ret.lon1 := f_lon1;
        ret.lon2 := 360.0;
        ret.lon3 := 0.0;
        ret.lon4 := f_lon2;
      END IF;
      IF f_lat1 < f_lat2 THEN
        ret.lat1 := f_lat1;
        ret.lat2 := f_lat2;
        ret.lat3 := -91;
        ret.lat4 := -91;
      ELSE
        ret.lat1 := -90.0;
        ret.lat2 := f_lat2;
        ret.lat3 := f_lat1;
        ret.lat4 := 90.0;
      END IF;
      RETURN ret;
    END
  $$ IMMUTABLE LANGUAGE 'plpgsql';

