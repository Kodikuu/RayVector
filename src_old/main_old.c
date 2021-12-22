#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"
#include "matoya.h"

#include "winwrap.h"

#define FPS 165
#define VECTORCOL   CLITERAL(Color){ 38, 37, 36, 255 }   // My own White (raylib logo)

struct visualiser {
    uint32_t bands;
    float height;
    float width;
    float position_x;
    float position_y;
    float *data;

    Color colour;
    float alpha;
};

struct context {
    uint32_t scr_width;
    uint32_t scr_height;

    uint32_t vis_count;
    struct visualiser *vis;

    struct audio_device *device;
    void *audio_buffer;
    void *audio_buffer_tmp;
};

void auto_resize(uint32_t *width, uint32_t *height) {
    uint32_t monitor = GetCurrentMonitor();
    uint32_t new_width = GetMonitorWidth(monitor);
    uint32_t new_height = GetMonitorHeight(monitor);

    bool changed = false;
    if (new_width != *width) {
        *width = new_width;
        changed = true;
    }
    if (new_height != *height) {
        *height = new_height;
        changed = true;
    }
    if (changed) {
        SetWindowSize(*width, *height+1);
        SetWindowPosition(0, 0);
    }
}

void draw_vis(struct visualiser vis, uint32_t scr_width, uint32_t scr_height) {
    float start_y = (vis.position_y * scr_height) - 30.0f;
    float width_step = (float) scr_width / (float) (vis.bands - 1);

    
    for (uint32_t i=0; i < vis.bands-1; i++) {
        float min_top = min(vis.data[i], vis.data[i+1]) * vis.height;

        // Cap
        Vector2 p1 = {width_step*(i+1), start_y - floorf(vis.data[i + 1]*vis.height)};
        Vector2 p2 = {width_step*i, start_y - floorf(vis.data[i]*vis.height)};
        Vector2 p3;
        p3.y = start_y;
        if (vis.data[i] <= vis.data[i+1]) {
            p3.x = width_step*(i+1);
        } else {
            p3.x = width_step*(i);
        }
        DrawTriangle(p1, p2, p3, VECTORCOL);

        // Base
        Rectangle rect = {
            width_step*i, start_y - min_top,
            width_step, min_top
        };
        DrawRectangleRec(rect, VECTORCOL);
    }
    return;
}

void draw_main(void *opaque) {
    struct context *ctx = (struct context *) opaque;

    BeginDrawing();
    ClearBackground(BLANK);

    for (uint32_t i=0; i<ctx->vis_count; i++) {
        draw_vis(ctx->vis[i], ctx->scr_width, ctx->scr_height);
    }

    EndDrawing();
}

void calc_main(void *opaque) {
    struct context *ctx = (struct context *) opaque;

    while (!WindowShouldClose()) {
        for (uint32_t i=0; i<ctx->vis_count; i++) {
            for (uint32_t j=0; j<ctx->vis[i].bands; j++) {
                ctx->vis[i].data[j] = 0.5 * (1 + sinf((GetTime()*-8) + (j*2)));
            }
        }

        uint32_t frames = 0;
        GetAudioSize(ctx->device, &frames);
        if (frames) {
            void *buffer = NULL;
            if (IsAudioFloat(ctx->device)) {
                buffer = calloc(frames, sizeof(float));
                GetAudioBuffer(ctx->device, buffer, &frames);

                BufferShiftAppend(ctx->audio_buffer, 4800 * sizeof(float), buffer, frames * sizeof(float));
            } else {
                buffer = calloc(frames, sizeof(uint32_t));
                GetAudioBuffer(ctx->device, buffer, &frames);

                BufferShiftAppend(ctx->audio_buffer, 4800 * sizeof(uint32_t), buffer, frames * sizeof(uint32_t));
            }
            free(buffer);

            // FFT

            // Bin

        }
        
        WaitTime(5);
    }

    return;
}

uint32_t main(void)
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, NULL);
    SetTargetFPS(FPS);
    SetFlags(GetWindowHandle());

    struct context *ctx = calloc(1, sizeof(struct context));
    ctx->vis_count = 1;

    ctx->vis = calloc(ctx->vis_count, sizeof(struct visualiser));
    ctx->vis[0].bands = 24;
    ctx->vis[0].height = 100.0f;
    ctx->vis[0].width = 1.0f;
    ctx->vis[0].position_x = 0.0f;
    ctx->vis[0].position_y = 1.0f;
    ctx->vis[0].data = calloc(ctx->vis[0].bands, sizeof(float));

    uint32_t err = InitAudio(&ctx->device);
    if (err != 0) {
        printf("Error 0x%x\n", err);
        exit(err);
    }

    if (IsAudioFloat(ctx->device)) {
        ctx->audio_buffer = calloc(4800, sizeof(float));
        printf("I am a float\n");
    } else {
        ctx->audio_buffer = calloc(4800, sizeof(uint32_t));
    }

    MTY_Thread *work_thread = MTY_ThreadCreate((MTY_ThreadFunc) calc_main, ctx);
    uint32_t step = 0;
    
    while (!WindowShouldClose())
    {
        if (!step) {
            auto_resize(&ctx->scr_width, &ctx->scr_height);
        }
        step = (step+1) % FPS;

        draw_main(ctx);
    }
    
    MTY_ThreadDestroy(&work_thread);

    FreeAudio(&ctx->device);

    for (uint32_t i=0; i<ctx->vis_count; i++) {
        free(ctx->vis[i].data);
    }
    free(ctx->vis);
    free(ctx->audio_buffer);
    free(ctx);

    CloseWindow();

    return 0;
}

int32_t WinMain(void)
{
	return main();
}