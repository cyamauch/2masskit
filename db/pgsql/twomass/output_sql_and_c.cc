/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2014-09-05 17:24:32 cyamauch> */

static const char *get_child_id( size_t ix, size_t n_ix, char *out )
{
    size_t i, n_ch;
    if ( 0 < n_ix ) {
	n_ix --;
	for ( i=0 ; 0 < n_ix ; i++ ) {
	    n_ix /= 36;
	}
	n_ch = i;
	out[n_ch] = '\0';
	for ( i=0 ; i < n_ch ; i++ ) {
	    int ch = ix % 36;
	    if ( ch < 10 ) out[n_ch - i - 1] =  '0' + ch;
	    else out[n_ch - i - 1] = 'a' + (ch - 10);
	    ix /= 36;
	}
    }
    return out;
}

static const char *get_main_db_filename( const char *cat_name,
					 size_t ix, size_t n_ix, tstring *out )
{
    if ( out != NULL ) {
	tstring cname;
	cname.assign(cat_name).tolower();
	if ( 0 < n_ix ) {
	    char istr[8];
	    out->printf("%s_main_%s.db.txt", 
			cname.cstr(), get_child_id(ix, n_ix, istr));
	}
	else {
	    if ( ix == 0 ) {
		out->printf("%s_main_*.db.txt", cname.cstr());
	    }
	    else {
		out->printf("%s_main_[0-9a-z][0-9a-z]*[.]db[.]txt", 
			    cname.cstr());
	    }
	}
	return out->cstr();
    }
    else {
	return NULL;
    }
}

static const char *get_eqi_db_filename( const char *cat_name,
					size_t ix, size_t n_ix, tstring *out )
{
    if ( out != NULL ) {
	tstring cname;
	cname.assign(cat_name).tolower();
	if ( 0 < n_ix ) {
	    char istr[8];
	    out->printf("%s_eqi_%s.db.txt", 
			cname.cstr(), get_child_id(ix, n_ix, istr));
	}
	else {
	    if ( ix == 0 ) {
		out->printf("%s_eqi_*.db.txt", cname.cstr());
	    }
	    else {
		out->printf("%s_eqi_[0-9a-z][0-9a-z]*[.]db[.]txt", 
			    cname.cstr());
	    }
	}
	return out->cstr();
    }
    else {
	return NULL;
    }
}

static const char *get_eqi_db_tmp_filename( const char *cat_name,
					    size_t ix, size_t n_ix, tstring *out )
{
    if ( out != NULL ) {
	tstring cname;
	cname.assign(cat_name).tolower();
	if ( 0 < n_ix ) {
	    char istr[8];
	    out->printf("%s_eqi_%s.tmp.bin", 
			cname.cstr(), get_child_id(ix, n_ix, istr));
	}
	else {
	    if ( ix == 0 ) {
		out->printf("%s_eqi_*.tmp.bin", cname.cstr());
	    }
	    else {
		out->printf("%s_eqi_[0-9a-z][0-9a-z]*[.]tmp[.]bin", 
			    cname.cstr());
	    }
	}
	return out->cstr();
    }
    else {
	return NULL;
    }
}

static const char *get_table_name( const char *cat_name,
				   const char *type,
				   size_t ix, size_t n_ix, tstring *out )
{
    if ( out != NULL ) {
	if ( 0 < n_ix ) {	/* child table */
	    tstring cname;
	    char istr[8];
	    out->printf("%s_%s_%s", 
			cname.assign(cat_name).tolower().cstr(), 
			type, get_child_id(ix, n_ix, istr));
	}
	else {			/* parent table */
	    out->printf("%s_%s", cat_name, type);
	}
	return out->cstr();
    }
    else {
	return NULL;
    }
}

static const char *get_index_name( const char *cat_name,
				   const char *table_type,
				   const char *index_type,
				   size_t ix, size_t n_ix, tstring *out )
{
    if ( out != NULL ) {
	tstring cname;
	char istr[8];
	out->printf("%s_%s_%s_%s", 
		    cname.assign(cat_name).tolower().cstr(), 
		    table_type, get_child_id(ix, n_ix, istr), index_type);
	return out->cstr();
    }
    else {
	return NULL;
    }
}


