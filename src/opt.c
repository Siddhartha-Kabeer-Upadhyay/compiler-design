#include "opt.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    int x;
    int y;
    int dir;
} Node;

static int instr_is_direction_non_skip(int instr)
{
    return instr == INSTR_RIGHT || instr == INSTR_DOWN || instr == INSTR_LEFT || instr == INSTR_UP;
}

static int next_dir_for_instr(int current_dir, int instr)
{
    switch (instr)
    {
        case INSTR_RIGHT:
        case INSTR_RIGHT_SKIP:
            return 0;
        case INSTR_DOWN:
        case INSTR_DOWN_SKIP:
            return 1;
        case INSTR_LEFT:
        case INSTR_LEFT_SKIP:
            return 2;
        case INSTR_UP:
        case INSTR_UP_SKIP:
            return 3;
        default:
            return current_dir;
    }
}

static int q_push(Node *q, int *tail, int cap, int x, int y, int dir)
{
    if (*tail >= cap) return 0;
    q[*tail].x = x;
    q[*tail].y = y;
    q[*tail].dir = dir;
    (*tail)++;
    return 1;
}

static int build_in(const unsigned char *type, const int *instrs,
                    int w, int h, int *in_mask)
{
    int count = w * h;
    int queue_cap = count * 8;
    unsigned char *visited_state = (unsigned char *)calloc((size_t)count * 4, sizeof(unsigned char));
    Node *q = (Node *)malloc((size_t)queue_cap * sizeof(Node));
    int head = 0;
    int tail = 0;

    if (!visited_state || !q)
    {
        free(visited_state);
        free(q);
        return 0;
    }

    if (!q_push(q, &tail, queue_cap, 0, 0, 0))
    {
        free(visited_state);
        free(q);
        return 0;
    }

    while (head < tail)
    {
        Node it = q[head++];
        int x = it.x;
        int y = it.y;
        int dir = it.dir;

        if (!in_bounds(x, y, w, h))
            continue;

        int idx = y * w + x;
        int state_idx = idx * 4 + dir;
        if (visited_state[state_idx])
            continue;

        visited_state[state_idx] = 1;
        in_mask[idx] |= (1 << dir);

        if (type[idx] == PIXEL_HALT || type[idx] == PIXEL_ERROR)
            continue;

        int instr = instrs[idx];
        int next_dir = dir;
        if (type[idx] == PIXEL_CODE)
            next_dir = next_dir_for_instr(dir, instr);

        int nx = x;
        int ny = y;
        move_once(&nx, &ny, next_dir);
        if (!in_bounds(nx, ny, w, h))
            continue;

        int sx = nx;
        int sy = ny;
        if (type[idx] == PIXEL_CODE && instr_is_skip(instr))
        {
            move_once(&sx, &sy, next_dir);
            if (!in_bounds(sx, sy, w, h))
                continue;
        }

        if (!q_push(q, &tail, queue_cap, sx, sy, next_dir))
        {
            free(visited_state);
            free(q);
            return 0;
        }

        if (type[idx] == PIXEL_CODE && (instr == INSTR_JZ || instr == INSTR_JNZ))
        {
            int cx = sx;
            int cy = sy;
            move_once(&cx, &cy, next_dir);
            if (in_bounds(cx, cy, w, h))
            {
                if (!q_push(q, &tail, queue_cap, cx, cy, next_dir))
                {
                    free(visited_state);
                    free(q);
                    return 0;
                }
            }
        }
    }

    free(visited_state);
    free(q);
    return 1;
}

static int dir_from_mask(int mask)
{
    if (mask == 1) return 0;
    if (mask == 2) return 1;
    if (mask == 4) return 2;
    if (mask == 8) return 3;
    return -1;
}

static int run_nop_norm(const unsigned char *type, int *instrs, int count)
{
    int changes = 0;
    for (int i = 0; i < count; i++)
    {
        if (type[i] == PIXEL_CODE && instrs[i] == INSTR_NONE)
        {
            // normalize unknown op to NOP so emit path stays deterministic
            instrs[i] = INSTR_NOP;
            changes++;
        }
    }
    return changes;
}

