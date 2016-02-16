/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_FIREPHP_H
#define PHP_FIREPHP_H

extern zend_module_entry firephp_module_entry;
#define phpext_firephp_ptr &firephp_module_entry

#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif


#define PHP_FIREPHP_VERSION "0.1.0" /* Replace with version number for your extension */
#define FIREPHP_HEADER_MAX_LEN			5000
#define FIREPHP_PLUGIN_HEADER			"X-Wf-1-Plugin-1: http://meta.firephp.org/Wildfire/Plugin/FirePHP/Library-FirePHPCore/0.2.1"
#define FIREBUG_CONSOLE_HEADER			"X-Wf-1-Structure-1: http://meta.firephp.org/Wildfire/Structure/FirePHP/FirebugConsole/0.1"
#define FIREPHP_PROTOCOL_JSONSTREAM		"X-Wf-Protocol-1: http://meta.wildfirehq.org/Protocol/JsonStream/0.2"
#define FIREPHP_HTTP_USER_AGENT_NAME	"FirePHP"

#ifdef PHP_WIN32
#	define PHP_FIREPHP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_FIREPHP_API __attribute__ ((visibility("default")))
#else
#	define PHP_FIREPHP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


#define FIREPHP_VERSION					"1.0.0"
#define FIREPHP_SUPPORT_URL				"http://www.curlme.net/"


#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION <= 2))
#define Z_SET_REFCOUNT_P(pz, rc)      (pz)->refcount = rc
#define Z_SET_REFCOUNT_PP(ppz, rc)    Z_SET_REFCOUNT_P(*(ppz), rc)
#define Z_ADDREF_P 	 ZVAL_ADDREF
#define Z_REFCOUNT_P ZVAL_REFCOUNT
#define Z_DELREF_P 	 ZVAL_DELREF
#endif

zval * firephp_request_query(uint type, char * name, uint len TSRMLS_DC);
zend_bool firephp_detect_client(TSRMLS_D);
zend_bool firephp_check_headers_send(TSRMLS_D);
void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC);
zval * firephp_encode_data(zval *format_data TSRMLS_DC);
zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC);
void firephp_set_header(char *header_str TSRMLS_DC);
char * firephp_json_encode(zval *data TSRMLS_DC);
void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC);
void firephp_init_object_handle_ht(TSRMLS_D);
void firephp_clean_object_handle_ht(TSRMLS_D);


PHP_MINIT_FUNCTION(firephp);
PHP_MSHUTDOWN_FUNCTION(firephp);
PHP_RINIT_FUNCTION(firephp);
PHP_RSHUTDOWN_FUNCTION(firephp);
PHP_MINFO_FUNCTION(firephp);

static PHP_METHOD(FirePHP, __construct);
static PHP_METHOD(FirePHP, __destruct);
static PHP_METHOD(FirePHP, record);
static PHP_METHOD(FirePHP, getInstance);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/
ZEND_BEGIN_MODULE_GLOBALS(firephp)
   HashTable *object_handle_ht;
   int message_index;
   double microtime;
ZEND_END_MODULE_GLOBALS(firephp)

/* In every utility function you add that needs to use variables 
   in php_firephp_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as FIREPHP_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define FIREPHP_G(v) TSRMG(firephp_globals_id, zend_firephp_globals *, v)
#else
#define FIREPHP_G(v) (firephp_globals.v)
#endif

#endif	/* PHP_FIREPHP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
