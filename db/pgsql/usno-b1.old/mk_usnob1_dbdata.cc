#include <sli/stdstreamio.h>
#include <sli/digeststreamio.h>
#include <sli/tstring.h>
#include <sli/tarray_tstring.h>
#include <sli/mdarray.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
using namespace sli;

/*
 * Settings for USNO-B1
 */
/* ObiID */
#define OBJID_OFFSET 2200000000LL
/* Number of child tables */
#define N_XYZ_CHILD_TABLES 1600
#define N_RADEC_CHILD_TABLES 92

#define CAT_LINE_BYTE 80

/*
 * Settings for data files of DB
 */
#define DB_DELIMITER '\t'
#define DB_NULL "null"

/*
 * should not be changed
 */
#define XYZ_INDEX_RESOLUTION 32400

#define CC_DEG_TO_RAD 0.017453292519943295
#define SC_XYZI 2e9
#define RA2RAI(v) round_d2i(((v)-180.0)*1e7)
#define DEC2DECI(v) round_d2i((v)*1e7)



/**
 * Round to nearest integer
 *
 * @param      v  input (double)
 * @return     Rounded integer value
 */
static int round_d2i( double v )
{
    if ( v < 0 ) return (int)(v-0.5);
    else return (int)(v+0.5);
}

/**
 * Obtain list of required files
 *
 * @param      cmd  Command to obtain file names
 * @param      reg_pattern  Regular expression of required file names
 * @param      out  list of files
 * @return     status
 */
#if 0
static int get_filelist( const char *cmd, const char *reg_pattern,
			 tarray_tstring *out )
{
    int ret = -1;
    stdstreamio sio;
    digeststreamio din;
    tstring line;

    if ( din.openp(cmd) != 0 ) {
	sio.eprintf("[ERROR] ls failed\n");
	goto quit;
    }
    out->resize(0);
    while ( (line=din.getline()) != NULL ) {
	if ( 0 <= line.regmatch(reg_pattern) ) out->append(line.chomp(),1);
    }
    din.close();
  
    ret = 0;
 quit:
    return ret;
}
#endif

/*
 * General struct definitions
 */

typedef struct _xyzi_entry {
    long long _objid;
    int cxi;
    int cyi;
    int czi;
    int rai;
    int deci;
} xyzi_entry;

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

    char designation[13];

    double ra;		/* [0-3] */
    double dec;		/* [4-7] */
    short e_ra;		/* [16-19] */
    short e_dec;	/* [16-19] */
    float epoch;	/* mean epoch [16-19] */
    int pm_ra;		/* [8-11] */
    int pm_dec;		/* [8-11] */
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

static unsigned long get_four( const unsigned char *buf, int idx )
{
    return (unsigned long)(buf[idx]) |
	(unsigned long)(buf[idx+1]) << 8 |
	(unsigned long)(buf[idx+2]) << 16 |
	(unsigned long)(buf[idx+3]) << 24;
}

/**
 * Parse an entry of usno-b1.0 binary data
 *
 * @param      serial  Serial ID (beginning 0)
 * @param      filename   Name of cat file
 * @param      buf  an entry of binary data
 * @param      o_main  Result main table (top of address)
 * @param      o_xyzi  Result xyzi table (top of address)
 * @param      l_no  Number of line (beginning 0)
 * @return     status
 */
