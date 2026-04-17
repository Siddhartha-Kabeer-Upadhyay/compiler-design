#include "opt.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    int x;
    int y;
    int dir;
} WorkItem;

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

static int push_item(WorkItem *queue, int *tail, int cap, int x, int y, int dir)
{
    if (*tail >= cap) return 0;
    queue[*tail].x = x;
    queue[*tail].y = y;
    queue[*tail].dir = dir;
    (*tail)++;
    return 1;
}

static int run_nop_canonicalization(const unsigned char *pixel_type, int *pixel_instr, int count)
{
    int changes = 0;
    for (int i = 0; i < count; i++)
    {
        if (pixel_type[i] == PIXEL_CODE && pixel_instr[i] == INSTR_NONE)
        {
            // normalize unknown op to NOP so emit path stays deterministic
            pixel_instr[i] = INSTR_NOP;
            changes++;
        }
    }
    return changes;
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
    WorkItem *queue = (WorkItem *)malloc((size_t)queue_cap * sizeof(WorkItem));
    int head = 0;
    int tail = 0;

    if (!visited_cells || !visited_state || !queue)
    {
        free(visited_cells);
        free(visited_state);
        free(queue);
        return 0;
    }

    if (!push_item(queue, &tail, queue_cap, 0, 0, 0))
    {
        free(visited_cells);
        free(visited_state);
        free(queue);
        return 0;
    }

    while (head < tail)
    {
        WorkItem it = queue[head++];
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

        if (!push_item(queue, &tail, queue_cap, sx, sy, next_dir))
        {
            free(visited_cells);
            free(visited_state);
            free(queue);
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
                if (!push_item(queue, &tail, queue_cap, cx, cy, next_dir))
                {
                    free(visited_cells);
                    free(visited_state);
                    free(queue);
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
        free(queue);
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
        free(queue);
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
    free(queue);
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
    int canonicalized = run_nop_canonicalization(*pixel_type, *pixel_instr, count);
    if (stats)
    {
        stats->passes_run += 1;
        stats->changes += canonicalized;
        stats->canonicalized_nops += canonicalized;
    }

    int before_w = *width;
    int before_h = *height;
    // pass 2 removes unreachable border area by reachability crop
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
