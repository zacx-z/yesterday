/*
 * yesterday - v0.1.2 - a minimalistic ECS implementation that allows you to trace back in time to a moment you wish to relive.
 * by Zack Zhang (github.com/zacx-z) 2026
 */
#ifndef YST_YESTERDAY_H
#define YST_YESTERDAY_H

#ifndef YST_RELEASE
#define YST_REMEMBRANCE
#endif

#if !defined(YST_DEFAULT_COMP_CAPACITY)
#define YST_DEFAULT_COMP_CAPACITY 8
#endif

#if !defined(YST_INIT_FRAME_CAPACITY)
#define YST_INIT_FRAME_CAPACITY 512
#endif
#if !defined(YST_FORWARD_REC_POOL_CAPACITY)
#define YST_FORWARD_REC_POOL_CAPACITY 2048
#endif

#if !defined(YST_ARCHIVE_CHUNK_SIZE)
#define YST_ARCHIVE_CHUNK_SIZE (8*8192)
#endif

#define YST_COMP_REGION_SIZE 128

#ifndef YST_API
    #define YST_API extern
#endif
#ifndef YST_LIB
    #define YST_LIB static
#endif

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#define nullptr 0
#endif

static_assert(YST_FORWARD_REC_POOL_CAPACITY <= 65535, "Forward record pool's capacity shouldn't exceed uint16_t max value");

typedef uint32_t yst_entity_id;
typedef uint64_t yst_frame_t;
typedef uint8_t yst_flags;

enum yst_node_flags
{
    YST_COMP_DIRTY   = 1 << 0,
    YST_COMP_INVALID = 1 << 1,
    YST_COMP_ADD = 1 << 2,
};

enum yst_alloc_type
{
    YST_ALLOC_DEFAULT,
    YST_ALLOC_COMP,
    YST_ALLOC_COMP_TYPE,
    YST_ALLOC_ENTITY_LOOKUP,
    YST_ALLOC_FRAME_DATA,
    YST_ALLOC_ARCHIVE,
    YST_ALLOC_FORWARD_RECORD
};

typedef void* (*yst_alloc_f)(size_t size, enum yst_alloc_type alloc_type);
typedef void (*yst_dealloc_f)(void* ptr, enum yst_alloc_type alloc_type);

struct yst_comp_node_header
{
#ifdef YST_REMEMBRANCE
    struct yst_comp_node_header *prev_in_time;
    struct yst_comp_forward_record *forward_in_time;
    yst_frame_t timestamp;
#endif
    struct yst_comp_node_header *next_recycled;
    yst_entity_id entity;
    uint32_t next_sibling; // next component of the same type on the same entity
    yst_flags flags;
};

struct yst_comp_forward_record
{
    struct yst_comp_node_header *next_in_time;
};

struct yst_comp_array
{
    void *array;
    size_t elem_size;
    uint32_t elem_count;
    uint32_t capacity;
};

struct yst_comp_type_node
{
    struct yst_comp_type_node *next;
    struct yst_comp_node_header *recycled;
    uint32_t *lookup; // lookup by entity id
    yst_entity_id lookup_capacity;
    struct yst_comp_array array;
};

struct yst_comp_forward_record_pool_header
{
    struct yst_comp_forward_record_pool_header *prev;
    uint16_t allocated;
    uint16_t capacity;
};

struct yst_comp_forward_record_pool_node
{
    struct yst_comp_forward_record_pool_node *next_free;
    struct yst_comp_forward_record data;
};

typedef struct yst_comp_type_node *yst_comp_type;

typedef struct yst_comp_id
{
    struct yst_comp_array *array;
    uint32_t index;
} yst_comp_id;

struct yst_frame_archive_header
{
    struct yst_frame_archive_header* prev;
    struct yst_frame_archive_region_header *region;
    struct yst_comp_type_node *comp_type;
    struct yst_comp_node_header *recycled;
};

struct yst_frame_data
{
    float time;
    struct yst_frame_archive_header* archive;
};

struct yst_comp_type_region
{
    struct yst_comp_type_region *prev;
    uint16_t allocated;
    struct yst_comp_type_node nodes[YST_COMP_REGION_SIZE];
};

struct yst_frame_archive_region_header
{
    struct yst_frame_archive_region_header *prev;
    void* next_alloc_ptr;
};

