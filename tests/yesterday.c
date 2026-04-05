#define YST_IMPLEMENTATION
#include "yesterday.h"
#include <stdlib.h>

void* alloc_func(size_t size, enum yst_alloc_type alloc_type)
{
    return malloc(size);
}
void dealloc_func(void* ptr, enum yst_alloc_type alloc_type)
{
    free(ptr);
}

struct my_comp
{
    float my_float;
};

int main(int argc, char **argv)
{
    struct yst_context ctx;
    yst_init(&ctx, alloc_func, dealloc_func);
    yst_make_comp_type(&ctx, sizeof(struct my_comp));
    yst_finalize(&ctx);
    return 0;
}
