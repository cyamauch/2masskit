
UPDATE Tycho2 SET cx_m = fEq2X(ra_m,dec_m), cy_m = fEq2Y(ra_m,dec_m), cz_m = fEq2Z(ra_m,dec_m)
              WHERE (ra_m IS NOT NULL);

UPDATE Tycho2 SET cx = fEq2X(ra,dec), cy = fEq2Y(ra,dec), cz = fEq2Z(ra,dec);

