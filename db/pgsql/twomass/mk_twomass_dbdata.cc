/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2012-04-26 14:06:13 cyamauch> */

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
 *    $ ./mk_twomass_dbdata /somewhere/2mass/allsky
 *    $ ./mk_twomass_dbdata ftp://ftp.ipac.caltech.edu/pub/2mass/allsky
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_twomass_dbdata /somewhere/2mass/allsky -
 *
 */

#include <sli/stdstreamio.h>
#include <sli/digeststreamio.h>
#include <sli/tstring.h>
#include <sli/tarray_tstring.h>
#include <sli/mdarray.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
using namespace sli;

#include "../pg_module/common_conv.h"

/*
 * Settings for 2MASS
 */
/* ObiID */
#define OBJID_OFFSET  500000000LL
/* Number of child tables */
#define N_EQI_CHILD_TABLES (92*8)	/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

#define CAT_NAME "Twomass"
#define CAT_LIST_FILE "twomass_list.txt"
#define N_CAT_COLS 60
#define CAT_DELIMITER '|'
#define CAT_NULL "\\N"
#define N_CAT_LINE_ALLOC_CHRS 511
/* Set *AROUND* maximum number of entries in a cat file */
#define N_CAT_ENTRIES 6000000
/* Set number of rows in this catalog */
#define N_ALL_ENTRIES 470992970LL

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
} ipos_entry;

