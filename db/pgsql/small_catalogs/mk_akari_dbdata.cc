#include <sli/stdstreamio.h>
#include <sli/tstring.h>
#include <sli/fitscc.h>
#include <math.h>

using namespace sli;

/* Program to create AKARI database files for 2MASS Kit */

/* [How to compile]                 */
/* s++ mk_akari_dbdata.cc -lsfitsio */

/* [Requirements]                                       */
/* Install latest SLLIB and SFITSIO before running s++. */
/* You can download SLLIB and SFITSIO from here:        */
/* http://www.ir.isas.jaxa.jp/~cyamauch/sli/            */

int main( int argc, char *argv[] )
{
    int status = -1;
    stdstreamio sio;
    fitscc fits;

    if ( argc <= 1 ) {
       sio.eprintf("[USAGE]\n");
       sio.eprintf("%s http://www.ir.isas.jaxa.jp/AKARI/Observation/PSC/Public/data/AKARI-FIS_BSC_V1.fits.gz > akarifis_all.db.txt\n",argv[0]);
       sio.eprintf("%s http://www.ir.isas.jaxa.jp/AKARI/Observation/PSC/Public/data/AKARI-IRC_PSC_V1.fits.gz > akariirc_all.db.txt\n",argv[0]);
       goto quit;
    }
    else {
	long i, j;
	if ( fits.read_stream(argv[1]) < 0 ) {
	    sio.eprintf("[ERROR] cannot open %s\n",argv[1]);
	    goto quit;
	}
	fits_table &tbl = fits.table(1);
	for ( i=0 ; i < tbl.row_length() ; i++ ) {
	    for ( j=0 ; j < tbl.col_length() ; j++ ) {
		const char *sep = "\t";
		if ( j == tbl.col_length() - 1 ) {
		    /* append columns for cx,cy,cz */
		    sep = "\t0\t0\t0";
		}
		sio.printf("%s%s",tbl.col(j).svalue(i),sep);
	    }
	    sio.printf("\n");
	}
    }

    status = 0;
 quit:
    return status;
}
