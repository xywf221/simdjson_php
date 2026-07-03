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

extern "C" {
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "zend_smart_str.h"
#include "ext/standard/info.h"
#include "ext/spl/spl_exceptions.h"

#include "php_simdjson.h"

/**
 * Both the declaration and the definition of PHP_SIMDJSON_API variables, functions must be within an 'extern "C"' block for Windows
 */
PHP_SIMDJSON_API zend_class_entry *simdjson_decoder_exception_ce;
PHP_SIMDJSON_API zend_class_entry *simdjson_encoder_exception_ce;

} /* end extern "C" */

/* C++ header file for simdjson_php helper methods/classes */
#include "src/simdjson_compatibility.h"
#include "src/simdjson_integer_writer.h"
#include "src/simdjson_smart_str.h"
#include "src/simdjson_decoder_defs.h"
#include "src/simdjson_encoder.h"
/* Single header file from fork of simdjson C project (to imitate php's handling of infinity/overflowing integers in json_decode) */
#include "src/simdjson.h"

#include "simdjson_arginfo.h"

static zend_string *simdjson_json_empty_array;
zend_string *simdjson_json_serialize;

ZEND_DECLARE_MODULE_GLOBALS(simdjson);

#define SIMDJSON_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(simdjson, v)

#define SIMDJSON_CAPACITY_RECLAIM_THRESHOLD 1024 * 1024 * 1 // 1MB
#define SIMDJSON_SHOULD_REUSE_PARSER(zstr_len) EXPECTED(zstr_len <= SIMDJSON_CAPACITY_RECLAIM_THRESHOLD)

// Get parser that can be reused multiple times to avoid allocation and deallocation
static simdjson_php_parser *simdjson_get_reused_parser() {
    simdjson_php_parser *parser = SIMDJSON_G(parser);
    if (parser == NULL) {
        parser = php_simdjson_create_parser();
        SIMDJSON_G(parser) = parser;
        ZEND_ASSERT(parser != NULL);
    }
    return parser;
}

// The simdjson parser accepts strings with at most 32-bit lengths, for now.
#define SIMDJSON_MAX_DEPTH ((zend_long)((SIZE_MAX / 8) < (UINT32_MAX / 2) ? (SIZE_MAX / 8) : (UINT32_MAX / 2)))

static zend_always_inline bool simdjson_validate_depth(zend_long depth, const int arg_num) {
    if (UNEXPECTED(depth <= 0)) {
        zend_argument_value_error(arg_num, "must be greater than zero");
        return false;
    } else if (UNEXPECTED(depth > SIMDJSON_MAX_DEPTH)) {
        zend_argument_value_error(arg_num, "exceeds maximum allowed value of " ZEND_LONG_FMT, SIMDJSON_MAX_DEPTH);
        return false;
    }
    return true;
}

PHP_FUNCTION(simdjson_validate) {
    zend_string *json = NULL;
    zend_long depth = SIMDJSON_PARSE_DEFAULT_DEPTH;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(json)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(depth)
    ZEND_PARSE_PARAMETERS_END();

    if (!simdjson_validate_depth(depth, 2)) {
        RETURN_THROWS();
    }

    simdjson_php_error_code error;
    if (SIMDJSON_SHOULD_REUSE_PARSER(ZSTR_LEN(json))) {
        error = php_simdjson_validate(simdjson_get_reused_parser(), json, depth);
    } else {
        simdjson_php_parser *simdjson_php_parser = php_simdjson_create_parser();
        error = php_simdjson_validate(simdjson_php_parser, json, depth);
        php_simdjson_free_parser(simdjson_php_parser);
    }

    ZVAL_BOOL(return_value, !error);
}

// Decode simple and common JSON values without allocating and using simdjson parser
static zend_always_inline bool simdjson_simple_decode(const char *json, size_t len, zval *return_value, bool associative) {
    // Empty object
    if (len == 2 && json[0] == '{' && json[1] == '}') {
        if (associative) {
            RETVAL_EMPTY_ARRAY();
        } else {
            object_init(return_value);
        }
        return true;
    }

    // Empty array
    if (len == 2 && json[0] == '[' && json[1] == ']') {
        RETVAL_EMPTY_ARRAY();
        return true;
    }

    if (len == 4 && memcmp(json, "true", 4) == 0) {
        RETVAL_TRUE;
        return true;
    } else if (len == 5 && memcmp(json, "false", 5) == 0) {
        RETVAL_FALSE;
        return true;
    }
    return false;
}

