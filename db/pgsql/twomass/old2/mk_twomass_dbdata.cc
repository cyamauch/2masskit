/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2011-10-17 15:12:16 cyamauch> */

/*
 *  mk_twomass_dbdata.cc
 *  - Create DB files for 2massKit using original 2MASS PSC text files.
 *
 *  Note:
 *  - This program requires SLLIB-1.1.0a or newer.  SLLIB is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *  - This program read compressed (.gz) text files.
 *
 *  How to compile:
 *    $ s++ mk_twomass_dbdata.cc
 *
 *  How to use:
 *    $ ./mk_twomass_dbdata /somewhere/2mass
 *    $ ./mk_twomass_dbdata ftp://...
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_twomass_dbdata /somewhere/2mass -
 *
 */

#include <sli/stdstreamio.h>
#include <sli/digeststreamio.h>
#include <sli/tstring.h>
#include <sli/tarray_tstring.h>
#include <sli/mdarray.h>
#include <stdlib.h>
#include <math.h>
using namespace sli;

/*
 * Settings for 2MASS
 */
/* ObiID */
#define OBJID_OFFSET  500000000LL
/* Number of child tables */
#define N_XYZI_CHILD_TABLES (92*8)
#define N_J2000I_CHILD_TABLES 92

#define CAT_NAME "twomass"
#define CAT_LIST_FILE "twomass_list.txt"
/* Set *AROUND* maximum number of entries in a cat file */
#define N_CAT_ENTRIES 6000000
/* Set number of rows in this catalog */
#define N_ALL_ENTRIES 470992970LL

/*
 * Settings for text DB files
 */
#define MAIN_DB_FILE CAT_NAME "_main_%04d.db.txt"
#define J2000I_DB_FILE CAT_NAME "_j2000i_%04d.db.txt"
#define XYZI_DB_FILE CAT_NAME "_xyzi_%04d.db.txt"

#define DB_DELIMITER "\t"
#define DB_NULL "~"

/*
 * Configurations for DB registration: should not be changed
 */
#define XYZ_INDEX_RESOLUTION 32400
#define SC_XYZI 2e9
#define RA2RAI(v) round_d2i(((v)-180.0)*1e7)
#define DEC2DECI(v) round_d2i((v)*1e7)
#define RAI2RA(v)  ((double)((long long)(v) + 1800000000LL)/1e7)
#define DECI2DEC(v) ((double)(v)/1e7)
#define XYZ2XYZI(v) round_d2i((v) * SC_XYZI)
#define XYZI2XYZ(v) ((double)(v) / SC_XYZI)

#define CC_DEG_TO_RAD 0.017453292519943295


/**
 * Round to nearest integer
 *
 * @param      v  Input (double)
 * @return     Rounded integer value
 */
static int round_d2i( double v )
{
    if ( v < 0 ) return (int)(v-0.5);
    else return (int)(v+0.5);
}

/*
 * General struct definitions
 */

typedef struct _ipos_entry {
    long long _objid;
    int cxi;
    int cyi;
    int czi;
    int rai;
    int deci;
} ipos_entry;

/*
 * for qsort()
 */
static int cmp_xyzi_tbl( const void *_p0, const void *_p1 )
{
    const ipos_entry *p0 = (const ipos_entry *)_p0;
    const ipos_entry *p1 = (const ipos_entry *)_p1;

    return p0->deci - p1->deci;
}

#include "../twomass/output_sql_and_c.cc"

/*
 * Main function
 */
