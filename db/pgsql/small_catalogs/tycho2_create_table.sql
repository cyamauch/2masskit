
CREATE TABLE Tycho2 (
  objid        INT4        CONSTRAINT tycho2_pkey PRIMARY KEY,
  tycho2_id    VARCHAR(12) UNIQUE NOT NULL, -- fixed length
  p_flag       VARCHAR(1)  NOT NULL,	    -- fixed length
  ra_m         FLOAT8,              -- ICRS mean R.A.
  dec_m        FLOAT8,              -- ICRS mean Dec.
  e_ra_m       INT2,                -- 6
  e_dec_m      INT2,                -- 7
  ep_ra_m      FLOAT4,              -- 10
  ep_dec_m     FLOAT4,              -- 11
  pm_ra        FLOAT4,              -- 4
  pm_dec       FLOAT4,              -- 5
  e_pm_ra      FLOAT4,              -- 8
  e_pm_dec     FLOAT4,              -- 9
  n_upos       INT2,
  q_ra_m       FLOAT4,
  q_dec_m      FLOAT4,
  q_pm_ra      FLOAT4,
  q_pm_dec     FLOAT4,
  bt_mag       FLOAT4,
  e_bt_mag     FLOAT4,
  vt_mag       FLOAT4,
  e_vt_mag     FLOAT4,
  prox         INT2       NOT NULL,
  tycho1_flag  VARCHAR(1) NOT NULL, -- fixed length
  hip_id       INT4,
  ccdm_hip     VARCHAR(3),
  ra           FLOAT8     NOT NULL, -- ICRS R.A.
  dec          FLOAT8     NOT NULL, -- ICRS Dec.
  e_ra         FLOAT4     NOT NULL, -- 28
  e_dec        FLOAT4     NOT NULL, -- 29
  ep_ra        FLOAT4     NOT NULL, -- 26
  ep_dec       FLOAT4     NOT NULL, -- 27
  t_flag       VARCHAR(1) NOT NULL, -- fixed length : "posflg" in original name
  corr         FLOAT4     NOT NULL,
  cx_m         FLOAT8,              -- Unit Vector of mean ra and dec
  cy_m         FLOAT8,
  cz_m         FLOAT8,
  cx           FLOAT8     NOT NULL, -- Unit Vector of ra and dec
  cy           FLOAT8     NOT NULL,
  cz           FLOAT8     NOT NULL
);

CREATE VIEW _Tycho2m AS
  SELECT objid, ra_m AS ra, dec_m AS dec, cx_m AS cx, cy_m AS cy, cz_m AS cz
  FROM Tycho2;

