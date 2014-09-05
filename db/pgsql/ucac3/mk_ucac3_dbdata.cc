/* -*- Mode: C++ ; Coding: euc-japan -*- */
/* Time-stamp: <2011-12-19 15:12:08 cyamauch> */

/*
 *  mk_ucac3_dbdata.cc
 *  - Create DB files for 2massKit using original UCAC3 binary bz2 files.
 *
 *  Note:
 *  - This program requires SLLIB-1.1.0a or newer.  SLLIB is available at 
 *    http://www.ir.isas.jaxa.jp/~cyamauch/sli/
 *
 *  How to compile:
 *    $ s++ mk_ucac3_dbdata.cc
 *
 *  How to use:
 *    $ ./mk_ucac3_dbdata /somewhere/UCAC3/UCAC3
 *    $ ./mk_ucac3_dbdata ftp://cdsarc.u-strasbg.fr/pub/cats/I/315/UCAC3
 *    $ ./mk_ucac3_dbdata http://cdsarc.u-strasbg.fr/ftp/cats/aliases/U/UCAC3/UCAC3
 *  If you want create SQL and C source files, set 2nd arg like this:
 *    $ ./mk_ucac3_dbdata /somewhere/UCAC3/UCAC3 -
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
 * Settings for UCAC3
 */
/* ObiID */
#define OBJID_OFFSET 3300000000LL
/* Number of files for main table */
#define N_MAIN_FILES 16			/* set to < 2 GByte in each file */
/* Number of child tables */
#define N_EQI_CHILD_TABLES 160		/* Should be tuned */
/* Number of sections to sort (order by rai) in an EQI child table */
#define N_DIV_EQI_SECTION 1

#define CAT_NAME "Ucac3"

#define CAT_LINE_BYTE 84

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
 * Settings for UCAC3
 */

typedef struct _ucac3_entry {

    long long objid;

    char ucac3_id[11];

    double ra;
    double dec;
    float fit_mag;
    float ap_mag;
    float e_ap_mag;
    short obj_type;
    short ds_flag;

    short e_ra;
    short e_dec;
    short n_obs;
    short n_uobs;
    short n_upos;
    short n_pos;

    float ep_ra;
    float ep_dec;
    float pm_ra;
    float pm_dec;
    float e_pm_ra;
    float e_pm_dec;

    int id_2mass;
    float j_mag;
    float h_mag;
    float k_mag;
    short q_j_mag;
    short q_h_mag;
    short q_k_mag;
    float e_j_mag;
    float e_h_mag;
    float e_k_mag;

    float b_mag;
    float r2_mag;
    float i_mag;
    short class_sc;
    short q_b_mag;
    short q_r2_mag;
    short q_i_mag;

    char cat_flags[11];
    short spm_g_flag;
    short spm_c_flag;
    short leda_match;
    short x2m_match;
    int mpos_id;

    double cx;
    double cy;
    double cz;

} ucac3_entry;


/*
 * Functions for UCAC3
 */

/**
 * Obtain 4-byte int from little-endian binary
 *
 * @param      buf  Input buffer
 * @param      idx  Byte offset of buf
 * @return     4-byte int value
 */
static int32_t get_four( const unsigned char *buf, int idx )
{
    return (int32_t)((uint32_t)(buf[idx]) |
		     (uint32_t)(buf[idx+1]) << 8 |
		     (uint32_t)(buf[idx+2]) << 16 |
		     (uint32_t)(buf[idx+3]) << 24);
}

/**
 * Obtain 2-byte int from little-endian binary
 *
 * @param      buf  Input buffer
 * @param      idx  Byte offset of buf
 * @return     2-byte int value
 */
static int16_t get_two( const unsigned char *buf, int idx )
{
    return (int16_t)((uint16_t)(buf[idx]) | 
		     (uint16_t)(buf[idx+1]) << 8);
}