int main( int argc, char *argv[] )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio fh_in;
    tstring line;
    size_t i, lcnt;

    tarray_tstring cat_file_list;
    const char *dir_name;

    const long long n_all_entries = N_ALL_ENTRIES;

    mdarray_int deci_check;
    mdarray_int czi_check;
    int max_rai;
    int max_deci;
    int max_czi;

    long long serial_id;
    long long min_serial_id;

    stdstreamio fh_main;
    stdstreamio fh_xyzi;
    stdstreamio fh_j2000i;
    int j2000i_ix;
    int xyzi_ix;

    ipos_entry *ipos_table_ptr;
    mdarray ipos_table(sizeof(*ipos_table_ptr), (void *)(&ipos_table_ptr));

    /*
     * Check args
     */
    if ( argc < 2 ) {
	sio.eprintf("[USAGE]\n");
	sio.eprintf("  $ %s directory_name [-]\n",argv[0]);
	sio.eprintf(" If last arg is set, "
		    "SQL and C files are also created.\n");
	sio.eprintf("[Example]\n");
	sio.eprintf("  $ %s /somewhere/2masspsc\n",argv[0]);
	sio.eprintf("  $ %s ftp://...\n", argv[0]);
	goto quit;
    }
    dir_name = argv[1];

    if ( argc < 3 ) {
	sio.eprintf("[INFO] SQL and C files will not be created.\n");
    }

    /*
     * Read list file
     */
    if ( fh_in.open("r", CAT_LIST_FILE) < 0 ) {
	sio.eprintf("[ERROR] fh_in.open() failed: %s.\n",CAT_LIST_FILE);
	goto quit;
    }
    i = 0;
    cat_file_list.resize(92);
    while ( (line=fh_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() ) {
	    cat_file_list[i] = line;
	    i++;
	}
    }
    fh_in.close();
    cat_file_list.resize(i);

    /*
     * Open files to output
     */

    j2000i_ix = 0;
    xyzi_ix = 0;

    if ( fh_j2000i.openf("w", J2000I_DB_FILE,j2000i_ix) < 0 ) {
	sio.eprintf("[ERROR] fh_j2000i.openf() failed\n");
	goto quit;
    }
    if ( fh_xyzi.openf("w", XYZI_DB_FILE,xyzi_ix) < 0 ) {
	sio.eprintf("[ERROR] fh_xyzi.openf() failed\n");
	goto quit;
    }

    j2000i_ix ++;
    xyzi_ix ++;

    /*
     * Main loop
     */
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    max_czi = XYZ2XYZI(-1.0);
    ipos_table.resize(N_CAT_ENTRIES);
    /* */
    min_serial_id = 1;
    max_rai = RA2RAI(0.0);
    lcnt = 0;
    /* */
    for ( i=0 ; i < cat_file_list.length() ; i++ ) {
	tstring filename;
	digeststreamio cat_in;
	tarray_tstring elms;

	long long n_unit_xyzi = n_all_entries / N_XYZI_CHILD_TABLES;
	long long n_unit_j2000i = n_all_entries / N_J2000I_CHILD_TABLES;

	filename.printf("%s/%s",dir_name,cat_file_list[i].cstr());
	sio.printf("Reading ... %s\r", filename.cstr());
	sio.flush();

	/* Open a cat file */
	if ( cat_in.open("r", filename.cstr()) < 0 ) {
	    sio.eprintf("[ERROR] cat_in.open() failed: %s.\n",
			filename.cstr());
	    goto quit;
	}

	if ( fh_main.openf("w", MAIN_DB_FILE, (int)i) < 0 ) {
	    sio.eprintf("[ERROR] fh_main.openf() failed\n");
	    goto quit;
	}

	while ( (line=cat_in.getline()) != NULL ) {
	    int rai,deci, cxi,czi,cyi;
	    double ra_r, dec_r, cx,cy,cz;
	    size_t k;
	    bool go_out;
	    line.chomp();
	    line.strreplace("\\N", DB_NULL, true);
	    line.strreplace(" |", DB_DELIMITER ,true);
	    line.strreplace("|", DB_DELIMITER, true);
	    elms.split(line, DB_DELIMITER, true);
	    /* */
	    rai = RA2RAI(elms[0].atof());
	    deci = DEC2DECI(elms[1].atof());
	    ra_r = RAI2RA(rai) * CC_DEG_TO_RAD;
	    dec_r = DECI2DEC(deci) * CC_DEG_TO_RAD;
	    cxi = XYZ2XYZI(cos(ra_r) * cos(dec_r));
	    cyi = XYZ2XYZI(sin(ra_r) * cos(dec_r));
	    czi = XYZ2XYZI(sin(dec_r));
	    cx = XYZI2XYZ(cxi);
	    cy = XYZI2XYZ(cyi);
	    cz = XYZI2XYZ(czi);
	    /* output main data files */
	    /* objid */
	    fh_main.printf("%lld%s", (long long)(OBJID_OFFSET + serial_id),
			   DB_DELIMITER);
	    /* designation */
	    fh_main.printf("%s%s", elms[5].cstr(), DB_DELIMITER);
	    /* ra, dec, ... */
	    for ( k=0 ; k < 5 ; k++ ) {
		fh_main.printf("%s%s", elms[k].cstr(), DB_DELIMITER);
	    }
	    for ( k++ ; k < elms.length() ; k++ ) {
		fh_main.printf("%s%s", elms[k].cstr(), DB_DELIMITER);
	    }
	    fh_main.printf("%.15g%s%.15g%s%.15g\n",
			   cx, DB_DELIMITER, cy, DB_DELIMITER, cz);

	    go_out = false;
	    if ( rai < max_rai || serial_id == n_all_entries ) {
		/* Sort ipos_entry */
		qsort(ipos_table_ptr, lcnt, sizeof(*ipos_table_ptr), 
		      &cmp_xyzi_tbl);
		/* */
		if ( ipos_table_ptr[0].deci < max_deci ) {
		    //sio.eprintf("[ERROR] Invalid DEC in cat file: %s\n",
		    //		filename.cstr());
		    //goto quit;
		    /* to be continued ... */
		    sio.eprintf("[INFO] Overwrapped: %lld\n", serial_id);
		}
		else {
		    go_out = true;
		}
		if ( serial_id == n_all_entries ) go_out = true;
		/* */
		max_deci = ipos_table_ptr[n_entries - 1].deci;
		max_czi = ipos_table_ptr[n_entries - 1].czi;
	    }

	    /* store rai,deci,etc. to buffer */
	    if ( go_out ) {

		size_t j;
		size_t n_entries = lcnt;

		/* close/open output files with partitioning */
		for ( j=0 ; j < n_entries ; j++ ) {
		    /* Write an entry */
		    fh_xyzi.printf("%lld%s%d%s%d%s%d\n",
			       ipos_table_ptr[j]._objid, DB_DELIMITER,
			       ipos_table_ptr[j].cxi, DB_DELIMITER,
			       ipos_table_ptr[j].cyi, DB_DELIMITER,
			       ipos_table_ptr[j].czi);
		    fh_j2000i.printf("%lld%s%d%s%d\n",
				 ipos_table_ptr[j]._objid, DB_DELIMITER,
				 ipos_table_ptr[j].rai, DB_DELIMITER,
				 ipos_table_ptr[j].deci);
		    if ( j + 1 < n_entries ) {
			/*
			  sio.printf("[%lld, %lld]\n",
			  n_unit_j2000i * j2000i_ix, min_serial_id + j);
			*/
			if ( xyzi_ix < N_XYZI_CHILD_TABLES &&
			     n_unit_xyzi * xyzi_ix <= min_serial_id + j &&
			     ipos_table_ptr[j].czi < ipos_table_ptr[j+1].czi ) {
			    fh_xyzi.close();
			    if ( fh_xyzi.openf("w", XYZI_DB_FILE,xyzi_ix) < 0 ) {
				sio.eprintf("[ERROR] fh_xyzi.openf() failed\n");
				goto quit;
			    }
			    czi_check[xyzi_ix-1] = ipos_table_ptr[j+1].czi;
			    xyzi_ix ++;
			}
			if ( j2000i_ix < N_J2000I_CHILD_TABLES &&
			     n_unit_j2000i * j2000i_ix <= min_serial_id + j &&
			     ipos_table_ptr[j].deci < ipos_table_ptr[j+1].deci ) {
			    fh_j2000i.close();
			    if ( fh_j2000i.openf("w", J2000I_DB_FILE,j2000i_ix) < 0 ) {
				sio.eprintf("[ERROR] fh_j2000i.openf() failed\n");
				goto quit;
			    }
			    deci_check[j2000i_ix-1] = ipos_table_ptr[j+1].deci;
			    j2000i_ix ++;
			}
		    }
		}
		/* reset */
		min_serial_id += n_entries;
		max_rai = RA2RAI(0.0);
		lcnt = 0;
	    }
	    if ( max_rai < rai ) max_rai = rai;
	    /* */
	    if ( ipos_table.length() <= lcnt ) {
		ipos_table.resize(lcnt + N_CAT_ENTRIES / 2);
	    }
	    ipos_table_ptr[lcnt]._objid = serial_id;	/* not apply offset */
	    ipos_table_ptr[lcnt].cxi = cxi;
	    ipos_table_ptr[lcnt].cyi = cyi;
	    ipos_table_ptr[lcnt].czi = czi;
	    ipos_table_ptr[lcnt].rai = rai;
	    ipos_table_ptr[lcnt].deci = deci;
	    /* */
	    lcnt++;
	    serial_id++;
	}

	cat_in.close();
	fh_main.close();

    }

    fh_xyzi.close();
    fh_j2000i.close();

    if ( serial_id - 1 != n_all_entries ) {
	sio.eprintf("[WARNING] broken catalg? ");
	sio.eprintf("          Invalid number of rows: %lld\n", serial_id - 1);
    }

    if ( argc < 3 ) {
	/* Skip output of SQL and C files */
	ret_status = 0;
	goto quit;
    }

    /*
     * Output SQL statements and C source
     */

    if ( output_sql_and_c(cat_file_list.length(), 
			  czi_check, deci_check, max_czi, false) < 0 ) {
	sio.eprintf("[ERROR] output_sql_and_c() failed\n");
	goto quit;
    }

    ret_status = 0;
 quit:
    return ret_status;
}