PHP_FUNCTION(simdjson_decode) {
    zend_bool associative = 0;
    zend_long depth = SIMDJSON_PARSE_DEFAULT_DEPTH;
    zend_string *json = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(json)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(associative)
        Z_PARAM_LONG(depth)
    ZEND_PARSE_PARAMETERS_END();

    if (!simdjson_validate_depth(depth, 3)) {
        RETURN_THROWS();
    }

    if (simdjson_simple_decode(ZSTR_VAL(json), ZSTR_LEN(json), return_value, associative)) {
        return;
    }

    simdjson_php_error_code error;
    if (SIMDJSON_SHOULD_REUSE_PARSER(ZSTR_LEN(json))) {
        error = php_simdjson_parse(simdjson_get_reused_parser(), json, return_value, associative, depth);
    } else {
        simdjson_php_parser *simdjson_php_parser = php_simdjson_create_parser();
        error = php_simdjson_parse(simdjson_php_parser, json, return_value, associative, depth);
        php_simdjson_free_parser(simdjson_php_parser);
    }

    if (UNEXPECTED(error)) {
        php_simdjson_throw_jsonexception(error);
        RETURN_THROWS();
    }
}

static const char *simdjson_get_error_msg(simdjson_encoder_error_code error_code) {
    switch(error_code) {
        case SIMDJSON_ERROR_NONE:
            return "No error";
        case SIMDJSON_ERROR_DEPTH:
            return "Maximum stack depth exceeded";
        case SIMDJSON_ERROR_STATE_MISMATCH:
            return "State mismatch (invalid or malformed JSON)";
        case SIMDJSON_ERROR_CTRL_CHAR:
            return "Control character error, possibly incorrectly encoded";
        case SIMDJSON_ERROR_SYNTAX:
            return "Syntax error";
        case SIMDJSON_ERROR_UTF8:
            return "Malformed UTF-8 characters, possibly incorrectly encoded";
        case SIMDJSON_ERROR_RECURSION:
            return "Recursion detected";
        case SIMDJSON_ERROR_INF_OR_NAN:
            return "Inf and NaN cannot be JSON encoded";
        case SIMDJSON_ERROR_UNSUPPORTED_TYPE:
            return "Type is not supported";
        case SIMDJSON_ERROR_INVALID_PROPERTY_NAME:
            return "The decoded property name is invalid";
        case SIMDJSON_ERROR_UTF16:
            return "Single unpaired UTF-16 surrogate in unicode escape";
        case SIMDJSON_ERROR_NON_BACKED_ENUM:
            return "Non-backed enums have no default serialization";
        default:
            return "Unknown error";
    }
}

static zend_always_inline bool simdjson_validate_encode_depth(const zend_long depth, const int arg_num) {
    if (UNEXPECTED(depth <= 0)) {
        zend_argument_value_error(arg_num, "must be greater than 0");
        return false;
    }

    if (UNEXPECTED(depth > INT_MAX)) {
        zend_argument_value_error(arg_num, "must be less than %d", INT_MAX);
        return false;
    }

    return true;
}

#if PHP_VERSION_ID >= 80200
static zend_string* simdjson_encode_long(zend_long value) {
    zend_string* str = zend_string_alloc(strlen("-9223372036854775807"), 0);
    char *end = simdjson_write_int_jeaiii(ZSTR_VAL(str), value);
    ZSTR_LEN(str) = end - ZSTR_VAL(str);
    ZSTR_VAL(str)[ZSTR_LEN(str)] = '\0';
    return str;
}

/** For simple types we can just return direct interned string without allocating new strings or for integers we can write to fixed size buffer */
static zend_always_inline bool simdjson_encode_simple(const zval *parameter, zval *return_value) {
    switch (Z_TYPE_P(parameter)) {
        case IS_NULL:
            RETVAL_INTERNED_STR(ZSTR_KNOWN(ZEND_STR_NULL_LOWERCASE));
            return true;

        case IS_TRUE:
            RETVAL_INTERNED_STR(ZSTR_KNOWN(ZEND_STR_TRUE));
            return true;

        case IS_FALSE:
            RETVAL_INTERNED_STR(ZSTR_KNOWN(ZEND_STR_FALSE));
            return true;

        case IS_LONG:
            if (Z_LVAL_P(parameter) >= 0 && Z_LVAL_P(parameter) < 10) {
                RETVAL_INTERNED_STR(ZSTR_CHAR((unsigned char) '0' + Z_LVAL_P(parameter)));
                return true;
            }
            RETVAL_NEW_STR(simdjson_encode_long(Z_LVAL_P(parameter)));
            return true;

        case IS_ARRAY:
            if (zend_hash_num_elements(Z_ARRVAL_P(parameter)) == 0) {
                RETVAL_INTERNED_STR(simdjson_json_empty_array);
                return true;
            }
            break;
    }

    return false;
}
#endif // PHP_VERSION_ID >= 80200