/**
 * Parse an entry of UCAC3 binary data
 *
 * @param      serial  Serial ID (beginning 0)
 * @param      filename   Name of cat file
 * @param      buf  An entry of binary data
 * @param      o_main  Result main table (top of address)
 * @param      o_eqi  Result eqi table (top of address)
 * @param      l_no  Number of line (beginning 0)
 * @return     Status
 */
static int parse_ucac3_line( long long serial,
			     const tstring &filename,
			     const unsigned char *buf, 
			     ucac3_entry *o_main,
			     ipos_entry *o_eqi,
			     size_t l_no )
{
    stdstreamio sio;
    tstring designation;
    long lv;
    short sv;
    double pv;
    int rai, deci;
    double ra_r, dec_r;

    /*
     * Start main table
     */

    /* objid */
    o_main[l_no].objid = OBJID_OFFSET + serial;

    /* ucac3_id */
    filename.copy(filename.length()-7,3, &designation);
    designation.replacef(3,8,"-%06zd",l_no+1);
    designation.getstr(o_main[l_no].ucac3_id,11);

    /* ra, dec */
    lv = get_four(buf, 0);
    pv = (double)lv / 3600000.0;
    rai = RA2RAI(pv);
    if ( RA2RAI(360.0) <= rai ) rai = RA2RAI(0.0);
    o_main[l_no].ra = RAI2RA(rai);

    lv = get_four(buf, 4);
    pv = (double)(lv - 324000000) / 3600000.0;
    deci = DEC2DECI(pv);
    if ( DEC2DECI(90.0) < deci ) deci = DEC2DECI(90.0);
    else if ( deci < DEC2DECI(-90.0) ) deci = DEC2DECI(-90.0);
    o_main[l_no].dec = DECI2DEC(deci);

    /* fit_mag, ... */
    sv = get_two(buf, 8);
    if ( sv == 18000 ) o_main[l_no].fit_mag = 18.0;
    else o_main[l_no].fit_mag = (float)sv / 1000.0;
    sv = get_two(buf, 10);
    if ( sv == 18000 ) o_main[l_no].ap_mag = 18.0;
    else o_main[l_no].ap_mag = (float)sv / 1000.0;
    sv = get_two(buf, 12);
    o_main[l_no].e_ap_mag = (float)sv / 1000.0;
    o_main[l_no].obj_type = buf[14];
    o_main[l_no].ds_flag = buf[15];

    /* e_ra, ... */
    o_main[l_no].e_ra =   get_two(buf, 16);
    o_main[l_no].e_dec =  get_two(buf, 18);
    o_main[l_no].n_obs =  buf[20];
    o_main[l_no].n_uobs = buf[21];
    o_main[l_no].n_upos = buf[22];
    o_main[l_no].n_pos =  buf[23];

    /* ep_ra, ... */
    sv = get_two(buf, 24);
    o_main[l_no].ep_ra = (float)sv * 0.01 + 1900.0;
    sv = get_two(buf, 26);
    o_main[l_no].ep_dec = (float)sv * 0.01 + 1900.0;
    lv = get_four(buf, 28);
    o_main[l_no].pm_ra = (float)lv * 0.1;
    lv = get_four(buf, 32);
    o_main[l_no].pm_dec = (float)lv * 0.1;
    sv = get_two(buf, 36);
    o_main[l_no].e_pm_ra = (float)sv * 0.1;
    sv = get_two(buf, 38);
    o_main[l_no].e_pm_dec = (float)sv * 0.1;

    /* 2MASS */
    o_main[l_no].id_2mass = get_four(buf, 40);
    sv = get_two(buf, 44);
    if ( sv == 30000 ) o_main[l_no].j_mag = 30.0;
    else o_main[l_no].j_mag = (float)sv / 1000.0;
    sv = get_two(buf, 46);
    if ( sv == 30000 ) o_main[l_no].h_mag = 30.0;
    else o_main[l_no].h_mag = (float)sv / 1000.0;
    sv = get_two(buf, 48);
    if ( sv == 30000 ) o_main[l_no].k_mag = 30.0;
    else o_main[l_no].k_mag = (float)sv / 1000.0;
    o_main[l_no].q_j_mag = buf[50];
    o_main[l_no].q_h_mag = buf[51];
    o_main[l_no].q_k_mag = buf[52];
    o_main[l_no].e_j_mag = buf[53];
    o_main[l_no].e_j_mag /= 100.0;
    o_main[l_no].e_h_mag = buf[54];
    o_main[l_no].e_h_mag /= 100.0;
    o_main[l_no].e_k_mag = buf[55];
    o_main[l_no].e_k_mag /= 100.0;

    /* SuperCosmos */
    sv = get_two(buf, 56);
    if ( sv == 30000 ) o_main[l_no].b_mag = 30.0;
    else o_main[l_no].b_mag = (float)sv / 1000.0;
    sv = get_two(buf, 58);
    if ( sv == 30000 ) o_main[l_no].r2_mag = 30.0;
    else o_main[l_no].r2_mag = (float)sv / 1000.0;
    sv = get_two(buf, 60);
    if ( sv == 30000 ) o_main[l_no].i_mag = 30.0;
    else o_main[l_no].i_mag = (float)sv / 1000.0;
    o_main[l_no].class_sc = buf[62];
    o_main[l_no].q_b_mag = buf[63];
    o_main[l_no].q_r2_mag = buf[64];
    o_main[l_no].q_i_mag = buf[65];

    /* flags */
    o_main[l_no].cat_flags[0] = '0' + buf[66];
    o_main[l_no].cat_flags[1] = '0' + buf[67];
    o_main[l_no].cat_flags[2] = '0' + buf[68];
    o_main[l_no].cat_flags[3] = '0' + buf[69];
    o_main[l_no].cat_flags[4] = '0' + buf[70];
    o_main[l_no].cat_flags[5] = '0' + buf[71];
    o_main[l_no].cat_flags[6] = '0' + buf[72];
    o_main[l_no].cat_flags[7] = '0' + buf[73];
    o_main[l_no].cat_flags[8] = '0' + buf[74];
    o_main[l_no].cat_flags[9] = '0' + buf[75];
    o_main[l_no].cat_flags[10] = '\0';

    o_main[l_no].spm_g_flag = buf[76];
    o_main[l_no].spm_c_flag = buf[77];
    o_main[l_no].leda_match = buf[78];
    o_main[l_no].x2m_match  = buf[79];
    o_main[l_no].mpos_id = get_four(buf, 80);


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
    o_eqi[l_no]._objid = serial;	/* not apply offset */

    o_eqi[l_no].rai = rai;
    o_eqi[l_no].deci = deci;


    return 0;
}

