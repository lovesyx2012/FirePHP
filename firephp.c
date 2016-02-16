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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "ext/standard/head.h"
#include "php_firephp.h"

/* If you declare any globals in php_firephp.h uncomment this:*/
ZEND_DECLARE_MODULE_GLOBALS(firephp)

/* True global resources - no need for thread safety here */
static int le_firephp;
static zend_class_entry *firephp_ce;  //申明类

ZEND_BEGIN_ARG_INFO(record_msg_arginfo, 0)
    ZEND_ARG_INFO(0, arg)
    ZEND_ARG_INFO(0, lable)
ZEND_END_ARG_INFO()

/* {{{ firephp_functions[]
 *
 * Every user visible function must have an entry in firephp_functions[].
 */
const zend_function_entry firephp_methods[] = {
        PHP_ME(FirePHP, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) /*构造函数申明*/    
        PHP_ME(FirePHP, __destruct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR) /*析构函数申明*/
        PHP_ME(FirePHP, record, record_msg_arginfo , ZEND_ACC_PUBLIC) /*打印函数申明*/
        PHP_ME(FirePHP, getInstance, NULL , ZEND_ACC_PUBLIC | ZEND_ACC_STATIC) /*获取实例函数申明*/
        { NULL, NULL, NULL }
};
/* }}} */

/** {{{ zend_bool firephp_detect_client(TSRMLS_D)
*/
zend_bool firephp_detect_client(TSRMLS_D)
{
	zval *http_user_agent;
	http_user_agent = firephp_request_query(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT") TSRMLS_CC);
	if (strstr(Z_STRVAL_P(http_user_agent), FIREPHP_HTTP_USER_AGENT_NAME) != NULL) {
		return true;
	} else {
		return false;
	}
}
/* }}} */

/** {{{ zend_bool firephp_check_headers_send(TSRMLS_D)
*/
zend_bool firephp_check_headers_send(TSRMLS_D)
{
	const char *file = "";
	int line = 0;

	if (SG(headers_sent)) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
		line = php_output_get_start_lineno(TSRMLS_C);
		file = php_output_get_start_filename(TSRMLS_C);
#else
		line = php_get_output_start_lineno(TSRMLS_C);
		file = php_get_output_start_filename(TSRMLS_C);
#endif
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "You Have Already Outputted In File:%s, Line:%d", file, line);
		return false;
	} else {
		return true;
	}
}
/* }}} */


zval * firephp_request_query(uint type, char * name, uint len TSRMLS_DC) {
	zval 		**carrier = NULL, **ret;

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	zend_bool 	jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	zend_bool 	jit_initialization = PG(auto_globals_jit);
#endif

	switch (type) {
		case TRACK_VARS_POST:
		case TRACK_VARS_GET:
		case TRACK_VARS_FILES:
		case TRACK_VARS_COOKIE:
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_ENV") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
			}
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}

	if (!carrier || !(*carrier)) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	if (!len) {
		Z_ADDREF_P(*carrier);
		return *carrier;
	}

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, len + 1, (void **)&ret) == FAILURE) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	Z_ADDREF_P(*ret);
	return *ret;
}
/* }}} */

/** {{{ void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC)
*/
void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC)
{
	zval **arg, *element = NULL;
	HashTable *format_ht;

	MAKE_STD_ZVAL(*carrier);
	array_init(*carrier);
	format_ht = Z_ARRVAL_P(format_array);
	do {
		if (zend_hash_num_elements(format_ht) <= 0) break;
		int key_type;
		char *key;
		uint keylen;
		ulong idx;
		zend_hash_internal_pointer_reset(format_ht);
		for (; zend_hash_has_more_elements(format_ht) == SUCCESS; zend_hash_move_forward(format_ht)) {
			key_type = zend_hash_get_current_key_ex(format_ht, &key, &keylen, &idx, 1, NULL);
			if (zend_hash_get_current_data(format_ht, (void **)&arg) == FAILURE) continue;
			if (key_type == HASH_KEY_IS_STRING) {
				if (strcmp(key, "GLOBALS") == 0 && Z_TYPE_PP(arg) == IS_ARRAY
						&& zend_hash_exists(Z_ARRVAL_PP(arg), "GLOBALS", sizeof("GLOBALS"))) {
					add_assoc_string(*carrier, key, "** Recursion (GLOBALS) **", 1);
				} else {
					element = firephp_encode_data(*arg TSRMLS_CC);
					if (Z_TYPE_P(element) == IS_ARRAY) {
						add_assoc_zval(*carrier, key, element);
					} else if (Z_TYPE_P(element) == IS_STRING){
						add_assoc_string(*carrier, key, Z_STRVAL_P(element), 1);
					}
				}
			} else if (key_type == HASH_KEY_IS_LONG) {
				element = firephp_encode_data(*arg TSRMLS_CC);
				if (Z_TYPE_P(element) == IS_ARRAY) {
					add_index_zval(*carrier, idx, element);
				} else if (Z_TYPE_P(element) == IS_STRING){
					add_index_string(*carrier, idx, Z_STRVAL_P(element), 1);
				}
			}
		}
	} while(0);
}
/* }}} */

