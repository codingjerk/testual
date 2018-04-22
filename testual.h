int __testual__assert_count = 0;

struct testual_error_t {
    int is_error;
    const char *error_description;
};

#define __testual__return_error(message) { \
    struct testual_error_t err; err.is_error = 1; \
    err.error_description = message; \
    return err; \
}

#define __testual__return_if(cond, message) { \
    __testual__assert_count++;\
    if (cond) __testual__return_error(message) \
}

#define assert_true(b) \
    __testual__return_if(!b, "Expected false be equal to true")

// TODO: print arguments
#define assert_equal(a, b) \
    __testual__return_if(a != b, "Expected {a} be equal to {b}")

#define assert_throw(block) { \
    __testual__assert_count++;\
    try {\
        block;\
        __testual__return_error("Exception wasn't raised")\
    } catch (...) {}\
}
