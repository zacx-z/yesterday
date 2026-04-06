#define YST_IMPLEMENTATION
#include "yesterday.h"
#include "fenster.h"
#include <stdlib.h>

#define W 320
#define H 240
#define MAX_PARTICLES 200

struct comp_movement
{
    float x, y;
    float sx, sy;
};

void* alloc_func(size_t size, enum yst_alloc_type alloc_type)
{
    return malloc(size);
}
void dealloc_func(void* ptr, enum yst_alloc_type alloc_type)
{
    free(ptr);
}

int main() {
    uint32_t buf[W * H];
    struct fenster f = { .title = "moving stars", .width = W, .height = H, .buf = buf };
    fenster_open(&f);

    struct yst_context ctx;
    yst_init(&ctx, alloc_func, dealloc_func);

    yst_comp_type movement_ct = yst_make_comp_type(&ctx, sizeof(struct comp_movement));

    yst_entity_id entities[MAX_PARTICLES];

    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        yst_entity_id entity = yst_new_entity(&ctx);
        yst_comp_id comp = yst_add_component(&ctx, entity, movement_ct);
        struct comp_movement* movement = (struct comp_movement*)yst_mutate(&ctx, comp);

        movement->x = rand() / (float)RAND_MAX * W;
        movement->y = rand() / (float)RAND_MAX * H;
        movement->sx = (rand() / (float)RAND_MAX - 0.5f) * 20;
        movement->sy = (rand() / (float)RAND_MAX - 0.5f) * 20;

        entities[i] = entity;
    }

    int64_t cur_time = fenster_time();

    while (fenster_loop(&f) == 0)
    {
        if (f.keys[27])
            break;

        int64_t next_time = fenster_time();
#ifdef FPS
        if (next_time - cur_time < 1000 / FPS)
        {
            fenster_sleep(1000 / FPS + cur_time - next_time);
        }
#endif
        float delta_time = (next_time - cur_time) * 0.001f;
        cur_time = next_time;

        for (int i = 0; i < MAX_PARTICLES; ++i)
        {
            yst_entity_id entity = entities[i];
            yst_comp_id comp = yst_get_component(&ctx, entity, movement_ct);
            struct comp_movement* movement = (struct comp_movement*)yst_mutate(&ctx, comp);
            movement->x += movement->sx * delta_time;
            movement->y += movement->sy * delta_time;

            if (movement->x < 0)
            {
                movement->x = -movement->x;
                movement->sx = -movement->sx;
            }
            if (movement->x >= W)
            {
                movement->x = W * 2 - movement->x;
                movement->sx = -movement->sx;
            }
            if (movement->y < 0)
            {
                movement->y = -movement->y;
                movement->sy = -movement->sy;
            }
            if (movement->y >= H)
            {
                movement->y = H * 2 - movement->y;
                movement->sy = -movement->sy;
            }
        }

        memset(buf, 0, W * H * sizeof(uint32_t));

        for (int i = 0; i < MAX_PARTICLES; ++i)
        {
            yst_entity_id entity = entities[i];
            yst_comp_id comp = yst_get_component(&ctx, entity, movement_ct);
            const struct comp_movement* movement = (const struct comp_movement*)yst_read(&ctx, comp);
            int x = (int)movement->x;
            int y = (int)movement->y;
            if (x >= 0 && x < W && y >= 0 && y < H)
            {
                fenster_pixel(&f, x, y) = 0xffffff;
            }
        }

        yst_elapse(&ctx, delta_time);
        if (ctx.now % 10000 == 0)
        {
            printf("total frames: %llu\n", ctx.now);
            fflush(stdout);
        }
    }

    yst_finalize(&ctx);
    fenster_close(&f);
    return 0;
}