/** {{{ void firephp_encode_object(zval *format_object, zval **carrier TSRMLS_DC)
*/
void firephp_encode_object(zval *format_object, zval **carrier TSRMLS_DC)
{
	char *class_name, *carrier_key, *recursion_str, *none_value;
	zend_property_info *property_info;
	zval *prop_value, *element;
	zend_object_handle current_obj_h;
	ulong current_num_index;

	MAKE_STD_ZVAL(*carrier);
	do {
		zend_class_entry *ce = Z_OBJCE_P(format_object);
		class_name = (char *)ce->name;
		current_obj_h = Z_OBJ_HANDLE_P(format_object);
		current_num_index = (ulong) current_obj_h;
		if (zend_hash_index_exists(FIREPHP_G(object_handle_ht), current_num_index)) {
			spprintf(&recursion_str, 0, "** Recursion (%s) **", class_name);
			ZVAL_STRING(*carrier, recursion_str, 1);
			efree(recursion_str);
			break;
		} else {
			zend_hash_index_update(FIREPHP_G(object_handle_ht), current_num_index, &current_obj_h, sizeof(zend_object_handle), NULL);
		}
		array_init(*carrier);
		add_assoc_string(*carrier, "__className", class_name, 1);

		HashTable *properties_info = &ce->properties_info;
		if (zend_hash_num_elements(properties_info) <= 0) break;
		int key_type;
		char *key;
		uint keylen;
		ulong idx;
		zend_hash_internal_pointer_reset(properties_info);
		for (; zend_hash_has_more_elements(properties_info) == SUCCESS;
			zend_hash_move_forward(properties_info)) {
			key_type = zend_hash_get_current_key_ex(properties_info, &key, &keylen, &idx, 1, NULL);
			if (zend_hash_get_current_data(properties_info, (void **)&property_info) == FAILURE) continue;
			if (key_type == HASH_KEY_IS_STRING) {
				if (property_info->flags & ZEND_ACC_STATIC) {
					spprintf(&carrier_key, 0, "static:%s", key);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
					zend_update_class_constants(ce TSRMLS_CC);
					if (CE_STATIC_MEMBERS(ce)[property_info->offset] && (property_info->flags & ZEND_ACC_PUBLIC)) {
						*prop_value = *CE_STATIC_MEMBERS(ce)[property_info->offset];
					} else {
						spprintf(&none_value, 0, "** You Can't Get %s's Value **", carrier_key);
						add_assoc_string(*carrier, carrier_key, none_value, 1);
						efree(none_value);
						continue;
					}
#else
					zval **member;
					zend_hash_find(ce->static_members, key, strlen(key) + 1, (void **) &member);
					prop_value = *member;
#endif
				} else if (property_info->flags & ZEND_ACC_PUBLIC) {
					spprintf(&carrier_key, 0, "public:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				} else if (property_info->flags & ZEND_ACC_PROTECTED) {
					spprintf(&carrier_key, 0, "protected:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				} else if (property_info->flags & ZEND_ACC_PRIVATE) {
					spprintf(&carrier_key, 0, "private:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				}
				if (prop_value == NULL) continue;

				element = firephp_encode_data(prop_value TSRMLS_CC);
				if (Z_TYPE_P(element) == IS_ARRAY) {
					add_assoc_zval(*carrier, carrier_key, element);
				} else if (Z_TYPE_P(element) == IS_STRING) {
					add_assoc_string(*carrier, carrier_key, Z_STRVAL_P(element), 1);
				}
			}
		}
	} while(0);
}
/* }}} */

/** {{{ zval * firephp_encode_data(zval *format_data TSRMLS_DC)
*/
zval * firephp_encode_data(zval *format_data TSRMLS_DC)
{
	zval *carrier, z_tmp;
	char *c_tmp;

	do {
		switch (Z_TYPE_P(format_data)) {
			case IS_BOOL:{
				MAKE_STD_ZVAL(carrier);
				if (!Z_BVAL_P(format_data)) {
					ZVAL_STRING(carrier, "false", 1);
				} else {
					ZVAL_STRING(carrier, "true", 1);
				}
				break;
			}
			case IS_LONG:
			case IS_DOUBLE:{
				MAKE_STD_ZVAL(carrier);
				z_tmp = *format_data;
				zval_copy_ctor(&z_tmp);
				INIT_PZVAL(&z_tmp);
				convert_to_string(&z_tmp);
				ZVAL_STRING(carrier, Z_STRVAL(z_tmp), 1);
				zval_dtor(&z_tmp);
				break;
			}
			case IS_NULL: {
				MAKE_STD_ZVAL(carrier);
				ZVAL_STRING(carrier, "NULL", 1);
				break;
			}
			case IS_RESOURCE: {
				MAKE_STD_ZVAL(carrier);
				spprintf(&c_tmp, 0, "** Resource ID #%ld **", Z_RESVAL_P(format_data));
				ZVAL_STRING(carrier, c_tmp, 1);
				efree(c_tmp);
				break;
			}
			case IS_STRING: {
				MAKE_STD_ZVAL(carrier);
				if (Z_STRLEN_P(format_data) == 0) {
					ZVAL_STRING(carrier, "** Empty String **", 1);
				} else {
					ZVAL_STRING(carrier, Z_STRVAL_P(format_data), 1);
				}
				break;
			}
			case IS_ARRAY: {
				firephp_encode_array(format_data, &carrier TSRMLS_CC);
				break;
			}
			case IS_OBJECT: {
				firephp_encode_object(format_data, &carrier TSRMLS_CC);
				break;
			}
		}
	} while(0);

	return carrier;
}
/* }}} */

/** {{{ void firephp_set_header(char *header TSRMLS_DC)
*/
void firephp_set_header(char *header TSRMLS_DC)
{
	sapi_header_line ctr = {0};
	ctr.line = header;
	ctr.line_len = strlen(header);
	sapi_header_op(SAPI_HEADER_REPLACE, &ctr TSRMLS_CC);
}
/* }}} */

/** {{{ void firephp_init_object_handle_ht(TSRMLS_D)
*/
void firephp_init_object_handle_ht(TSRMLS_D)
{
    ALLOC_HASHTABLE(FIREPHP_G(object_handle_ht));
    if (zend_hash_init(FIREPHP_G(object_handle_ht), 0, NULL, NULL, 0) == FAILURE) {
    	FREE_HASHTABLE(FIREPHP_G(object_handle_ht));
    }
}
/* }}} */

/** {{{ void firephp_clean_object_handle_ht(TSRMLS_D)
*/
void firephp_clean_object_handle_ht(TSRMLS_D)
{
	zend_hash_destroy(FIREPHP_G(object_handle_ht));
	FREE_HASHTABLE(FIREPHP_G(object_handle_ht));
}
/* }}} */

/** {{{ zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC)
*/
zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC)
{
	zval *carrier;
	char *elem, *p, *q;
	size_t str_len = strlen(format_str);
	MAKE_STD_ZVAL(carrier);
	array_init(carrier);

	if (max_len <= 0 ) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Function firephp_split_str_to_array Second Parameter Must Be Greater Than 0");
		return carrier;
	}

	if (str_len <= max_len) {
		add_next_index_string(carrier, format_str, 1);
		return carrier;
	}

	elem = (char *)safe_emalloc(max_len + 1, sizeof(char), 0);
	q = elem;
	p = format_str;
	*q = *p;
	q++;
	p++;
	while (*p != '\0') {
		if ((p - format_str) % max_len == 0) {
			*q = '\0';
			add_next_index_string(carrier, elem, 1);
			q -= max_len;
		}
		*q = *p;
		q++;
		p++;
	}
	if (*(q-1)) {
		*q = '\0';
		add_next_index_string(carrier, elem, 1);
	}
	efree(elem);

	return carrier;
}
/* }}} */

/** {{{ char * firephp_json_encode(zval *data TSRMLS_DC)
*/
char * firephp_json_encode(zval *data TSRMLS_DC)
{
	zval *function, *ret = NULL, **params[1];
	char *json_data = "";

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, "json_encode", 0);
	params[0] = &data;
	zend_fcall_info fci = {
		sizeof(fci),
		EG(function_table),
		function,
		NULL,
		&ret,
		1,
		(zval ***)params,
		NULL,
		1
	};

	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
	} else {
		json_data = Z_STRVAL_P(ret);
	}

	return json_data;
}