static int run_dir_nop(const unsigned char *type, int *instrs, int w, int h)
{
    int count = w * h;
    int changes = 0;
    int *in_mask = (int *)calloc((size_t)count, sizeof(int));
    if (!in_mask) return 0;
    if (!build_in(type, instrs, w, h, in_mask))
    {
        free(in_mask);
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        int instr = instrs[i];
        if (type[i] != PIXEL_CODE || !instr_is_direction_non_skip(instr))
            continue;

        int mask = in_mask[i];
        if (mask == 0 || (mask & (mask - 1)) != 0)
            continue;

        int incoming_dir = dir_from_mask(mask);
        int target_dir = (instr == INSTR_RIGHT) ? 0 : (instr == INSTR_DOWN) ? 1 : (instr == INSTR_LEFT) ? 2 : 3;

        if (incoming_dir == target_dir)
        {
            instrs[i] = INSTR_NOP;
            changes++;
        }
    }

    free(in_mask);
    return changes;
}

static int run_store_load_nop(const unsigned char *type, int *instrs, int w, int h)
{
    int count = w * h;
    int changes = 0;
    int *in_mask = (int *)calloc((size_t)count, sizeof(int));
    if (!in_mask) return 0;

    if (!build_in(type, instrs, w, h, in_mask))
    {
        free(in_mask);
        return 0;
    }

    for (int idx = 0; idx < count; idx++)
    {
        int instr = instrs[idx];
        int expected_load;
        if (type[idx] != PIXEL_CODE) continue;
        if (instr != INSTR_STORE_A && instr != INSTR_STORE_B) continue;

        int dir = dir_from_mask(in_mask[idx]);
        if (dir < 0) continue;

        int x = idx % w;
        int y = idx / w;
        int nx = x;
        int ny = y;
        move_once(&nx, &ny, dir);
        if (!in_bounds(nx, ny, w, h)) continue;

        int nidx = ny * w + nx;
        if (type[nidx] != PIXEL_CODE) continue;

        expected_load = (instr == INSTR_STORE_A) ? INSTR_LOAD_A : INSTR_LOAD_B;
        if (instrs[nidx] != expected_load) continue;

        if (in_mask[nidx] != (1 << dir)) continue;

        instrs[idx] = INSTR_NOP;
        instrs[nidx] = INSTR_NOP;
        changes += 2;
    }

    free(in_mask);
    return changes;
}

static int run_lit_fold(unsigned char *pixel_type, int *pixel_instr, int *pixel_data, int width, int height)
{
    int count = width * height;
    int folds = 0;
    int *in_mask = (int *)calloc((size_t)count, sizeof(int));
    if (!in_mask) return 0;

    if (!build_in(pixel_type, pixel_instr, width, height, in_mask))
    {
        free(in_mask);
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        int d = dir_from_mask(in_mask[i]);
        int x = i % width;
        int y = i / width;
        int x1 = x;
        int y1 = y;
        int x2, y2;
        int i1, i2;
        int op;
        int a, b, out;

        if (d < 0) continue;
        if (pixel_type[i] != PIXEL_DATA) continue;

        move_once(&x1, &y1, d);
        if (!in_bounds(x1, y1, width, height)) continue;
        i1 = y1 * width + x1;
        if (in_mask[i1] != (1 << d)) continue;
        if (pixel_type[i1] != PIXEL_DATA) continue;

        x2 = x1;
        y2 = y1;
        move_once(&x2, &y2, d);
        if (!in_bounds(x2, y2, width, height)) continue;
        i2 = y2 * width + x2;
        if (in_mask[i2] != (1 << d)) continue;
        if (pixel_type[i2] != PIXEL_CODE) continue;

        op = pixel_instr[i2];
        if (op != INSTR_ADD && op != INSTR_SUB && op != INSTR_MUL && op != INSTR_DIV && op != INSTR_MOD)
            continue;

        a = pixel_data[i];
        b = pixel_data[i1];

        // literal fold keeps exact stack order semantics from runtime
        if (op == INSTR_ADD) out = a + b;
        else if (op == INSTR_SUB) out = a - b;
        else if (op == INSTR_MUL) out = a * b;
        else if (op == INSTR_DIV)
        {
            if (b == 0) continue;
            out = a / b;
        }
        else
        {
            if (b == 0) continue;
            out = a % b;
        }

        pixel_data[i] = out;
        pixel_type[i1] = PIXEL_CODE;
        pixel_instr[i1] = INSTR_NOP;
        pixel_data[i1] = 0;
        pixel_instr[i2] = INSTR_NOP;

        folds++;
        i = i2;
    }

    free(in_mask);
    return folds;
}

