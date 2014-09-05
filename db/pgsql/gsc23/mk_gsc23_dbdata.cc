/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2014-09-05 17:21:35 cyamauch> */

/*
 *  mk_gsc23_dbdata.cc
 *  - Create DB files for 2massKit using original GSC-2.3 FITS catalog files.
 *
 *  Note:
 *  - This program requires SLLIB-1.4.0 and SFITSIO-1.4.0 or newer.
 *    SLLIB and SFITSIO is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *  - This program can read compressed (.gz or .bz2) FITS files.
 *
 *  How to compile:
 *    $ s++ mk_gsc23_dbdata.cc -lsfitsio
 *
 *  How to use:
 *    $ ./mk_gsc23_dbdata /somewhere/GSC2.3
 *    $ ./mk_gsc23_dbdata ftp://catftp:PASSWD@vizier.nao.ac.jp/FTP/GSC2.3
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_gsc23_dbdata /somewhere/GSC2.3 -
 *
 */

#include <sli/stdstreamio.h>
#include <sli/pipestreamio.h>
#include <sli/mdarray.h>
#include <sli/tarray.h>
#include <sli/fitscc.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
using namespace sli;

#include "../pg_module/common_conv.h"

/*
 * Settings for GSC-2.3
 */
/* ObiID */
#define OBJID_OFFSET 900000000000LL	/* waiting GSC-2.3.3 ... */
/* Number of files for main table */
#define N_MAIN_FILES 106		/* set to < 2 GByte in each file */
/* Number of child tables */
#define N_EQI_CHILD_TABLES 1440		/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

//#define GET_ZONEID(v) (((v) + DEC2DECI(90)) / 1000000)       /* div = 1800 */
#define GET_ZONEID(v) (((v) + DEC2DECI(90)) / 2500000)	       /* div = 720 */

#define CAT_NAME "Gsc23"
#define CAT_LIST_FILE "gsc23_list.txt"

#define N_ALL_ENTRIES 945592683LL


/*
 * Settings for text DB files
 */
#define DB_DELIMITER "\t"	/* <- do not change */
#define DB_NULL "~"


/*
 * General struct definitions
 */

typedef struct _ipos_entry {
    long long _objid;
    int rai;
    int deci;
    double cx;
    double cy;
    double cz;
    int fileid;
    int rowid;
} ipos_entry;


/*
 * function given to qsort(): sort by deci
 */
static int cmp_eqi_tbl_deci( const void *_p0, const void *_p1 )
{
    const ipos_entry *p0 = (const ipos_entry *)_p0;
    const ipos_entry *p1 = (const ipos_entry *)_p1;

    if ( p1->deci < p0->deci ) return 1;
    else if ( p0->deci < p1->deci ) return -1;
    else return 0;
}

/*
 * function given to qsort(): sort by rai
 */
static int cmp_eqi_tbl_rai( const void *_p0, const void *_p1 )
{
    const ipos_entry *p0 = (const ipos_entry *)_p0;
    const ipos_entry *p1 = (const ipos_entry *)_p1;

    if ( p1->rai < p0->rai ) return 1;
    else if ( p0->rai < p1->rai ) return -1;
    else return 0;
}

#include "../twomass/output_sql_and_c.cc"