/*
 * read a tmp binary eqi file, sort rows order by rai for each deci section
 * for optimization, and output a text DB file.
 */
static int read_optimize_and_write_eqi( const char *cat_name, 
				      size_t n_prialloc_entries,
				      const char *db_delimiter,
				      int eqi_ix, size_t n_eqi_tables,
				      size_t n_div_eqi_section,
				      int(*compar)(const void *, const void *) )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio fh_eqi;
    tstring fname, tmp_fname;
    size_t k, n_entries, l_begin;
    ipos_entry *ipos_tmp_ptr;
    mdarray ipos_tmp(sizeof(*ipos_tmp_ptr), true, (void *)(&ipos_tmp_ptr));

    /* read all of tmp */
    get_eqi_db_tmp_filename(cat_name, eqi_ix, 
			    n_eqi_tables, &tmp_fname);
    if ( fh_eqi.open("r", tmp_fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_eqi.open() failed\n");
	goto quit;
    }
    k = 0;
    ipos_tmp.resize(n_prialloc_entries);
    while ( 1 ) {
	if ( ipos_tmp.length() <= k ) {
	    ipos_tmp.resize(k + n_prialloc_entries / 2);
	}
	if ( fh_eqi.read(ipos_tmp_ptr + k,
			 sizeof(*ipos_tmp_ptr)) <= 0 ) {
	    break;
	}
	k++;
    }
    fh_eqi.close();
    n_entries = k;

    /* sort rows order by rai for each deci section */
    l_begin = 0;
    for ( k=0 ; k < n_div_eqi_section ; k++ ) {
	size_t l_next;
	if ( k + 1 == n_div_eqi_section ) l_next = n_entries;
	else l_next = (size_t)(((double)n_entries / n_div_eqi_section) * (k+1));
	/* sort by xxx */
	qsort(ipos_tmp_ptr + l_begin, l_next - l_begin, 
	      sizeof(*ipos_tmp_ptr), compar);
	l_begin = l_next;
    }

    /* output */
    get_eqi_db_filename(cat_name, eqi_ix, n_eqi_tables, &fname);
    if ( fh_eqi.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_eqi.open() failed\n");
	goto quit;
    }
    for ( k=0 ; k < n_entries ; k++ ) {
	fh_eqi.printf("%lld%s%d%s%d\n",
		      ipos_tmp_ptr[k]._objid, db_delimiter,
		      ipos_tmp_ptr[k].rai, db_delimiter,
		      ipos_tmp_ptr[k].deci);
    }
    fh_eqi.close();

    /* erase tmp file */
    unlink(tmp_fname.cstr());

    ret_status = 0;
 quit:
    return ret_status;
}