static int run_reachability_crop(unsigned char **pixel_type, int **pixel_instr, int **pixel_data,
                                 int *width, int *height, OptStats *stats)
{
    int w = *width;
    int h = *height;
    int count = w * h;
    int queue_cap = count * 8;
    // x,y,dir visited to stop infinite loops from cycling programs
    unsigned char *visited_cells = (unsigned char *)calloc((size_t)count, sizeof(unsigned char));
    unsigned char *visited_state = (unsigned char *)calloc((size_t)count * 4, sizeof(unsigned char));
    Node *q = (Node *)malloc((size_t)queue_cap * sizeof(Node));
    int head = 0;
    int tail = 0;

    if (!visited_cells || !visited_state || !q)
    {
        free(visited_cells);
        free(visited_state);
        free(q);
        return 0;
    }

    if (!q_push(q, &tail, queue_cap, 0, 0, 0))
    {
        free(visited_cells);
        free(visited_state);
        free(q);
        return 0;
    }

    while (head < tail)
    {
        Node it = q[head++];
        int x = it.x;
        int y = it.y;
        int dir = it.dir;

        if (!in_bounds(x, y, w, h))
            continue;

        int idx = y * w + x;
        int state_idx = idx * 4 + dir;
        if (visited_state[state_idx])
            continue;

        visited_state[state_idx] = 1;
        visited_cells[idx] = 1;

        if ((*pixel_type)[idx] == PIXEL_HALT || (*pixel_type)[idx] == PIXEL_ERROR)
            continue;

        int next_dir = dir;
        int instr = (*pixel_instr)[idx];

        if ((*pixel_type)[idx] == PIXEL_CODE)
            next_dir = next_dir_for_instr(dir, instr);

        int nx = x;
        int ny = y;
        move_once(&nx, &ny, next_dir);
        if (!in_bounds(nx, ny, w, h))
            continue;

        int sx = nx;
        int sy = ny;
        if ((*pixel_type)[idx] == PIXEL_CODE && instr_is_skip(instr))
        {
            move_once(&sx, &sy, next_dir);
            if (!in_bounds(sx, sy, w, h))
                continue;
        }

        if (!q_push(q, &tail, queue_cap, sx, sy, next_dir))
        {
            free(visited_cells);
            free(visited_state);
            free(q);
            return 0;
        }

        if ((*pixel_type)[idx] == PIXEL_CODE && (instr == INSTR_JZ || instr == INSTR_JNZ))
        {
            // static reachability explores both branch outcomes conservatively
            int cx = sx;
            int cy = sy;
            move_once(&cx, &cy, next_dir);
            if (in_bounds(cx, cy, w, h))
            {
                if (!q_push(q, &tail, queue_cap, cx, cy, next_dir))
                {
                    free(visited_cells);
                    free(visited_state);
                    free(q);
                    return 0;
                }
            }
        }
    }

    int min_x = w;
    int min_y = h;
    int max_x = -1;
    int max_y = -1;
    int removed = 0;

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int idx = y * w + x;
            if (visited_cells[idx])
            {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
            else
            {
                removed++;
            }
        }
    }

    if (max_x < min_x || max_y < min_y)
    {
        free(visited_cells);
        free(visited_state);
        free(q);
        return 1;
    }

    int new_w = max_x - min_x + 1;
    int new_h = max_y - min_y + 1;
    int new_count = new_w * new_h;
    // crop only bounding box of reachable cells so output C becomes smaller
    unsigned char *new_type = (unsigned char *)malloc((size_t)new_count * sizeof(unsigned char));
    int *new_instr = (int *)malloc((size_t)new_count * sizeof(int));
    int *new_data = (int *)malloc((size_t)new_count * sizeof(int));

    if (!new_type || !new_instr || !new_data)
    {
        free(visited_cells);
        free(visited_state);
        free(q);
        free(new_type);
        free(new_instr);
        free(new_data);
        return 0;
    }

    for (int y = 0; y < new_h; y++)
    {
        for (int x = 0; x < new_w; x++)
        {
            int old_x = min_x + x;
            int old_y = min_y + y;
            int old_idx = old_y * w + old_x;
            int new_idx = y * new_w + x;
            new_type[new_idx] = (*pixel_type)[old_idx];
            new_instr[new_idx] = (*pixel_instr)[old_idx];
            new_data[new_idx] = (*pixel_data)[old_idx];
        }
    }

    free(*pixel_type);
    free(*pixel_instr);
    free(*pixel_data);

    *pixel_type = new_type;
    *pixel_instr = new_instr;
    *pixel_data = new_data;
    *width = new_w;
    *height = new_h;

    if (stats)
        stats->removed_cells += removed;

    free(visited_cells);
    free(visited_state);
    free(q);
    return 1;
}

