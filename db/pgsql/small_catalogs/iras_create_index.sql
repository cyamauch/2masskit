
-- CREATE INDEX iras_pscname ON Iras (pscname);
CREATE INDEX iras_ra ON Iras (ra);
CREATE INDEX iras_dec ON Iras (dec);

CREATE INDEX iras_fnu_12 ON Iras (fnu_12);
CREATE INDEX iras_fnu_25 ON Iras (fnu_25);
CREATE INDEX iras_fnu_60 ON Iras (fnu_60);
CREATE INDEX iras_fnu_100 ON Iras (fnu_100);

CREATE INDEX iras_fqual_12 ON Iras (fqual_12);
CREATE INDEX iras_fqual_25 ON Iras (fqual_25);
CREATE INDEX iras_fqual_60 ON Iras (fqual_60);
CREATE INDEX iras_fqual_100 ON Iras (fqual_100);

CREATE INDEX iras_xyz ON Iras (cx,cy,cz);