static int output_sql_and_c( const char *catname, size_t n_main_files,
			     const char *db_delimiter, const char *db_null,
			     const mdarray_int &deci_check,
			     int max_deci, bool i8_objid )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio fh_out;
    tstring l_catname;
    tstring pname;
    size_t i, n_eqi_tables;
    const char *objid_type;
    const char *type_prefix;

    n_eqi_tables = deci_check.length() + 1;

    /* "Foo" => "foo" */
    l_catname.assign(catname).tolower();

    /*
     * Output SQL statements for main table
     */
    if ( fh_out.openf("w", "%s_main_copy._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    for ( i=0 ; i < n_main_files ; i++ ) {
	tstring fname;
	fh_out.printf("\\copy %s FROM %s WITH NULL as '%s'\n",
		      catname, 
		      get_main_db_filename(catname, i, n_main_files, &fname),
		      db_null);
    }
    fh_out.printf("set default_tablespace = @TS_PREFIX@%s_main_pkey;\n",
		  l_catname.cstr());
    fh_out.printf("ALTER TABLE %s ADD PRIMARY KEY (objid);\n",
		  catname);
    fh_out.printf("set default_tablespace = '';\n");
    fh_out.close();

    if ( fh_out.openf("w", "%s_main_create_index._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("CREATE INDEX %s_@COL@ ON %s (@COL@)\n",l_catname.cstr(),catname);
    fh_out.printf(" TABLESPACE @TS_PREFIX@%s_main_index;\n",l_catname.cstr());
    fh_out.close();

    if ( fh_out.openf("w", "%s_main_grant._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("GRANT SELECT ON %s TO @GUEST_USER@;\n",catname);
    fh_out.close();


    /*
     * Output SQL statements for eqi tables
     */
    get_table_name(catname, "eqi", 0,0, &pname);

    if ( fh_out.openf("w", "%s_eqi_create_table._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    
    for ( i=0 ; i < deci_check.length() ; i++ ) {
	tstring cname;
	if ( i == 0 ) {
	    fh_out.printf("CREATE TABLE %s\n"
			  " (CHECK(deci < %d))\n"
			  " INHERITS (%s)\n"
			  " TABLESPACE @TS_PREFIX@%s_eqi;\n",
			  get_table_name(catname, "eqi", i,n_eqi_tables,
					 &cname), 
			  deci_check[i], pname.cstr(), l_catname.cstr());
	}
	else {
	    fh_out.printf("CREATE TABLE %s\n"
			  " (CHECK(%d <= deci AND deci < %d))\n"
			  " INHERITS (%s)\n"
			  " TABLESPACE @TS_PREFIX@%s_eqi;\n",
			  get_table_name(catname, "eqi", i,n_eqi_tables,
					 &cname), 
			  deci_check[i-1], deci_check[i],
			  pname.cstr(), l_catname.cstr());
	}
	if ( i + 1 == deci_check.length() ) {
	    fh_out.printf("CREATE TABLE %s\n"
			  " (CHECK(%d <= deci))\n"
			  " INHERITS (%s)\n"
			  " TABLESPACE @TS_PREFIX@%s_eqi;\n",
			  get_table_name(catname, "eqi", i+1,n_eqi_tables,
					 &cname), 
			  deci_check[i], pname.cstr(), l_catname.cstr());
	}
    }
    fh_out.close();

    if ( fh_out.openf("w", "%s_eqi_copy.sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring cname;
	tstring fname;
	fh_out.printf("\\copy %s FROM %s WITH NULL as '%s'\n",
		      get_table_name(catname,"eqi", i,n_eqi_tables, &cname),
		      get_eqi_db_filename(catname, i,n_eqi_tables, &fname),
		      db_null);
    }
    fh_out.close();

    if ( fh_out.openf("w", "%s_eqi_create_index._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring iname;
	tstring cname;
	fh_out.printf("CREATE INDEX %s ON %s (rai,deci)\n"
		      " TABLESPACE @TS_PREFIX@%s_eqi_index_radeci;\n",
		      get_index_name(catname, "eqi", "radeci", 
				     i,n_eqi_tables, &iname), 
		      get_table_name(catname, "eqi", 
				     i,n_eqi_tables, &cname), 
		      l_catname.cstr());
    }
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring iname;
	tstring cname;
	fh_out.printf("CREATE INDEX %s ON %s (deci,rai)\n"
		      " TABLESPACE @TS_PREFIX@%s_eqi_index_radeci;\n",
		      get_index_name(catname, "eqi", "decrai", 
				     i,n_eqi_tables, &iname), 
		      get_table_name(catname, "eqi", 
				     i,n_eqi_tables, &cname), 
		      l_catname.cstr());
    }
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring iname;
	tstring cname;
	fh_out.printf("CREATE INDEX %s ON %s (_fEqI2XI16(rai,deci),"
		      "_fEqI2YI16(rai,deci),_fEqI2ZI16(rai,deci)) "
		      " TABLESPACE @TS_PREFIX@%s_eqi_index_xyzi16;\n",
		      get_index_name(catname, "eqi", "xyzi16", 
				     i,n_eqi_tables, &iname), 
		      get_table_name(catname, "eqi", 
				     i,n_eqi_tables, &cname), 
		      l_catname.cstr());
    }
    fh_out.close();

    if ( fh_out.openf("w", "%s_eqi_grant._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("GRANT SELECT ON %s TO @GUEST_USER@;\n",pname.cstr());
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring cname;
	fh_out.printf("GRANT SELECT ON %s TO @GUEST_USER@;\n",
		      get_table_name(catname, "eqi", 
				     i,n_eqi_tables, &cname));
    }
    fh_out.close();

    if ( fh_out.openf("w", "%s_vacuum.sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring cname;
	fh_out.printf("VACUUM ANALYZE %s;\n",
		      get_table_name(catname,"eqi", i,n_eqi_tables, &cname));
    }
    fh_out.printf("VACUUM ANALYZE %s;\n", catname);
    fh_out.close();


    /*
     * Output foo_create_function.sql
     */
    if ( i8_objid ) {	/* for 8-byte ObjID */
	objid_type = "INT8";
	type_prefix = "tObj8";
    }
    else {		/* for 4-byte ObjID */
	objid_type = "INT4";
	type_prefix = "tObj";
    }
    if ( fh_out.openf("w", "%s_create_function.sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("\n");
    {
      const char *sql = 
      "-- public: Function for cross-matching (J2000)\n"
      "-- argument: ra[deg], dec[deg], radius[arcmin]\n"
      "CREATE FUNCTION f%sGetNearestObjIDEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)\n"
      "  RETURNS %s\n"
      "  AS $$\n"
      "    DECLARE\n"
      "      rt %s;\n"
      "    BEGIN\n"
      "      EXECUTE _f%sGetSqlForRadialSearch(arg1, arg2, arg3, TRUE) INTO rt;\n"
      "      RETURN rt;\n"
      "    END\n"
      "  $$ IMMUTABLE LANGUAGE 'plpgsql';\n"
      "\n";
      fh_out.printf(sql, catname, objid_type, objid_type, catname);
    }
    {
      const char *sql = 
      "-- public: Radial Search (J2000)\n"
      "-- argument: ra[deg], dec[deg], radius[arcmin]\n"
      "CREATE FUNCTION f%sGetNearbyObjEq(arg1 FLOAT8, arg2 FLOAT8, arg3 FLOAT8)\n"
      "  RETURNS SETOF %sCelAndDistance\n"
      "  AS $$\n"
      "    BEGIN\n"
      "      RETURN QUERY EXECUTE _f%sGetSqlForRadialSearch(arg1, arg2, arg3, FALSE);\n"
      "      RETURN;\n"
      "    END\n"
      "  $$ LANGUAGE 'plpgsql';\n"
      "\n";
      fh_out.printf(sql, catname, type_prefix, catname);
    }
    {
      const char *sql = 
      "-- public: Radial Search\n"
      "-- argument: coordsys, lon[deg], lat[deg], radius[arcmin]\n"
      "CREATE FUNCTION f%sGetNearbyObjCel(TEXT, FLOAT8, FLOAT8, FLOAT8)\n"
      "  RETURNS SETOF %sCelAndDistance\n"
      "  AS 'SELECT objid,\n"
      "             fJ2Lon(lon,lat,$1), fJ2Lat(lon,lat,$1), distance\n"
      "      FROM f%sGetNearbyObjEq(fCel2Ra($1,$2,$3),fCel2Dec($1,$2,$3),$4)'\n"
      "  LANGUAGE 'sql';\n"
      "\n";
      fh_out.printf(sql, catname, type_prefix, catname);
    }
    {
      const char *sql = 
      "-- for performance test only\n"
      "CREATE FUNCTION f%sGetNearbyObjFromBoxCelSimple(arg1 TEXT, \n"
      "                arg2 FLOAT8, arg3 FLOAT8, arg4 FLOAT8, arg5 FLOAT8)\n"
      "  RETURNS SETOF %sCel\n"
      "  AS $$\n"
      "    BEGIN\n"
      "      RETURN QUERY EXECUTE _f%sGetSqlForBoxSearchSimple(arg1, arg2, arg3, arg4, arg5);\n"
      "      RETURN;\n"
      "    END\n"
      "  $$ LANGUAGE 'plpgsql';\n"
      "\n";
      fh_out.printf(sql, catname, type_prefix, catname);
    }
    {
      const char *sql = 
      "-- public: Box Search\n"
      "-- argument: coordsys, lon[deg], lat[deg], \n"
      "--           half_width_lon[arcmin], half_width_lat[arcmin]\n"
      "CREATE FUNCTION f%sGetNearbyObjFromBoxCel(coo TEXT, lon_c FLOAT8, lat_c FLOAT8,\n"
      "                lon_r FLOAT8, lat_r FLOAT8 )\n"
      "  RETURNS SETOF %sCel AS $$\n"
      "    DECLARE\n"
      "      min_ndiv INT4;\n"
      "      max_ndiv INT4;\n"
      "      i INT4;\n"
      "      ndiv INT4;\n"
      "    BEGIN\n"
      "      -- followings should be tuned for performance\n"
      "      min_ndiv := 2;    -- constant: minimum num of division\n"
      "      max_ndiv := 180;  -- constant: maximum num of division\n"
      "      i := 0;\n"
      "      IF ( CAST(min_ndiv AS FLOAT8) <= lon_r/lat_r ) THEN\n"
      "        ndiv := CAST(floor(lon_r/lat_r) AS INT4);\n"
      "        IF (max_ndiv < ndiv) THEN\n"
      "          ndiv := max_ndiv;\n"
      "        END IF;\n"
      "        WHILE ( i < ndiv ) LOOP\n"
      "          RETURN QUERY EXECUTE _f%sGetSqlForBoxSearchDiv(coo,lon_c,lon_r,i,ndiv,lat_c,lat_r,0,1);\n"
      "          i := i + 1;\n"
      "        END LOOP;\n"
      "      ELSIF ( CAST(min_ndiv AS FLOAT8) <= lat_r/lon_r ) THEN\n"
      "        ndiv := CAST(floor(lat_r/lon_r) AS INT4);\n"
      "        IF (max_ndiv < ndiv) THEN\n"
      "          ndiv := max_ndiv;\n"
      "        END IF;\n"
      "        WHILE ( i < ndiv ) LOOP\n"
      "          RETURN QUERY EXECUTE _f%sGetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,i,ndiv);\n"
      "          i := i + 1;\n"
      "        END LOOP;\n"
      "      ELSE\n"
      "        RETURN QUERY EXECUTE _f%sGetSqlForBoxSearchDiv(coo,lon_c,lon_r,0,1,lat_c,lat_r,0,1);\n"
      "      END IF;\n"
      "      RETURN;\n"
      "    END\n"
      "  $$ LANGUAGE 'plpgsql';\n"
      "\n";
      fh_out.printf(sql, catname, type_prefix, 
		    catname, catname, catname);
    }
    {
      const char *sql = 
      "-- public: Rectangular Search (J2000)\n"
      "-- argument: ra_1[deg], ra_2[deg], dec_1[deg], dec_2[deg]\n"
      "CREATE FUNCTION f%sGetObjFromRectEq(ra1 FLOAT8, ra2 FLOAT8, dec1 FLOAT8, dec2 FLOAT8)\n"
      "  RETURNS SETOF %sCel\n"
      "  AS $$\n"
      "    DECLARE\n"
      "      a1 tInternalRectArgs;\n"
      "    BEGIN\n"
      "      a1 := _fMakeInternalRectArgs(ra1,ra2,dec1,dec2);\n"
      "      RETURN QUERY EXECUTE\n"
      "        _f%sGetSqlForRectangularSearch(a1.lon1,a1.lon2,a1.lon3,a1.lon4,a1.lat1,a1.lat2);\n"
      "      IF -90.0 <= a1.lat3 AND -90.0 <= a1.lat4 THEN\n"
      "        RETURN QUERY EXECUTE\n"
      "          _f%sGetSqlForRectangularSearch(a1.lon1,a1.lon2,a1.lon3,a1.lon4,a1.lat3,a1.lat4);\n"
      "      END IF;\n"
      "      RETURN;\n"
      "    END\n"
      "  $$ LANGUAGE 'plpgsql';\n"
      "\n";
      fh_out.printf(sql, catname, type_prefix, 
		    catname, catname);
    }
    fh_out.close();


    /*
     * Output C source for table info
     */
    if ( fh_out.openf("w", "%s_table_info._c",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    if ( i8_objid ) {	/* 8-byte objid */
	fh_out.printf("static const char *Huge_objid_str = "
		      "\"_objid\";\n");
	fh_out.printf("static const char *Huge_o_objid_str = "
		      "\"CAST(o._objid AS INT8)+%lld\";\n", 
		      (long long)OBJID_OFFSET);
    }
    else {		/* 4-byte objid */
	fh_out.printf("static const char *Huge_objid_str = "
		      "\"_objid\";\n");
	fh_out.printf("static const char *Huge_o_objid_str = "
		      "\"o._objid+%lld\";\n", 
		      (long long)OBJID_OFFSET);
    }
    fh_out.printf("static const int Huge_n_tables = %ld;\n",
		  (long)n_eqi_tables);
    fh_out.printf("static const char *Huge_tables[] = {\n");
    for ( i=0 ; i < n_eqi_tables ; i++ ) {
	tstring cname;
	fh_out.printf("  \"%s\"", 
		      get_table_name(catname,"eqi", i,n_eqi_tables, &cname));
	if ( i + 1 < n_eqi_tables ) fh_out.printf(",\n");
	else fh_out.printf("\n");
    }
    fh_out.printf("};\n");
    fh_out.printf("static const int Huge_check_deci[] = {\n");
    for ( i=0 ; i < deci_check.length() ; i++ ) {
	fh_out.printf("  %d,\n", deci_check[i]);
    }
    if ( DEC2DECI(90.0) <= max_deci ) fh_out.printf("  %d\n",max_deci + 1);
    else fh_out.printf("  %d\n",DEC2DECI(90.0));
    fh_out.printf("};\n");
    fh_out.close();

    if ( fh_out.openf("w", "%s_search_fnc.c",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("#define _TABLE_INFO_SRC__C \"%s_table_info._c\"\n",
		  l_catname.cstr());
    fh_out.printf("#define _RADIALSEARCH_FNC_NAME _%sgetsqlforradialsearch\n",
		  l_catname.cstr());
    fh_out.printf("#define _BOXSEARCHSIMPLE_FNC_NAME _%sgetsqlforboxsearchsimple\n",
		  l_catname.cstr());
    fh_out.printf("#define _BOXSEARCHDIV_FNC_NAME _%sgetsqlforboxsearchdiv\n",
		  l_catname.cstr());
    fh_out.printf("#define _RECTANGULARSEARCH_FNC_NAME _%sgetsqlforrectangularsearch\n",
		  l_catname.cstr());
    fh_out.printf("#include \"huge_search_fnc_template._c\"\n");
    fh_out.close();

    /*
     * Output others
     */
    if ( fh_out.openf("w", "%s_create_dir._sh",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("#!/bin/sh\n");
    fh_out.printf("\n");
    fh_out.printf("# UNIX \"postgres\" user has to do this:\n");
    fh_out.printf("\n");
    fh_out.printf("mkdir -p @TS_DIR@/%s_main\n", l_catname.cstr());
    fh_out.printf("mkdir -p @TS_DIR@/%s_main_pkey\n", l_catname.cstr());
    fh_out.printf("mkdir -p @TS_DIR@/%s_main_index\n", l_catname.cstr());
    fh_out.printf("mkdir -p @TS_DIR@/%s_eqi\n", l_catname.cstr());
    fh_out.printf("mkdir -p @TS_DIR@/%s_eqi_index_radeci\n", l_catname.cstr());
    fh_out.printf("mkdir -p @TS_DIR@/%s_eqi_index_xyzi16\n", l_catname.cstr());
    fh_out.close();

    if ( fh_out.openf("w", "%s_create_tablespace._sql",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    fh_out.printf("\n");
    fh_out.printf("-- Do after \"psql -U postgres\"\n");
    fh_out.printf("\n");
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_main OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_main';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_main_pkey OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_main_pkey';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_main_index OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_main_index';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("\n");
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_eqi OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_eqi';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_eqi_index_radeci OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_eqi_index_radeci';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("CREATE TABLESPACE @TS_PREFIX@%s_eqi_index_xyzi16 OWNER @TS_OWNER@ LOCATION '@TS_DIR@/%s_eqi_index_xyzi16';\n",l_catname.cstr(),l_catname.cstr());
    fh_out.printf("\n");
    fh_out.close();

    ret_status = 0;
 quit:
    return ret_status;
}
