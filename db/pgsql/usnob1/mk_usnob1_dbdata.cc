/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2014-09-05 17:30:55 cyamauch> */

/*
 *  mk_usnob1_dbdata.cc
 *  - Create DB files for 2massKit using original USNO-B1.0 binary cat files.
 *
 *  Note:
 *  - This program requires SLLIB-1.4.0 or newer.  SLLIB is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *  - This program can read compressed (.gz or .bz2) cat files.
 *
 *  How to compile:
 *    $ s++ mk_usnob1_dbdata.cc
 *
 *  How to use:
 *    $ ./mk_usnob1_dbdata /somewhere/USNO-B1
 *    $ ./mk_usnob1_dbdata ftp://catftp:PASSWD@vizier.nao.ac.jp/FTP/USNO-B1
 *    $ ./mk_usnob1_dbdata http://archive.noao.edu/catalogs/usnob
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_usnob1_dbdata /somewhere/USNO-B1 -
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
 * Settings for USNO-B1
 */
/* ObiID */
#define OBJID_OFFSET 2200000000LL
/* Number of files for main table */
#define N_MAIN_FILES 134		/* set to < 2 GByte in each file */
/* Number of child tables */
#define N_EQI_CHILD_TABLES 1440		/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

#define CAT_NAME "Usnob1"
#define CAT_LIST_FILE "usnob1_list.txt"
/* for test */
//#define CAT_LIST_FILE "usnob1_0900.txt"
//#define CAT_LIST_FILE "usnob1_list_tycho2.txt"

#define CAT_LINE_BYTE 80

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
 * Settings for USNO-B1
 */

typedef struct _usnob1_band_data {

    float mag;
    short c;
    short s;
    short f;
    short sg;
    short xi;
    short eta;

} usnob1_band_data;

typedef struct _usnob1_entry {

    long long objid;

    char usnob1_id[13];
    char tycho2_id[13];

    double ra;		/* [0-3] */
    double dec;		/* [4-7] */
    short e_ra;		/* [16-19] */
    short e_dec;	/* [16-19] */
    float epoch;	/* mean epoch [16-19] */
    short pm_ra;	/* [8-11] */
    short pm_dec;	/* [8-11] */
    short mu_pr;	/* total u probability [8-11] */
    short e_pm_ra;	/* [12-15] */
    short e_pm_dec;	/* [12-15] */
    short fit_ra;	/* [12-15] */
    short fit_dec;	/* [12-15] */
    short n_det;	/* [12-15] */
    char flags[4];	/* [MsY] M: motion catalog [8-11] */
			/*       s: diffraction spike [8-11] */
			/*       Y: YS4.0 correlation flag [16-19] */

    usnob1_band_data bdata[5];

    double cx;
    double cy;
    double cz;

} usnob1_entry;


/*
 * Functions for USNO-B1
 */

/**
 * Obtain unsigned 4-byte int from little-endian binary
 *
 * @param      buf  Input buffer
 * @param      idx  Byte offset of buf
 * @return     Unsigned 4-byte int value
 */
static uint32_t get_ufour( const unsigned char *buf, int idx )
{
    return (uint32_t)((uint32_t)(buf[idx]) |
		      (uint32_t)(buf[idx+1]) << 8 |
		      (uint32_t)(buf[idx+2]) << 16 |
		      (uint32_t)(buf[idx+3]) << 24);
}

/*
 * The basic structure (except Tycho-2 entries) of USNO-B1.0 original `cat'
 * files is shown in http://tdc-www.harvard.edu/catalogs/ub1.format.html.
 *
 * If you want to know details of the structure of `cat' files,
 * see also usnob_make.c in Vizier USNO-B1.0 software package.
 *
 * /dc01b/Strasbourg/usnob1.0/src @ db01
 */

/**
 * Parse an entry of usno-b1.0 binary data
 *
 * @param      serial  Serial ID (beginning 0)
 * @param      filename   Name of cat file
 * @param      buf  An entry of binary data
 * @param      o_main  Result main table (top of address)
 * @param      o_ipos  Result eqi table (top of address)
 * @param      l_no  Number of line (beginning 0)
 * @return     Status
 */