struct yst_context
{
    yst_alloc_f alloc;
    yst_dealloc_f dealloc;
    struct yst_comp_type_node *first;
    yst_entity_id next_entity;
    struct yst_comp_type_region *comp_type_region;

    yst_frame_t now;
    yst_frame_t latest;

#ifdef YST_REMEMBRANCE
    struct yst_frame_data *frame_data;
    uint64_t frame_capacity;
    struct yst_frame_archive_region_header *frame_archive_region;
    struct yst_comp_forward_record_pool_header *forward_rec_pool;
    struct yst_comp_forward_record_pool_node *forward_rec_next_recycled;
#endif
};

YST_API void yst_init(struct yst_context *ctx, yst_alloc_f alloc, yst_dealloc_f dealloc);
YST_API void yst_finalize(struct yst_context *ctx);
YST_API yst_entity_id yst_new_entity(struct yst_context *ctx);
YST_API void yst_delete_entity(struct yst_context *ctx, yst_entity_id entity);
/**
 * Delete all entities and history.
 */
YST_API void yst_clear(struct yst_context *ctx);
YST_API yst_comp_type yst_make_component_type(struct yst_context *ctx, size_t data_size);
YST_API yst_comp_id yst_add_component(struct yst_context *ctx, yst_entity_id entity, yst_comp_type comp_type);
YST_API yst_comp_id yst_get_component(struct yst_context *ctx, yst_entity_id entity, yst_comp_type comp_type);
YST_API yst_comp_id yst_next_component_sibling(struct yst_context *ctx, yst_comp_id comp_id);
YST_API void yst_delete_component(struct yst_context *ctx, yst_comp_type comp_type, yst_comp_id comp_id);
YST_API const void* yst_read(struct yst_context *ctx, yst_comp_id id);
YST_API void* yst_mutate(struct yst_context *ctx, yst_comp_id id);
YST_API void yst_elapse(struct yst_context *ctx, float delta_time);

YST_API yst_comp_id yst_first(struct yst_context *ctx, yst_comp_type comp_type);
YST_API yst_comp_id yst_next(struct yst_context *ctx, yst_comp_id comp_id);

#define yst_foreach(ctx, comp, comp_type) for (yst_comp_id comp = yst_first(ctx, comp_type); comp.array != 0; comp = yst_next(ctx, comp))

#ifdef YST_REMEMBRANCE
YST_API void yst_relive(struct yst_context *ctx, yst_frame_t time);
#endif

#ifdef YST_IMPLEMENTATION
#include <string.h>

YST_LIB void* yst_realloc(struct yst_context *ctx, void* src, size_t new_size, size_t original_size, enum yst_alloc_type alloc_type);
YST_LIB struct yst_comp_type_node* yst_alloc_comp_type(struct yst_context *ctx, size_t elem_size, uint32_t capacity);
YST_LIB void yst_mark_comp_deleted(struct yst_context *ctx, struct yst_comp_type_node *comp_type_node, struct yst_comp_node_header *comp);
#ifdef YST_REMEMBRANCE
YST_LIB void* yst_alloc_archive(struct yst_context *ctx, struct yst_frame_archive_header **p_archive, yst_comp_type comp_type, size_t size);
YST_LIB void yst_dealloc_archive_since(struct yst_context *ctx, struct yst_frame_archive_header *archive);
YST_LIB void yst_alloc_archive_region(struct yst_context *ctx, struct yst_frame_archive_region_header **p_archive);
YST_LIB void yst_create_forward_rec_pool(struct yst_context *ctx, uint16_t capacity, struct yst_comp_forward_record_pool_header** p_header);
YST_LIB struct yst_comp_forward_record* yst_new_forward_record(struct yst_context *ctx, struct yst_comp_node_header *next);
YST_LIB void yst_delete_forward_record(struct yst_context *ctx, struct yst_comp_forward_record* record);
YST_LIB void yst_fast_forward(struct yst_context *ctx, yst_frame_t time);
#endif

YST_LIB inline struct yst_comp_node_header* yst_get_comp_at(struct yst_comp_array* array, uint32_t index)
{
    return (struct yst_comp_node_header*)(((char*)array->array) + array->elem_size * index);
}