/**
 * Output an entry of UCAC3
 *
 * @param      main  Main table (top of address)
 * @param      eqi  Eqi table (top of address)
 * @param      l_no  Number of line (beginning 0)
 * @param      fh_main  File handler to output main table
 * @param      fh_eqi  File handler to output eqi table
 * @return     Status
 */
static int output_ucac3_line( const ucac3_entry *main,
			      const ipos_entry *eqi,
			      size_t l_no,
			      cstreamio &fh_main, 
			      cstreamio &fh_eqi )
{
    /* id */
    fh_main.printf("%lld%s", main[l_no].objid, DB_DELIMITER);
    fh_main.printf("%s%s", main[l_no].ucac3_id, DB_DELIMITER);

    /* ra, dec, ... */
    fh_main.printf("%.15g%s", main[l_no].ra, DB_DELIMITER);
    fh_main.printf("%.15g%s", main[l_no].dec, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].e_ra, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].e_dec, DB_DELIMITER);
    fh_main.printf("%7.2f%s", main[l_no].ep_ra, DB_DELIMITER);
    fh_main.printf("%7.2f%s", main[l_no].ep_dec, DB_DELIMITER);

    /* pm_ra, pm_dec, ... */
    if ( main[l_no].n_upos < 2 ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%.1f%s", main[l_no].pm_ra, DB_DELIMITER);
	fh_main.printf("%.1f%s", main[l_no].pm_dec, DB_DELIMITER);
	if ( main[l_no].e_pm_ra < 0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.1f%s", main[l_no].e_pm_ra, DB_DELIMITER);
	}
	if ( main[l_no].e_pm_dec < 0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.1f%s", main[l_no].e_pm_dec, DB_DELIMITER);
	}
    }

    /* flux */
    if ( main[l_no].fit_mag == 18.0 ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%.3f%s", main[l_no].fit_mag, DB_DELIMITER);
    }
    if ( main[l_no].ap_mag == 18.0 ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%.3f%s", main[l_no].ap_mag, DB_DELIMITER);
	if ( main[l_no].e_ap_mag < 0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].e_ap_mag, DB_DELIMITER);
	}
    }

    /* obj type, ds */
    fh_main.printf("%hd%s", main[l_no].obj_type, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].ds_flag, DB_DELIMITER);

    /* counts, ... */
    fh_main.printf("%hd%s", main[l_no].n_obs, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].n_uobs, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].n_pos, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].n_upos, DB_DELIMITER);

    /* 2MASS */
    if ( main[l_no].id_2mass == 0 ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%d%s", main[l_no].id_2mass, DB_DELIMITER);
	if ( main[l_no].j_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].j_mag, DB_DELIMITER);
	    fh_main.printf("%.2f%s", main[l_no].e_j_mag, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].q_j_mag, DB_DELIMITER);
	}
	if ( main[l_no].h_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].h_mag, DB_DELIMITER);
	    fh_main.printf("%.2f%s", main[l_no].e_h_mag, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].q_h_mag, DB_DELIMITER);
	}
	if ( main[l_no].k_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].k_mag, DB_DELIMITER);
	    fh_main.printf("%.2f%s", main[l_no].e_k_mag, DB_DELIMITER);
	    fh_main.printf("%hd%s", main[l_no].q_k_mag, DB_DELIMITER);
	}
    }

    /* SuperCosmos */
    if ( main[l_no].class_sc == 0 ) {
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
    }
    else {
	fh_main.printf("%hd%s", main[l_no].class_sc, DB_DELIMITER);
	if ( main[l_no].b_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].b_mag, DB_DELIMITER);
	    if ( main[l_no].q_b_mag < 0 ) {
		fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    }
	    else {
		fh_main.printf("%hd%s", main[l_no].q_b_mag, DB_DELIMITER);
	    }
	}
	if ( main[l_no].r2_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].r2_mag, DB_DELIMITER);
	    if ( main[l_no].q_r2_mag < 0 ) {
		fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    }
	    else {
		fh_main.printf("%hd%s", main[l_no].q_r2_mag, DB_DELIMITER);
	    }
	}
	if ( main[l_no].i_mag == 30.0 ) {
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	}
	else {
	    fh_main.printf("%.3f%s", main[l_no].i_mag, DB_DELIMITER);
	    if ( main[l_no].q_i_mag < 0 ) {
		fh_main.printf("%s%s", DB_NULL, DB_DELIMITER);
	    }
	    else {
		fh_main.printf("%hd%s", main[l_no].q_i_mag, DB_DELIMITER);
	    }
	}
    }

    /* other flags, etc.  */
    fh_main.printf("%s%s", main[l_no].cat_flags, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].spm_g_flag, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].spm_c_flag, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].leda_match, DB_DELIMITER);
    fh_main.printf("%hd%s", main[l_no].x2m_match, DB_DELIMITER);
    fh_main.printf("%d%s", main[l_no].mpos_id, DB_DELIMITER);

    /* cx, cy, cz */
    fh_main.printf("%.17g%s", main[l_no].cx, DB_DELIMITER);
    fh_main.printf("%.17g%s", main[l_no].cy, DB_DELIMITER);
    fh_main.printf("%.17g\n", main[l_no].cz);

    /* Write an entry of EQI table (binary) */
    fh_eqi.write(eqi + l_no, sizeof(*eqi));

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
    digeststreamio fh_in;
    tstring line;
    size_t i;

    tarray_tstring cat_file_cnt_list;
    const char *dir_name;
    tstring list_file;

    size_t max_line_count;
    long long n_all_entries;

    mdarray_int deci_check;
    int max_deci;

    long long serial_id;
    long long min_serial_id;

    tstring fname;
    stdstreamio fh_main;
    stdstreamio fh_eqi;
    int eqi_ix;
    int main_ix;

    unsigned char cat_line_buffer[CAT_LINE_BYTE];

    ucac3_entry *main_table_ptr;
    mdarray main_table(sizeof(*main_table_ptr), (void *)(&main_table_ptr));

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
	sio.eprintf("  $ %s /somewhere/UCAC3/UCAC3\n",argv[0]);
	sio.eprintf("  $ %s ftp://cdsarc.u-strasbg.fr/pub/cats/I/315/UCAC3\n",argv[0]);
	sio.eprintf("  $ %s http://cdsarc.u-strasbg.fr/ftp/cats/aliases/U/UCAC3/UCAC3\n",argv[0]);
	goto quit;
    }
    dir_name = argv[1];

    if ( argc < 3 ) {
	sio.eprintf("[INFO] SQL and C files will not be created.\n");
    }

    /*
     * Read list file
     */
    list_file.printf("%s/table_zones", dir_name);
    if ( fh_in.open("r", list_file.cstr()) < 0 ) {
      list_file.printf("%s/table_zones.txt", dir_name);
      if ( fh_in.open("r", list_file.cstr()) < 0 ) {
	sio.eprintf("[ERROR] fh_in.open() failed: %s.\n", list_file.cstr());
	goto quit;
      }
    }
    i = 0;
    cat_file_cnt_list.resize(360);
    while ( (line=fh_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() && line.isdigit(0) == true ) {
	    cat_file_cnt_list[i] = line;
	    i++;
	}
    }
    fh_in.close();
    cat_file_cnt_list.resize(i);


    /*
     * Get max line count and num of all entries
     */
    max_line_count = 0;
    n_all_entries = 0;
    for ( i=0 ; i < cat_file_cnt_list.length() ; i++ ) {
	tarray_tstring el;
	size_t line_count;
	el.split(cat_file_cnt_list[i], " ");
	line_count = el[2].atol();
	if ( line_count == 0 ) {
	    sio.eprintf("[ERROR] Invalid line_count: %s.\n",
			cat_file_cnt_list[i].cstr());
	    goto quit;
	}
	if ( max_line_count < line_count ) max_line_count = line_count;
	n_all_entries += line_count;
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

    eqi_ix ++;
    main_ix ++;


    /*
     * Main loop
     */
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    for ( i=0 ; i < cat_file_cnt_list.length() ; i++ ) {
	tarray_tstring el;
	tstring filename;
	size_t line_count, n_entries;
	ssize_t bytes;
	digeststreamio cat_in;
	size_t j;

	long long n_unit_main = n_all_entries / N_MAIN_FILES;
	long long n_unit_eqi = n_all_entries / N_EQI_CHILD_TABLES;

	min_serial_id = serial_id;

	/* A line -> filename, line_count */
	el.split(cat_file_cnt_list[i], " ");
	filename.printf("%s/z%03ld.bz2", dir_name, el[0].atol());
	line_count = el[2].atol();

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

	/* Read all in a cat file */
	j = 0;
	while ( (bytes=cat_in.read(cat_line_buffer, CAT_LINE_BYTE))
		== CAT_LINE_BYTE ) {
	    if ( max_line_count <= j ) {
		sio.eprintf("[ERROR] too large cat file. broken?: %s\n", 
			    filename.cstr());
		goto quit;
	    }

	    parse_ucac3_line(serial_id, filename, cat_line_buffer,
			     main_table_ptr, ipos_table_ptr, j);

	    j++;
	    serial_id++;
	}
	if ( bytes != 0 ) {
	    sio.eprintf("[ERROR] cat file is broken[0]: %s\n", 
			filename.cstr());
	    goto quit;
	}
	if ( j != line_count ) {
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
	    output_ucac3_line(main_table_ptr, ipos_table_ptr, j,
			      fh_main, fh_eqi);
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