static int parse_usnob1_line( long long serial,
			      const tstring &filename,
			      const unsigned char *buf, 
			      usnob1_entry *o_main,
			      ipos_entry *o_ipos,
			      size_t l_no )
{
    stdstreamio sio;
    tstring designation;
    long ul, k,e,v,u, a,s,p,i, x,y,q,r,j, m,g,f,c, nd;
    int bix, rai, deci;
    double pv, ra_r, dec_r;
    unsigned long tycho2_fileid, tycho2_seqid;
    unsigned long tycho2_id0, tycho2_id1, tycho2_id2;
    bool tycho2_entry;

    /*
     * Start main table
     */

    /* objid */
    o_main[l_no].objid = OBJID_OFFSET + serial;

    /* usnob1_id */
    filename.copy(filename.length()-8,4, &designation);
    designation.replacef(4,8,"-%07zd",l_no+1);
    designation.getstr(o_main[l_no].usnob1_id,13);

    /* aaaaaaaaa */
    ul = get_ufour(buf, 0);

    pv = (double)ul / 360000.0;
    rai = RA2RAI(pv);
    if ( RA2RAI(360.0) <= rai ) rai = RA2RAI(0.0);
    o_main[l_no].ra = RAI2RA(rai);

    /* ssssssss */
    ul = get_ufour(buf, 4);

    pv = (double)((long)ul - 32400000) / 360000.0;
    deci = DEC2DECI(pv);
    if ( DEC2DECI(90.0) < deci ) deci = DEC2DECI(90.0);
    else if ( deci < DEC2DECI(-90.0) ) deci = DEC2DECI(-90.0);
    o_main[l_no].dec = DECI2DEC(deci);

    /* jMRQyyyxxx or fileid of tycho2 */
    ul = get_ufour(buf, 12);

    x = ul % 1000;
    ul /=    1000;
    y = ul % 1000;
    ul /=    1000;
    q = ul % 10;
    ul /=    10;
    r = ul % 10;
    ul /=    10;
    nd = ul % 10;
    ul /=    10;
    j = ul % 10;

    o_main[l_no].n_det = nd;

    tycho2_entry = ( o_main[l_no].n_det == 0 );

    if ( tycho2_entry ) {
	tycho2_fileid = ul;
	j = 0;
        o_main[l_no].fit_ra = 0;
	o_main[l_no].fit_dec = 0;
	o_main[l_no].e_pm_ra = 0;
	o_main[l_no].e_pm_dec = 0;
    }
    else {
        o_main[l_no].fit_ra = q;
	o_main[l_no].fit_dec = r;
	o_main[l_no].e_pm_ra = x;
	o_main[l_no].e_pm_dec = y;
    }

    /* keeevvvuuu or seqid of tycho2 */
    ul = get_ufour(buf, 16);

    if ( tycho2_entry ) {
	tycho2_seqid = ul;
	u = 0;
	v = 0;
	e = 500;
	k = 0;
    }
    else {
	u = ul % 1000;
	ul /=    1000;
	v = ul % 1000;
	ul /=    1000;
	e = ul % 1000;
	ul /=    1000;
	k = ul % 10;
    }

    o_main[l_no].e_ra  = u;
    o_main[l_no].e_dec = v;
    o_main[l_no].epoch = 1950.0 + 0.1 * e;

    /* iPSSSSAAAA */
    ul = get_ufour(buf, 8);

    a = ul % 10000;
    ul /=    10000;
    s = ul % 10000;
    ul /=    10000;

    if ( tycho2_entry ) {
	p = 0;
	i = 0;
    }
    else {
	p = ul % 10;
	ul /=    10;
	i = ul % 10;
    }

    o_main[l_no].pm_ra  = 2*(a - 5000);

    /* This is in out.sam: wrong sign? */
    /* o_main[l_no].pm_dec = -2*(s - 5000); */

    o_main[l_no].pm_dec = 2*(s - 5000);

    o_main[l_no].mu_pr = p;

    if ( i != 0 ) o_main[l_no].flags[0] = 'M';
    else o_main[l_no].flags[0] = '.';
    if ( j != 0 ) o_main[l_no].flags[1] = 's';
    else o_main[l_no].flags[1] = '.';
    if ( k != 0 ) o_main[l_no].flags[2] = 'Y';
    else o_main[l_no].flags[2] = '.';
    o_main[l_no].flags[3] = '\0';


    /*
     * 5-bands
     */

    o_main[l_no].tycho2_id[0] = '\0';

    for ( bix=0 ; bix < 5 ; bix++ ) {

	/* GGSFFFmmmm */
	ul = get_ufour(buf, 20 + 4*bix);

	if ( tycho2_entry ) {
	    m = (int16_t)(ul & 0x0ffff);
	    f = 0;
	    s = 0;
	    g = 0;

	    /* tycho2_id */
	    q = 0;
	    r = 0;
	    c = 0;
	    if ( bix == 0 ) {
	        ul = get_ufour(buf, 40);
		tycho2_id0 = ul;
	        ul = get_ufour(buf, 44);
		tycho2_id1 = ul / 10;
		tycho2_id2 = ul % 10;
		designation.printf("%04ld-%05ld-%01ld",
				   tycho2_id0,tycho2_id1,tycho2_id2);
		designation.getstr(o_main[l_no].tycho2_id,13);
	    }
	}
	else {
	    m = ul % 10000;
	    ul /=    10000;
	    f = ul % 1000;
	    ul /=    1000;
	    s = ul % 10;
	    ul /=    10;
	    g = ul % 100;

	    /* CrrrrRRRR */
	    ul = get_ufour(buf, 40 + 4*bix);

	    q = ul % 10000;
	    ul /=    10000;
	    r = ul % 10000;
	    ul /=    10000;
	    c = ul % 10;
	}

	o_main[l_no].bdata[bix].mag = m * 0.01;
	o_main[l_no].bdata[bix].c = c;
	o_main[l_no].bdata[bix].s = s;
	o_main[l_no].bdata[bix].f = f;
	o_main[l_no].bdata[bix].sg = g;
	
	/*
	 * check when not Tycho-2 entry
	 */

#if 0	/* Confirmed that this is not reported */
	if ( (! tycho2_entry) && 
	     o_main[l_no].bdata[bix].f == 0 && 
	     ( o_main[l_no].bdata[bix].mag != 0.0 ||
	       o_main[l_no].bdata[bix].sg != 0 ) ) {
	    sio.eprintf("[INFO] mark1\n");
	}
#endif
	if ( (! tycho2_entry) && 0 < o_main[l_no].bdata[bix].f && 
	     o_main[l_no].bdata[bix].mag == 0.0 ) {
	    sio.eprintf("[INFO] zero mag ??? (n_det=%hd,band=%d), ",
			o_main[l_no].n_det, bix);
	    sio.eprintf("usnob1_id=%s (ra dec)=(%10.6f %10.6f) mag=%g "
			"c,s,f,s/g=%ld,%ld,%ld,%ld\n",
			o_main[l_no].usnob1_id,
			o_main[l_no].ra, o_main[l_no].dec,
			o_main[l_no].bdata[bix].mag,
			c,s,f,g);
	}

	if ( o_main[l_no].bdata[bix].f == 0 ) {
	    o_main[l_no].bdata[bix].xi = 0;
	    o_main[l_no].bdata[bix].eta = 0;
	}
	else {
	    o_main[l_no].bdata[bix].xi = q - 5000;
	    o_main[l_no].bdata[bix].eta = r - 5000;
	}
    }

    /*
     * fill cx,cy,cz of main table
     */
    ra_r  = o_main[l_no].ra * CC_DEG_TO_RAD;
    dec_r = o_main[l_no].dec * CC_DEG_TO_RAD;
    o_main[l_no].cx = cos(ra_r) * cos(dec_r);
    o_main[l_no].cy = sin(ra_r) * cos(dec_r);
    o_main[l_no].cz = sin(dec_r);


    /*
     * eqi table
     */
    o_ipos[l_no]._objid = serial;	/* not apply offset */

    o_ipos[l_no].rai = rai;
    o_ipos[l_no].deci = deci;


    return 0;
}