YST_API void yst_init(struct yst_context *ctx, yst_alloc_f alloc, yst_dealloc_f dealloc)
{
    ctx->alloc = alloc;
    ctx->dealloc = dealloc;
    ctx->first = nullptr;
    ctx->now = 0;
    ctx->latest = 0;
    ctx->next_entity = 0;

    ctx->comp_type_region = ctx->alloc(sizeof(struct yst_comp_type_region), YST_ALLOC_COMP_TYPE);
    ctx->comp_type_region->allocated = 0;
    ctx->comp_type_region->prev = nullptr;

#ifdef YST_REMEMBRANCE
    ctx->forward_rec_pool = nullptr;
    ctx->forward_rec_next_recycled = nullptr;

    ctx->frame_archive_region = nullptr;

    ctx->frame_capacity = YST_INIT_FRAME_CAPACITY;
    ctx->frame_data = ctx->alloc(YST_INIT_FRAME_CAPACITY * sizeof(struct yst_frame_data), YST_ALLOC_FRAME_DATA);
    yst_create_forward_rec_pool(ctx, YST_FORWARD_REC_POOL_CAPACITY, &ctx->forward_rec_pool);
#endif
}

YST_API void yst_finalize(struct yst_context *ctx)
{
#ifdef YST_REMEMBRANCE
    for (struct yst_frame_archive_region_header* archive_region = ctx->frame_archive_region; archive_region != nullptr;)
    {
        struct yst_frame_archive_region_header* prev = archive_region->prev;
        ctx->dealloc(archive_region, YST_ALLOC_ARCHIVE);
        archive_region = prev;
    }
    ctx->dealloc(ctx->frame_data, YST_ALLOC_FRAME_DATA);
    for (struct yst_comp_forward_record_pool_header* pool = ctx->forward_rec_pool; pool != nullptr;)
    {
        struct yst_comp_forward_record_pool_header* prev = pool->prev;
        ctx->dealloc(pool, YST_ALLOC_FORWARD_RECORD);
        pool = prev;
    }
#endif
    for (struct yst_comp_type_node *node = ctx->first; node != nullptr;)
    {
        struct yst_comp_type_node *next = node->next;
        ctx->dealloc(node->lookup, YST_ALLOC_ENTITY_LOOKUP);
        ctx->dealloc(node->array.array, YST_ALLOC_COMP);
        node = next;
    }
    for (struct yst_comp_type_region *comp_type = ctx->comp_type_region; comp_type != nullptr;)
    {
        struct yst_comp_type_region *prev = comp_type->prev;
        ctx->dealloc(comp_type, YST_ALLOC_COMP_TYPE);
        comp_type = prev;
    }
}

YST_API yst_entity_id yst_new_entity(struct yst_context *ctx)
{
    return ctx->next_entity++;
}

YST_API void yst_delete_entity(struct yst_context *ctx, yst_entity_id entity)
{
    for (struct yst_comp_type_node *node = ctx->first; node != nullptr; node = node->next)
    {
        if (entity >= node->lookup_capacity) continue;

        uint32_t id = node->lookup[entity];
        if (id != 0)
        {
            struct yst_comp_array *arr = &node->array;
            struct yst_comp_node_header* comp_header = yst_get_comp_at(arr, id - 1);
            while (comp_header != nullptr)
            {
                yst_mark_comp_deleted(ctx, node, comp_header);
                comp_header = comp_header->next_sibling > 0 ? yst_get_comp_at(arr, comp_header->next_sibling - 1) : nullptr;
            }
        }
        node->lookup[entity] = 0;
    }
}

YST_API void yst_clear(struct yst_context *ctx)
{
    for (struct yst_comp_type_node *comp_type = ctx->first; comp_type != nullptr; comp_type = comp_type->next)
    {
        comp_type->recycled = nullptr;
        comp_type->array.elem_count = 0;

        memset(comp_type->lookup, 0, comp_type->lookup_capacity * sizeof(uint32_t));
    }

#ifdef YST_REMEMBRANCE
    struct yst_comp_forward_record_pool_header *pool = ctx->forward_rec_pool;
    while (pool->prev != nullptr)
    {
        struct yst_comp_forward_record_pool_header *prev = pool->prev;
        ctx->dealloc(pool, YST_ALLOC_FORWARD_RECORD);
        pool = prev;
    }
    ctx->forward_rec_pool = pool;
    ctx->forward_rec_next_recycled = nullptr;

    for(struct yst_frame_archive_region_header *archive_region = ctx->frame_archive_region; archive_region != nullptr;)
    {
        struct yst_frame_archive_region_header *prev = archive_region->prev;
        ctx->dealloc(archive_region, YST_ALLOC_ARCHIVE);
        archive_region = prev;
    }
    ctx->frame_archive_region = nullptr;
#endif

    ctx->next_entity = 0;
    ctx->now = 0;
    ctx->latest = 0;
}