PHP_FUNCTION(simdjson_encode) {
    zval *parameter;
    simdjson_encoder encoder = {0};
    smart_str buf = {0};
    zend_long options = 0;
    zend_long depth = 512;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_ZVAL(parameter)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(options)
        Z_PARAM_LONG(depth)
    ZEND_PARSE_PARAMETERS_END();

    if (!simdjson_validate_encode_depth(depth, 3)) {
        RETURN_THROWS();
    }

#if PHP_VERSION_ID >= 80200
    if (!(options & SIMDJSON_APPEND_NEWLINE) && simdjson_encode_simple(parameter, return_value)) {
        return;
    }
#endif

    encoder.max_depth = (int)depth;
    encoder.options = (int)options;
    // Allocate output buffer to smallest size, so we remove checks if buffer was allocated in simdjson_encode_zval method
    simdjson_smart_str_init(&buf);
    simdjson_encode_zval(&buf, parameter, &encoder);

    if (UNEXPECTED(encoder.error_code != SIMDJSON_ERROR_NONE)) {
        efree(buf.s);
        zend_throw_exception(simdjson_encoder_exception_ce, simdjson_get_error_msg(encoder.error_code), encoder.error_code);
        RETURN_THROWS();
    }

    if (options & SIMDJSON_APPEND_NEWLINE) {
        simdjson_smart_str_appendc(&buf, '\n');
    }

    RETURN_NEW_STR(simdjson_smart_str_extract(&buf));
}

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION (simdjson) {
#if defined(COMPILE_DL_SIMDJSON) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
}
/* }}} */

#define SIMDJSON_NEW_INTERNED_STRING(_dest, _string) do { \
    _dest = zend_string_init_interned(_string, strlen(_string), 1); \
    GC_ADD_FLAGS(_dest, IS_STR_VALID_UTF8); \
} while(0); \

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION (simdjson) {
#if PHP_VERSION_ID >= 80200
    SIMDJSON_NEW_INTERNED_STRING(simdjson_json_empty_array, "[]");
#endif
    SIMDJSON_NEW_INTERNED_STRING(simdjson_json_serialize, "jsonserialize");

	simdjson_decoder_exception_ce = spl_ce_RuntimeException;
	simdjson_encoder_exception_ce = spl_ce_RuntimeException;

    register_simdjson_symbols(0);

    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION (simdjson) {
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION (simdjson) {
    SIMDJSON_G(parser) = NULL;
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION (simdjson) {
    simdjson_php_parser *parser = SIMDJSON_G(parser);
    if (parser != NULL) {
        php_simdjson_free_parser(parser);
        SIMDJSON_G(parser) = NULL;
    }
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION (simdjson) {
    php_info_print_table_start();

    php_info_print_table_row(2, "simdjson support", "enabled");
    php_info_print_table_row(2, "Version", PHP_SIMDJSON_VERSION);
    php_info_print_table_row(2, "Support", SIMDJSON_SUPPORT_URL);
    php_info_print_table_row(2, "simdjson library version", SIMDJSON_VERSION);
    php_info_print_table_row(2, "Decoder implementation", simdjson::get_active_implementation()->description().c_str());
    php_info_print_table_row(2, "Encoder implementation", simdjson_encode_implementation());

    php_info_print_table_end();
}
/* }}} */

/** {{{ module depends
 */
zend_module_dep simdjson_deps[] = {
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ simdjson_module_entry
*/
zend_module_entry simdjson_module_entry = {
    STANDARD_MODULE_HEADER_EX, NULL,
    simdjson_deps,
    "simdjson",
    ext_functions,
    PHP_MINIT(simdjson),
    PHP_MSHUTDOWN(simdjson),
    PHP_RINIT(simdjson),
    PHP_RSHUTDOWN(simdjson),
    PHP_MINFO(simdjson),
    PHP_SIMDJSON_VERSION,
    PHP_MODULE_GLOBALS(simdjson),
    PHP_GINIT(simdjson),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/** {{{ DL support
 */
#ifdef COMPILE_DL_SIMDJSON
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif

ZEND_GET_MODULE(simdjson)
#endif
/* }}} */
