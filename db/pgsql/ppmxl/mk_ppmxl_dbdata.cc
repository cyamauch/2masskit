/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2014-09-05 17:16:22 cyamauch> */

/*
 *  mk_ppmxl_dbdata.cc
 *  - Create DB files for 2massKit using original PPMXL compressed text files.
 *
 *  Note:
 *  - This program requires SLLIB-1.4.0 or newer.  SLLIB is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *  - This program read compressed (.gz) text files.
 *
 *  How to compile:
 *    $ s++ mk_ppmxl_dbdata.cc
 *
 *  How to use:
 *    $ ./mk_ppmxl_dbdata /somewhere/PPMXL/dat
 *    $ ./mk_ppmxl_dbdata ftp://cdsarc.u-strasbg.fr/pub/cats/I/317/dat
 *    $ ./mk_ppmxl_dbdata http://cdsarc.u-strasbg.fr/ftp/cats/aliases/P/PPMXL/dat
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_ppmxl_dbdata /somewhere/PPMXL/dat -
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
 * Settings for PPMXL
 */
/* ObiID */
#define OBJID_OFFSET 3500000000LL
/* Number of files for main table */
#define N_MAIN_FILES 98			/* set to < 2 GByte in each file */
/* Number of child tables */
#define N_EQI_CHILD_TABLES 1440		/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

