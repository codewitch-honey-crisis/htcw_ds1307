# DS1307

A device driver for the DS1307 real time clock

This library allows you to use one of the common real time clocks.

```
[env:node32s]
platform = espressif32
board = node32s
framework = arduino
lib_deps = 
	codewitch-honey-crisis/htcw_ds1307@^0.9.1
lib_ldf_mode = deep
```