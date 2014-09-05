/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2014-09-05 17:27:28 cyamauch> */

/*
 *  mk_wise3band_dbdata.cc
 *  - Create DB files for 2massKit using original WISE 3-band compressed text files.
 *
 *  Note:
 *  - This program requires SLLIB-1.4.0 or newer.  SLLIB is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *  - This program read compressed (.gz or .bz2) text files.
 *
 *  How to compile:
 *    $ s++ mk_wise3band_dbdata.cc
 *
 *  How to use:
 *    $ ./mk_wise3band_dbdata /somewhere/WISE/wise-3band
 *    $ ./mk_wise3band_dbdata http://irsadist.ipac.caltech.edu/wise-3band
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_wise3band_dbdata /somewhere/WISE/wise-3band -
 *
 */

#include <sli/stdstreamio.h>
#include <sli/gzstreamio.h>
#include <sli/digeststreamio.h>
#include <sli/tstring.h>
#include <sli/tarray_tstring.h>
#include <sli/tarray.h>
#include <sli/mdarray.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
using namespace sli;

#include "../pg_module/common_conv.h"

/*
 * Settings for WISE 3-band
 */
/* ObiID */
#define OBJID_OFFSET 1600000000LL
/* Number of files for main table */
#define N_MAIN_FILES 180		/* set to < 2 GByte in each file */
/* Number of child tables */
#define N_EQI_CHILD_TABLES 512		/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

#define N_ZONE 1800
#define GET_ZONEID(v) (((v) + DEC2DECI(90)) / 1000000)       /* div = 1800 */
//#define N_ZONE 720
//#define GET_ZONEID(v) (((v) + DEC2DECI(90)) / 2500000)     /* div = 720 */

#define CAT_NAME "Wise3band"
#define CAT_LIST_FILE "wise3band_list.txt"
#define N_CAT_COLS 242
/* Set *AROUND* maximum number of entries in a cat file */
//#define N_CAT_ENTRIES 11280000
/* Set *AROUND* maximum number of entries in a zone */
#define N_ZONE_ENTRIES 428100
/* Set number of rows in this catalog */
#define N_ALL_ENTRIES 261418479LL

/*
 * Settings for text DB files
 */
#define DB_DELIMITER "\t"	/* <- do not change */
#define DB_NULL "~"

/*
 * Settings for text zone files
 */
#define OUTPUT_ZONE_FILES 1	/* <- enable when creating sorted zone files */


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

