--TEST--
simdjson current public API
--EXTENSIONS--
simdjson
--FILE--
<?php
$functions = get_defined_functions()['internal'];
$simdjsonFunctions = array_values(array_filter(
    $functions,
    static fn (string $name): bool => str_starts_with($name, 'simdjson')
));
sort($simdjsonFunctions);
var_export($simdjsonFunctions);
echo "\n";

var_dump(defined('SIMDJSON_PRETTY_PRINT'));
var_dump(defined('SIMDJSON_ENCODE_NON_NULL'));
var_dump(simdjson_validate('{"a":1}'));

var_export(simdjson_decode('{"a":1,"b":true}', true));
echo "\n";

echo simdjson_encode(['a' => 1, 'b' => null], SIMDJSON_ENCODE_NON_NULL), "\n";
echo simdjson_encode(['a' => 1], SIMDJSON_PRETTY_PRINT), "\n";
?>
--EXPECT--
array (
  0 => 'simdjson_decode',
  1 => 'simdjson_encode',
  2 => 'simdjson_validate',
)
bool(true)
bool(true)
bool(true)
array (
  'a' => 1,
  'b' => true,
)
{"a":1}
{
    "a": 1
}