YST_API yst_comp_type yst_make_component_type(struct yst_context *ctx, size_t data_size)
{
    struct yst_comp_type_node** p = &ctx->first;
    while (*p != nullptr)
    {
        p = &(*p)->next;
    }
    struct yst_comp_type_node* comp_type = *p = yst_alloc_comp_type(ctx, sizeof(struct yst_comp_node_header) + data_size, YST_DEFAULT_COMP_CAPACITY);
    return comp_type;
}

YST_API yst_comp_id yst_add_component(struct yst_context *ctx, yst_entity_id entity, yst_comp_type comp_type)
{
    struct yst_comp_node_header *header;
#ifdef YST_REMEMBRANCE
    struct yst_comp_node_header *prev_in_time;
#endif
    uint32_t index;
    if (comp_type->recycled)
    {
        header = comp_type->recycled;
        comp_type->recycled = header->next_recycled;
        index = ((char*)header - (char*)comp_type->array.array) / comp_type->array.elem_size;
        header->next_recycled = nullptr;
#ifdef YST_REMEMBRANCE
        prev_in_time = header->prev_in_time;
#endif
    }
    else
    {
        index = comp_type->array.elem_count++;
        if (index >= comp_type->array.capacity)
        {
            comp_type->array.array = yst_realloc(ctx, comp_type->array.array, comp_type->array.capacity * 2 * comp_type->array.elem_size, comp_type->array.capacity * comp_type->array.elem_size, YST_ALLOC_COMP);
            comp_type->array.capacity *= 2;
        }

        void* comp = yst_get_comp_at(&comp_type->array, index);
        header = (struct yst_comp_node_header*)comp;
#ifdef YST_REMEMBRANCE
        prev_in_time = nullptr;
#endif
    }

    if (entity >= comp_type->lookup_capacity)
    {
        yst_entity_id new_capacity = entity * 2;
        if (new_capacity < YST_DEFAULT_COMP_CAPACITY) new_capacity = YST_DEFAULT_COMP_CAPACITY;
        size_t original_size = comp_type->lookup_capacity * sizeof(yst_entity_id);
        size_t new_size = new_capacity * sizeof(yst_entity_id);
        comp_type->lookup = yst_realloc(ctx, comp_type->lookup, new_size, original_size, YST_ALLOC_ENTITY_LOOKUP);
        memset((char*)comp_type->lookup + original_size, 0, new_size - original_size);
        comp_type->lookup_capacity = new_capacity;
    }
    uint32_t* id = &comp_type->lookup[entity];

    *header = (struct yst_comp_node_header){
#ifdef YST_REMEMBRANCE
        .prev_in_time = prev_in_time,
        .forward_in_time = nullptr,
        .timestamp = ctx->now,
#endif
        .next_recycled = nullptr,
        .entity = entity,
        .next_sibling = *id,
        .flags = YST_COMP_DIRTY | YST_COMP_ADD,
    };

    *id = index + 1;

    return (yst_comp_id){
        .array = &comp_type->array,
        .index = index,
    };
}

YST_API yst_comp_id yst_get_component(struct yst_context *ctx, yst_entity_id entity, yst_comp_type comp_type)
{
    if (entity < comp_type->lookup_capacity)
    {
        uint32_t id = comp_type->lookup[entity];
        if (id != 0)
        {
            return (yst_comp_id){
                .array = &comp_type->array,
                .index = id - 1,
            };
        }
    }

    return (yst_comp_id){
        .array = nullptr,
        .index = 0,
    };
}

YST_API yst_comp_id yst_next_component_sibling(struct yst_context *ctx, yst_comp_id comp_id)
{
    uint32_t id = yst_get_comp_at(comp_id.array, comp_id.index)->next_sibling;
    if (id != 0)
    {
        return (yst_comp_id){
            .array = comp_id.array,
            .index = id - 1,
        };
    }
    else
    {
        return (yst_comp_id){
            .array = nullptr,
            .index = 0,
        };
    }
}

