<?php

/**
 * @generate-class-entries
 * @generate-legacy-arginfo 80000
 */

/**
 * @var int
 * @cvalue SIMDJSON_PRETTY_PRINT
 */
const SIMDJSON_PRETTY_PRINT = UNKNOWN;

/**
 * @var int
 * @cvalue SIMDJSON_ENCODE_NON_NULL
 */
const SIMDJSON_ENCODE_NON_NULL = UNKNOWN;

function simdjson_validate(string $json, int $depth = 512): bool {}

/**
 * Takes a JSON encoded string and converts it into a PHP variable.
 * Similar to json_decode().
 *
 * @param string $json The JSON string being decoded
 * @param bool $associative When true, JSON objects will be returned as associative arrays.
 *                          When false, JSON objects will be returned as objects.
 * @param int $depth the maximum nesting depth of the structure being decoded.
 * @return array|stdClass|string|float|int|bool|null
 * @throws RuntimeException for invalid JSON
 * @throws ValueError for invalid $depth
 */
function simdjson_decode(string $json, bool $associative = false, int $depth = 512): mixed {}

/**
 * Returns the JSON representation of a value.
 *
 * @param mixed $value The value being encoded. Can be any type except a resource.
 * @param int $flags Bitmask of supported simdjson encode options.
 * @param int $depth Set the maximum depth. Must be greater than zero.
 * @return string
 * @throws RuntimeException
 * @throws ValueError for invalid $depth
 */
function simdjson_encode(mixed $value, int $flags = 0, int $depth = 512): string {}
