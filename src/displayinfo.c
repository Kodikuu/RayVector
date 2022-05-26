#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "common.h"
#include "displayinfo.h"

//#define DEBUG false
//#if DEBUG
//	#define DEBUG_PRINT(x) printf_s(x)
//#else
//	#define DEBUG_PRINT(x)
//#endif

// Struct Management funcitions

static void resize_display_array(struct display_list **displays_in, uint16_t newSize){
	//DEBUG_PRINT("||DEBUG|| Resizing Display Array\n");

	struct display_list *disps = *displays_in;

	if(disps->disp_array) {
		free(disps->disp_array);
	}

	disps->array_size = newSize;

	disps->disp_array = calloc((disps->array_size), sizeof(struct display));
}


static void destroy_display_info(struct display_list **displays_in){
	//DEBUG_PRINT("||DEBUG|| Destroying Display Info\n");
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
	//DEBUG_PRINT("||DEBUG|| Initializing Displays\n");
	uint32_t e = 0;
	destroy_display_info(displays_out);
	struct display_list *displays = *displays_out = calloc (1, sizeof(struct display_list));

	displays->disp_count = 0;
	displays->array_size = 0;
	displays->disp_array = NULL;
	return e;
}


uint32_t destroy_bounds(struct bounds **bnds_in){
	//DEBUG_PRINT("||DEBUG|| Destroying Boundries\n");
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
	//DEBUG_PRINT("||DEBUG|| Initializing Boundries\n");
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
	//DEBUG_PRINT("||DEBUG|| Saving Monitor Info\n");
	DEVMODEA dm = {0};
	dm.dmSize = sizeof(DEVMODEA);
	if (EnumDisplaySettingsA(md.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
		//printf_s("    -Resolution:   %ldx%ld\r\n", dm.dmPelsWidth, dm.dmPelsHeight);
		disp->width = dm.dmPelsWidth;
		disp->height = dm.dmPelsHeight;
		//printf_s("    -Refresh Rate: %ldHz\r\n", dm.dmDisplayFrequency);
		//printf_s("    -Position:     [%ld %ld]\r\n", dm.dmPosition.x, dm.dmPosition.y);
		disp->abs_x = dm.dmPosition.x;
		disp->abs_y = dm.dmPosition.y;
	}
}


static uint16_t countDisplays(){
	//DEBUG_PRINT("||DEBUG|| Counting Displays\n");
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
	//if(DEBUG) printf_s("||DEBUG|| %u Displays Found\n", monitor_count);
	return monitor_count;
}


static void get_display_info(struct display_list *disps)
{
	//DEBUG_PRINT("||DEBUG|| Getting Display info\n");
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
			//if (cur_dev_id[0] != '\0') printf_s("\r\n");
			//monitor_count = 0;
			//printf_s("GPU %d\r\n", gpu_count++);
			//print_adapter_information(dd);
		}

		DISPLAY_DEVICEA md = {0};
		md.cb = sizeof(DISPLAY_DEVICEA);
		if (EnumDisplayDevicesA(dd.DeviceName, 0, &md, 0))
		{
			if(md.StateFlags & DISPLAY_DEVICE_ACTIVE) // needs EnumDisplayDevicesA from previous line to be done first
			{
				save_monitor_information(dd, &(disps->disp_array[monitor_count++]));
				//if (md.DeviceString[0] == '\0') {
				//	printf_s("    -Name:         Generic PnP Monitor\r\n");
				//} else {
				//	printf_s("    -Name:         %s\r\n", md.DeviceString);
				//}
				//if(DEBUG) printf_s("\r\n    Monitor %d\r\n", monitor_count);
			}
		}
	}
}


void findBounds(struct bounds *bnds){ 
	//DEBUG_PRINT("||DEBUG|| Finding Boundries\n");
	get_display_info(bnds->displays);

	bnds->x_min = 0;
	bnds->x_max = 0;
	bnds->y_min = 0;
	bnds->y_max = 0;

	for(size_t i = 0; i < bnds->displays->disp_count; i++){
		//if(DEBUG) printf_s("||DEBUG|| Getting details on monitor %u:\n", i);
		//if(DEBUG) printf_s("||DEBUG|| Monitor %u Location: %dx%d\n", i, bnds->displays->disp_array[i].abs_x, bnds->displays->disp_array[i].abs_y);

		if (bnds->displays->disp_array[i].abs_x < bnds->x_min){
			//printf_s("\t||DEBUG|| Absolute x position = %d; x_min = %d \n", bnds->displays->disp_array[i].abs_x, bnds->x_min);
			bnds->x_min = bnds->displays->disp_array[i].abs_x;
		}

		if(bnds->displays->disp_array[i].abs_y < bnds->y_min){
			//printf_s("\t||DEBUG|| Absolute y position = %d; y_min = %d \n", bnds->displays->disp_array[i].abs_y, bnds->y_min);
			bnds->y_min = bnds->displays->disp_array[i].abs_y;
		}

		int32_t rightPos = bnds->displays->disp_array[i].abs_x + bnds->displays->disp_array[i].width;
		if(bnds->x_max < rightPos){
			//printf_s("\t||DEBUG|| rightPos = %d; x_max = %d \n", rightPos, bnds->x_max);
			bnds->x_max = rightPos;
		}

		int32_t bottomPos = bnds->displays->disp_array[i].abs_y + bnds->displays->disp_array[i].height;
		if(bnds->y_max < bottomPos){
			//printf_s("\t||DEBUG|| bottomPos = %d; y_max = %d \n", bottomPos, bnds->y_max);
			bnds->y_max = bottomPos;
		}
		//if(DEBUG) printf_s("||DEBUG|| Monitor %u Resolution: %dx%d\n", i, GetMonitorWidth(i), GetMonitorHeight(i));
	}

	// Calculate new width and height
	bnds->width = bnds->x_max - bnds->x_min;
	bnds->height = bnds->y_max - bnds->y_min;

	// Calculate Relative positions of displays (setting top left of everything to 0,0)
	for(size_t i = 0; i < bnds->displays->disp_count; i++){
		bnds->displays->disp_array[i].rel_x = abs(bnds->x_min - bnds->displays->disp_array[i].abs_x);
		bnds->displays->disp_array[i].rel_y = abs(bnds->y_min - bnds->displays->disp_array[i].abs_y);
	}

	//if(DEBUG) printf_s("||DEBUG|| Bounds Values:\n\tx_min = %d\n\tx_max = %d\n\ty_min = %d\n\ty_max = %d\n\twidth = %u\n\theight = %u\n", bnds->x_min, bnds->x_max, bnds->y_min, bnds->y_max, bnds->width, bnds->height);
}