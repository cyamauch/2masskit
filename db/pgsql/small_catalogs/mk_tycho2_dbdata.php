<?
  /* create db file for Tycho-2 Catalogue */

  /* USAGE: */
  /* $ php mk_tycho2_dbdata.php */

  /* $objid = 490000000; */
  $objid = 980000000;
  $outfile = "tycho2_all.db.txt";

  $fp_out = fopen($outfile, "w");
  if ( $fp_out === false ) {
    fprintf(STDERR,"[ERROR] fopen() failed.\n");
    exit(1);
  }

  $objid++;
  for ( $i = 0 ; $i < 20 ; $i++ ) {
    printf("Downloading: %d/%d ...\n",$i + 1,20);
    output_catalog($i, $objid, $fp_out);
    sleep(5);
  }

  fclose($fp_out);

  function output_catalog($idx_arg, &$objid, $fp_out) {
    $url = "ftp://cdsarc.u-strasbg.fr/pub/cats/I/259/";
    $idx = sprintf("%02d",$idx_arg);
    $fp = popen("wget -q -O - $url" . "tyc2.dat." . $idx . ".gz | gzip -dc",
                "r");
    while ( ($s=fgets($fp)) !== false ) {
      $arr = explode("|", $s);
      for ( $i = 0 ; $i < count($arr) ; $i++ ) {
        if ( $i == 0 ) {
          $objname = str_replace(' ','-',$arr[$i]);
          sscanf($arr[$i], "%4[^\a]%1[^\a]%5[^\a]%1[^\a]%1[^\a]",
		 $e0,$e1,$e2,$e3,$e4);
          $e0 = trim($e0);
          $e2 = trim($e2);
          $e4 = trim($e4);
          if ( $e0 == '' ) $e0 = 'null';
          if ( $e2 == '' ) $e2 = 'null';
          if ( $e4 == '' ) $e4 = 'null';
          $arr[$i] = $e0 . "\t" . $e2 . "\t" . $e4;
        }
        else if ( $i == 23 ) {
          sscanf($arr[$i], "%6[^\a]%3[^\a]",
		 $e0,$e1);
          $e0 = trim($e0);
          $e1 = trim($e1);
          if ( $e0 == '' ) $e0 = 'null';
          if ( $e1 == '' ) $e1 = 'null';
          $arr[$i] = $e0 . "\t" . $e1;
        }
        else if ( $i == 26 || $i == 27 ) {
          $v = trim($arr[$i]);
	  $arr[$i] = sprintf("%7.2f",1990.0 + $v);
	}
        else if ( $i == 1 || $i == 22 || $i == 30 ) {
          // as is
	}
        else {
          $arr[$i] = trim($arr[$i]);
          if ( $arr[$i] == '' ) $arr[$i] = 'null';
        }
      }
      /* objid,objname */
      fprintf($fp_out,"%d\t%s\t",$objid,$objname);
      for ( $i = 1 ; $i < 4 ; $i++ ) {
        fprintf($fp_out,"%s\t",$arr[$i]);
      }
      fprintf($fp_out,"%s\t",$arr[6]);
      fprintf($fp_out,"%s\t",$arr[7]);
      fprintf($fp_out,"%s\t",$arr[10]);
      fprintf($fp_out,"%s\t",$arr[11]);
      fprintf($fp_out,"%s\t",$arr[4]);
      fprintf($fp_out,"%s\t",$arr[5]);
      fprintf($fp_out,"%s\t",$arr[8]);
      fprintf($fp_out,"%s\t",$arr[9]);
      for ( $i = 12 ; $i < 26 ; $i++ ) {
        fprintf($fp_out,"%s\t",$arr[$i]);
      }
      fprintf($fp_out,"%s\t",$arr[28]);
      fprintf($fp_out,"%s\t",$arr[29]);
      fprintf($fp_out,"%s\t",$arr[26]);
      fprintf($fp_out,"%s\t",$arr[27]);
      for ( $i = 30 ; $i + 1 < count($arr) ; $i++ ) {
        fprintf($fp_out,"%s\t",$arr[$i]);
      }
      /* last and cxm,cym,czm,cx,cy,cz */
      fprintf($fp_out,"%s\tnull\tnull\tnull\t0\t0\t0\n",$arr[$i]);
      $objid++;
    }
    fclose($fp);

    return;
  }

?>
