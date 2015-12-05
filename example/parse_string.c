#include <stdio.h>
#include <string.h>
#include "ini_parser.h"

int main(int argc, char *argv[]) {
    dictionary_t *dict;
    if (argc < 2) {
        printf("Param Invalid.");
        return 1;
    }

    FILE *fp;
    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("Open ini file failed.");
        return 1;
    }

    fseek (fp , 0 , SEEK_END);
    size_t l_size = ftell (fp);
    rewind (fp); 

    char *buffer = (char*) malloc (sizeof(char)*(l_size+1));
    size_t result = fread(buffer, 1, l_size, fp);
    if (result != l_size) {
        printf("Reading error.");
        return 1;
    }

    fclose(fp);
    buffer[l_size] = '\0';
    INI_ERROR err = ini_parse_string(buffer, &dict);
    free(buffer);

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