# ini_parser
a simple ini file parser

##Usage
```c
dictionary_t *dict;

// parse file
INI_ERROR err  = ini_parse_file("./php.ini", &dict);

// OR parse string
// INI_ERROR err  = ini_parse_string("XXXX"/*something to parse*/, &dict);

if (err != SUCCESS) {
	printf("Failed info:%s:\n", get_parse_error());
}

// dump parsed info
ini_dump(dict);

// find the key of the section
bucket_t *p_bucket = ini_parameter_find(dict, "soap", "soap.wsdl_cache_dir");
if (p_bucket) {
	printf("Find key %s, val is %s\n", p_bucket->p_key, (char*)p_bucket->p_val);
}

// destroy the all
ini_destroy(dict);
```
