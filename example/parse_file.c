#include <stdio.h>
#include <string.h>
#include "ini_parser.h"

int main(int argc, char *argv[]) {
    dictionary_t *dict;
    if (argc < 2) {
        printf("Param Invalid.");
        return 1;
    }
    INI_ERROR err  = ini_parse_file(argv[1], &dict);

    if (err != SUCCESS) {
        printf("\n================\n%s: %s\n", "FAILED", get_parse_error());
    }

    ini_dump(dict);

    bucket_t *p_bucket = ini_parameter_find(dict, "soap", "soap.wsdl_cache_dir");
    if (p_bucket) {
        printf("Find key %s, val is %s\n", p_bucket->p_key, (char*)p_bucket->p_val);
    }

    ini_destroy(dict);

    return 0;
}