double firephp_microtime(TSRMLS_D)
{
	zval *function, *ret = NULL, **params[1], *param;
	double microtime;

	MAKE_STD_ZVAL(function);
	MAKE_STD_ZVAL(param);
	ZVAL_TRUE(param);
	params[0] = &param;
	ZVAL_STRING(function, "microtime", 0);

	zend_fcall_info fci = {
		sizeof(fci),
		EG(function_table),
		function,
		NULL,
		&ret,
		1,
		(zval ***)params,
		NULL,
		1
	};

	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
	} else {
		microtime = Z_DVAL_P(ret);
	}
	efree(param);

	return microtime;
}
/* }}} */

/** {{{ void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC)
*/
void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC)
{
	zval **elem;
	HashTable *headers;
	int header_total_num, i;
	char *header;

	headers = Z_ARRVAL_P(data);
	if ((header_total_num = zend_hash_num_elements(headers)) <= 0) return;
	zend_hash_internal_pointer_reset(headers);
	for(i = 0; zend_hash_has_more_elements(headers) == SUCCESS; zend_hash_move_forward(headers), i++) {
		if (zend_hash_get_current_data(headers, (void **)&elem) == FAILURE) continue;
		if (header_total_num >= 2) {
			if (i == 0) {
				spprintf(&header, 0, "X-Wf-1-1-1-%d: %d|%s|%s", FIREPHP_G(message_index),
						header_total_len, Z_STRVAL_PP(elem), "\\");
			} else {
				spprintf(&header, 0, "X-Wf-1-1-1-%d: |%s|%s", FIREPHP_G(message_index)
										 , Z_STRVAL_PP(elem), (i < (header_total_num - 1)) ? "\\" : "");
			}
		}else {
			spprintf(&header, 0, "X-Wf-1-1-1-%d: %d|%s|", FIREPHP_G(message_index), Z_STRLEN_PP(elem), Z_STRVAL_PP(elem));
		}
		firephp_set_header(header TSRMLS_CC);
		FIREPHP_G(message_index)++;
		efree(header);
	}

	spprintf(&header, 0, "X-Wf-1-Index: %d", FIREPHP_G(message_index) - 1);
	firephp_set_header(FIREPHP_PLUGIN_HEADER TSRMLS_CC);
	firephp_set_header(FIREBUG_CONSOLE_HEADER TSRMLS_CC);
	firephp_set_header(FIREPHP_PROTOCOL_JSONSTREAM TSRMLS_CC);
	firephp_set_header(FIREPHP_PROTOCOL_JSONSTREAM TSRMLS_CC);
	firephp_set_header(header TSRMLS_CC);
	efree(header);
}
/* }}} */



