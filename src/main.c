#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#define Color Color

#include "common.h"

#include "displayinfo.h"

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


void auto_resize(struct bounds *bnds)
{
	// Get the updated Window and Position
	findBounds(bnds);
	uint32_t new_width = bnds->width; 
	uint32_t new_height = bnds->height;

	int32_t new_x_position = bnds->x_min;
	int32_t new_y_position = bnds->y_min;

	// Get the Current Window's resolution & position
	uint32_t height = GetScreenHeight();
	uint32_t width = GetScreenWidth();
	Vector2 position = GetWindowPosition();

	if (width != new_width || height != new_height || (int32_t) position.x != new_x_position || (int32_t) position.y != new_y_position)
	{
		SetWindowSize(new_width, new_height);
		SetWindowPosition(bnds->x_min, bnds->y_min);
	}
}


void draw_vis(struct visualiser vis, struct display disp)// TODO - ACCOUNT FOR TASKBAR HERE
{
	float y0 = (float) disp.rel_y + disp.height;
	float x0 = (float) disp.rel_x;
	float x_step = (float) disp.width / (float) (vis.bands - 1);
	
	for (uint32_t i = 0; i < vis.bands-1; i++) {
		float x1 = x0 + (x_step * i);
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


void draw_main(void *opaque, struct display_list *disps)
{
	struct context *ctx = (struct context *) opaque;

	BeginDrawing();
	ClearBackground(BLANK);

	MTY_MutexLock(ctx->lock);
	for(size_t d = 0; d < disps->disp_count; d++){
		uint32_t i = ctx->vis_count;
		while (i--) {
			draw_vis(ctx->vis_array[i], disps->disp_array[d]);
		}
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

	struct bounds *bnds = NULL;
	init_bounds(&bnds);

	SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_UNFOCUSED);
	InitWindow(1, 1, "RayVector");
	SetFlags(GetWindowHandle());

	findBounds(bnds);

	MTY_Time time = 0;
	while (ctx->running)
	{
		MTY_Time new_time = MTY_GetTime();
		if (MTY_TimeDiff(time, new_time) > 1000.0f) {
			auto_resize(bnds);
			time = new_time;
		}

		draw_main(ctx, bnds->displays);
		ctx->running = !WindowShouldClose();
	}

	while (ctx->processing->work_running) {
		WaitTime(1);
	}
	MTY_ThreadDestroy(&thread);
	destroy_all(&ctx);
	destroy_bounds(&bnds);

	return 0;
}


int32_t WinMain(void)
{
	return main();
}