YST_API void yst_delete_component(struct yst_context *ctx, yst_comp_type comp_type, yst_comp_id comp_id)
{
    struct yst_comp_node_header* const header = yst_get_comp_at(comp_id.array, comp_id.index);
    const yst_entity_id entity = header->entity;
    uint32_t *id = &comp_type->lookup[entity];
    while (*id != 0)
    {
        if (*id - 1 == comp_id.index)
        {
            *id = header->next_sibling + 1;
            break;
        }

        id = &yst_get_comp_at(&comp_type->array, *id - 1)->next_sibling;
    }

    yst_mark_comp_deleted(ctx, comp_type, header);
}

YST_API const void* yst_read(struct yst_context *ctx, yst_comp_id id)
{
    struct yst_comp_node_header* header = yst_get_comp_at(id.array, id.index);
    if (header->flags & YST_COMP_INVALID) return nullptr;

    return (char*)header + sizeof(struct yst_comp_node_header);
}

YST_API yst_comp_id yst_first(struct yst_context *ctx, yst_comp_type comp_type)
{
    struct yst_comp_array *array = &comp_type->array;
    if (array->elem_count == 0) return (yst_comp_id){
        .array = nullptr,
        .index = 0,
    };

    yst_comp_id ret = (yst_comp_id){ .array = array, .index = 0 };
    if (yst_get_comp_at(array, ret.index)->flags & YST_COMP_INVALID)
        return yst_next(ctx, ret);

    return ret;
}

YST_API yst_comp_id yst_next(struct yst_context *ctx, yst_comp_id comp_id)
{
    struct yst_comp_array *array = comp_id.array;
    if (array == nullptr) return comp_id;

    ++comp_id.index;
    while (comp_id.index < array->elem_count && (yst_get_comp_at(array, comp_id.index)->flags & YST_COMP_INVALID))
    {
        ++comp_id.index;
    }

    if (comp_id.index != array->elem_count) return comp_id;

    return (yst_comp_id){
        .array = nullptr,
        .index = 0,
    };
}

YST_API void* yst_mutate(struct yst_context *ctx, yst_comp_id id)
{
    struct yst_comp_node_header* header = yst_get_comp_at(id.array, id.index);
    if (header->flags & YST_COMP_INVALID) return nullptr;

    header->flags |= YST_COMP_DIRTY;
    return (char*)header + sizeof(struct yst_comp_node_header);
}

#ifdef YST_REMEMBRANCE
YST_API void yst_relive(struct yst_context *ctx, yst_frame_t time)
{
    if (time == ctx->now) return;

    if (time > ctx->now) {
        yst_fast_forward(ctx, time);
        return;
    }

    for (struct yst_comp_type_node* comp_type = ctx->first; comp_type != nullptr; comp_type = comp_type->next)
    {
        struct yst_comp_array* array = &comp_type->array;
        yst_frame_t closest_time = 0;
        for (uint32_t i = 0; i < array->elem_count; ++i)
        {
            struct yst_comp_node_header* current = yst_get_comp_at(array, i);
            struct yst_comp_node_header* header = current;

            if (current->flags & YST_COMP_DIRTY)
            {
                // NOTE: latest changes will not be saved
                if ((current->flags & YST_COMP_ADD) && current->prev_in_time == nullptr)
                {
                    current->flags = YST_COMP_INVALID;
                    continue;
                }
            }

            if ((header->flags & YST_COMP_INVALID) && header->prev_in_time == nullptr) continue; 

            if (header->forward_in_time)
            {
                // find the actual node in the archive
                header = header->forward_in_time->next_in_time->prev_in_time;
                assert(current != header);
            }
            else
            {
                // the head is the same as the latest in the archive
                header = header->prev_in_time;
            }

            assert(header != nullptr);

            struct yst_comp_node_header* look_ahead = header->prev_in_time;
            while (look_ahead != nullptr && header->timestamp > time)
            {
                look_ahead->forward_in_time = yst_new_forward_record(ctx, header);

                assert(header != look_ahead);
                header = look_ahead;
                look_ahead = header->prev_in_time;
            }

            if ((header->flags & YST_COMP_ADD) != 0 && header->timestamp > time)
            {
                *current = (struct yst_comp_node_header){
                    .prev_in_time = nullptr,
                    .forward_in_time = yst_new_forward_record(ctx, header),
                    .timestamp = 0,
                    .flags = YST_COMP_INVALID,
                };
            }
            else
            {
                assert(header->timestamp <= time);

                if (current != header && header->forward_in_time)
                {
                    memcpy(current, header, array->elem_size);
                }
            }

            if (header->timestamp > closest_time) closest_time = header->timestamp;
        }

        // roll back recycled pool
        for (struct yst_frame_archive_header *archive = ctx->frame_data[closest_time].archive; archive != nullptr; archive = archive->prev)
        {
            if (archive->comp_type == comp_type)
            {
                comp_type->recycled = archive->recycled;
                break;
            }
        }
    }

    ctx->now = time;
}