int optimize_decoded_program(unsigned char **pixel_type, int **pixel_instr, int **pixel_data,
                             int *width, int *height, const OptConfig *config, OptStats *stats)
{
    if (!pixel_type || !pixel_instr || !pixel_data || !width || !height)
        return 0;

    if (stats)
    {
        memset(stats, 0, sizeof(*stats));
        stats->width_before = *width;
        stats->height_before = *height;
        stats->width_after = *width;
        stats->height_after = *height;
    }

    if (!config || !config->enabled)
        return 1;

    if (config->level <= 0)
        return 1;

    int count = (*width) * (*height);
    // pass 1 is cheap normalization pass before structural changes
    int can_nops = run_nop_norm(*pixel_type, *pixel_instr, count);
    if (stats)
    {
        stats->passes_run += 1;
        stats->changes += can_nops;
        stats->can_nops += can_nops;
    }

    // pass 2 turns redundant direction writes into nop on unique path
    int can_dirs = run_dir_nop(*pixel_type, *pixel_instr, *width, *height);
    if (stats)
    {
        stats->passes_run += 1;
        stats->changes += can_dirs;
        stats->can_dirs += can_dirs;
    }

    if (config->level >= 2)
    {
        // level 2 runs extra conservative rounds to converge further if possible
        for (int round = 0; round < 3; round++)
        {
            int extra_dirs = run_dir_nop(*pixel_type, *pixel_instr, *width, *height);
            if (stats)
            {
                stats->passes_run += 1;
                stats->changes += extra_dirs;
                stats->can_dirs += extra_dirs;
            }

            if (extra_dirs == 0)
                break;
        }

        // eliminate immediate STORE_X -> LOAD_X roundtrips when path is uniquely linear
        int roundtrip_changes = run_store_load_nop(*pixel_type, *pixel_instr, *width, *height);
        if (stats)
        {
            stats->passes_run += 1;
            stats->changes += roundtrip_changes;
            stats->can_nops += roundtrip_changes;
        }

        // fold DATA DATA OP into single DATA when path is uniquely linear
        int lit = run_lit_fold(*pixel_type, *pixel_instr, *pixel_data, *width, *height);
        if (stats)
        {
            stats->passes_run += 1;
            stats->changes += lit;
            stats->lit_ops += lit;
        }
    }

    int before_w = *width;
    int before_h = *height;
    // pass 3 removes unreachable border area by reachability crop
    if (!run_reachability_crop(pixel_type, pixel_instr, pixel_data, width, height, stats))
        return 0;

    if (stats)
    {
        stats->passes_run += 1;
        if (*width != before_w || *height != before_h)
            stats->changes += 1;
        stats->width_after = *width;
        stats->height_after = *height;
    }

    return 1;
}