/**
 * Output an entry of usno-b1.0
 *
 * @param      main  Main table (top of address)
 * @param      eqi  eqi table (top of address)
 * @param      l_no  Number of line (beginning 0)
 * @param      fh_main  File handler to output main table
 * @param      fh_eqi  File handler to output eqi table
 * @param      fh_tycho2  File handler to output usnob1-tycho2 relation
 * @return     Status
 */
static int output_usnob1_line( const usnob1_entry *main,
			       const ipos_entry *eqi,
			       size_t l_no,
			       cstreamio &fh_main, 
			       cstreamio &fh_eqi,
			       cstreamio &fh_tycho2 )
{
    size_t bix;
    bool tycho2_entry = (main[l_no].n_det == 0);

    fh_main.printf("%lld%s", main[l_no].objid, DB_DELIMITER);
    fh_main.printf("%s%s", main[l_no].usnob1_id, DB_DELIMITER);
    if ( tycho2_entry ) {
        fh_main.printf("%s%s", main[l_no].tycho2_id, DB_DELIMITER);
    }
    else {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    fh_main.printf("%.15g%s", main[l_no].ra, DB_DELIMITER);
    fh_main.printf("%.15g%s", main[l_no].dec, DB_DELIMITER);
    if ( tycho2_entry ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%hd%s", main[l_no].e_ra, DB_DELIMITER);
	fh_main.printf("%hd%s", main[l_no].e_dec, DB_DELIMITER);
	fh_main.printf("%6.1f%s", main[l_no].epoch, DB_DELIMITER);
    }
    
    fh_main.printf("%hd%s", main[l_no].pm_ra, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].pm_dec, DB_DELIMITER);

    if ( tycho2_entry ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%hd%s", main[l_no].e_pm_ra, DB_DELIMITER);
	fh_main.printf("%hd%s", main[l_no].e_pm_dec, DB_DELIMITER);
	fh_main.printf("%hd%s", main[l_no].mu_pr, DB_DELIMITER);
	fh_main.printf("%g%s", main[l_no].fit_ra * 0.1, DB_DELIMITER);
	fh_main.printf("%g%s", main[l_no].fit_dec * 0.1, DB_DELIMITER);
    }
    fh_main.printf("%hd%s", main[l_no].n_det, DB_DELIMITER);

    fh_main.printf("%s%s", main[l_no].flags, DB_DELIMITER);

    for ( bix = 0 ; bix < 5 ; bix ++ ) {
	bool usnob1_null = ((! tycho2_entry) && main[l_no].bdata[bix].f == 0);
	if ( usnob1_null ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%5.2f%s", main[l_no].bdata[bix].mag, DB_DELIMITER);
	}
	if ( usnob1_null || tycho2_entry ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%hd%s", main[l_no].bdata[bix].c, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].bdata[bix].s, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].bdata[bix].f, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].bdata[bix].sg, DB_DELIMITER);
	    fh_main.printf("%g%s", main[l_no].bdata[bix].xi * 0.01, DB_DELIMITER);
	    fh_main.printf("%g%s", main[l_no].bdata[bix].eta * 0.01, DB_DELIMITER);
	}
    }

    fh_main.printf("%.17g%s", main[l_no].cx, DB_DELIMITER);
    fh_main.printf("%.17g%s", main[l_no].cy, DB_DELIMITER);
    fh_main.printf("%.17g\n", main[l_no].cz);


    /* Write an entry of EQI table (binary) */
    fh_eqi.write(eqi + l_no, sizeof(*eqi));

    if ( tycho2_entry ) {
	fh_tycho2.printf("%s%s", main[l_no].usnob1_id, DB_DELIMITER);
	fh_tycho2.printf("%s%s", main[l_no].tycho2_id, DB_DELIMITER);
	fh_tycho2.printf("%.15g%s", main[l_no].ra, DB_DELIMITER);
	fh_tycho2.printf("%.15g\n", main[l_no].dec);
    }

    return 0;
}


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

    tstring l_catname;
    tarray_tstring cat_file_sz_list;
    const char *dir_name;

    size_t max_line_count;
    long long n_all_entries;

    mdarray_int deci_check;
    int max_deci;

    long long serial_id;
    long long min_serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    stdstreamio fh_tycho2;
    int eqi_ix;
    int main_ix;

    unsigned char cat_line_buffer[CAT_LINE_BYTE];

    usnob1_entry *main_table_ptr;
    mdarray main_table(sizeof(*main_table_ptr), true, 
		       (void *)(&main_table_ptr));

    ipos_entry *ipos_table_ptr;
    mdarray ipos_table(sizeof(*ipos_table_ptr), true, 
		       (void *)(&ipos_table_ptr));

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
	sio.eprintf("  $ %s /somewhere/USNO-B1\n",argv[0]);
	sio.eprintf("  $ %s ftp://catftp:PASSWD@vizier.nao.ac.jp/FTP/USNO-B1\n",
		    argv[0]);
	sio.eprintf("  $ %s http://archive.noao.edu/catalogs/usnob\n",argv[0]);
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
     * Read list file
     */
    if ( fh_in.open("r", CAT_LIST_FILE) < 0 ) {
	sio.eprintf("[ERROR] fh_in.open() failed: %s.\n", CAT_LIST_FILE);
	goto quit;
    }
    i = 0;
    cat_file_sz_list.resize(1800);
    while ( (line=fh_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() ) {
	    cat_file_sz_list[i] = line;
	    i++;
	}
    }
    fh_in.close();
    cat_file_sz_list.resize(i);


    /*
     * Get max filesize and num of all entries
     */
    max_line_count = 0;
    n_all_entries = 0;
    for ( i=0 ; i < cat_file_sz_list.length() ; i++ ) {
	tarray_tstring el;
	size_t filesize, l_cnt;
	el.split(cat_file_sz_list[i], " ");
	filesize = el[1].atol();
	if ( filesize == 0 || 0 < filesize % CAT_LINE_BYTE ) {
	    sio.eprintf("[ERROR] Invalid filesize: %s.\n",
			cat_file_sz_list[i].cstr());
	    goto quit;
	}
	l_cnt = filesize / CAT_LINE_BYTE;
	if ( max_line_count < l_cnt ) max_line_count = l_cnt;
	n_all_entries += l_cnt;
    }

    sio.eprintf("[INFO] n_all_entries = %lld, max_line_count = %zd\n",
		n_all_entries, max_line_count);


    /*
     * Alloc buffer
     */
    main_table.resize(max_line_count);
    ipos_table.resize(max_line_count);


    /*
     * Open files to output
     */

    eqi_ix = 0;
    main_ix = 0;

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
    if ( fh_tycho2.openf("w", "%s_tycho2.txt",l_catname.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_tycho2.openf() failed\n");
	goto quit;
    }

    eqi_ix ++;
    main_ix ++;


    /*
     * Main loop
     */
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    for ( i=0 ; i < cat_file_sz_list.length() ; i++ ) {
	tarray_tstring el;
	tstring filename;
	size_t filesize, n_entries;
	ssize_t bytes;
	digeststreamio cat_in;
	size_t j;

	long long n_unit_main = n_all_entries / N_MAIN_FILES;
	long long n_unit_eqi = n_all_entries / N_EQI_CHILD_TABLES;

	min_serial_id = serial_id;

	/* A line -> filename, filesize */
	el.split(cat_file_sz_list[i], " ");
	filename.printf("%s/%s", dir_name, el[0].cstr());
	filesize = el[1].atol();

	sio.printf("Reading ... %s\r", filename.cstr());
	sio.flush();

	/* Open a cat file */
	j=0;
	while ( 1 ) {
	  if ( ! (cat_in.open("r", filename.cstr()) < 0) ) break;
	  if ( ! (cat_in.openf("r", "%s.gz", filename.cstr()) < 0) ) break;
	  if ( ! (cat_in.openf("r", "%s.bz2", filename.cstr()) < 0) ) break;
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

	/* Read all in a cat file */
	j = 0;
	while ( (bytes=cat_in.read(cat_line_buffer, CAT_LINE_BYTE))
		== CAT_LINE_BYTE ) {
	    if ( max_line_count <= j ) {
		sio.eprintf("[ERROR] too large cat file. broken?: %s\n", 
			    filename.cstr());
		goto quit;
	    }

	    parse_usnob1_line(serial_id, filename, cat_line_buffer,
			      main_table_ptr, ipos_table_ptr, j);

	    j++;
	    serial_id++;
	}
	if ( bytes != 0 ) {
	    sio.eprintf("[ERROR] cat file is broken[0]: %s\n",
			filename.cstr());
	    goto quit;
	}
	if ( CAT_LINE_BYTE * j != filesize ) {
	    sio.eprintf("[ERROR] cat file is broken[1]: %s\n", 
			filename.cstr());
	    goto quit;
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
	    /* Write an entry */
	    output_usnob1_line(main_table_ptr, ipos_table_ptr, j,
			       fh_main, fh_eqi, fh_tycho2);
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
    }	/* main loop ends here */

    fh_tycho2.close();
    fh_eqi.close();
    fh_main.close();
    /* read tmp binary, sort order for optimization,
       and write DB text file */
    if ( read_optimize_and_write_eqi(CAT_NAME, max_line_count, DB_DELIMITER,
				     eqi_ix-1, N_EQI_CHILD_TABLES,
				     N_DIV_EQI_SECTION,
				     &cmp_eqi_tbl_rai) < 0 ) {
	sio.eprintf("[ERROR] read_optimize_and_write_eqi() failed\n");
	goto quit;
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
