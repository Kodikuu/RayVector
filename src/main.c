#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#define Color Color

#include "common.h"

#include "audio.h"
void SetFlags(void *handle);

static void destroy_all(struct context **ctx_in)
{

	if (!*ctx_in)
		return;
	
	struct context *ctx = *ctx_in;

	MTY_MutexDestroy(&ctx->lock);

	// Audio Processing
	if (ctx->processing) {
		free(ctx->processing);
	}
	
	// Visualisers
	if (ctx->vis_array) {
		for (uint32_t i = 0; i < ctx->vis_count; i++) {
			free(ctx->vis_array[i].band_data);
		}

		free(ctx->vis_array);
	}

	// Context
	free(ctx);

	*ctx_in = NULL;
}

static uint32_t init_all(struct context **ctx_out)
{
	uint32_t e = 0;

	destroy_all(ctx_out);

	// Context
	struct context *ctx = *ctx_out = calloc(1, sizeof(struct context));
	
	// Visualisers
	ctx->vis_count = 4;
	ctx->vis_array = calloc(ctx->vis_count, sizeof(struct visualiser));

	// Settings

	uint32_t heights[4] = {150, 160, 170, 180};
	uint32_t bands[4] = {65, 33, 17, 9};

	Color colours[4] = {
		{38, 37, 36, 255},
		{38, 37, 36, 191},
		{38, 37, 36, 127},
		{38, 37, 36, 63},
	};
	
	for (uint32_t i = 0; i < ctx->vis_count ; i++) {
		ctx->vis_array[i].colour = colours[i];
		ctx->vis_array[i].height = heights[i];
		ctx->vis_array[i].bands = bands[i];
	
		ctx->vis_array[i].width = 2560;
		ctx->vis_array[i].position_x = 0;
		ctx->vis_array[i].position_y = 1410;
		ctx->vis_array[i].freq_min = 20;
		ctx->vis_array[i].freq_max = 20000;
		ctx->vis_array[i].sensitivity = 35;
		calc_band_freqs(&ctx->vis_array[i]);
		ctx->vis_array[i].band_data = calloc(ctx->vis_array[i].bands, sizeof(float));
	}

	// Audio Processing
	ctx->processing = calloc(1, sizeof(struct audio_processing));
	ctx->processing->fft_ms = 100;

	ctx->lock = MTY_MutexCreate();

	return e;
}

void auto_resize()
{
	uint32_t monitor = GetCurrentMonitor();
	uint32_t new_width = GetMonitorWidth(monitor);
	uint32_t new_height = GetMonitorHeight(monitor);

	uint32_t height = GetScreenHeight();
	uint32_t width = GetScreenWidth();

	if (width != new_width || height != new_height) {
		SetWindowSize(new_width, new_height);
		SetWindowPosition(0, 0);
	}
}

void draw_vis(struct visualiser vis)
{
	float y0 = (float) vis.position_y;
	float x_step = (float) vis.width / (float) (vis.bands - 1);
	
	for (uint32_t i = 0; i < vis.bands-1; i++) {
		float x1 = x_step * i;
		float x2 = x1 + x_step;

		float y1 = y0 - floorf(vis.band_data[i] * vis.height);
		float y2 = y0 - floorf(vis.band_data[i + 1] * vis.height);

		// Cap
		Vector2 p1 = {x2, y2};
		Vector2 p2 = {x1, y1};
		Vector2 p3 = {x1, y0};
		DrawTriangle(p1, p2, p3, vis.colour);

		// Base
		Vector2 p4 = {x2, y2};
		Vector2 p5 = {x1, y0};
		Vector2 p6 = {x2, y0};
		DrawTriangle(p4, p5, p6, vis.colour);
	}
	return;
}

void draw_main(void *opaque)
{
	struct context *ctx = (struct context *) opaque;

	BeginDrawing();
	ClearBackground(BLANK);

	MTY_MutexLock(ctx->lock);
	uint32_t i = ctx->vis_count;
	while (i--) {
		draw_vis(ctx->vis_array[i]);
	}
	MTY_MutexUnlock(ctx->lock);

	EndDrawing();
}


uint32_t main(void)
{
	struct context *ctx = NULL;
	init_all(&ctx);
	ctx->running = true;
	MTY_Thread *thread = MTY_ThreadCreate((MTY_ThreadFunc) work_thread, ctx);

	SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_UNFOCUSED);
	InitWindow(1, 1, "RayVector");
	SetFlags(GetWindowHandle());

	MTY_Time time = 0;
	while (ctx->running)
	{
		MTY_Time new_time = MTY_GetTime();
		if (MTY_TimeDiff(time, new_time) > 1000.0f) {
			auto_resize();
			time = new_time;
		}

		draw_main(ctx);
		ctx->running = !WindowShouldClose();
	}

	while (ctx->processing->work_running) {
		WaitTime(1);
	}
	MTY_ThreadDestroy(&thread);
	destroy_all(&ctx);

	return 0;
}

int32_t WinMain(void)
{
	return main();
}
