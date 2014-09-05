<?php

  /***************************************************************************/
  /** An Example of Client Program for 2MASS Kit Server **********************/
  /***************************************************************************/

  /* Preparation for this script    */

  /* # yum install php              */
  /* # yum install php-pear         */
  /* # pear install MDB2            */
  /* # pear install pear/MDB2#pgsql */

  /* Usage */

  /*
     $ php sql2mass.php "SELECT * FROM twomass LIMIT 10"
   */

  require_once 'MDB2.php';

  /***************************************************************************/
  /** S E T T I N G S ********************************************************/
  /***************************************************************************/

  /* You should replace PASSWORD with actual one */
  /* (Do not set 'localhost') */
  $Dsn = "pgsql://guest:PASSWORD@127.0.0.1:5432/2MASS";
  $Query_timeout = 90;


  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   *
   *  D B   F U N C T I O N S
   *
   *  Following function is used only for PostgreSQL or MySQL
   *   - cas_db_get_column_names()
   *
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * Connect to DB
   *
   * @param      $dsn DSN (e.g., "pgsql://guest:PASSWD@localhost:5432/TEST_DB")
   * @param      $query_timeout Query Timeout (msec). Only for PostgreSQL.
   *                            Giving "" disables this feature.
   * @param      $err_message Error message
   * @return     DB instance (false is returned when Error)
   */
  function cas_db_connect($dsn, $query_timeout, &$err_message) {

    /* set option of MDB2::connect */
    $options = array(
      'debug' => 2,  'result_buffering' => false,
    );

    /* connect */
    $db_ref =& MDB2::connect($dsn, $options);
    if (PEAR::isError($db_ref)) {
      $err_message = $db_ref->getMessage();
      return false;
    }

    /* get db type string */
    $arr = explode(':', $dsn);
    $db_type = $arr[0];

    /* It seems that MySQL does not support query timeout */
    /* http://stackoverflow.com/questions/2137084/setup-mysql-query-timeout */

    /* set query timeout */
    if ( 0 < strlen($query_timeout) ) {
      if ( $db_type == "pgsql" ) {
        $res =& $db_ref->exec("SET statement_timeout=" . $query_timeout);
        if (PEAR::isError($res)) {
          $err_message = $res->getMessage();
          $db_ref->disconnect();
          return false;
        }
      }
      else {
        fprintf(STDERR, "[WARNING] cas_db_connect(): " . 
                        "%s does not support query timeout.\n",$db_type);
      }
    }

    $db_ins = array( 'type' => $db_type,  'ins' => $db_ref );

    return $db_ins;
  }

  /**
   * Disconnect DB
   *
   * @param      $db_ins DB instance
   */
  function cas_db_disconnect($db_ins) {
    $db_ins['ins']->disconnect();
  }

  /**
   * Send query.
   *
   * @param      $db_ins DB instance
   * @param      $query_str Query string
   * @return     Result instance
   */
  function cas_db_query($db_ins, $query_str) {
    $res_ref =& $db_ins['ins']->query($query_str);
    $res_ins = array( 'db_type' => $db_ins['type'],  'result' => $res_ref );
    return $res_ins;
  }

  /**
   * Obtain column names of result table.
   *
   * @param      $res_ins Result instance
   * @return     Column names (array)
   */
  function cas_db_get_column_names($res_ins) {
    $id = $res_ins['result']->getResource();
    if ( $res_ins['db_type'] == 'pgsql' ) {
      $num_cols = @pg_num_fields($id);
      $columns = array();
      for ( $i=0 ; $i < $num_cols ; $i++ ) {
        $columns[$i] = @pg_field_name($id, $i);
      }
    }
    else if ( $res_ins['db_type'] == 'mysql' ) {
      $num_cols = @mysql_num_fields($id);
      $columns = array();
      for ( $i=0 ; $i < $num_cols ; $i++ ) {
        $columns[$i] = @mysql_field_name($id, $i);
      }
    }
    else {
      $columns = false;
      fprintf(STDERR, "[ERROR] cas_db_get_column_names() " . 
              "does not support %s.\n", $res_ins['db_type']);
    }
    return $columns;
  }

  /**
   * Obtain 1 row from result instance.
   *
   * @param      $res_ins Result instance
   * @return     Cell contents (array)
   */
  function cas_db_fetch_row($res_ins) {
    return $res_ins['result']->fetchRow();
  }


  /***************************************************************************/
  /** M A I N ****************************************************************/
  /***************************************************************************/

  if ( $argc < 2 ) {
    fprintf(STDERR,"Specify an SQL statement on 1st arg.\n");
    exit(1);
  }

  /* Connect to DB */
  $db_ins = cas_db_connect($Dsn, $Query_timeout, $s);
  if ( $db_ins === false ) {
    fprintf(STDERR,"%s\n",$s);
    exit(1);
  }

  /* Send an SQL Statement */
  $res_ins = cas_db_query($db_ins, $argv[1]);

  /* Obtain Raw Column Names */
  $cols = cas_db_get_column_names($res_ins);

  /* Display Column Names */
  for ( $i=0 ; $i < count($cols) ; $i++ ) {
    if ( $i == 0 ) printf("%s",$cols[$i]);
    else printf(",%s",$cols[$i]);
  }
  printf("\n");

  /* Display All Rows */
  while ($row = cas_db_fetch_row($res_ins)) {
    for ( $i=0 ; $i < count($row) ; $i++ ) {
      if ( $i == 0 ) printf("%s",$row[$i]);
      else printf(",%s",$row[$i]);
    }
    printf("\n");
  }

  /* Disconnect */
  cas_db_disconnect($db_ins);

?>