static int output_zone( int zone_id, tarray_tstring &text_table,
			mdarray &ipos_table, ipos_entry *ipos_table_ptr,
			stdstreamio &fh_main, stdstreamio &fh_eqi,
			mdarray_int &deci_check,
			mdarray_int &width_min, mdarray_int &width_max,
			mdarray_double &numeric_abs_max,
			size_t *ret_ipos_cntr, long long *ret_serial_id,
			int *ret_main_ix, int *ret_eqi_ix,
			int *ret_max_deci, bool finish )
{
    int return_status = -1;
    size_t ipos_cntr = *ret_ipos_cntr;
    long long serial_id = *ret_serial_id;
    int main_ix = *ret_main_ix;
    int eqi_ix = *ret_eqi_ix;
    int max_deci = *ret_max_deci;
    const long long n_unit_main = N_ALL_ENTRIES / N_MAIN_FILES;
    const long long n_unit_eqi = N_ALL_ENTRIES / N_EQI_CHILD_TABLES;
    stdstreamio sio;
    size_t k;

    /* Sort ipos_entry [rai] */
    qsort(ipos_table_ptr, ipos_cntr, sizeof(*ipos_table_ptr), 
	  &cmp_eqi_tbl_rai);

#ifdef OUTPUT_ZONE_FILES
    /* output sorted zone files (for general purpose) */
    digeststreamio fh_out;
    if ( fh_out.openf("w", "wise-3band-wdb-zone%04d.txt.gz", zone_id) < 0 ) {
        sio.eprintf("[ERROR] fh_out.openf() failed\n");
	goto quit;
    }
    for ( k=0 ; k < ipos_cntr ; k++ ) {
        const tstring &to_out = text_table[ipos_table_ptr[k]._objid];
	fh_out.putstr(to_out.cstr());
    }
    if ( fh_out.close() < 0 ) {
        sio.eprintf("[ERROR] fh_out.close() failed\n");
	goto quit;
    }
#endif

    /* output main data files */
    for ( k=0 ; k < ipos_cntr ; k++ ) {
        tstring &to_out = text_table[ipos_table_ptr[k]._objid];
	size_t l;
	ssize_t p0, p1, width;

	/* objid */
	fh_main.printf("%lld%s", (long long)(OBJID_OFFSET + serial_id + k),
		       DB_DELIMITER);

	/* loop for a row */
	p0 = 0;
	for ( l = 0 ; l < N_CAT_COLS ; l++ ) {
	    p1 = to_out.find(p0,'|');
	    if ( p1 < 0 ) {
	        sio.eprintf("[ERROR] too small elements\n");
		goto quit;
	    }
	    /* check width of data */
	    width = p1 - p0;
	    if ( width < width_min[l] ) width_min[l] = width;
	    if ( width_max[l] < width ) width_max[l] = width;
	    /* check maximum numeric abs value */
	    if ( 0 < width ) {
	        double vv = fabs(to_out.atof(p0));
		if ( numeric_abs_max[l] < vv ) numeric_abs_max[l] = vv;
	    }
	    /* output columns that are not xyz */
	    if ( l != 44 && l != 45 && l != 46 && /* w?frtr */
        l != 203 && /* rel */
		 l != 238 && l != 239 && l != 240 /* x,y,z */ ) {
	        /* check null */
	        if ( width == 0 ) {
		    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
		}
		else {
		    to_out[p1] = '\0';	       /* overwrite! */
		    fh_main.printf("%s%s", to_out.cstr() + p0, DB_DELIMITER);
		}
	    }
	    p0 = p1 + 1;
	}
	/* output cx,cy,cz */
	fh_main.printf("%.17g%s", ipos_table_ptr[k].cx, DB_DELIMITER);
	fh_main.printf("%.17g%s", ipos_table_ptr[k].cy, DB_DELIMITER);
	fh_main.printf("%.17g%s", ipos_table_ptr[k].cz, "\n");

	/* Partitioning for main table files */
	if ( main_ix < N_MAIN_FILES &&
	     n_unit_main * main_ix <= serial_id + (long long)k ) {
	    tstring fname;
	    if ( fh_main.close() < 0 ) {
	        sio.eprintf("[ERROR] fh_main.close() failed\n");
		goto quit;
	    }
	    get_main_db_filename(CAT_NAME, main_ix, 
				 N_MAIN_FILES, &fname);
	    if ( fh_main.open("w", fname.cstr()) < 0 ) {
	        sio.eprintf("[ERROR] fh_main.open() failed\n");
		goto quit;
	    }
	    main_ix ++;
	}
	
    }	/* end of loop for output of main */

    /* cleanup text_table */
    text_table.clean();

    /* update _objid of ipos_table */
    for ( k=0 ; k < ipos_cntr ; k++ ) {
        /* not apply offset */
        ipos_table_ptr[k]._objid = serial_id + k;
    }

    /* Sort ipos_entry [deci] */
    qsort(ipos_table_ptr, ipos_cntr, sizeof(*ipos_table_ptr), 
	  &cmp_eqi_tbl_deci);

    if ( ipos_table_ptr[0].deci < max_deci ) {
        sio.eprintf("[ERROR] Invalid DEC in cat file: %.15g\n",
		    DECI2DEC(ipos_table_ptr[0].deci));
	goto quit;
    }
    max_deci = ipos_table_ptr[ipos_cntr - 1].deci;

    /* close/open output files with partitioning */
    for ( k=0 ; k < ipos_cntr ; k++ ) {
        /* Write an entry (binary) */
        fh_eqi.write(ipos_table_ptr + k, sizeof(*ipos_table_ptr));
		      
	/* Partitioning */
	if ( k + 1 < ipos_cntr ) {
	    if ( eqi_ix < N_EQI_CHILD_TABLES &&
		 n_unit_eqi * eqi_ix <= serial_id + (long long)k &&
		 ipos_table_ptr[k].deci < ipos_table_ptr[k+1].deci ) {
	        tstring fname;
	        /* close current */
	        if ( fh_eqi.close() < 0 ) {
		    sio.eprintf("[ERROR] fh_eqi.close() failed\n");
		    goto quit;
		}
		/* register new check deci value */
		deci_check[eqi_ix-1] = ipos_table_ptr[k+1].deci;
		/* read tmp binary, sort order for optimization,
		   and write DB text file */
		if ( read_optimize_and_write_eqi(CAT_NAME,
						 N_ZONE_ENTRIES,
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

    serial_id += ipos_cntr;
    ipos_cntr = 0;


    /* to flush at last */
    if ( finish ) {

      if ( fh_main.close() < 0 ) {
          sio.eprintf("[ERROR] fh_main.close() failed\n");
	  goto quit;
      }
      if ( fh_eqi.close() < 0 ) {
          sio.eprintf("[ERROR] fh_eqi.close() failed\n");
	  goto quit;
      }

	/* read tmp binary, sort order for optimization,
	   and write DB text file */
	if ( read_optimize_and_write_eqi(CAT_NAME,
					 N_ZONE_ENTRIES,
					 DB_DELIMITER,
					 eqi_ix-1, N_EQI_CHILD_TABLES,
					 N_DIV_EQI_SECTION,
					 &cmp_eqi_tbl_rai) < 0 ) {
	  sio.eprintf("[ERROR] "
		      "read_optimize_and_write_eqi() failed\n");
	  goto quit;
	}

    }

    return_status = 0;

 quit:

    /* return */
    *ret_ipos_cntr = ipos_cntr;
    *ret_serial_id = serial_id;
    *ret_main_ix = main_ix;
    *ret_eqi_ix = eqi_ix;
    *ret_max_deci = max_deci;

    return return_status;
}


/*
 * Main function
 */
int main( int argc, char *argv[] )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio fh_in;
    tstring line;
    size_t i;

    tarray_tstring cat_file_list;
    const char *dir_name;

    mdarray_int deci_check;
    int max_deci;

    size_t ipos_cntr;
    int prev_zone_id;
    int n_zone_id;

    long long serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    int main_ix;
    int eqi_ix;

    /* to store all text entries in zone */
    tarray_tstring text_table;

    /* to store j2000 table */
    ipos_entry *ipos_table_ptr;
    mdarray ipos_table(sizeof(*ipos_table_ptr), true, 
		       (void *)(&ipos_table_ptr));

    mdarray_int width_min, width_max;
    mdarray_double numeric_abs_max;

    /*
     * Check args
     */
    if ( argc < 2 ) {
	sio.eprintf("[USAGE]\n");
	sio.eprintf("  $ %s directory_name [-]\n",argv[0]);
	sio.eprintf(" If last arg is set, "
		    "SQL and C files are also created.\n");
	sio.eprintf("[Example]\n");
	sio.eprintf("  $ %s /somewhere/WISE/wise-3band\n",argv[0]);
	sio.eprintf("  $ %s http://irsadist.ipac.caltech.edu/wise-3band\n", argv[0]);
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
    cat_file_list.resize(50);
    while ( (line=fh_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() ) {
	    cat_file_list[i] = line;
	    i++;
	}
    }
    if ( fh_in.close() < 0 ) {
        sio.eprintf("[ERROR] fh_in.close() failed\n");
	goto quit;
    }
    cat_file_list.resize(i);

#if 1

    /*
     * Create zone files (not sorted)
     */
    n_zone_id = GET_ZONEID(DEC2DECI(90.0));
    sio.printf("[INFO] n_zone_id = %d\n", n_zone_id);
    if ( N_ZONE < n_zone_id ) {
        sio.eprintf("[ERROR] too many n_zone_id\n");
	goto quit;
    }

    sio.printf("[INFO] creating zone files ...\n");

    {
        gzstreamio zout[N_ZONE];
	mdarray_int zflg(false);		/* -1: not open  -2: closed */
	zflg.resize(N_ZONE).fill(-1);
	size_t j;

	for ( i=0 ; i < cat_file_list.length() ; i++ ) {

	    tstring filename;
	    digeststreamio cat_in;

	    tstring col_top(70);		/* source_id, coadd_id, src, ra, dec only */
	    const char *l_ptr;

	    for ( j=0 ; j < N_ZONE ; j++ ) {
	        if ( 0 <= zflg[j] && (size_t)(zflg[j]+2) == i ) {
		    if ( zout[j].close() < 0 ) {
		        sio.eprintf("[ERROR] zout[j].close() failed\n");
			goto quit;
		    }
		    sio.printf("closed ... output zone file: %04d\n", (int)j);
		    zflg[j] = -2;
		}
	    }

	    filename.printf("%s/%s",dir_name,cat_file_list[i].cstr());
	    sio.printf("Reading ... %s\n", filename.cstr());
	    sio.flush();

	    /* Open a cat file */
	    j=0;
	    while ( 1 ) {
	      if ( ! (cat_in.openf("r", "%s.gz",filename.cstr()) < 0) ) break;
	      if ( ! (cat_in.openf("r", "%s.bz2",filename.cstr()) < 0) ) break;
	      if ( j < 64 ) {
		j++;
		sleep(8);
		sio.eprintf("[WARNING] Retrying ... %s.\n",filename.cstr());
	      }
	      else {
		sio.eprintf("[ERROR] cat_in.open() failed: %s.\n",
			    filename.cstr());
		goto quit;
	      }
	    }

	    while ( (l_ptr=cat_in.getline()) != NULL ) {
	      
	      if ( l_ptr[0] != '#' && l_ptr[0] != '\n' ) {
		ssize_t p0, p1, p2, p3, p4;
		int zone_id;
		int deci;

	        col_top.assign(l_ptr);

		p0 = col_top.find('|');
		if ( p0 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p0 ++;
		p1 = col_top.find(p0,'|');
		if ( p1 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p1 ++;
		p2 = col_top.find(p1,'|');
		if ( p2 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p2 ++;
		p3 = col_top.find(p2,'|');
		if ( p3 < 0 ) {
		    sio.eprintf("p3.\n");
		    goto quit;
		}
		p3 ++;
		p4 = col_top.find(p3,'|');
		if ( p4 < 0 ) {
		    sio.eprintf("p4.\n");
		    goto quit;
		}

		/* obtain deci */
		deci = DEC2DECI(col_top.atof(p3));
		if ( DEC2DECI(90.0) < deci ) deci = DEC2DECI(90.0);
		else if ( deci < DEC2DECI(-90.0) ) deci = DEC2DECI(-90.0);

		/* check zone */
		zone_id = GET_ZONEID(deci);
		if ( n_zone_id <= zone_id ) {
		    sio.eprintf("[ERROR] invalid zone id; broken catalog?\n");
		    goto quit;
		}

		/* open if needed */
		if ( zflg[zone_id] < 0 ) {
		    if ( zflg[zone_id] < -1 ) {
		        sio.eprintf("[ERROR] internal error\n");
			goto quit;
		    }
		    if ( zout[zone_id].openf("wb1", /* cmp-level-1 => fast */
					     "_wise-3band-wdb-zone%04d.txt.gz",
					     zone_id) < 0 ) {
		        sio.eprintf("[ERROR] cannot open zone file"
				    " to be written\n");
			goto quit;
		    }
		    sio.printf("open ... output zone file: %04d\n", zone_id);
		    zflg[zone_id] = i;		/* mark */
		}

		/* output a line */
		zout[zone_id].putstr(l_ptr);

	      }
	    }	/* loop of getline() */
	    
	    if ( cat_in.close() < 0 ) {
	        sio.eprintf("[ERROR] cat_in.close() failed\n");
		goto quit;
	    }

	}	/* main loop ends here */

	/* close all output files */
	for ( j=0 ; j < N_ZONE ; j++ ) {
	    if ( 0 <= zflg[j] ) {
	        if ( zout[j].close() < 0 ) {
		    sio.eprintf("[ERROR] zout[j].close() failed\n");
		    goto quit;
		}
		sio.printf("closed ... output zone file: %04d\n", (int)j);
		zflg[j] = -2;
	    }
	}

    }
    
    sio.printf("\n");

    //exit(0);

#endif	/* #if 0 or 1 */


    /*
     * Open files to output
     */

    sio.printf("[INFO] creating db files ...\n");

    main_ix = 0;
    eqi_ix = 0;

    get_main_db_filename(CAT_NAME, main_ix, N_MAIN_FILES, &fname);
    if ( fh_main.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_main.open() failed\n");
	goto quit;
    }
    get_eqi_db_tmp_filename(CAT_NAME, eqi_ix, N_EQI_CHILD_TABLES, &fname);
    if ( fh_eqi.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_eqi.open() failed\n");
	goto quit;
    }

    main_ix ++;
    eqi_ix ++;

    /*
     * Main loop
     */
    ipos_cntr = 0;
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    prev_zone_id = GET_ZONEID(max_deci);
    ipos_table.resize(N_ZONE_ENTRIES);
    text_table.resize(N_ZONE_ENTRIES);
    width_min.resize(N_CAT_COLS).fill(9999);
    width_max.resize(N_CAT_COLS).fill(-9999);
    numeric_abs_max.resize(N_CAT_COLS).fill(0.0);
    for ( i=0 ; i < N_ZONE ; i++ ) {
        tstring filename;
	digeststreamio cat_in;

	tstring col_top(70);		/* source_id, coadd_id, src, ra, dec only */
	const char *l_ptr;

	filename.printf("_wise-3band-wdb-zone%04d.txt.gz",(int)i);
	sio.printf("Reading ... %s\r", filename.cstr());
	sio.flush();

	/* Open a cat file */
	if ( cat_in.open("r", filename.cstr()) < 0 ) {
	    sio.eprintf("[ERROR] cat_in.open() failed: %s.\n",
			filename.cstr());
	    goto quit;
	}

	while ( (l_ptr=cat_in.getline()) != NULL ) {

	    if ( l_ptr[0] != '#' && l_ptr[0] != '\n' ) {
		ssize_t p0, p1, p2, p3, p4;
		int zone_id;
		int rai, deci;
		double ra_r, dec_r, cx,cy,cz;

	        col_top.assign(l_ptr);

		p0 = col_top.find('|');
		if ( p0 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p0 ++;
		p1 = col_top.find(p0,'|');
		if ( p1 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p1 ++;
		p2 = col_top.find(p1,'|');
		if ( p2 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p2 ++;
		p3 = col_top.find(p2,'|');
		if ( p3 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		p3 ++;
		p4 = col_top.find(p3,'|');
		if ( p4 < 0 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}

		/* obtain rai, deci, cx, cy, cz */
		rai = RA2RAI(col_top.atof(p2));
		deci = DEC2DECI(col_top.atof(p3));
		if ( DEC2DECI(90.0) < deci ) deci = DEC2DECI(90.0);
		else if ( deci < DEC2DECI(-90.0) ) deci = DEC2DECI(-90.0);
		if ( RA2RAI(360.0) <= rai ) rai = RA2RAI(0.0);
		ra_r = RAI2RA(rai) * CC_DEG_TO_RAD;
		dec_r = DECI2DEC(deci) * CC_DEG_TO_RAD;
		cx = cos(ra_r) * cos(dec_r);
		cy = sin(ra_r) * cos(dec_r);
		cz = sin(dec_r);

		/* check zone */
		zone_id = GET_ZONEID(deci);
		if ( zone_id < prev_zone_id ) {
		    sio.eprintf("[ERROR] invalid zone id; broken catalog?\n");
		    goto quit;
		}
		else if ( prev_zone_id < zone_id ) {	/* next ZONE ! */

		    if ( output_zone(prev_zone_id,
				   text_table, ipos_table, ipos_table_ptr, 
				   fh_main, fh_eqi, deci_check,
				   width_min, width_max, numeric_abs_max,
				   &ipos_cntr, &serial_id,
				   &main_ix, &eqi_ix, &max_deci, false) < 0 ) {
		        goto quit;
		    }
		    
		    prev_zone_id = zone_id;

		}

		/* store rai,deci,etc. to buffer */
		if ( ipos_table.length() <= ipos_cntr ) {
		    ipos_table.resizeby(ipos_cntr / 2);
		}
		ipos_table_ptr[ipos_cntr]._objid = ipos_cntr;
		ipos_table_ptr[ipos_cntr].rai = rai;
		ipos_table_ptr[ipos_cntr].deci = deci;
		ipos_table_ptr[ipos_cntr].cx = cos(ra_r) * cos(dec_r);
		ipos_table_ptr[ipos_cntr].cy = sin(ra_r) * cos(dec_r);
		ipos_table_ptr[ipos_cntr].cz = sin(dec_r);

		/* store all column to buffer */
		if ( text_table.length() <= ipos_cntr ) {
		    text_table.resizeby(ipos_cntr / 2);
		}
		text_table[ipos_cntr] = l_ptr;

		/* count up */
		ipos_cntr ++;

	    }
	}

	if ( cat_in.close() < 0 ) {
	    sio.eprintf("[ERROR] cat_in.close() failed\n");
	    goto quit;
	}

    }	/* main loop ends here */

    if ( output_zone(prev_zone_id,
		     text_table, ipos_table, ipos_table_ptr, 
		     fh_main, fh_eqi, deci_check,
		     width_min, width_max, numeric_abs_max,
		     &ipos_cntr, &serial_id,
		     &main_ix, &eqi_ix, &max_deci, true) ) {
        goto quit;
    }

    /* output min and max of each column width */
    sio.eprintf("================\n");
    sio.eprintf("col\tmin_w\tmax_w\tnumeric_abs_max\n");
    for ( i=0 ; i < width_min.length() ; i++ ) {
      sio.eprintf("%d\t%d\t%d\t%.15g\n",
		  (int)(i+1),width_min[i],width_max[i],numeric_abs_max[i]);
    }
    sio.eprintf("================\n");

    if ( serial_id - 1 != N_ALL_ENTRIES ) {
	sio.eprintf("[WARNING] broken catalog?\n");
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

    if ( output_sql_and_c(CAT_NAME, N_MAIN_FILES, DB_DELIMITER, DB_NULL,
			  deci_check, max_deci, false) < 0 ) {
	sio.eprintf("[ERROR] output_sql_and_c() failed\n");
	goto quit;
    }

    ret_status = 0;
 quit:
    return ret_status;
}