static int read_and_process_fits( const char *dir_name, 
				  tarray_tstring &cat_file_list, size_t fileid,
				  fitscc *fits, mdarray *ipos_table,
				  long long *out_serial_id )
{
    stdstreamio sio;
    fits::table_def tdef;
    int ret_status = -1;
    long idx, idx_gsc1id, idx_hstid, idx_ra, idx_dec, j, k;
    long long serial_id = 1;
    ipos_entry *ipos_table_ptr = NULL;
    tstring _filename;
    const char *filename;
    bool fits_ok = true;

    if ( out_serial_id ) serial_id = *out_serial_id;

    filename = 
	_filename.printf("%s/%s",dir_name,cat_file_list[fileid].cstr()).cstr();
    sio.printf("Reading ... %s\r", filename);
    sio.flush();

    j=0;
    while ( 1 ) {
	if ( ! (fits->read_stream(filename) < 0) ) break;
	if ( ! (fits->readf_stream("%s.gz",filename) < 0) ) break;
	if ( ! (fits->readf_stream("%s.bz2",filename) < 0) ) break;
	if ( j < 64 ) {
	    j++;
	    sleep(8);
	    sio.eprintf("[WARNING] Retrying ... %s.\n",filename);
	}
	else {
	    sio.eprintf("[ERROR] fits->read_stream() failed: %s.\n",filename);
	    fits_ok = false;
	    break;
	}
    }
    if ( fits_ok == true ) {
	if ( fits->length() < 3 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[0]: %s\n",filename);
	    fits_ok = false;
	}
	else if ( fits->hdutype(2L) != FITS::BINARY_TABLE_HDU ) {
	    sio.eprintf("[ERROR] Invalid FITS file[1]: %s\n",filename);
	    fits_ok = false;
	}
    }
    if ( fits_ok == true ) {

	fits_table &tbl = fits->table(2L);
	//sio.printf("table row length = %ld\n",tbl.row_length());
	/* Erase Unused Cols */
	idx = tbl.col_index("gscID2");
	if ( idx < 0 ) idx = tbl.col_index("GSCID2");
	if ( idx < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[2]: %s\n",filename);
	    goto quit;
	}
	tbl.erase_cols(idx,1);
	/* */
	idx = tbl.col_index("raProperMotion");
	if ( idx < 0 ) idx = tbl.col_index("RAPROPERMOTION");
	if ( idx < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[3]: %s\n",filename);
	    goto quit;
	}
	tbl.erase_cols(idx,5);
	/* */
	idx = tbl.col_index("RMag");
	if ( idx < 0 ) idx = tbl.col_index("RMAG");
	if ( idx < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[4]: %s\n",filename);
	    goto quit;
	}
	tbl.erase_cols(idx,3*5);
	/* */
	idx = tbl.col_index("variableFlag");
	if ( idx < 0 ) idx = tbl.col_index("VARIABLEFLAG");
	if ( idx < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[5]: %s\n",filename);
	    goto quit;
	}
	tbl.erase_cols(idx,1);
	/* */
	idx_gsc1id = tbl.col_index("gsc1ID");
	if ( idx_gsc1id < 0 ) idx_gsc1id = tbl.col_index("GSC1ID");
	if ( idx_gsc1id < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[6]: %s\n",filename);
	    goto quit;
	}
	/* */
	idx_hstid = tbl.col_index("hstID");
	if ( idx_hstid < 0 ) idx_hstid = tbl.col_index("HSTID");
	if ( idx_hstid < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[7]: %s\n",filename);
	    goto quit;
	}
	/* */
	idx_ra = tbl.col_index("RightAsc");
	if ( idx_ra < 0 ) idx_ra = tbl.col_index("RIGHTASC");
	if ( idx_ra < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[8]: %s\n",filename);
	    goto quit;
	}
	/* */
	idx_dec = tbl.col_index("Declination");
	if ( idx_dec < 0 ) idx_dec = tbl.col_index("DECLINATION");
	if ( idx_dec < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[9]: %s\n",filename);
	    goto quit;
	}
	/* change NULL string to output */
	tbl.assign_null_svalue(DB_NULL);
	/* set TNULL on 2nd column */
	/* (original FITS files should have this...) */
	tdef = tbl.col(idx_gsc1id).definition();
	tdef.tnull = "___NULL___ ";
	tbl.col(idx_gsc1id).define(tdef);
	/* set output format */
	for ( k=0 ; k < tbl.col_length() ; k++ ) {
	    tdef = tbl.col(k).definition();
	    if ( tbl.col(k).type() == FITS::FLOAT_T ) {
		tstring cn = tbl.col_name(k);
		cn.toupper();
		if ( cn.strcmp("RAEPSILON") == 0 ) tdef.tdisp = "F.2";
		else if ( cn.strcmp("DECEPSILON") == 0 ) tdef.tdisp = "F.2";
		else if ( cn.strcmp("SEMIMAJORAXIS") == 0 ) tdef.tdisp = "F.2";
		else if ( cn.strcmp("ECCENTRICITY") == 0 ) tdef.tdisp = "F.2";
		else if ( cn.strcmp("POSITIONANGLE") == 0 ) tdef.tdisp = "F.1";
		else if ( 0 <= cn.find("MAGCODE") ) { /* nothing */ }
		else if ( 0 <= cn.find("MAGERR") ) tdef.tdisp = "F.2";
		else if ( 0 <= cn.find("MAG") ) tdef.tdisp = "F.2";
		else if ( tdef.tdisp == NULL || tdef.tdisp[0] == '\0' ) {
		    tdef.tdisp = "G.7";
		}
		tbl.col(k).define(tdef);
	    }
	    else if ( tbl.col(k).type() == FITS::DOUBLE_T ) {
		if ( tdef.tdisp == NULL || tdef.tdisp[0] == '\0' ) {
		    tdef.tdisp = "G.15";
		    tbl.col(k).define(tdef);
		}
	    }
	}
	/* alloc ipos_table */
	if ( ipos_table ) {
	    ipos_table->resize(tbl.row_length());
	    ipos_table_ptr = (ipos_entry *)(ipos_table->data_ptr());
	}
	/* convert radian to degree, etc. */
	for ( j=0 ; j < tbl.row_length() ; j++ ) {
	    double v;
	    long long _objid;
	    int rai, deci;
	    /* id */
	    _objid = serial_id;	/* not apply offset */
	    /* ra */
	    v = tbl.col(idx_ra).dvalue(j) * CC_RAD_TO_DEG;
	    if ( 360.0 <= v ) v = 0.0;
	    rai = RA2RAI(v);
	    v = RAI2RA(rai);
	    tbl.col(idx_ra).assign(v, j);
	    /* dec */
	    v = tbl.col(idx_dec).dvalue(j) * CC_RAD_TO_DEG;
	    if ( 90.0 < v ) v = 90.0;
	    else if ( v < -90.0 ) v = -90.0;
	    deci = DEC2DECI(v);
	    v = DECI2DEC(deci);
	    tbl.col(idx_dec).assign(v, j);
	    /* */
	    if ( ipos_table_ptr ) {
		double ra_r, dec_r;
		/* cx,cy,cz */
		ra_r  = tbl.col(idx_ra).dvalue(j) * CC_DEG_TO_RAD;
		dec_r = tbl.col(idx_dec).dvalue(j) * CC_DEG_TO_RAD;
		ipos_table_ptr[j].cx = cos(ra_r) * cos(dec_r);
		ipos_table_ptr[j].cy = sin(ra_r) * cos(dec_r);
		ipos_table_ptr[j].cz = sin(dec_r);
		/* */
		ipos_table_ptr[j].fileid = fileid;
		ipos_table_ptr[j].rowid = j;
		/* */
		ipos_table_ptr[j]._objid = _objid;
		ipos_table_ptr[j].rai = rai;
		ipos_table_ptr[j].deci = deci;
	    }
	    /* */
	    serial_id ++;
	}
	
	if ( out_serial_id ) *out_serial_id = serial_id;

    }	
    else {	/* error of reading FITS */
	goto quit;
    }
    
    ret_status = 0;
 quit:
    return ret_status;
}