#define CAT_NAME "Ppmxl"
#define CAT_LIST_FILE "ppmxl_list.txt"
#define N_CAT_LINE_BYTES 173
/* Set *AROUND* maximum number of entries in a cat file */
#define N_CAT_ENTRIES 2000000
/* Set number of rows in this catalog */
#define N_ALL_ENTRIES 910468710LL

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

    long long serial_id;
    long long min_serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    int main_ix;
    int eqi_ix;

    ipos_entry *ipos_table_ptr;
    mdarray ipos_table(sizeof(*ipos_table_ptr), true, 
		       (void *)(&ipos_table_ptr));

    /*
     * Check args
     */
    if ( argc < 2 ) {
	sio.eprintf("[USAGE]\n");
	sio.eprintf("  $ %s directory_name [-]\n",argv[0]);
	sio.eprintf(" If last arg is set, "
		    "SQL and C files are also created.\n");
	sio.eprintf("[Example]\n");
	sio.eprintf("  $ %s /somewhere/PPMXL/dat\n",argv[0]);
	sio.eprintf("  $ %s ftp://cdsarc.u-strasbg.fr/pub/cats/I/317/dat\n", argv[0]);
	sio.eprintf("  $ %s http://cdsarc.u-strasbg.fr/ftp/cats/aliases/P/PPMXL/dat\n", argv[0]);
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
    cat_file_list.resize(720);
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
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    ipos_table.resize(N_CAT_ENTRIES);
    for ( i=0 ; i < cat_file_list.length() ; i++ ) {
	tstring filename;
	size_t n_entries;
	digeststreamio cat_in;
	tarray_tstring elms;
	mdarray_size spos;
	const char *l_ptr;
	size_t j;
	size_t odr[] = {0,1,2,7,8,5,6,3,4,9,10,
			11,12,13,14,15,16,17,18,19,20,21,22,23,24};

	long long n_unit_main = n_all_entries / N_MAIN_FILES;
	long long n_unit_eqi = n_all_entries / N_EQI_CHILD_TABLES;

	min_serial_id = serial_id;

	filename.printf("%s/%s",dir_name,cat_file_list[i].cstr());
	sio.printf("Reading ... %s\r", filename.cstr());
	sio.flush();

	/* Open a cat file */
	j=0;
	while ( 1 ) {
	  if ( ! (cat_in.open("r", filename.cstr()) < 0) ) break;
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

	j = 0;
	elms.resize(25);
	spos.resize(25);
	while ( (l_ptr=cat_in.getline()) != NULL ) {
	    int rai,deci;
	    double ra_r, dec_r, cx,cy,cz;
	    size_t k;
	    if ( l_ptr[0] != '#' && l_ptr[0] != '\n' ) {
		for ( k=0 ; l_ptr[k] != '\0' ; k++ );
		if ( k != N_CAT_LINE_BYTES + 1 ) {
		    sio.eprintf("[ERROR] broken line is found in cat file.\n");
		    goto quit;
		}
		elms[0].put(0, l_ptr+0,19);
		elms[1].put(0, l_ptr+20,10);	/* RAdec */
		elms[2].put(0, l_ptr+30,10);	/* DEdeg */
		elms[3].put(0, l_ptr+41,8);	/* pmRA */
		elms[4].put(0, l_ptr+50,8);	/* pmDE */
		elms[5].put(0, l_ptr+59,7);	/* epRA */
		elms[6].put(0, l_ptr+67,7);	/* epDE */
		elms[7].put(0, l_ptr+75,3);	/* e_RAdeg */
		elms[8].put(0, l_ptr+79,3);	/* e_DEdeg */
		elms[9].put(0, l_ptr+83,4);	/* e_pmRA */
		elms[10].put(0, l_ptr+88,4);	/* e_pmDE */
		elms[11].put(0, l_ptr+93,6);	/* Jmag */
		elms[12].put(0, l_ptr+100,5);
		elms[13].put(0, l_ptr+106,6);
		elms[14].put(0, l_ptr+113,5);
		elms[15].put(0, l_ptr+119,6);
		elms[16].put(0, l_ptr+126,5);
		elms[17].put(0, l_ptr+132,5);
		elms[18].put(0, l_ptr+138,5);
		elms[19].put(0, l_ptr+144,5);
		elms[20].put(0, l_ptr+150,5);
		elms[21].put(0, l_ptr+156,5);	/* imag */
		elms[22].put(0, l_ptr+162,5);	/* Smags */
		elms[23].put(0, l_ptr+168,2);	/* No */
		elms[24].put(0, l_ptr+171,2);	/* fl */
		/* to skip white space */
		for ( k=0 ; k < elms.length() ; k++ ) {
		    spos[k] = elms[k].strspn(' ');
		}
		/* set rai, deci and xyz */
		k = 1;
		rai = RA2RAI(elms[k].atof(spos[k]));
		if ( RA2RAI(360.0) <= rai ) rai = RA2RAI(0.0);
		k = 2;
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
		/* 0-24 */
		for ( k=0 ; k < elms.length() ; k++ ) {
		    size_t l = elms[odr[k]].length() - spos[odr[k]]
			- elms[odr[k]].strrspn(' ');
		    size_t l0;
		    /* column:22 'NOT NULL' att. */
		    if ( odr[k] == 22 ) l0 = 0;
		    else l0 = elms[odr[k]].strspn(spos[odr[k]],'-');
		    /* "", "---" => NULL */
		    if ( l == 0 || l == l0 ) {
			fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
			/* Hmm, this is not reported. "No" is `NOT NULL' column */
			if ( odr[k] == 23 ) {
			    sio.eprintf("[INFO] found NULL in 'No' column: "
					"(da dec)=(%.15g %.15g).\n",
					RAI2RA(rai),DECI2DEC(deci));
			}
		    }
		    else {
			fh_main.write(elms[odr[k]].cstr()+spos[odr[k]], l);
			fh_main.printf("%s", DB_DELIMITER);
		    }
		}
		/* cx, cy, cz */
		fh_main.printf("%.17g%s%.17g%s%.17g\n",
			       cx, DB_DELIMITER, cy, DB_DELIMITER, cz);
		/* Partitioning */
		if ( main_ix < N_MAIN_FILES &&
		     n_unit_main * main_ix <= min_serial_id + (long long)j ) {
		    fh_main.close();
		    get_main_db_filename(CAT_NAME, main_ix, 
					 N_MAIN_FILES, &fname);
		    if ( fh_main.open("w", fname.cstr()) < 0 ) {
			sio.eprintf("[ERROR] fh_main.open() failed\n");
			goto quit;
		    }
		    main_ix ++;
		}
		/* store rai,deci,etc. to buffer */
		if ( ipos_table.length() <= j ) {
		    ipos_table.resize(j + N_CAT_ENTRIES / 2);
		}
		ipos_table_ptr[j]._objid = serial_id;	/* not apply offset */
		ipos_table_ptr[j].rai = rai;
		ipos_table_ptr[j].deci = deci;
		/* */
		j++;
		serial_id++;
	    }
	}
	n_entries = j;

	cat_in.close();

	/* Sort ipos_entry */
	qsort(ipos_table_ptr, n_entries, sizeof(*ipos_table_ptr), 
	      &cmp_eqi_tbl_deci);
	if ( ipos_table_ptr[0].deci < max_deci ) {
	    sio.eprintf("[ERROR] Invalid DEC in cat file: %s\n",
			filename.cstr());
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
		    if ( read_optimize_and_write_eqi(CAT_NAME, N_CAT_ENTRIES,
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

    }	/* main loop ends here */

    fh_main.close();
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

    if ( output_sql_and_c(CAT_NAME, N_MAIN_FILES, DB_DELIMITER, DB_NULL,
			  deci_check, max_deci, true) < 0 ) {
	sio.eprintf("[ERROR] output_sql_and_c() failed\n");
	goto quit;
    }

    ret_status = 0;
 quit:
    return ret_status;
}
