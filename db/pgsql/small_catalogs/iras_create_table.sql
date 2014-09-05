
CREATE TABLE Iras (
        objid           INT4      CONSTRAINT iras_pkey PRIMARY KEY,
        pscname         VARCHAR(11)  UNIQUE NOT NULL,
        ra              FLOAT8    NOT NULL,
        dec             FLOAT8    NOT NULL,
-- 5
        semimajor       INT2      NOT NULL,
	semiminor	INT2	  NOT NULL,
	posang		INT2	  NOT NULL,
	nhcon		INT2	  NOT NULL,
	fnu_12		FLOAT8	  NOT NULL,
-- 10
	fnu_25		FLOAT8	  NOT NULL,
	fnu_60		FLOAT8	  NOT NULL,
	fnu_100		FLOAT8	  NOT NULL,
	fqual_12	INT2  NOT NULL,
	fqual_25	INT2  NOT NULL,
-- 15
	fqual_60	INT2  NOT NULL,
	fqual_100	INT2  NOT NULL,
	nlrs		INT2  NOT NULL,
	lrschar		VARCHAR(2),	      -- fixed length
	relunc_12	INT2  NOT NULL,
-- 20
	relunc_25	INT2  NOT NULL,
	relunc_60	INT2  NOT NULL,
	relunc_100	INT2  NOT NULL,
	tsnr_12		INT4  NOT NULL,
	tsnr_25		INT4  NOT NULL,
-- 25
	tsnr_60		INT4  NOT NULL,
	tsnr_100	INT4  NOT NULL,
	cc_12		VARCHAR(2),
	cc_25		VARCHAR(2),
	cc_60		VARCHAR(2),
-- 30
	cc_100		VARCHAR(2),
	var		INT2,
	disc		VARCHAR(1)  NOT NULL, -- fixed length
	confuse		VARCHAR(1)  NOT NULL, -- fixed length
	pnearh		INT2  NOT NULL,
-- 35
	pnearw		INT2  NOT NULL,
	ses1_12		INT2  NOT NULL,
	ses1_25		INT2  NOT NULL,
	ses1_60		INT2  NOT NULL,
	ses1_100	INT2  NOT NULL,
-- 40
	ses2_12		INT2  NOT NULL,
	ses2_25		INT2  NOT NULL,
	ses2_60		INT2  NOT NULL,
	ses2_100	INT2  NOT NULL,
	hsdflag		VARCHAR(1)  NOT NULL, -- fixed length
-- 45
	cirr1		INT2  NOT NULL,
	cirr2		INT2  NOT NULL,
	cirr3		INT2  NOT NULL,
	nid		INT2  NOT NULL,
	idtype		INT2  NOT NULL,
-- 50
	mhcon		INT2,
	fcor_12		INT2,
	fcor_25		INT2,
	fcor_60		INT2,
	fcor_100	INT2,
-- 55
	rat_12_25	FLOAT8,
	err_12_25	FLOAT8,
	rat_25_60	FLOAT8,
	err_25_60	FLOAT8,
	rat_60_100	FLOAT8,
-- 60
	err_60_100	FLOAT8,
	cx		FLOAT8  NOT NULL,
	cy		FLOAT8  NOT NULL,
	cz		FLOAT8  NOT NULL
);