#include "../twomass/output_sql_and_c.cc"

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

    const long long n_all_entries = N_ALL_ENTRIES;

    mdarray_int deci_check;
    int max_deci;
    int max0_deci;

    long long serial_id;
    long long min_serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    int eqi_ix;

    mdarray ipos_table_0(sizeof(ipos_entry));
    mdarray ipos_table_1(sizeof(ipos_entry));
    size_t ipos_table_cnt_a;
    size_t ipos_table_cnt_b;

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
	sio.eprintf("  $ %s /somewhere/2mass/allsky\n",argv[0]);
	sio.eprintf("  $ %s ftp://ftp.ipac.caltech.edu/pub/2mass/allsky\n", argv[0]);
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

    eqi_ix = 0;

    /* open new tmp binary */
    get_eqi_db_tmp_filename(CAT_NAME, eqi_ix, N_EQI_CHILD_TABLES, &fname);
    if ( fh_eqi.open("w", fname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_eqi.open() failed\n");
	goto quit;
    }

    eqi_ix ++;

    /*
     * Main loop
     */
    serial_id = 1;
    min_serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    max0_deci = DEC2DECI(-90.0);
    ipos_table_0.resize(N_CAT_ENTRIES);		/* alloc memory */
    ipos_table_1.resize(N_CAT_ENTRIES);
    ipos_table_cnt_a = 0;
    ipos_table_cnt_b = 0;
    width_min.resize(N_CAT_COLS).fill(9999);
    width_max.resize(N_CAT_COLS).fill(-9999);
    numeric_abs_max.resize(N_CAT_COLS).fill(0.0);
    for ( i=0 ; i < cat_file_list.length() + 1 ; i++ ) {
	mdarray *ipos_tbl_a;
	mdarray *ipos_tbl_b;
	ipos_entry *ipos_table_ptr;
	
	long long n_unit_eqi = n_all_entries / N_EQI_CHILD_TABLES;

	/* swap pointer and setup counters */
	if ( i % 2 == 0 ) {
	    ipos_tbl_a = &ipos_table_0;		/* current */
	    ipos_tbl_b = &ipos_table_1;		/* previous */
	}
	else {
	    ipos_tbl_a = &ipos_table_1;		/* current */
	    ipos_tbl_b = &ipos_table_0;		/* previous */
	}
	ipos_table_cnt_b = ipos_table_cnt_a;	/* take over */
	ipos_table_cnt_a = 0;			/* reset */

	/* this is done except last one */
	if ( i < cat_file_list.length() ) {
	    tstring cat_filename;
	    digeststreamio cat_in;
	    tarray_tstring elms;
	    mdarray_size spos;
	    mdarray_size slen;
	    tstring catline(N_CAT_LINE_ALLOC_CHRS);	/* fixed-length mode */
	    size_t j;

	    /* prepare actual file name */
	    cat_filename.printf("%s/%s",dir_name,cat_file_list[i].cstr());
	    sio.printf("Reading ... %s\r", cat_filename.cstr());
	    sio.flush();

	    /* Open a cat file to read */
	    j=0;
	    while ( 1 ) {
	      if ( ! (cat_in.open("r", cat_filename.cstr()) < 0) ) break;
	      if ( j < 64 ) {
		j++;
		sleep(8);
		sio.eprintf("[WARNING] Retrying ... %s.\n",cat_filename.cstr());
	      }
	      else {
		sio.eprintf("[ERROR] cat_in.open() failed: %s.\n",
			    cat_filename.cstr());
		goto quit;
	      }
	    }

	    /* Open a main text DB file to write */
	    get_main_db_filename(CAT_NAME, i, cat_file_list.length(), &fname);
	    //fname = "/dev/null";	/* to supress output of main DB files */
	    if ( fh_main.open("w", fname.cstr()) < 0 ) {
		sio.eprintf("[ERROR] fh_main.open() failed\n");
		goto quit;
	    }

	    /* Read all rows in a catalog file */
	    elms.resize(N_CAT_COLS);
	    spos.resize(N_CAT_COLS);
	    slen.resize(N_CAT_COLS);
	    catline.put(0,' ',N_CAT_LINE_ALLOC_CHRS);
	    while ( cat_in.getstr(catline.str_ptr(), N_CAT_LINE_ALLOC_CHRS+1)
		    != NULL ) {
	      if ( catline[0] != '\n' ) {
		int rai,deci;
		double ra_r, dec_r, cx,cy,cz;
		size_t k, p0, p1;
		/* scan the line */
		p0 = 0;
		for ( k=0 ; k < N_CAT_COLS ; k++ ) {
		    ssize_t p, width;
		    int to_find;
		    if ( k+1 == N_CAT_COLS ) to_find = '\n';
		    else to_find = CAT_DELIMITER;
		    p = catline.find(p0, to_find, &p1);
		    if ( p < 0 ) {
			sio.eprintf("[ERROR] cannot find delimiter: "
				    "cat file is broken.\n");
			goto quit;
		    }
		    elms[k].clean(' ');
		    if ( catline.strncmp(p0, CAT_NULL, 2) == 0 ) {
			width = 0;
			elms[k].put(0, DB_NULL);
			/* check */
			if ( width < width_min[k] ) width_min[k] = width;
			if ( width_max[k] < width ) width_max[k] = width;
		    }
		    else {
			width = p-p0;
			elms[k].put(0,catline,p0,width);
			/* check */
			if ( width < width_min[k] ) width_min[k] = width;
			if ( width_max[k] < width ) width_max[k] = width;
			/* check maximum numeric abs value */
			if ( 0 < width ) {
			    double vv = fabs(elms[k].atof());
			    if ( numeric_abs_max[k] < vv ) numeric_abs_max[k] = vv;
			}
		    }
		    p0 = p1;
		}
		/* to skip white space */
		for ( k=0 ; k < elms.length() ; k++ ) {
		    spos[k] = elms[k].strspn(' ');
		}
		/* to obtain length of valid string */
		for ( k=0 ; k < elms.length() ; k++ ) {
		    slen[k] = elms[k].length() - spos[k] - elms[k].strrspn(' ');
		}
		/* set rai, deci and xyz */
		k = 0;
		rai = RA2RAI(elms[k].atof(spos[k]));
		if ( RA2RAI(360.0) <= rai ) rai = RA2RAI(0.0);
		k = 1;
		deci = DEC2DECI(elms[k].atof(spos[k]));
		if ( DEC2DECI(90.0) < deci ) deci = DEC2DECI(90.0);
		else if ( deci < DEC2DECI(-90.0) ) deci = DEC2DECI(-90.0);
		ra_r = RAI2RA(rai) * CC_DEG_TO_RAD;
		dec_r = DECI2DEC(deci) * CC_DEG_TO_RAD;
		cx = cos(ra_r) * cos(dec_r);
		cy = sin(ra_r) * cos(dec_r);
		cz = sin(dec_r);
		/* output main data files */
		/* objid */
		fh_main.printf("%lld%s", (long long)(OBJID_OFFSET + serial_id),
			       DB_DELIMITER);
		/* designation */
		k = 5;
		fh_main.write(elms[k].cstr()+spos[k], slen[k]);
		fh_main.putstr(DB_DELIMITER);
		/* ra, dec, ... */
		for ( k=0 ; k < 5 ; k++ ) {
		    fh_main.write(elms[k].cstr()+spos[k], slen[k]);
		    fh_main.putstr(DB_DELIMITER);
		}
		for ( k++ ; k < elms.length() ; k++ ) {
		    fh_main.write(elms[k].cstr()+spos[k], slen[k]);
		    fh_main.putstr(DB_DELIMITER);
		}
		/* last is xyz */
		fh_main.printf("%.17g%s%.17g%s%.17g\n",
			       cx, DB_DELIMITER, cy, DB_DELIMITER, cz);
		/* store rai,deci,etc. to buffer */
		/* NOTE: this code is required to handle overlapped DECs */
		/*       between two cat files. */
		if ( deci < max0_deci ) {
		    /* to previous */
		    if ( ipos_tbl_b->length() <= ipos_table_cnt_b ) {
			ipos_tbl_b->resize(ipos_table_cnt_b + N_CAT_ENTRIES / 2);
		    }
		    ipos_table_ptr = (ipos_entry *)(ipos_tbl_b->data_ptr());
		    ipos_table_ptr[ipos_table_cnt_b]._objid = serial_id;
		    ipos_table_ptr[ipos_table_cnt_b].rai = rai;
		    ipos_table_ptr[ipos_table_cnt_b].deci = deci;
		    ipos_table_cnt_b ++;
		}
		else {
		    /* to current */
		    if ( ipos_tbl_a->length() <= ipos_table_cnt_a ) {
			ipos_tbl_a->resize(ipos_table_cnt_a + N_CAT_ENTRIES / 2);
		    }
		    ipos_table_ptr = (ipos_entry *)(ipos_tbl_a->data_ptr());
		    ipos_table_ptr[ipos_table_cnt_a]._objid = serial_id;
		    ipos_table_ptr[ipos_table_cnt_a].rai = rai;
		    ipos_table_ptr[ipos_table_cnt_a].deci = deci;
		    ipos_table_cnt_a ++;
		}
		serial_id++;
	      }
	    }
	    cat_in.close();
	    fh_main.close();

	    /* This should not be reported */
	    if ( i == 0 && 0 < ipos_table_cnt_b ) {
		sio.eprintf("[ERROR] Strange... ipos_table_cnt_b = %zd\n",
			    ipos_table_cnt_b);
		goto quit;
	    }

	    /* get max0_deci from *current* */
	    ipos_table_ptr = (ipos_entry *)(ipos_tbl_a->data_ptr());
	    for ( j=0 ; j < ipos_table_cnt_a ; j++ ) {
		if ( max0_deci < ipos_table_ptr[j].deci ) {
		    max0_deci = ipos_table_ptr[j].deci;
		}
	    }

	}	/* if ( i < cat_file_list.length() ) ... */

	/* output *previous*: this is done expect i == 0 */
	if ( 0 < i ) {
	    size_t n_entries = ipos_table_cnt_b;
	    size_t j;

	    /* obtain adr */
	    ipos_table_ptr = (ipos_entry *)(ipos_tbl_b->data_ptr());

	    /* Sort ipos_entry */
	    qsort(ipos_table_ptr, n_entries, sizeof(*ipos_table_ptr), 
		  &cmp_eqi_tbl_deci);
	    if ( ipos_table_ptr[0].deci < max_deci ) {
		sio.eprintf("[ERROR] Invalid DEC is found: i=%zd\n", i);
		goto quit;
	    }
	    max_deci = ipos_table_ptr[n_entries - 1].deci;

	    /* close/open output files with partitioning */
	    for ( j=0 ; j < n_entries ; j++ ) {
		/* Write an entry (binary) */
		fh_eqi.write(ipos_table_ptr + j, sizeof(*ipos_table_ptr));
		/* Partitioning */
		if ( j + 1 < n_entries ) {
		    if ( eqi_ix < N_EQI_CHILD_TABLES &&
			 n_unit_eqi * eqi_ix <= min_serial_id + (long long)j &&
			 ipos_table_ptr[j].deci < ipos_table_ptr[j+1].deci ) {
			/* close current */
			fh_eqi.close();
			/* register new check deci value */
			deci_check[eqi_ix-1] = ipos_table_ptr[j+1].deci;
			/* read tmp binary, sort order for optimization,
			   and write DB text file */
			if ( read_optimize_and_write_eqi(CAT_NAME, 
						N_CAT_ENTRIES, DB_DELIMITER,
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
	    min_serial_id += n_entries;
	}	/* if ( 0 < i ) ... */

    }	/* main loop ends here */

    fh_eqi.close();
    /* read tmp binary, sort order for optimization,
       and write DB text file */
    if ( read_optimize_and_write_eqi(CAT_NAME, N_CAT_ENTRIES, DB_DELIMITER,
				     eqi_ix-1, N_EQI_CHILD_TABLES,
				     N_DIV_EQI_SECTION,
				     &cmp_eqi_tbl_rai) < 0 ) {
	sio.eprintf("[ERROR] read_optimize_and_write_eqi() failed\n");
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

    if ( serial_id - 1 != n_all_entries ) {
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

    if ( output_sql_and_c(CAT_NAME, cat_file_list.length(), 
			  DB_DELIMITER, DB_NULL,
			  deci_check, max_deci, false) < 0 ) {
	sio.eprintf("[ERROR] output_sql_and_c() failed\n");
	goto quit;
    }

    ret_status = 0;
 quit:
    return ret_status;
}
