
CREATE TABLE AkariIrc (
        objID           INT4      CONSTRAINT akariirc_pkey PRIMARY KEY,
        objName         VARCHAR(14)  UNIQUE NOT NULL, -- fixed length
        ra              FLOAT8    NOT NULL,
        dec             FLOAT8    NOT NULL,
        posErrMj        FLOAT4    NOT NULL,
        posErrMi        FLOAT4    NOT NULL,
        posErrPA        FLOAT4    NOT NULL,
        flux_09         FLOAT4,
        flux_18         FLOAT4,
        fErr_09         FLOAT4,
        fErr_18         FLOAT4,
        fQual_09        INT2      NOT NULL,
        fQual_18        INT2      NOT NULL,
        flags_09        INT2,
        flags_18        INT2,
        nScanC_09       INT2      NOT NULL,
        nScanC_18       INT2      NOT NULL,
        nScanP_09       INT2      NOT NULL,
        nScanP_18       INT2      NOT NULL,
        mConf_09        INT2,
        mConf_18        INT2,
        nDens_09        INT2,
        nDens_18        INT2,
        extended_09     INT2,
        extended_18     INT2,
        meanAB_09       FLOAT4,
        meanAB_18       FLOAT4,
        nDataPos        INT2      NOT NULL,
        nData_09        INT2      NOT NULL,
        nData_18        INT2      NOT NULL,
        cx              FLOAT8    NOT NULL,
        cy              FLOAT8    NOT NULL,
        cz              FLOAT8    NOT NULL
);