YST_API void yst_elapse(struct yst_context *ctx, float delta_time)
{
    if (ctx->latest > ctx->now)
    {
        ++ctx->now; // keep this frame's data and remove the data after it

        // remove forward records
        for (struct yst_comp_type_node* comp_type = ctx->first; comp_type != nullptr; comp_type = comp_type->next)
        {
            for (uint32_t i = 0; i < comp_type->array.elem_count; ++i)
            {
                struct yst_comp_node_header *header = yst_get_comp_at(&comp_type->array, i);
                if (header->forward_in_time)
                {
                    struct yst_comp_node_header *current = header;
                    // find the actual node in the archive
                    header = header->forward_in_time->next_in_time->prev_in_time;
                    current->prev_in_time = header;

                    if (header == nullptr)
                    {
                        assert((current->forward_in_time->next_in_time->flags & YST_COMP_ADD) != 0);
                        yst_delete_forward_record(ctx, current->forward_in_time);
                        // start with the next record
                        header = current->forward_in_time->next_in_time;
                    }
                    current->forward_in_time = nullptr;

                    assert(current != header);
                    assert(current->timestamp < ctx->now);
                }
                while (header->forward_in_time != nullptr)
                {
                    struct yst_comp_node_header* next = header->forward_in_time->next_in_time;
                    yst_delete_forward_record(ctx, header->forward_in_time);
                    header->forward_in_time = nullptr;
                    header = next;
                }
            }
        }

        // delete the future records as there is a divergence
        for (struct yst_frame_archive_header *archive = ctx->frame_data[ctx->now].archive; archive != nullptr;)
        {
            struct yst_frame_archive_header *prev = archive->prev;
            yst_dealloc_archive_since(ctx, archive);
            archive = prev;
        }

        for (uint32_t t = ctx->now; t < ctx->latest; ++t)
        {
            ctx->frame_data[t].archive = nullptr;
        }
    }

    for (struct yst_comp_type_node* comp_type = ctx->first; comp_type != nullptr; comp_type = comp_type->next)
    {
        struct yst_comp_array* array = &comp_type->array;
        uint32_t archive_count = 0;
        for (uint32_t i = 0; i < array->elem_count; ++i)
        {
            struct yst_comp_node_header* header = yst_get_comp_at(array, i);
            if (header->flags & YST_COMP_DIRTY)
            {
                ++archive_count;
            }
        }

        if (archive_count > 0)
        {
            size_t elem_size = comp_type->array.elem_size;
            void* archive = yst_alloc_archive(ctx, &ctx->frame_data[ctx->now].archive, comp_type, archive_count * elem_size);
            uint32_t cur_archive_index = 0;
            for (uint32_t i = 0; i < array->elem_count; ++i)
            {
                struct yst_comp_node_header* header = yst_get_comp_at(array, i);
                if (header->flags & YST_COMP_DIRTY) {
                    header->flags &= ~YST_COMP_DIRTY;
                    header->timestamp = ctx->now;
                    void* dst = (((char*)archive) + cur_archive_index * elem_size);
                    memcpy(dst, header, elem_size);
                    header->prev_in_time = (struct yst_comp_node_header*)dst;
                    header->flags &= ~YST_COMP_ADD;

                    assert(header != dst);

                    ++cur_archive_index;
                }
            }
        }
    }

    if (ctx->now + 1 >= ctx->frame_capacity)
    {
        ctx->frame_data = yst_realloc(ctx, ctx->frame_data, ctx->frame_capacity * 2 * sizeof(struct yst_frame_data), ctx->frame_capacity * sizeof(struct yst_frame_data), YST_ALLOC_FRAME_DATA);
        ctx->frame_capacity *= 2;
    }

    ctx->frame_data[ctx->now + 1].time = ctx->frame_data[ctx->now].time + delta_time;
    ctx->frame_data[ctx->now + 1].archive = nullptr;
    ++ctx->now;
    ctx->latest = ctx->now;
}