/*
 * Main function
 */
int main( int argc, char *argv[] )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio fh_in;
    pipestreamio ph_in;
    tstring line;
    size_t i;

    tstring l_catname;
    tarray_tstring cat_file_list;
    mdarray_size fits_ref_cntr;
    tarray<fitscc> fits_rec;
    const char *dir_name;

    mdarray_size n_entries;
    size_t max_line_count;
    long long n_all_entries;

    mdarray_int deci_check;
    int max_deci;

    long long serial_id;
    long long min_serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    int main_ix;
    int eqi_ix;

    mdarray_long col_odr;

    /* "Foo" => "foo" */
    l_catname.assign(CAT_NAME).tolower();

    /*
     * Check args
     */
    if ( argc < 2 ) {
	sio.eprintf("[USAGE]\n");
	sio.eprintf("  $ %s directory_name [-]\n",argv[0]);
	sio.eprintf(" If last arg is set, "
		    "SQL and C files are also created.\n");
	sio.eprintf("[Example]\n");
	sio.eprintf("  $ %s /somewhere/GSC2.3\n",argv[0]);
	sio.eprintf("  $ %s ftp://catftp:PASSWD@vizier.nao.ac.jp/FTP/GSC2.3\n",
		    argv[0]);
	sio.eprintf(" If you want to use vizier.nao.ac.jp, "
		    "contact to data_center@dbc.nao.ac.jp\n"
		    " to obtain PASSWD.\n");
	goto quit;
    }
    dir_name = argv[1];

    if ( argc < 3 ) {
	sio.eprintf("[INFO] SQL and C files will not be created.\n");
    }

    /*
     * Check temporary file
     */
    if ( ph_in.openf("r", "ls | grep %s_tmp_", l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] ph_in.openf() failed.\n");
	goto quit;
    }
    i = 0;
    while ( ph_in.getline() != NULL ) i++;
    ph_in.close();
    if ( 0 < i ) {
	sio.eprintf("[ERROR] Found temporary file(s).  Please remove!\n");
	sio.eprintf("        $ rm -f %s_tmp_*.bin\n", l_catname.cstr());
	goto quit;
    }

    /*
     * Read list file
     */
    if ( fh_in.open("r", CAT_LIST_FILE) < 0 ) {
	sio.eprintf("[ERROR] fh_in.open() failed: %s.\n", CAT_LIST_FILE);
	goto quit;
    }
    i = 0;
    cat_file_list.resize(32768);
    while ( (line=fh_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() ) {
	    cat_file_list[i] = line;
	    i++;
	}
    }
    fh_in.close();

    cat_file_list.resize(i);
    fits_ref_cntr.resize(i);
    fits_rec.resize(i);

    /*
     * Output temp binary files,
     * and get max n_entries and n_all_entries
     */
    n_entries.resize(2*GET_ZONEID(0));
    for ( i=0 ; i < cat_file_list.length() ; i++ ) {
	fitscc fits;
	tstring cell;
	long j;
	int max_zoneid = 0, min_zoneid = 0, n_zoneid;

	ipos_entry *ipos_table_ptr;
	mdarray ipos_table(sizeof(*ipos_table_ptr), true,
			   (void *)(&ipos_table_ptr));

	/* read and prepare a FITS file  */
	if ( read_and_process_fits( dir_name, cat_file_list, i, &fits, 
				    &ipos_table, NULL ) < 0 ) {
	    sio.eprintf("[ERROR] read_and_process_fits() failed.\n");
	    goto quit;
	}
	fits_table &tbl = fits.table(2L);

	if ( i == 0 ) {
	    /* change output order of columns */
	    col_odr.resize(tbl.col_length());
	    for ( j=0 ; j < tbl.col_length() ; j++ ) col_odr[j] = j;
	    col_odr[0] = 1;
	    col_odr[1] = 0;
	    col_odr[4] = 5;
	    col_odr[5] = 6;
	    col_odr[6] = 4;
	}

	/* check range of J2000 dec */
	for ( j=0 ; j < tbl.row_length() ; j++ ) {
	    int zid = GET_ZONEID(ipos_table_ptr[j].deci);
	    if ( j == 0 ) {
		max_zoneid = zid;
		min_zoneid = zid;
	    }
	    else {
		if ( max_zoneid < zid ) max_zoneid = zid;
		if ( zid < min_zoneid ) min_zoneid = zid;
	    }
	}
	if ( min_zoneid < 0 ) {
	    sio.eprintf("[ERROR] Invalid FITS file[10]: %zd\n",i);
	    goto quit;
	}
	n_zoneid = 1 + max_zoneid - min_zoneid;
	//sio.printf("n_files = %d\n",n_zoneid);

	/* create temporary binary files */
	{
	    stdstreamio fh_bin[n_zoneid];
	    /* open with "append" mode */
	    for ( j=0 ; j < n_zoneid ; j++ ) {
		if ( fh_bin[j].openf("a", "%s_tmp_%05ld.bin", l_catname.cstr(),
				     (long)(min_zoneid + j)) < 0 ) {
		    sio.eprintf("[ERROR] fh_bin[].openf() failed\n");
		    goto quit;
		}
	    }
	    /* distribute positional data ... */
	    for ( j=0 ; j < tbl.row_length() ; j++ ) {
		int zid = GET_ZONEID(ipos_table_ptr[j].deci);
		fh_bin[zid - min_zoneid].write(ipos_table_ptr + j, 
					       sizeof(*ipos_table_ptr));
		n_entries[zid] ++;
	    }
	    /* close */
	    for ( j=0 ; j < n_zoneid ; j++ ) {
		fh_bin[j].close();
	    }
	}

    }	/* LOOP: i < cat_file_list.length() */


    /*
     * Get Maximum values
     */

    n_all_entries = 0;
    max_line_count = 0;
    for ( i=0 ; i < n_entries.length() ; i++ ) {
	n_all_entries += n_entries[i];
	if ( max_line_count < n_entries[i] ) max_line_count = n_entries[i];
    }

    if ( N_ALL_ENTRIES != n_all_entries ) {
	sio.eprintf("[WARNING] N_ALL_ENTRIES = %lld, but\n",N_ALL_ENTRIES);
	sio.eprintf("[WARNING] n_all_entries = %lld\n",n_all_entries);
    }

    /*
     * Open files to output
     */

    /* create DB files */
    main_ix = 0;

    get_main_db_filename(CAT_NAME, main_ix, N_MAIN_FILES, &fname);
    if ( fh_main.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_main.open() failed\n");
	goto quit;
    }

    main_ix ++;

    eqi_ix = 0;

    get_eqi_db_tmp_filename(CAT_NAME, eqi_ix, N_EQI_CHILD_TABLES, &fname);
    if ( fh_eqi.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_eqi.open() failed\n");
	goto quit;
    }

    eqi_ix ++;

    /*
     * Sort and output EQI data files
     */
    max_deci = DEC2DECI(-90.0);

    serial_id = 1;
    min_serial_id = 1;
    for ( i=0 ; i < n_entries.length() ; i++ ) {
	size_t j;
	tstring filename, cell;
	ipos_entry *ipos_table_ptr;
	mdarray ipos_table(sizeof(*ipos_table_ptr), true,
			   (void *)(&ipos_table_ptr));
	char htm6[16];

	long long n_unit_main = n_all_entries / N_MAIN_FILES;
	long long n_unit_eqi = n_all_entries / N_EQI_CHILD_TABLES;

	/* read all binary data in a file */
	ipos_table.resize(n_entries[i]);
	filename.printf("%s_tmp_%05ld.bin", l_catname.cstr(), (long)i);
	if ( fh_in.open("r", filename.cstr()) < 0 ) {
	    sio.eprintf("[ERROR] fh_in.open() failed: %s.\n", filename.cstr());
	    goto quit;
	}
	fh_in.read(ipos_table_ptr, n_entries[i] * sizeof(*ipos_table_ptr));
	fh_in.close();

	/*
	 * [MAIN]
	 */

	/* Sort ipos_entry by rai */
	qsort(ipos_table_ptr, n_entries[i], sizeof(*ipos_table_ptr), 
	      &cmp_eqi_tbl_rai);

	fits_ref_cntr.fill(0);
	/* count each FITS reference */
	for ( j=0 ; j < n_entries[i] ; j++ ) {
	    size_t fid = ipos_table_ptr[j].fileid;
	    fits_ref_cntr[fid] ++;
	}

	/* close/open output files with partitioning [MAIN] */
	for ( j=0 ; j < n_entries[i] ; j++ ) {
	    long k;
	    size_t fid = ipos_table_ptr[j].fileid;
	    long rowid = ipos_table_ptr[j].rowid;
	    if ( fits_rec[fid].length() != 3 ) {
		/* read FITS file and prepare a fitscc object */
		if ( read_and_process_fits( dir_name, cat_file_list, fid, 
					 &(fits_rec[fid]), NULL, NULL ) < 0 ) {
		    sio.eprintf("[ERROR] read_and_process_fits() failed.\n");
		    goto quit;
		}
	    }
	    fits_table &tbl = fits_rec[fid].table(2L);

	    /* assign objid ... */
	    ipos_table_ptr[j]._objid = serial_id;

	    /* output objid */
	    fh_main.printf("%lld%s", 
			  (long long)(OBJID_OFFSET + ipos_table_ptr[j]._objid),
			  DB_DELIMITER);
	    /* output catalog contents */
	    for ( k=0 ; k < tbl.col_length() ; k++ ) {
		const char *to_out;
		if ( tbl.col(col_odr[k]).type() == FITS::STRING_T ) {
		    cell.assign(tbl.col(col_odr[k]).svalue(rowid)).rtrim();
		    to_out = cell.cstr();
		}
		else to_out = tbl.col(col_odr[k]).svalue(rowid);
		fh_main.printf("%s%s", to_out, DB_DELIMITER);
	    }
	    /* output HTM6 */
	    cat_file_list[fid].getstr(9, htm6, 9);
	    fh_main.printf("%s%s", htm6, DB_DELIMITER);
	    /* output cx,cy,cz */
	    fh_main.printf("%.17g%s", ipos_table_ptr[j].cx,
			   DB_DELIMITER);
	    fh_main.printf("%.17g%s", ipos_table_ptr[j].cy,
			   DB_DELIMITER);
	    fh_main.printf("%.17g%s", ipos_table_ptr[j].cz,
			   "\n");
	    /* Partitioning */
	    if ( main_ix < N_MAIN_FILES &&
		 n_unit_main * main_ix <= min_serial_id + (long long)j ) {
		fh_main.close();
		get_main_db_filename(CAT_NAME, main_ix, N_MAIN_FILES, &fname);
		if ( fh_main.open("w", fname.cstr()) < 0 ) {
		    sio.eprintf("[ERROR] fh_main.open() failed\n");
		    goto quit;
		}
		main_ix ++;
	    }

	    /* if count==0, free memory space of FITS */
	    fits_ref_cntr[fid] --;
	    if ( fits_ref_cntr[fid] == 0 ) {
		fits_rec[fid].init();
	    }

	    serial_id ++;
	}

	/*
	 * [EQI]
	 */

	/* Sort ipos_entry by deci */
	qsort(ipos_table_ptr, n_entries[i], sizeof(*ipos_table_ptr), 
	      &cmp_eqi_tbl_deci);
	if ( ipos_table_ptr[0].deci < max_deci ) {
	    sio.eprintf("[ERROR] Invalid DEC in cat file: %s\n",
			filename.cstr());
	    goto quit;
	}
	max_deci = ipos_table_ptr[n_entries[i] - 1].deci;

	/* close/open output files with partitioning [EQI] */
	for ( j=0 ; j < n_entries[i] ; j++ ) {
	    /* Write an entry (binary) */
	    fh_eqi.write(ipos_table_ptr + j, sizeof(*ipos_table_ptr));
	    /* Partitioning */
	    if ( j + 1 < n_entries[i] ) {
		if ( eqi_ix < N_EQI_CHILD_TABLES &&
		     n_unit_eqi * eqi_ix <= min_serial_id + (long long)j &&
		     ipos_table_ptr[j].deci < ipos_table_ptr[j+1].deci ) {
		    /* close current */
		    fh_eqi.close();
		    /* register new check deci value */
		    deci_check[eqi_ix-1] = ipos_table_ptr[j+1].deci;
		    /* read tmp binary, sort order for optimization,
		       and write DB text file */
		    if ( read_optimize_and_write_eqi(CAT_NAME, max_line_count,
						  DB_DELIMITER,
						  eqi_ix-1, N_EQI_CHILD_TABLES,
						  N_DIV_EQI_SECTION,
						  &cmp_eqi_tbl_rai) < 0 ) {
			sio.eprintf("[ERROR] "
				    "read_optimize_and_write_eqi() failed\n");
			goto quit;
		    }
		    /* open new tmp binary */
		    get_eqi_db_tmp_filename(CAT_NAME, eqi_ix, 
					    N_EQI_CHILD_TABLES, &fname);
		    if ( fh_eqi.open("w", fname.cstr()) < 0 ) {
			sio.eprintf("[ERROR] fh_eqi.open() failed\n");
			goto quit;
		    }
		    eqi_ix ++;
		}
	    }
	}
	min_serial_id += n_entries[i];
    }

    fh_main.close();
    fh_eqi.close();
    /* read tmp binary, sort order for optimization,
       and write DB text file */
    if ( read_optimize_and_write_eqi(CAT_NAME, max_line_count, DB_DELIMITER,
				     eqi_ix-1, N_EQI_CHILD_TABLES,
				     N_DIV_EQI_SECTION,
				     &cmp_eqi_tbl_rai) < 0 ) {
	sio.eprintf("[ERROR] read_optimize_and_write_eqi() failed\n");
	goto quit;
    }

    /*
     * Output SQL statements and C source
     */

    if ( output_sql_and_c(CAT_NAME, N_MAIN_FILES, DB_DELIMITER, DB_NULL,
			  deci_check, max_deci, true) < 0 ) {
	sio.eprintf("[ERROR] output_sql_and_c() failed\n");
	goto quit;
    }

    ret_status = 0;
 quit:
    return ret_status;
}
