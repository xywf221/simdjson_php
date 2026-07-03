/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: simplified-local
 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_simdjson_validate, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, depth, IS_LONG, 0, "512")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_simdjson_decode, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, associative, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, depth, IS_LONG, 0, "512")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_simdjson_encode, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, depth, IS_LONG, 0, "512")
ZEND_END_ARG_INFO()

ZEND_FUNCTION(simdjson_validate);
ZEND_FUNCTION(simdjson_decode);
ZEND_FUNCTION(simdjson_encode);

static const zend_function_entry ext_functions[] = {
	ZEND_FE(simdjson_validate, arginfo_simdjson_validate)
	ZEND_FE(simdjson_decode, arginfo_simdjson_decode)
	ZEND_FE(simdjson_encode, arginfo_simdjson_encode)
	ZEND_FE_END
};

static void register_simdjson_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("SIMDJSON_PRETTY_PRINT", SIMDJSON_PRETTY_PRINT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIMDJSON_ENCODE_NON_NULL", SIMDJSON_ENCODE_NON_NULL, CONST_PERSISTENT);
}
