/*
  +----------------------------------------------------------------------+
  | simdjson_php                                                         |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.0 of the Apache license,    |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.apache.org/licenses/LICENSE-2.0.html                      |
  +----------------------------------------------------------------------+
  | Author: Jinxi Wang  <1054636713@qq.com>                              |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_SIMDJSON_H
#define PHP_SIMDJSON_H

#define SIMDJSON_PHP_ERR_SUCCESS 0
#define SIMDJSON_PHP_ERR_INVALID_PHP_PROPERTY 255

/*
 * Put all of the publicly visible functionality and macros into the same header file
 * (On windows, the include paths used by the c compiler may be different)
 */
#include "Zend/zend.h"
#include "Zend/zend_portability.h"

/*
 * All code in this header file should be changed to go within BEGIN_EXTERN_C/END_EXTERN_C macros
 * (both header definitions, and C++ declarations, including function implementations),
 * so that pecls written in C can use this functionality without separate C++ files to load bindings.h.
 *
 * This header file deliberately does not depend on other header files in this project,
 * to make including this header file easier for other PECLs (avoid include path issues)
 *
 * BEGIN_EXTERN_C is needed for symbols to be mangled using C rules instead of C++ rules in all includers.
 * (This macro can be used from both C and C++ source files)
 */
BEGIN_EXTERN_C()

extern zend_module_entry simdjson_module_entry;
#define phpext_simdjson_ptr &simdjson_module_entry

#define PHP_SIMDJSON_VERSION                  "5.0.0dev"
/**
 * PHP_SIMDJSON_VERSION_ID has the same format as PHP_VERSION_ID: Major version * 10000 + Minor version * 100 + Patch version.
 * This is meant for use by PECL extensions that depend on simdjson.
 * (e.g. 4.5.6dev and 4.5.6 would be 40506)
 */
#define PHP_SIMDJSON_VERSION_ID               50000

#define SIMDJSON_SUPPORT_URL                  "https://github.com/JakubOnderka/simdjson_php"

#define SIMDJSON_PARSE_DEFAULT_DEPTH          512
/**
 * Number of strings in array of array or object keys that will be deduplicated
 */
#define SIMDJSON_DEDUP_STRING_COUNT           256
/**
 * Maximum length of strings to be considered for deduplication.
 * Longer strings are less likely to be duplicated and the memory overhead
 * of storing them in the hash table might exceed the benefits.
 */
#define SIMDJSON_MAX_DEDUP_LENGTH             64
/**
* Configure simdjson library
*/
#define SIMDJSON_EXCEPTIONS 0

#ifdef ZEND_DEBUG
#define SIMDJSON_DEVELOPMENT_CHECKS 1 // enable development checks when using PHP debug build
#else
#define SIMDJSON_DEVELOPMENT_CHECKS 0
#endif

/*
 * NOTE: Namespaces and references(&) are C++ only functionality.
 * To expose this functionality to other C PECLs,
 * switch to a forward struct declaration of a struct that only wraps simdjson::dom::parser
 */
struct simdjson_php_parser;

ZEND_BEGIN_MODULE_GLOBALS(simdjson)
    /*
     * php::simdjson::parser pointer, constructed on first use with request-scope lifetime.
     * Note that in ZTS builds, the thread for each request will deliberately have different instances for each concurrently running request.
     * (The simdjson library is not thread safe)
     *
     * This is similar to php-src's ext/session session data storage.
     */
    struct simdjson_php_parser *parser;
ZEND_END_MODULE_GLOBALS(simdjson)

PHP_MINIT_FUNCTION(simdjson);
PHP_MSHUTDOWN_FUNCTION(simdjson);
PHP_RINIT_FUNCTION(simdjson);
PHP_RSHUTDOWN_FUNCTION(simdjson);
PHP_MINFO_FUNCTION(simdjson);

#ifdef ZTS
#ifdef COMPILE_DL_SIMDJSON
ZEND_TSRMLS_CACHE_EXTERN()
#endif
#endif

/* Only the functions and variables defined with PHP_SIMDJSON_API can be loaded by other PECLs */
#ifdef PHP_WIN32
#	define PHP_SIMDJSON_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SIMDJSON_API __attribute__ ((visibility("default")))
#else
#	define PHP_SIMDJSON_API /* nothing special */
#endif

extern PHP_SIMDJSON_API zend_class_entry *simdjson_decoder_exception_ce;
extern PHP_SIMDJSON_API zend_class_entry *simdjson_encoder_exception_ce;

extern zend_string *simdjson_json_serialize;

/**
 * NOTE: Namespaces and references(&) and classes (instead of structs) are C++ only functionality.
 *
 * To expose this functionality to other C PECLs,
 * switch to a forward class declaration of a class that only wraps simdjson::dom::parser
 */
typedef uint8_t simdjson_php_error_code;

/* NOTE: Callers should check if len is greater than 4GB - simdjson will always return a non zero error code for those */

/**
 * Returns the error message corresponding to a given error code returned by a call to simdjson_php.
 */
PHP_SIMDJSON_API const char* php_simdjson_error_msg(simdjson_php_error_code code);
/**
 * Throw a SimdJsonException with the provided error code and the corresponding error message.
 */
PHP_SIMDJSON_API void php_simdjson_throw_jsonexception(simdjson_php_error_code code);
/**
 * Create a brand new parser instance.
 *
 * The caller must free it with php_simdjson_free_parser once it is no longer used.
 *
 * Callers may use this instead of the shared singleton parser when memory usage is a concern
 * (e.g. the PECLs are likely to be used load a string that's megabytes long in a long-lived php process)
 *
 * Callers should free this parser before or during the request shutdown phase.
 */
PHP_SIMDJSON_API struct simdjson_php_parser* php_simdjson_create_parser(void);
/**
 * Release a parser **constructed by php_simdjson_create_parser** and all associated buffers.
 */
PHP_SIMDJSON_API void php_simdjson_free_parser(struct simdjson_php_parser* parser);
/**
 * Returns 0 if the given json string is valid
 */
PHP_SIMDJSON_API simdjson_php_error_code php_simdjson_validate(struct simdjson_php_parser* parser, const zend_string *json, size_t depth);
/**
 * Parses the given string into a return code.
 *
 * If the returned error code is 0, then return_value contains the parsed value.
 * If the returned error code is non-0, then return_value will not be initialized.
 */
PHP_SIMDJSON_API simdjson_php_error_code php_simdjson_parse(struct simdjson_php_parser* parser, const zend_string *json, zval *return_value, bool associative, size_t depth);

END_EXTERN_C()

#endif