YST_LIB void yst_fast_forward(struct yst_context *ctx, yst_frame_t time)
{
    for (struct yst_comp_type_node* comp_type = ctx->first; comp_type != nullptr; comp_type = comp_type->next)
    {
        struct yst_comp_array* array = &comp_type->array;
        for (uint32_t i = 0; i < array->elem_count; ++i)
        {
            struct yst_comp_node_header* current = yst_get_comp_at(array, i);
            struct yst_comp_node_header* header = current;

            struct yst_comp_forward_record* look_ahead = header->forward_in_time;
            while (look_ahead != nullptr && look_ahead->next_in_time->timestamp <= time)
            {
                yst_delete_forward_record(ctx, header->forward_in_time);
                header->forward_in_time = nullptr;
                header = look_ahead->next_in_time;
                look_ahead = header->forward_in_time;
            }

            assert(header->timestamp <= time);

            if (current != header)
            {
                memcpy(current, header, array->elem_size);
                if (!header->forward_in_time)
                {
                    current->prev_in_time = header;
                }
            }
        }
    }

    if (time > ctx->latest - 1) time = ctx->latest - 1;
    ctx->now = time;
}

#else // YST_REMEMBRANCE

YST_API void yst_elapse(struct yst_context *ctx, float delta_time)
{
    ++ctx->now;
    ctx->latest = ctx->now;
}

#endif // YST_REMEMBRANCE

///////////////////////////////////////////////////////////////////////////////
// Memory
///////////////////////////////////////////////////////////////////////////////
YST_LIB void* yst_realloc(struct yst_context *ctx, void* src, size_t new_size, size_t original_size, enum yst_alloc_type alloc_type)
{
    void* new_mem = ctx->alloc(new_size, alloc_type);
    if (src)
    {
        memcpy(new_mem, src, original_size < new_size ? original_size : new_size);
        ctx->dealloc(src, alloc_type);
    }
    return new_mem;
}

YST_LIB struct yst_comp_type_node* yst_alloc_comp_type(struct yst_context *ctx, size_t elem_size, uint32_t capacity)
{
    if (ctx->comp_type_region->allocated == YST_COMP_REGION_SIZE)
    {
        struct yst_comp_type_region* new_region = ctx->alloc(sizeof(struct yst_comp_type_region), YST_ALLOC_COMP_TYPE);
        new_region->allocated = 0;
        new_region->prev = ctx->comp_type_region;
        ctx->comp_type_region = new_region;
    }

    struct yst_comp_type_node* ret = &ctx->comp_type_region->nodes[ctx->comp_type_region->allocated++];

    ret->array.array = ctx->alloc(elem_size * capacity, YST_ALLOC_COMP);
    ret->array.elem_count = 0;
    ret->array.elem_size = elem_size;
    ret->array.capacity = capacity;

    ret->next = nullptr;
    ret->recycled = nullptr;
    ret->lookup = nullptr;
    yst_entity_id lookup_capacity = ctx->next_entity * 2 > YST_DEFAULT_COMP_CAPACITY ? ctx->next_entity * 2 : YST_DEFAULT_COMP_CAPACITY;
    ret->lookup = yst_realloc(ctx, ret->lookup, lookup_capacity * sizeof(yst_entity_id), 0, YST_ALLOC_ENTITY_LOOKUP);
    ret->lookup_capacity = lookup_capacity;
    return ret;
}

YST_LIB void yst_mark_comp_deleted(struct yst_context *ctx, struct yst_comp_type_node *comp_type_node, struct yst_comp_node_header *comp)
{
    comp->flags |= (YST_COMP_INVALID | YST_COMP_DIRTY);
    comp->next_recycled = comp_type_node->recycled;
    comp_type_node->recycled = comp;
}

#ifdef YST_REMEMBRANCE

