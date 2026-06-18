cat > ~/memory-lab/WEEK-3/harness_improved.c << 'EOF'
#include <json-c/json.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;
    
    char *buf = malloc(size + 1);
    if (!buf) return 0;
    memcpy(buf, data, size);
    buf[size] = '\0';
    
    // PATH 1: Basic parse
    {
        struct json_tokener *tok = json_tokener_new();
        json_object *obj = json_tokener_parse_ex(tok, buf, size, NULL);
        if (obj) {
            // Exercise different functions
            json_object_to_json_string(obj);
            json_object_get_type(obj);
            
            // Iterate if it's an object
            if (json_object_get_type(obj) == json_type_object) {
                struct json_object_iterator iter = json_object_iter_begin(obj);
                struct json_object_iterator iter_end = json_object_iter_end(obj);
                while (!json_object_iter_equal(&iter, &iter_end)) {
                    json_object_iter_next(&iter);
                }
            }
            
            // Iterate if it's an array
            if (json_object_get_type(obj) == json_type_array) {
                int len = json_object_array_length(obj);
                for (int i = 0; i < len && i < 100; i++) {
                    json_object_array_get_idx(obj, i);
                }
            }
            
            json_object_put(obj);
        }
        json_tokener_free(tok);
    }
    
    // PATH 2: Parse with size constraint
    {
        struct json_tokener *tok = json_tokener_new();
        enum json_tokener_error err;
        json_object *obj = json_tokener_parse_ex(tok, buf, size / 2, &err);
        if (obj) json_object_put(obj);
        json_tokener_free(tok);
    }
    
    // PATH 3: Direct object construction (tests allocation)
    {
        if (size > 10) {
            json_object *obj = json_object_new_object();
            json_object_object_add(obj, "key", json_object_new_string("value"));
            json_object_put(obj);
        }
    }
    
    // PATH 4: Array construction
    {
        if (size > 20) {
            json_object *arr = json_object_new_array();
            json_object_array_add(arr, json_object_new_string("item"));
            json_object_put(arr);
        }
    }
    
    free(buf);
    return 0;
}
EOF

cat ~/memory-lab/WEEK-3/harness_improved.c