/***************BEGIN FirePHP****************/
static PHP_METHOD(FirePHP, __construct) {
   //php_printf("call __construct function...\n");
    RETURN_TRUE;
}

static PHP_METHOD(FirePHP,  __destruct) {
    //php_printf("call __destruct function...\n");
    RETURN_TRUE;
}

static PHP_METHOD(FirePHP,  getInstance) {
  //  php_printf("call getInstance function...\n");
    RETURN_TRUE;
}

static PHP_METHOD(FirePHP,  record) {
/*
    char *msg;
    int msg_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &msg, &msg_len) == FAILURE) {
        return;
    }
    double microtime, pass_time = 0;
    microtime = firephp_microtime(TSRMLS_C);
    char *lable_str;
    spprintf(&lable_str, 0, "%09.5fs", microtime);
    char *strg;
    
    int len = spprintf(&strg, 0, "Output:: %s \n", msg);
    php_printf(strg);
    php_printf(lable_str);
    efree(lable_str);
    efree(strg);
    efree(msg);
    RETURN_TRUE;*/
    zval *arg, *format_data, *format_meta, *split_array;
	char *format_str, *format_data_json, *format_meta_json, *lable_str;
	int header_total_len;
	double microtime, pass_time = 0;
	zend_bool lable = false;

	firephp_init_object_handle_ht(TSRMLS_C);
	do {
		if (!firephp_detect_client(TSRMLS_C)) break;
		if (!firephp_check_headers_send(TSRMLS_C)) break;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &arg, &lable) == FAILURE) break;

		microtime = firephp_microtime(TSRMLS_C);
		if (lable) {
			if (FIREPHP_G(microtime) > 0) {
				pass_time = microtime - FIREPHP_G(microtime);
				spprintf(&lable_str, 0, "%09.5fs", pass_time);
			} else {
				spprintf(&lable_str, 0, "%09.5fs", microtime);
			}
		}
		FIREPHP_G(microtime) = microtime;

		MAKE_STD_ZVAL(format_meta);
		array_init(format_meta);
		add_assoc_string(format_meta, "Type", "INFO", 1);
		if (lable) {
			add_assoc_string(format_meta, "Label", lable_str, 1);
			efree(lable_str);
		}
		format_data = firephp_encode_data(arg TSRMLS_CC);
		format_data_json = firephp_json_encode(format_data TSRMLS_CC);
		format_meta_json = firephp_json_encode(format_meta TSRMLS_CC);
		spprintf(&format_str, 0, "[%s,%s]", format_meta_json, format_data_json);
		efree(format_meta_json);
		efree(format_data_json);
		zval_ptr_dtor(&format_meta);
		zval_ptr_dtor(&format_data);
		split_array = firephp_split_str_to_array(format_str, FIREPHP_HEADER_MAX_LEN TSRMLS_CC);
		header_total_len = strlen(format_str);


		efree(format_str);
		firephp_output_headers(split_array , header_total_len TSRMLS_CC);
		zval_ptr_dtor(&split_array);
		firephp_clean_object_handle_ht(TSRMLS_C);
		RETURN_TRUE;
	} while(0);
	firephp_clean_object_handle_ht(TSRMLS_C);
	RETURN_FALSE;
}
/***************END FirePHP****************/

