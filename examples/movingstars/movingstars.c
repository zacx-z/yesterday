#define YST_IMPLEMENTATION
#ifdef RELEASE
#define YST_RELEASE
#endif
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

struct app_state
{
    struct fenster *f;
    struct yst_context *ctx;
    bool is_playing;
    int was_key_down[256];
};

void* alloc_func(size_t size, enum yst_alloc_type alloc_type)
{
    return malloc(size);
}
void dealloc_func(void* ptr, enum yst_alloc_type alloc_type)
{
    free(ptr);
}

void fill_rect(struct fenster *f, int x, int y, int w, int h, uint32_t color)
{
    for (int v = y; v < y + h; ++v)
        for (int u = x; u < x + w; ++u)
            fenster_pixel(f, u, v) = color;
}

#ifndef RELEASE
void draw_timebar(struct app_state *app)
{
    fill_rect(app->f, 0, H - 10, W, 5, 0x808080);
    float progress = (float)app->ctx->now / (app->ctx->latest + 1);
    if (progress > 1) progress = 1;
    fill_rect(app->f, (int)(progress * (W - 5)), H - 15, 5, 15, 0xffffff);

    if (app->f->mouse)
    {
        int mx = app->f->x, my = app->f->y;
        if (my > H - 15)
        {
            yst_frame_t target = (mx * app->ctx->latest) / W;
            if (target != app->ctx->now)
            {
                yst_relive(app->ctx, target);
            }
        }
        app->is_playing = false;
    }

    if ((app->f->keys[','] && !app->was_key_down[',']) || app->f->keys['-'])
    {
        if (app->ctx->now > 0)
        {
            yst_relive(app->ctx, app->ctx->now - 1);
        }
        app->is_playing = false;
    }

    if ((app->f->keys['.'] && !app->was_key_down['.']) || app->f->keys['='])
    {
        if (app->ctx->now < app->ctx->latest)
        {
            yst_relive(app->ctx, app->ctx->now + 1);
        }
        app->is_playing = false;
    }

    if (app->f->keys['['] && !app->was_key_down['['])
    {
        yst_relive(app->ctx, 0);
        app->is_playing = false;
    }

    if (app->f->keys[']'] && !app->was_key_down[']'])
    {
        yst_relive(app->ctx, app->ctx->latest);
        app->is_playing = false;
    }
}
#endif

int main() {
    uint32_t buf[W * H];
    struct fenster f = { .title = "moving stars", .width = W, .height = H, .buf = buf };
    fenster_open(&f);

    struct yst_context ctx;
    yst_init(&ctx, alloc_func, dealloc_func);

    yst_comp_type movement_ct = yst_make_component_type(&ctx, sizeof(struct comp_movement));

    yst_entity_id entities[MAX_PARTICLES];

    struct app_state app = { .f = &f, .ctx = &ctx, .is_playing = true };

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

        if (f.keys[32] && !app.was_key_down[32])
        {
            app.is_playing = !app.is_playing;
        }

        int64_t next_time = fenster_time();
#ifdef FPS
        if (next_time - cur_time < 1000 / FPS)
        {
            fenster_sleep(1000 / FPS + cur_time - next_time);
        }
#endif
        float delta_time = (next_time - cur_time) * 0.001f;
        cur_time = next_time;

        if (app.is_playing)
        {
            yst_foreach (&ctx, comp, movement_ct)
            {
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
        }

        memset(buf, 0, W * H * sizeof(uint32_t));

        yst_foreach (&ctx, comp, movement_ct)
        {
            const struct comp_movement* movement = (const struct comp_movement*)yst_read(&ctx, comp);
            int x = (int)movement->x;
            int y = (int)movement->y;
            if (x >= 0 && x < W && y >= 0 && y < H)
            {
                fenster_pixel(&f, x, y) = 0xffffff;
            }
        }

#ifndef RELEASE
        draw_timebar(&app);
#endif

        if (app.is_playing)
        {
            yst_elapse(&ctx, delta_time);
            if (ctx.now % 10000 == 0)
            {
                printf("total frames: %llu\n", ctx.now);
                fflush(stdout);
            }
        }

        memcpy(app.was_key_down, f.keys, sizeof(f.keys));
    }

    yst_finalize(&ctx);
    fenster_close(&f);
    return 0;
}

