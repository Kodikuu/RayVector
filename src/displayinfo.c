#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "common.h"
#include "displayinfo.h"


// Struct Management funcitions

static void resize_display_array(struct display_list **displays_in, uint16_t newSize){
	struct display_list *disps = *displays_in;

	if(disps->disp_array) {
		free(disps->disp_array);
	}

	disps->array_size = newSize;

	disps->disp_array = calloc((disps->array_size), sizeof(struct display));
}


static void destroy_display_info(struct display_list **displays_in){
	if(!*displays_in){
		return;
	}

	struct display_list *displays = *displays_in;

	if(displays->disp_array) {
		free(displays->disp_array);
	}
	
	free(displays);

	*displays_in = NULL;
}


static int32_t init_displays(struct display_list **displays_out){
	uint32_t e = 0;
	destroy_display_info(displays_out);
	struct display_list *displays = *displays_out = calloc (1, sizeof(struct display_list));

	displays->disp_count = 0;
	displays->array_size = 0;
	displays->disp_array = NULL;
	return e;
}


uint32_t destroy_bounds(struct bounds **bnds_in){
	if(!*bnds_in){
		return 1;
	}

	struct bounds *bnds = *bnds_in;

	if(bnds->displays){
		destroy_display_info(&(bnds->displays));
	}

	free(bnds);

	*bnds_in = NULL;
	return 0;
}


uint32_t init_bounds(struct bounds **bnds_out){
	uint32_t e = 0;
	destroy_bounds(bnds_out);
	struct bounds *bnds = *bnds_out = calloc(1, sizeof(struct bounds));

	// Overall boundries
	bnds->x_min = 0;
	bnds->x_max = 0;
	bnds->y_min = 0;
	bnds->y_max = 0;
	bnds->width = 0;
	bnds->height = 0;

	// init displays
	init_displays(&(bnds->displays));

	return e;
}

// General display info functions

static void save_monitor_information(const DISPLAY_DEVICEA md, struct display *disp)
{
	DEVMODEA dm = {0};
	dm.dmSize = sizeof(DEVMODEA);
	if (EnumDisplaySettingsA(md.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
		disp->width = dm.dmPelsWidth;
		disp->height = dm.dmPelsHeight;
		disp->abs_x = dm.dmPosition.x;
		disp->abs_y = dm.dmPosition.y;
	}
}


static uint16_t countDisplays(){
	char cur_dev_id[128] = {0};
	uint16_t monitor_count = 0;

	DISPLAY_DEVICEA dd = {0};
	dd.cb = sizeof(DISPLAY_DEVICEA);
	for (int32_t di = 0; EnumDisplayDevicesA(NULL, di, &dd, 0); di++)
	{
		if (strcmp(dd.DeviceID, cur_dev_id))
		{
			snprintf(cur_dev_id, 128, "%s", dd.DeviceID);
		}

		DISPLAY_DEVICEA md = {0};
		md.cb = sizeof(DISPLAY_DEVICEA);
		if (EnumDisplayDevicesA(dd.DeviceName, 0, &md, 0))
		{
			if(md.StateFlags & DISPLAY_DEVICE_ACTIVE) // needs EnumDisplayDevicesA from previous line to be done first
			{
				monitor_count++;
			}
		}
	}
	return monitor_count;
}


static void get_display_info(struct display_list *disps)
{
	uint16_t numDisplays = countDisplays();

	if(numDisplays != disps->disp_count){
		if(numDisplays > disps->array_size){
			resize_display_array(&disps, numDisplays);
		}
		disps->disp_count = numDisplays;
	}

	char cur_dev_id[128] = {0};
	uint16_t monitor_count = 0;

	// Loop through GPU's and Displays
	DISPLAY_DEVICEA dd = {0};
	dd.cb = sizeof(DISPLAY_DEVICEA);
	for (int32_t di = 0; EnumDisplayDevicesA(NULL, di, &dd, 0) && (monitor_count < disps->disp_count); di++)
	{
		if (strcmp(dd.DeviceID, cur_dev_id))
		{
			snprintf(cur_dev_id, 128, "%s", dd.DeviceID);
		}

		DISPLAY_DEVICEA md = {0};
		md.cb = sizeof(DISPLAY_DEVICEA);
		if (EnumDisplayDevicesA(dd.DeviceName, 0, &md, 0))
		{
			if(md.StateFlags & DISPLAY_DEVICE_ACTIVE) // needs EnumDisplayDevicesA from previous line to be done first
			{
				save_monitor_information(dd, &(disps->disp_array[monitor_count++]));
			}
		}
	}
}


void findBounds(struct bounds *bnds){ 
	get_display_info(bnds->displays);

	bnds->x_min = 0;
	bnds->x_max = 0;
	bnds->y_min = 0;
	bnds->y_max = 0;

	for(size_t i = 0; i < bnds->displays->disp_count; i++){
		if (bnds->displays->disp_array[i].abs_x < bnds->x_min){
			bnds->x_min = bnds->displays->disp_array[i].abs_x;
		}

		if(bnds->displays->disp_array[i].abs_y < bnds->y_min){
			bnds->y_min = bnds->displays->disp_array[i].abs_y;
		}

		int32_t rightPos = bnds->displays->disp_array[i].abs_x + bnds->displays->disp_array[i].width;
		if(bnds->x_max < rightPos){
			bnds->x_max = rightPos;
		}

		int32_t bottomPos = bnds->displays->disp_array[i].abs_y + bnds->displays->disp_array[i].height;
		if(bnds->y_max < bottomPos){
			bnds->y_max = bottomPos;
		}
	}

	// Calculate new width and height
	bnds->width = bnds->x_max - bnds->x_min;
	bnds->height = bnds->y_max - bnds->y_min;

	// Calculate Relative positions of displays (setting top left of everything to 0,0)
	for(size_t i = 0; i < bnds->displays->disp_count; i++){
		bnds->displays->disp_array[i].rel_x = abs(bnds->x_min - bnds->displays->disp_array[i].abs_x);
		bnds->displays->disp_array[i].rel_y = abs(bnds->y_min - bnds->displays->disp_array[i].abs_y);
	}
}