/* {{{ firephp_module_entry
 */
zend_module_entry firephp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"firephp",
	NULL,
	PHP_MINIT(firephp),
	PHP_MSHUTDOWN(firephp),
	PHP_RINIT(firephp),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(firephp),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(firephp),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_FIREPHP_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FIREPHP
ZEND_GET_MODULE(firephp)
#endif



/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("firephp.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_firephp_globals, firephp_globals)
    STD_PHP_INI_ENTRY("firephp.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_firephp_globals, firephp_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_firephp_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_firephp_init_globals(zend_firephp_globals *firephp_globals)
{
	firephp_globals->global_value = 0;
	firephp_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(firephp)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
        /*定义一个temp class*/
        zend_class_entry ce;
        /*初始化这个class，第二个参数是class name, 第三个参数是class methods*/
        INIT_CLASS_ENTRY(ce, "FirePHP", firephp_methods);
        /*注册这个class到zend engine*/
        /*This function also returns the final class entry, so it can be stored in the global variable declared above*/
        firephp_ce = zend_register_internal_class(&ce TSRMLS_CC);
        zend_declare_class_constant_stringl(firephp_ce, "LOG", strlen("LOG"), "LOG", strlen("LOG") TSRMLS_CC);
        //zend_declare_class_constant_string(firephp_ce, "LOG", size_t name_length, const char *value TSRMLS_DC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(firephp)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(firephp)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(firephp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(firephp)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "firephp support", "enabled");
        php_info_print_table_row(2, "Version", FIREPHP_VERSION);
        php_info_print_table_row(2, "Support", FIREPHP_SUPPORT_URL);
	php_info_print_table_end();
	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_firephp_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_firephp_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "firephp", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