static int parse_usnob1_line( long long serial,
			      const tstring &filename,
			      const unsigned char *buf, 
			      usnob1_entry *o_main,
			      xyzi_entry *o_xyzi,
			      size_t l_no )
{
    stdstreamio sio;
    tstring designation;
    filename.copy(filename.length()-8,4, &designation);
    designation.replacef(4,8,"-%07zd",l_no+1);
    unsigned long ul, k,e,v,u, a,s,p,i, x,y,q,r,m,j, g,f,c;
    int bix;

    double ra_r, dec_r;


    /*
     * Start main table
     */

    o_main[l_no].objid = OBJID_OFFSET + serial;

    memcpy(o_main[l_no].designation,designation.cstr(),designation.length()+1);

    ul = get_four(buf, 0);

    o_main[l_no].ra = (double)ul * 0.01 / 3600.0;

    ul = get_four(buf, 4);

    o_main[l_no].dec = (double)ul * 0.01 / 3600.0 - 90.0;

    ul = get_four(buf, 16);

    u = ul % 1000;
    ul /=    1000;
    v = ul % 1000;
    ul /=    1000;
    e = ul % 1000;
    ul /=    1000;
    k = ul % 10;

    o_main[l_no].e_ra  = u;
    o_main[l_no].e_dec = v;
    o_main[l_no].epoch = 1950.0 + 0.1 * e;

    ul = get_four(buf, 8);

    a = ul % 10000;
    ul /=    10000;
    s = ul % 10000;
    ul /=    10000;
    p = ul % 10;
    ul /=    10;
    i = ul % 10;

    o_main[l_no].pm_ra  = 2*(a - 5000);

    /* This is in out.sam: wrong direction? */
    /* o_main[l_no].pm_dec = -2*(s - 5000); */

    o_main[l_no].pm_dec = 2*(s - 5000);

    o_main[l_no].mu_pr = p;

    ul = get_four(buf, 12);

    x = ul % 1000;
    ul /=    1000;
    y = ul % 1000;
    ul /=    1000;
    q = ul % 10;
    ul /=    10;
    r = ul % 10;
    ul /=    10;
    m = ul % 10;
    ul /=    10;
    j = ul % 10;

    o_main[l_no].e_pm_ra = x;
    o_main[l_no].e_pm_dec = y;
    o_main[l_no].fit_ra = q;
    o_main[l_no].fit_dec = r;
    o_main[l_no].n_det = m;

    if ( i != 0 ) o_main[l_no].flags[0] = 'M';
    else o_main[l_no].flags[0] = '.';
    if ( j != 0 ) o_main[l_no].flags[1] = 's';
    else o_main[l_no].flags[1] = '.';
    if ( k != 0 ) o_main[l_no].flags[2] = 'Y';
    else o_main[l_no].flags[2] = '.';
    o_main[l_no].flags[3] = '\0';

    if ( o_main[l_no].n_det < 2 ) {
	if ( o_main[l_no].mu_pr != 0 ) {
	    sio.eprintf("[WARNING] found garbage data, set zero: ");
	    sio.eprintf("%s ra,dec=%10.6f,%10.6f mu_pr=%hd -> 0\n",
			    o_main[l_no].designation,
			    o_main[l_no].ra, o_main[l_no].dec,
			    o_main[l_no].mu_pr);
	}
    }


    /*
     * 5-bands
     */

    for ( bix=0 ; bix < 5 ; bix++ ) {

	ul = get_four(buf, 20 + 4*bix);

	m = ul % 10000;
	ul /=    10000;
	f = ul % 1000;
	ul /=    1000;
	s = ul % 10;
	ul /=    10;
	g = ul % 100;

	ul = get_four(buf, 40 + 4*bix);

	q = ul % 10000;
	ul /=    10000;
	r = ul % 10000;
	ul /=    10000;
	c = ul % 10;

	o_main[l_no].bdata[bix].mag = m * 0.01;
	o_main[l_no].bdata[bix].c = c;
	o_main[l_no].bdata[bix].s = s;
	o_main[l_no].bdata[bix].f = f;
	o_main[l_no].bdata[bix].sg = g;

	/* clean up garbage data */
	if ( o_main[l_no].n_det < 2 ) {
	    bool flg = false;
	    if ( c != 0 ) {
		flg = true;
		o_main[l_no].bdata[bix].c = 0;
	    }
	    if ( s != 0 ) {
		flg = true;
		o_main[l_no].bdata[bix].s = 0;
	    }
	    if ( f != 0 ) {
		flg = true;
		o_main[l_no].bdata[bix].f = 0;
	    }
	    if ( g != 0 ) {
		flg = true;
		o_main[l_no].bdata[bix].sg = 0;
	    }
	    if ( flg == true ) {
		sio.eprintf("[WARNING] found garbage data (band=%d), set zero: ",
			    bix);
		sio.eprintf("%s ra,dec=%10.6f,%10.6f c,s,f,sg=%ld,%ld,%ld,%ld -> 0\n",
			    o_main[l_no].designation,
			    o_main[l_no].ra, o_main[l_no].dec,
			    c,s,f,g);
	    }
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
     * xyzi and j2000i tables
     */
    o_xyzi[l_no]._objid = serial;	/* not apply offset */
    o_xyzi[l_no].cxi = round_d2i(o_main[l_no].cx * SC_XYZI);
    o_xyzi[l_no].cyi = round_d2i(o_main[l_no].cy * SC_XYZI);
    o_xyzi[l_no].czi = round_d2i(o_main[l_no].cz * SC_XYZI);

    o_xyzi[l_no].rai = RA2RAI(o_main[l_no].ra);
    o_xyzi[l_no].deci = DEC2DECI(o_main[l_no].dec);


    /*
     * monitoring
     */

    //if ( 1 /* o_main[l_no].bdata[bix].f == 0 && o_main[l_no].bdata[bix].mag != 0.0 */ ) {
    //if ( o_main[l_no].bdata[bix].c == 0 && o_main[l_no].bdata[bix].mag != 0.0 ) {
    //if ( o_main[l_no].n_det == 1 ) {
    //if ( o_main[l_no].bdata[bix].mag == 0.0 && 0 < o_main[l_no].bdata[bix].c ) {

    //if ( o_main[l_no].n_det < 2 && o_main[l_no].bdata[bix].f != 0 ) {
    
    //if ( 12 <= o_main[l_no].bdata[4].sg && o_main[l_no].bdata[4].sg < 19 ) {
    if ( 0 ) {

	sio.printf("(%lld)",o_main[l_no].objid);
	sio.printf("(%s)",o_main[l_no].designation);
	sio.printf("(%10.6f)",o_main[l_no].ra);
	sio.printf("(%10.6f)",o_main[l_no].dec);
	sio.printf("(%hd)",o_main[l_no].e_ra);
	sio.printf("(%hd)",o_main[l_no].e_dec);
	sio.printf("(%6.1f)",o_main[l_no].epoch);

	sio.printf("(%d)",o_main[l_no].pm_ra);
	sio.printf("(%d)",o_main[l_no].pm_dec);

	/* be NULL if ( n_det < 2 ) */
	sio.printf("(%hd)",o_main[l_no].mu_pr);

	sio.printf("(%hd)",o_main[l_no].e_pm_ra);
	sio.printf("(%hd)",o_main[l_no].e_pm_dec);
	sio.printf("(%hd)",o_main[l_no].fit_ra);
	sio.printf("(%hd)",o_main[l_no].fit_dec);
	sio.printf("(n_det=%hd)",o_main[l_no].n_det);

	sio.printf("(%s)",o_main[l_no].flags);

	for ( bix = 0 ; bix < 5 ; bix ++ ) {
	    sio.printf("|(%5.2f)",o_main[l_no].bdata[bix].mag);
	    /* be NULL if (b1_mag == 0 || n_det < 2) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].c);
	    /* be NULL if (b1_mag == 0 || n_det < 2) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].s);
	    /* be NULL if (b1_f==0) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].f);
	    /* be NULL if (b1_mag == 0 || n_det < 2) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].sg);
	    /* be NULL if (b1_f==0) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].xi);
	    /* be NULL if (b1_f==0) */
	    sio.printf("(%hd)",o_main[l_no].bdata[bix].eta);
	}

	sio.printf("(%.17g)",o_main[l_no].cx);
	sio.printf("(%.17g)",o_main[l_no].cy);
	sio.printf("(%.17g)",o_main[l_no].cz);
	
	sio.printf("\n");

    }

    return 0;
}


/*
 * for qsort()
 */
static int cmp_xyzi_tbl( const void *_p0, const void *_p1 )
{
    const xyzi_entry *p0 = (const xyzi_entry *)_p0;
    const xyzi_entry *p1 = (const xyzi_entry *)_p1;

    return p0->deci - p1->deci;
}


/*
 * main function
 */
int main( int argc, char *argv[] )
{
    int ret_status = -1;
    stdstreamio sio;
    stdstreamio f_in;
    tstring line;
    tarray_tstring cat_file_sz_list;
    const char *dir_name;
    size_t max_filesize;
    size_t i, j;
    ssize_t bytes;
    int max_deci;

    long long serial_id;
    unsigned char cat_line_buffer[CAT_LINE_BYTE];

    usnob1_entry *main_table_ptr;
    mdarray main_table(sizeof(usnob1_entry), (void *)(&main_table_ptr));

    xyzi_entry *xyzi_table_ptr;
    mdarray xyzi_table(sizeof(xyzi_entry), (void *)(&xyzi_table_ptr));


    if ( argc < 2 ) {
	sio.eprintf("Specify directory name.\n");
	goto quit;
    }
    dir_name = argv[1];


    /*
     * Read usnob1_list.txt
     */
    if ( f_in.open("r", "usnob1_list.txt") < 0 ) {
    //if ( f_in.open("r", "usnob1_0900.txt") < 0 ) {
	sio.eprintf("[ERROR] f_in.open() failed.\n");
	goto quit;
    }
    while ( (line=f_in.getline()) != NULL ) {
	line.trim();
	if ( 0 < line.length() ) cat_file_sz_list.append(line,1);
    }
    f_in.close();
    

    /*
     * Get max filesize
     */
    max_filesize = 0;
    for ( i=0 ; i < cat_file_sz_list.length() ; i++ ) {
	tarray_tstring el;
	size_t filesize;
	el.split(cat_file_sz_list[i], " ");
	filesize = el[1].atol();
	if ( filesize == 0 || 0 < filesize % CAT_LINE_BYTE ) {
	    sio.eprintf("[ERROR] Invalid filesize: %s.\n",
			cat_file_sz_list[i].cstr());
	    goto quit;
	}
	if ( max_filesize < filesize ) max_filesize = filesize;
    }
    /* sio.printf("max_filesize = %zd\n",max_filesize); */


    /*
     * Alloc buffer
     */
    main_table.resize(max_filesize / CAT_LINE_BYTE);
    xyzi_table.resize(max_filesize / CAT_LINE_BYTE);


    /*
     * Main loop
     */
    serial_id = 1;
    max_deci = DEC2DECI(-90.0);
    for ( i=0 ; i < cat_file_sz_list.length() ; i++ ) {
	tarray_tstring el;
	const char *filename;
	size_t filesize, n_entry;
	digeststreamio cat_in;

	el.split(cat_file_sz_list[i], " ");
	el[0].insertf(0, "%s/", dir_name);
	filename = el[0].cstr();
	filesize = el[1].atol();

	sio.printf("Reading ... %s\r", filename);
	sio.flush();

	if ( cat_in.open("r", filename) < 0 ) {
	    sio.eprintf("[ERROR] cat_in.open() failed\n");
	    goto quit;
	}

	/* Read all in a cat file */
	j = 0;
	while ( (bytes=cat_in.read(cat_line_buffer, CAT_LINE_BYTE))
		== CAT_LINE_BYTE ) {

	    parse_usnob1_line(serial_id, filename, cat_line_buffer,
			      main_table_ptr, xyzi_table_ptr, j);

	    j++;
	    serial_id++;
	}
	if ( bytes != 0 ) {
	    sio.eprintf("[ERROR] cat file is broken[0]: %s\n", filename);
	    goto quit;
	}
	if ( CAT_LINE_BYTE * j != filesize ) {
	    sio.eprintf("[ERROR] cat file is broken[1]: %s\n", filename);
	    goto quit;
	}
	n_entry = j;

	/* Sort xyzi_entry */
	qsort(xyzi_table_ptr, n_entry, sizeof(*xyzi_table_ptr), &cmp_xyzi_tbl);
	if ( xyzi_table_ptr[0].deci < max_deci ) {
	    sio.eprintf("[ERROR] Invalid DEC in cat file: %s\n", filename);
	    goto quit;
	}
	max_deci = xyzi_table_ptr[n_entry - 1].deci;

	/*
	for ( j=0 ; j < n_entry ; j++ ) {
	    sio.printf("%lld,%d,%d,%d,%d,%d\n",xyzi_table_ptr[j]._objid,
		       xyzi_table_ptr[j].cxi, xyzi_table_ptr[j].cyi,
		       xyzi_table_ptr[j].czi,
		       xyzi_table_ptr[j].rai, xyzi_table_ptr[j].deci);
	}
	*/

	//sio.printf(":::%zd %zd\n",n_entry,bytes);
	//break;

	cat_in.close();
    }
    





    ret_status = 0;
 quit:
    return ret_status;
}