YST_LIB void* yst_alloc_archive(struct yst_context *ctx, struct yst_frame_archive_header **p_archive, yst_comp_type comp_type, size_t size)
{
    size_t total_size = size + sizeof(struct yst_frame_archive_header);
    if (!ctx->frame_archive_region || ((char*)ctx->frame_archive_region->next_alloc_ptr - (char*)ctx->frame_archive_region) + total_size > YST_ARCHIVE_CHUNK_SIZE)
    {
        yst_alloc_archive_region(ctx, &ctx->frame_archive_region);
    }

    assert(total_size <= YST_ARCHIVE_CHUNK_SIZE - sizeof(struct yst_frame_archive_region_header));
    struct yst_frame_archive_header *new_archive = (struct yst_frame_archive_header*)ctx->frame_archive_region->next_alloc_ptr;

    new_archive->prev = *p_archive;
    new_archive->region = ctx->frame_archive_region;
    new_archive->comp_type = comp_type;
    new_archive->recycled = comp_type->recycled;

    *p_archive = new_archive;
    ctx->frame_archive_region->next_alloc_ptr = (char*)ctx->frame_archive_region->next_alloc_ptr + total_size;

    // return the pointer to the data section of frame archive
    return ((char*)*p_archive) + sizeof(struct yst_frame_archive_header);
}

YST_LIB void yst_dealloc_archive_since(struct yst_context *ctx, struct yst_frame_archive_header *archive)
{
    struct yst_frame_archive_region_header *region = ctx->frame_archive_region;
    while (region != archive->region)
    {
        struct yst_frame_archive_region_header *prev = region->prev;
        ctx->dealloc(region, YST_ALLOC_ARCHIVE);
        region = prev;
    }

    if (region->next_alloc_ptr > (void*)archive)
    {
        region->next_alloc_ptr = archive;
    }

    ctx->frame_archive_region = region;
}

YST_LIB void yst_alloc_archive_region(struct yst_context *ctx, struct yst_frame_archive_region_header **p_archive)
{
    struct yst_frame_archive_region_header *region = (struct yst_frame_archive_region_header*)ctx->alloc(YST_ARCHIVE_CHUNK_SIZE, YST_ALLOC_ARCHIVE);
    region->prev = *p_archive;
    region->next_alloc_ptr = (char*)region + sizeof(struct yst_frame_archive_region_header);
    *p_archive = region;
}

YST_LIB void yst_create_forward_rec_pool(struct yst_context *ctx, uint16_t capacity, struct yst_comp_forward_record_pool_header** p_header)
{
    struct yst_comp_forward_record_pool_header* header = (struct yst_comp_forward_record_pool_header*)ctx->alloc(
            sizeof(struct yst_comp_forward_record_pool_header) +
            capacity * sizeof(struct yst_comp_forward_record_pool_node), YST_ALLOC_FORWARD_RECORD);
    header->capacity = capacity;
    header->allocated = 0;
    header->prev = *p_header;
    *p_header = header;
}

YST_LIB struct yst_comp_forward_record* yst_new_forward_record(struct yst_context *ctx, struct yst_comp_node_header *next)
{
    struct yst_comp_forward_record_pool_node *ret = ctx->forward_rec_next_recycled;
    if (!ret)
    {
        if (ctx->forward_rec_pool->allocated == ctx->forward_rec_pool->capacity)
        {
            yst_create_forward_rec_pool(ctx, YST_FORWARD_REC_POOL_CAPACITY, &ctx->forward_rec_pool);
        }

        ret = (struct yst_comp_forward_record_pool_node*)(((char*)ctx->forward_rec_pool) + sizeof(struct yst_comp_forward_record_pool_header) + ctx->forward_rec_pool->allocated * sizeof(struct yst_comp_forward_record_pool_node));
        ret->next_free = nullptr;
        ++ctx->forward_rec_pool->allocated;
    }
    else
    {
        ctx->forward_rec_next_recycled = ret->next_free;
        ret->next_free = nullptr;
    }

    ret->data.next_in_time = next;
    return &ret->data;
}

YST_LIB void yst_delete_forward_record(struct yst_context *ctx, struct yst_comp_forward_record* record)
{
    struct yst_comp_forward_record_pool_node *node = (struct yst_comp_forward_record_pool_node*)((char*)record - offsetof(struct yst_comp_forward_record_pool_node, data));
    node->next_free = ctx->forward_rec_next_recycled;
    ctx->forward_rec_next_recycled = node;
}

#endif // YST_REMEMBRANCE

#endif // YST_IMPLEMENTATION

#undef nullptr
#endif // YST_YESTERDAY_H

// MIT License
//
// Copyright (c) 2026 Zack Zhang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
