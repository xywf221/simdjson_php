# simdjson_php

Minimal PHP extension for JSON validation, decoding, and encoding.

This repository is intentionally trimmed down. It exposes only:

- `simdjson_validate()`
- `simdjson_decode()`
- `simdjson_encode()`
- `SIMDJSON_PRETTY_PRINT`
- `SIMDJSON_ENCODE_NON_NULL`

It does not expose stream APIs, JSON pointer/key helpers, UTF-8 helper functions, Base64 helpers, custom exception classes, or extra error constants.

## Requirements

- PHP 8.0+
- C++17 compiler
- 64-bit build

For the bundled Windows build script:

- PHP 8.5.7 NTS x64
- Visual Studio 2022 C++ build tools
- PHP devel pack matching PHP 8.5.7 NTS x64, downloaded automatically by `build.bat`

## Build

### Windows

Use the Visual Studio 2022 x64 toolchain. From this repository:

```bat
cmd /c "call ""D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"" && build.bat"
```

The output is:

```text
php_simdjson.dll
```

The current `build.bat` is hardcoded for PHP 8.5.7 NTS x64. If your PHP build differs, update the devel pack URL and make sure the compiler runtime matches your PHP binary.

### Linux/macOS

Use the normal PHP extension build flow:

```sh
phpize
./configure --enable-simdjson
make
make test
make install
```

## Installation

### Windows

Copy `php_simdjson.dll` into PHP's extension directory, then add this to `php.ini`:

```ini
extension=php_simdjson.dll
```

For a one-off test without changing `php.ini`:

```bat
php -n -d extension=I:\source\simdjson_php\php_simdjson.dll --ri simdjson
```

### Linux/macOS

After `make install`, add:

```ini
extension=simdjson.so
```

## API

### simdjson_validate

```php
simdjson_validate(string $json, int $depth = 512): bool
```

Returns `true` if the JSON string is valid, otherwise `false`.

### simdjson_decode

```php
simdjson_decode(string $json, bool $associative = false, int $depth = 512): mixed
```

Decodes JSON into PHP values. When `$associative` is `true`, JSON objects become arrays. Otherwise, JSON objects become `stdClass` instances.

Invalid JSON throws `RuntimeException`. Invalid depth throws `ValueError`.

### simdjson_encode

```php
simdjson_encode(mixed $value, int $flags = 0, int $depth = 512): string
```

Encodes a PHP value as JSON.

Exported flag constants:

```php
SIMDJSON_PRETTY_PRINT
SIMDJSON_ENCODE_NON_NULL
```

`SIMDJSON_ENCODE_NON_NULL` skips `null` object properties and associative-array entries. Packed numeric arrays keep `null` elements to preserve list positions.

Errors throw `RuntimeException`. Invalid depth throws `ValueError`.

## Examples

```php
$json = '{"id":1,"name":"雷少","active":true}';

var_dump(simdjson_validate($json));

$data = simdjson_decode($json, true);
var_dump($data);

echo simdjson_encode($data), PHP_EOL;
echo simdjson_encode($data, SIMDJSON_PRETTY_PRINT), PHP_EOL;
echo simdjson_encode(['a' => 1, 'b' => null], SIMDJSON_ENCODE_NON_NULL), PHP_EOL;
```

Output:

```text
bool(true)
array(3) {
  ["id"]=>
  int(1)
  ["name"]=>
  string(6) "雷少"
  ["active"]=>
  bool(true)
}
{"id":1,"name":"雷少","active":true}
{
    "id": 1,
    "name": "雷少",
    "active": true
}
{"a":1}
```

## Removed APIs

The following old APIs are intentionally not part of this fork:

- `simdjson_decode_from_stream`
- `simdjson_decode_from_input`
- `simdjson_key_value`
- `simdjson_key_count`
- `simdjson_key_exists`
- `simdjson_cleanup`
- `simdjson_is_valid_utf8`
- `simdjson_utf8_len`
- `simdjson_encode_to_stream`
- `simdjson_base64_encode`
- `simdjson_base64_decode`
- `simdjson_base64_encode_from_stream`
- `SimdJsonBase64Encode`
- `SimdJsonException`, `SimdJsonDecoderException`, `SimdJsonEncoderException`

The goal is a small JSON-only extension with a narrow public surface.
