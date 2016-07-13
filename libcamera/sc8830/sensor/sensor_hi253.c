/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include <utils/Timers.h>


#define HI253_I2C_ADDR_W                        0x20
#define HI253_I2C_ADDR_R                        0x20

static uint32_t set_hi253_ae_enable(uint32_t enable);
static uint32_t set_hi253_preview_mode(uint32_t preview_mode);
static uint32_t _hi253_Power_On(uint32_t power_on);
static uint32_t HI253_Identify(uint32_t param);
static uint32_t HI253_Before_Snapshot(uint32_t param);
static uint32_t HI253_After_Snapshot(uint32_t param);
static uint32_t HI253_set_brightness(uint32_t level);
static uint32_t HI253_set_contrast(uint32_t level);
static uint32_t HI253_set_image_effect(uint32_t effect_type);
static uint32_t HI253_set_work_mode(uint32_t mode);
static uint32_t HI253_set_whitebalance_mode(uint32_t mode);
static uint32_t set_hi253_video_mode(uint32_t mode);
static uint32_t set_hi253_ev(uint32_t level);
static uint32_t _hi253_GetResolutionTrimTab(uint32_t param);
static uint32_t HI253_flash(uint32_t param);
static uint32_t  g_flash_mode_en = 0;

static void    HI253_Write_Group_Regs( SENSOR_REG_T* sensor_reg_ptr );



const SENSOR_REG_T HI253_YUV_COMMON[] = {
	{0x01, 0x79}, //sleep on
	{0x08, 0x0f}, //Hi-Z on
	{0x01, 0x78}, //sleep off

	{0x03, 0x00}, // Dummy 750us START
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00}, // Dummy 750us END

	{0x0e, 0x03}, //PLL On
	{0x0e, 0x73}, //PLLx2

	{0x03, 0x00}, // Dummy 750us START
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00}, // Dummy 750us END

	{0x0e, 0x00}, //PLL off
	{0x01, 0x71}, //sleep on
	{0x08, 0x00}, //Hi-Z off

	{0x01, 0x73},
	{0x01, 0x71},

	// PAGE 20
	{0x03, 0x20}, //page 20
	{0x10, 0x1c}, //ae off

	// PAGE 22
	{0x03, 0x22}, //page 22
	{0x10, 0x69}, //awb off

	//Initial Start
	// PAGE 0 START //
	{0x03, 0x00},
	{0x10, 0x10}, // Sub1/2_Preview2 Mode_H binning
	{0x11, 0x93}, // aiden: old 0x90
	{0x12, 0x04},

	{0x0b, 0xaa}, // ESD Check Register
	{0x0c, 0xaa}, // ESD Check Register
	{0x0d, 0xaa}, // ESD Check Register

	{0x20, 0x00}, // Windowing start point Y
	{0x21, 0x04},
	{0x22, 0x00}, // Windowing start point X
	{0x23, 0x07},

	{0x24, 0x04},
	{0x25, 0xb0},
	{0x26, 0x06},
	{0x27, 0x40}, // WINROW END

	{0x40, 0x01}, //Hblank 360
	{0x41, 0x68},
	{0x42, 0x00},
	{0x43, 0x14},

	{0x45, 0x04},
	{0x46, 0x18},
	{0x47, 0xd8},

	//BLC
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x00},
	{0x84, 0x0c},
	{0x85, 0x00},
	{0x90, 0x0c}, //BLC_TIME_TH_ON
	{0x91, 0x0c}, //BLC_TIME_TH_OFF
	{0x92, 0x98}, //BLC_AG_TH_ON
	{0x93, 0x90}, //BLC_AG_TH_OFF

	//{0x92, 0xd8}, // BLCAGTH ON
	//{0x93, 0xd0}, // BLCAGTH OFF
	{0x94, 0x75},
	{0x95, 0x70},
	{0x96, 0xdc},
	{0x97, 0xfe},
	{0x98, 0x38},

	//OutDoor BLC
	{0x99,0x43},
	{0x9a,0x43},
	{0x9b,0x43},
	{0x9c,0x43},

	//Dark BLC
	{0xa0, 0x40},
	{0xa2, 0x40},
	{0xa4, 0x40},
	{0xa6, 0x40},

	//Normal BLC
	{0xa8, 0x45},
	{0xaa, 0x45},
	{0xac, 0x45},
	{0xae, 0x45},

	{0x03, 0x02},
	{0x12, 0x03},
	{0x13, 0x03},
	{0x16, 0x00},
	{0x17, 0x8C},
	{0x18, 0x4c},
	{0x19, 0x00},
	{0x1a, 0x39},
	{0x1c, 0x09},
	{0x1d, 0x40},
	{0x1e, 0x30},
	{0x1f, 0x10},
	{0x20, 0x77},
	{0x21, 0xde},
	{0x22, 0xa7},
	{0x23, 0x30},
	{0x27, 0x3c},
	{0x2b, 0x80},
	{0x2e, 0x00},
	{0x2f, 0x00},
	{0x30, 0x05},
	{0x50, 0x20},
	{0x52, 0x01},
	{0x53, 0xc1},
	{0x55, 0x1c},
	{0x56, 0x11},
	{0x5d, 0xA2},
	{0x5e, 0x5a},
	{0x60, 0x87},
	{0x61, 0x99},
	{0x62, 0x88},
	{0x63, 0x97},
	{0x64, 0x88},
	{0x65, 0x97},
	{0x67, 0x0c},
	{0x68, 0x0c},
	{0x69, 0x0c},
	{0x72, 0x89},
	{0x73, 0x96},
	{0x74, 0x89},
	{0x75, 0x96},
	{0x76, 0x89},
	{0x77, 0x96},
	{0x7c, 0x85},
	{0x7d, 0xaf},
	{0x80, 0x01},
	{0x81, 0x7f},
	{0x82, 0x13},
	{0x83, 0x24},
	{0x84, 0x7D},
	{0x85, 0x81},
	{0x86, 0x7D},
	{0x87, 0x81},
	{0x92, 0x48},
	{0x93, 0x54},
	{0x94, 0x7D},
	{0x95, 0x81},
	{0x96, 0x7D},
	{0x97, 0x81},
	{0xa0, 0x02},
	{0xa1, 0x7B},
	{0xa2, 0x02},
	{0xa3, 0x7B},
	{0xa4, 0x7B},
	{0xa5, 0x02},
	{0xa6, 0x7B},
	{0xa7, 0x02},
	{0xa8, 0x85},
	{0xa9, 0x8C},
	{0xaa, 0x85},
	{0xab, 0x8C},
	{0xac, 0x10},
	{0xad, 0x16},
	{0xae, 0x10},
	{0xaf, 0x16},
	{0xb0, 0x99},
	{0xb1, 0xA3},
	{0xb2, 0xA4},
	{0xb3, 0xAE},
	{0xb4, 0x9B},
	{0xb5, 0xA2},
	{0xb6, 0xA6},
	{0xb7, 0xAC},
	{0xb8, 0x9B},
	{0xb9, 0x9F},
	{0xba, 0xA6},
	{0xbb, 0xAA},
	{0xbc, 0x9B},
	{0xbd, 0x9F},
	{0xbe, 0xA6},
	{0xbf, 0xaa},
	{0xc4, 0x2c},
	{0xc5, 0x43},
	{0xc6, 0x63},
	{0xc7, 0x79},
	{0xc8, 0x2d},
	{0xc9, 0x42},
	{0xca, 0x2d},
	{0xcb, 0x42},
	{0xcc, 0x64},
	{0xcd, 0x78},
	{0xce, 0x64},
	{0xcf, 0x78},
	{0xd0, 0x0a},
	{0xd1, 0x09},

	{0xd4, 0x0c}, //DCDC_TIME_TH_ON
	{0xd5, 0x0c}, //DCDC_TIME_TH_OFF
	{0xd6, 0x98}, //DCDC_AG_TH_ON
	{0xd7, 0x90}, //DCDC_AG_TH_OFF

	{0xe0, 0xc4},
	{0xe1, 0xc4},
	{0xe2, 0xc4},
	{0xe3, 0xc4},
	{0xe4, 0x00},
	{0xe8, 0x80},
	{0xe9, 0x40},
	{0xea, 0x7f},
	{0xf0, 0x01}, //sram1_cfg
	{0xf1, 0x01}, //sram2_cfg
	{0xf2, 0x01}, //sram3_cfg
	{0xf3, 0x01}, //sram4_cfg
	{0xf4, 0x01}, //sram5_cfg

	// PAGE 3 //
	{0x03, 0x03},
	{0x10, 0x10},

	// PAGE 10 START //
	{0x03, 0x10},
	{0x10, 0x03}, // CrYCbY // For Demoset 0x03
	{0x12, 0x30},
	{0x13, 0x0a}, // contrast on
	{0x20, 0x00},

	{0x30, 0x00},
	{0x31, 0x00},
	{0x32, 0x00},
	{0x33, 0x00},

	{0x34, 0x30},
	{0x35, 0x00},
	{0x36, 0x00},
	{0x38, 0x00},
	{0x3e, 0x58},

	{0x3f, 0x00}, // 0x02 for preview and 0x00 for capture
	{0x40, 0x00}, // YOFS modify brightness
	{0x41, 0x10}, // DYOFS
	{0x48, 0x85},
	//Saturation;
	{0x60, 0x67}, // SATCTL
	{0x61, 0x88}, // SATB
	{0x62, 0x83}, // SATR
	{0x63, 0x50}, // AGSAT
	{0x64, 0x41},
	{0x66, 0x33}, // SATTIMETH
	{0x67, 0x20}, // SATOUTDEL

	{0x6a, 0x80}, //8a
	{0x6b, 0x84}, //74
	{0x6c, 0x80}, //7e //7a
	{0x6d, 0x80}, //8e

	{0x76, 0x01}, // white protection ON
	{0x74, 0x66},
	{0x79, 0x06},

	// PAGE 11 START //
	{0x03, 0x11},
	{0x10, 0x7f},
	{0x11, 0x40},
	{0x12, 0x0a}, // Blue Max-Filter Delete
	{0x13, 0xbb},

	{0x26, 0x31}, // Double_AG 31->20
	{0x27, 0x34}, // Double_AG 34->22
	{0x28, 0x0f}, // LPFOUTTHL
	{0x29, 0x10}, // LPFOUTTHH
	{0x2b, 0x30}, // LPFYMEANTHL
	{0x2c, 0x32}, // LPFYMEANTHH

	//Out2 D-LPF th
	{0x30, 0x70}, // OUT2YBOUNDH
	{0x31, 0x10}, // OUT2YBOUNDL
	{0x32, 0x58},
	{0x33, 0x09}, // OUT2THH
	{0x34, 0x06}, // OUT2THM
	{0x35, 0x03},

	//Out1 D-LPF th
	{0x36, 0x70}, // OUT1YBOUNDH
	{0x37, 0x18}, // OUT1YBOUNDL
	{0x38, 0x58},
	{0x39, 0x09}, // OUT1THH
	{0x3a, 0x06}, // OUT1THM
	{0x3b, 0x03},

	//Indoor D-LPF th
	{0x3c, 0x80}, // INYBOUNDH
	{0x3d, 0x18}, // INYBOUNDL
	{0x3e, 0x80}, // INRATIO//0x90
	{0x3f, 0x0c}, // INTHH
	{0x40, 0x09}, // INTHM
	{0x41, 0x06}, // INTHL

	{0x42, 0x80}, // DARK1YBOUNDH
	{0x43, 0x18}, // DARK1YBOUNDL
	{0x44, 0x80}, // DARK1RATIO//0x88
	{0x45, 0x12}, // DARK1THH
	{0x46, 0x10}, // DARK1THM
	{0x47, 0x10}, // DARK1THL

	{0x48, 0x90},
	{0x49, 0x40},
	{0x4a, 0x80}, // DARK2RATIO
	{0x4b, 0x13},
	{0x4c, 0x10},
	{0x4d, 0x11},

	{0x4e, 0x80},
	{0x4f, 0x30},
	{0x50, 0x80},
	{0x51, 0x13},
	{0x52, 0x10},
	{0x53, 0x13},

	{0x54, 0x11},
	{0x55, 0x17},
	{0x56, 0x20},
	{0x57, 0x01},
	{0x58, 0x00},
	{0x59, 0x00},

	{0x5a, 0x18}, //18
	{0x5b, 0x00},
	{0x5c, 0x00},

	{0x60, 0x3f},
	{0x62, 0x60},
	{0x70, 0x06},

	// PAGE 12 START //
	{0x03, 0x12},
	{0x20, 0x0f},
	{0x21, 0x0f},

	{0x25, 0x00},

	{0x28, 0x00},
	{0x29, 0x00},
	{0x2a, 0x00},

	{0x30, 0x50},
	{0x31, 0x18},
	{0x32, 0x32},
	{0x33, 0x40},
	{0x34, 0x50},
	{0x35, 0x70},
	{0x36, 0xa0},
	{0x3b, 0x06},
	{0x3c, 0x06},

	//Out2 th
	{0x40, 0xa0},
	{0x41, 0x40},
	{0x42, 0xa0},
	{0x43, 0x90}, // YCOUT2STDM
	{0x44, 0x90}, // YCOUT2STDL
	{0x45, 0x80},

	//Out1 th
	{0x46, 0xb0}, // YCOUT1THH
	{0x47, 0x55}, // YCOUT1THL
	{0x48, 0xa0}, // YCOUT1STDH
	{0x49, 0x90}, // YCOUT1STDM
	{0x4a, 0x90}, // YCOUT1STDL
	{0x4b, 0x80},

	//In door th
	{0x4c, 0xb0}, // YCINTHH
	{0x4d, 0x40}, // YCINTHL
	{0x4e, 0x90}, // YCINSTDH
	{0x4f, 0x90}, // YCINSTDM
	{0x50, 0xa0},
	{0x51, 0x80},

	//Dark1 th
	{0x52, 0xb0}, // YCDARK1THH
	{0x53, 0x40},
	{0x54, 0x90},
	{0x55, 0x90},
	{0x56, 0xa0},
	{0x57, 0x80},

	//Dark2 th
	{0x58, 0x90}, // YCDARK2THH
	{0x59, 0x40}, // YCDARK2THL
	{0x5a, 0xd0}, // YCDARK2STDH
	{0x5b, 0xd0}, // YCDARK2STDM
	{0x5c, 0xe0}, // YCDARK2STDL
	{0x5d, 0x80},

	//Dark3 th
	{0x5e, 0x88}, // YCDARK3THH
	{0x5f, 0x40}, // YCDARK3THL
	{0x60, 0xe0}, // YCDARK3STDH
	{0x61, 0xe0},
	{0x62, 0xe0},
	{0x63, 0x80},

	{0x70, 0x15},
	{0x71, 0x01}, //Don't Touch register

	{0x72, 0x18},
	{0x73, 0x01}, //Don't Touch register

	{0x74, 0x25},
	{0x75, 0x15},
	{0x80, 0x30},
	{0x81, 0x50},
	{0x82, 0x80},
	{0x85, 0x1a},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x90, 0x5d},

	{0xc5, 0x30},
	{0xC6, 0x2a},
	{0xD0, 0x0c}, //CI Option/CI DPC
	{0xD1, 0x80},
	{0xD2, 0x67},
	{0xD3, 0x00},
	{0xD4, 0x00},
	{0xD5, 0x02},
	{0xD6, 0xff},
	{0xD7, 0x18},

	/////// PAGE 13 START ///////
	{0x03, 0x13},
	//Edge
	{0x10, 0xcb},
	{0x11, 0x7b},
	{0x12, 0x07},
	{0x14, 0x00},

	{0x20, 0x15},
	{0x21, 0x13},
	{0x22, 0x33},
	{0x23, 0x04},
	{0x24, 0x09},

	{0x25, 0x08},

	{0x26, 0x18},
	{0x27, 0x30},
	{0x29, 0x12},
	{0x2a, 0x50},

	//Low clip th
	{0x2b, 0x02},
	{0x2c, 0x03},
	{0x25, 0x09},
	{0x2d, 0x11},
	{0x2e, 0x17},
	{0x2f, 0x19},

	//Out2 Edge
	{0x50, 0x18},
	{0x51, 0x1c},
	{0x52, 0x1b},
	{0x53, 0x15},
	{0x54, 0x18},
	{0x55, 0x15},

	//Out1 Edge
	{0x56, 0x18},
	{0x57, 0x1c},
	{0x58, 0x1b},
	{0x59, 0x15},
	{0x5a, 0x18},
	{0x5b, 0x15},

	//Indoor Edg
	{0x5c, 0x18},
	{0x5d, 0x18},
	{0x5e, 0x18},
	{0x5f, 0x18},
	{0x60, 0x18},
	{0x61, 0x18},

	//Dark1 Edge
	{0x62, 0x08},
	{0x63, 0x08},
	{0x64, 0x08},
	{0x65, 0x06},
	{0x66, 0x06},
	{0x67, 0x06},

	//Dark2 Edge
	{0x68, 0x07},
	{0x69, 0x07},
	{0x6a, 0x07},
	{0x6b, 0x07},
	{0x6c, 0x07},
	{0x6d, 0x07},

	//Dark3 Edge
	{0x6e, 0x07},
	{0x6f, 0x07},
	{0x70, 0x07},
	{0x71, 0x05},
	{0x72, 0x05},
	{0x73, 0x05},

	{0x80, 0xff},
	{0x81, 0x1f},
	{0x82, 0x05}, // EDGE2DCTL3
	{0x83, 0x11},

	{0x90, 0x05},
	{0x91, 0x05},
	{0x92, 0x33},
	{0x93, 0x30},
	{0x94, 0x10},
	{0x95, 0x5a},
	{0x97, 0x20},
	{0x99, 0x60},

	{0xa0, 0x01},
	{0xa1, 0x02},
	{0xa2, 0x01},
	{0xa3, 0x02},
	{0xa4, 0x01},
	{0xa5, 0x01},
	{0xa6, 0x07}, // EDGE2DLCD1N
	{0xa7, 0x08}, // EDGE2DLCD1P
	{0xa8, 0x07}, // EDGE2DLCD2N
	{0xa9, 0x08}, // EDGE2DLCD2P
	{0xaa, 0x07}, // EDGE2DLCD3N
	{0xab, 0x08}, // EDGE2DLCD3P

	//Out2
	{0xb0, 0x22},
	{0xb1, 0x2a},
	{0xb2, 0x28},
	{0xb3, 0x22},
	{0xb4, 0x2a},
	{0xb5, 0x28},

	//Out1
	{0xb6, 0x22},
	{0xb7, 0x2a},
	{0xb8, 0x28},
	{0xb9, 0x22},
	{0xba, 0x2a},
	{0xbb, 0x28},

	{0xbc, 0x25},
	{0xbd, 0x2a},
	{0xbe, 0x27},
	{0xbf, 0x25},
	{0xc0, 0x2a},
	{0xc1, 0x27},

	//Dark1
	{0xc2, 0x1e},
	{0xc3, 0x24},
	{0xc4, 0x20},
	{0xc5, 0x1e},
	{0xc6, 0x24},
	{0xc7, 0x20},

	//Dark2
	{0xc8, 0x18},
	{0xc9, 0x20},
	{0xca, 0x1e},
	{0xcb, 0x18},
	{0xcc, 0x20},
	{0xcd, 0x1e},

	//Dark3
	{0xce, 0x18},
	{0xcf, 0x20},
	{0xd0, 0x1e},
	{0xd1, 0x18},
	{0xd2, 0x20},
	{0xd3, 0x1e},

	// PAGE 14 START //
	{0x03, 0x14},
	{0x10, 0x11},


	{0x20, 0x40},
	{0x21, 0x80},

	{0x22, 0x78},
	{0x23, 0x80},
	{0x24, 0x80},

	{0x30, 0xc8},
	{0x31, 0x2b},
	{0x32, 0x00},
	{0x33, 0x00},
	{0x34, 0x90},

	{0x40, 0x4f},
	{0x50, 0x49},
	{0x60, 0x4b},
	{0x70, 0x49},

	// PAGE 15 START //
	{0x03, 0x15},
	{0x10, 0x0f},

	{0x14, 0x42},
	{0x15, 0x32},
	{0x16, 0x24},
	{0x17, 0x2f}, //CMC SIGN

	//CMC_Default_CWF
	{0x30, 0x8f},
	{0x31, 0x59},
	{0x32, 0x0a},
	{0x33, 0x15},
	{0x34, 0x5b},
	{0x35, 0x06},
	{0x36, 0x07},
	{0x37, 0x40},
	{0x38, 0x86}, // CMC33

	//CMC OFS

	{0x40, 0x92},
	{0x41, 0x1b},
	{0x42, 0x89},
	{0x43, 0x81},
	{0x44, 0x00},
	{0x45, 0x01},
	{0x46, 0x89},
	{0x47, 0x9e},
	{0x48, 0x28},

	//CMC POFS
	{0x50, 0x02},
	{0x51, 0x82},
	{0x52, 0x00}, // CMCOFSH13
	{0x53, 0x07}, // CMCOFSH21
	{0x54, 0x11},
	{0x55, 0x98},
	{0x56, 0x00}, // CMCOFSH31
	{0x57, 0x0b}, // CMCOFSH32
	{0x58, 0x8b},

	{0x80, 0x03},
	{0x85, 0x40},
	{0x87, 0x02},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8a, 0x00},

	// PAGE 16 START //
	{0x03, 0x16},
	{0x10, 0x31},//GMA_CTL
	{0x18, 0x5e},
	{0x19, 0x5d},
	{0x1A, 0x0e},//TIME_ON
	{0x1B, 0x01},//TIME_OFF
	{0x1C, 0xdc},//OUT_ON
	{0x1D, 0xfe},//OUT_OFF
	//GMA
	{0x30, 0x00},
	{0x31, 0x06},
	{0x32, 0x1a},
	{0x33, 0x32},
	{0x34, 0x53},
	{0x35, 0x6c},
	{0x36, 0x81},
	{0x37, 0x94},
	{0x38, 0xa4},
	{0x39, 0xb3},
	{0x3a, 0xc0},
	{0x3b, 0xcb},
	{0x3c, 0xd3},
	{0x3d, 0xd9},
	{0x3e, 0xdc},
	{0x3f, 0xe1},
	{0x40, 0xe5},
	{0x41, 0xe8},
	{0x42, 0xea},
	//RGMA
	{0x50, 0x00},
	{0x51, 0x09},
	{0x52, 0x1f},
	{0x53, 0x37},
	{0x54, 0x5b},
	{0x55, 0x76},
	{0x56, 0x8d},
	{0x57, 0xa1},
	{0x58, 0xb2},
	{0x59, 0xbe},
	{0x5a, 0xc9},
	{0x5b, 0xd2},
	{0x5c, 0xdb},
	{0x5d, 0xe3},
	{0x5e, 0xeb},
	{0x5f, 0xf0},
	{0x60, 0xf5},
	{0x61, 0xf7},
	{0x62, 0xf8},
	//BGMA
	{0x70, 0x00},
	{0x71, 0x07},
	{0x72, 0x0c},
	{0x73, 0x17},
	{0x74, 0x2a},
	{0x75, 0x3c},
	{0x76, 0x4d},
	{0x77, 0x5e},
	{0x78, 0x6e},
	{0x79, 0x7d},
	{0x7a, 0x8d},
	{0x7b, 0x9c},
	{0x7c, 0xaa},
	{0x7d, 0xb9},
	{0x7e, 0xc7},
	{0x7f, 0xcd},
	{0x80, 0xd6},
	{0x81, 0xdc},
	{0x82, 0xe5},

	// PAGE 17 START //
	{0x03, 0x17},
	{0xc4, 0x68},
	{0xc5, 0x56},

	// PAGE 20 START //
	{0x03, 0x20},
	{0x11, 0x1c},
	{0x18, 0x30},
	{0x1a, 0x08},
	{0x20, 0x01}, //05_lowtemp Y Mean off
	{0x21, 0x30},
	{0x22, 0x10},
	{0x23, 0x00},
	{0x24, 0x00}, //Uniform Scene Off

	{0x28, 0xe7},
	{0x29, 0x0d}, //20100305 ad->0d
	{0x2a, 0xff},
	{0x2b, 0x34}, //f4->Adaptive off

	{0x2c, 0xc2},
	{0x2d, 0xcf},  //fe->AE Speed option
	{0x2e, 0x33},
	{0x30, 0x78}, //f8
	{0x32, 0x03},
	{0x33, 0x2e},
	{0x34, 0x30},
	{0x35, 0xd4},
	{0x36, 0xfe},
	{0x37, 0x32},
	{0x38, 0x04},

	{0x39, 0x22}, //AE_escapeC10
	{0x3a, 0xde}, //AE_escapeC11

	{0x3b, 0x22}, //AE_escapeC1
	{0x3c, 0xde}, //AE_escapeC2

	{0x50, 0x45},
	{0x51, 0x88},

	{0x56, 0x03},
	{0x57, 0xf7},
	{0x58, 0x14},
	{0x59, 0x88},
	{0x5a, 0x04},

	//New Weight For Samsung
	{0x60, 0x55},
	{0x61, 0x55},
	{0x62, 0x6a},
	{0x63, 0xa9},
	{0x64, 0x6a},
	{0x65, 0xa9},
	{0x66, 0x6a},
	{0x67, 0xa9},
	{0x68, 0x6b},
	{0x69, 0xe9},
	{0x6a, 0x6a},
	{0x6b, 0xa9},
	{0x6c, 0x6a},
	{0x6d, 0xa9},
	{0x6e, 0x55},
	{0x6f, 0x55},
	{0x70, 0x7a},
	{0x71, 0xBb},

	// haunting control
	{0x76, 0x43},
	{0x77, 0xe2},

	{0x78, 0x23}, //Yth1
	{0x79, 0x46},
	{0x7a, 0x23}, //23
	{0x7b, 0x22}, //22
	{0x7d, 0x23},

	{0x83, 0x01}, //EXP Normal 33.33 fps
	{0x84, 0x5f},
	{0x85, 0x90},
	{0x86, 0x01}, //EXPMin 6000.00 fps
	{0x87, 0xf4},
	{0x88, 0x05}, //EXP Max 8.33 fps
	{0x89, 0x7e},
	{0x8a, 0x40},
	{0x8B, 0x75}, //EXP100
	{0x8C, 0x30},
	{0x8D, 0x61}, //EXP120
	{0x8E, 0xa8},
	{0x9c, 0x18}, //EXP Limit 480.00 fps
	{0x9d, 0x6a},
	{0x9e, 0x01}, //EXP Unit
	{0x9f, 0xf4},

	{0xb0, 0x18},
	{0xb1, 0x14}, //ADC 400->560
	{0xb2, 0xa0},
	{0xb3, 0x18},
	{0xb4, 0x1a},
	{0xb5, 0x44},
	{0xb6, 0x2f},
	{0xb7, 0x28},
	{0xb8, 0x25},
	{0xb9, 0x22},
	{0xba, 0x21},
	{0xbb, 0x20},
	{0xbc, 0x1f},
	{0xbd, 0x1f}, // AGBTH8
	{0xc0, 0x14},
	{0xc1, 0x1f},
	{0xc2, 0x1f},
	{0xc3, 0x18},
	{0xc4, 0x10},
	{0xc8, 0x80}, // DGMAX
	{0xc9, 0x40},

	// PAGE 22 START //
	{0x03, 0x22},
	{0x10, 0xfd},
	{0x11, 0x2e},
	{0x19, 0x01}, // Low On //
	{0x20, 0x30},
	{0x21, 0x80},
	{0x23, 0x08},
	{0x24, 0x01},
	//{0x25, 0x00}, //7f New Lock Cond & New light stable

	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x11},
	{0x39, 0x34},

	{0x40, 0xf7}, //
	{0x41, 0x44}, //44
	{0x42, 0x33}, //43
	{0x43, 0xf7},
	{0x44, 0x44},
	{0x45, 0x33},

	{0x46, 0x00},
	{0x47, 0x94},
	{0x50, 0xb2},
	{0x51, 0x81},
	{0x52, 0x98},

	{0x80, 0x38},
	{0x81, 0x20},
	{0x82, 0x38},

	{0x83, 0x5e}, // RMAX
	{0x84, 0x10},
	{0x85, 0x5e},
	{0x86, 0x24},

	{0x87, 0x49},
	{0x88, 0x30},
	{0x89, 0x3f}, // BMAXM
	{0x8a, 0x28},

	{0x8b, 0x49},
	{0x8c, 0x30},
	{0x8d, 0x3f},
	{0x8e, 0x28},

	{0x8f, 0x54},
	{0x90, 0x52},
	{0x91, 0x4e}, //4
	{0x92, 0x49},
	{0x93, 0x40}, //46
	{0x94, 0x36},
	{0x95, 0x2f},
	{0x96, 0x2a},
	{0x97, 0x27}, // BGAINPARA9
	{0x98, 0x27},

	{0x99, 0x27},
	{0x9a, 0x27},

	{0x9b, 0xbb},
	{0x9c, 0xbb},
	{0x9d, 0x48},
	{0x9e, 0x38},
	{0x9f, 0x30},

	{0xa0, 0x60},
	{0xa1, 0x34},
	{0xa2, 0x6f},
	{0xa3, 0xff},

	{0xa4, 0x14}, //1500fps
	{0xa5, 0x2c}, // 700fps
	{0xa6, 0xcf},

	{0xad, 0x40},
	{0xae, 0x4a},

	{0xaf, 0x28},  // low temp Rgain
	{0xb0, 0x26},  // low temp Rgain

	{0xb1, 0x00}, //0x20 -> 0x00 0405 modify
	{0xb8, 0xa0}, //a2: b-2, R+2  //b4 B-3, R+4 lowtemp
	{0xb9, 0x00},

	// PAGE 20
	{0x03, 0x20}, //page 20
	{0x10, 0x9c}, //ae off

	// PAGE 22
	{0x03, 0x22}, //page 22
	{0x10, 0xe9}, //awb off

	// PAGE 0
	{0x03, 0x00},
	{0x0e, 0x03}, //PLL On
	{0x0e, 0x73}, //PLLx2

	{0x03, 0x00}, // Dummy 750us
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},

	{0x03, 0x00}, // Page 0
	{0x01, 0xc0}, // Sleep Off 0xf8->0x50 for solve green line issue
	{0xff, 0xff},
};

const SENSOR_REG_T HI253_YUV_800x600[] = {
	{0x03, 0x00},
	{0x10, 0x10},
	{0x12, 0x04},
	{0x03, 0x12},
	{0x20, 0x0f},
	{0x21, 0x0f},

	//scaling VGA
	{0x03, 0x18},//Page 18
	{0x12, 0x20},
	{0x10, 0x00},//Open Scaler Function
	{0x11, 0x00},
	{0x20, 0x05},
	{0x21, 0x00},
	{0x22, 0x03},
	{0x23, 0xc0},
	{0x24, 0x00},
	{0x25, 0x00},
	{0x26, 0x00},
	{0x27, 0x00},
	{0x28, 0x05},
	{0x29, 0x00},
	{0x2a, 0x03},
	{0x2b, 0xc0},
	{0x2c, 0x0a},
	{0x2d, 0x00},
	{0x2e, 0x0a},
	{0x2f, 0x00},
	{0x30, 0x44},
	{0xff, 0xff},
};

SENSOR_REG_T HI253_YUV_1280X960[] = {
	{0x03, 0x00},
	{0x10, 0x00}, //0401//0x00}
	{0x12, 0x04},
	{0x03, 0x12},
	{0x20, 0x0f},
	{0x21, 0x0f},

	{0x03, 0x18},
	{0x10, 0x07},
	{0x11, 0x00},
	{0x12, 0x20},
	{0x20, 0x05},
	{0x21, 0x00},
	{0x22, 0x03},
	{0x23, 0xc0},
	{0x24, 0x00},
	{0x25, 0x04},
	{0x26, 0x00},
	{0x27, 0x04},
	{0x28, 0x05},
	{0x29, 0x04},
	{0x2a, 0x03},
	{0x2b, 0xc4},
	{0x2c, 0x0a},
	{0x2d, 0x00},
	{0x2e, 0x0a},
	{0x2f, 0x00},
	{0x30, 0x41}, //41->44

	{0x03,0x00},//dummy1
	{0x03,0x00},//dummy2
	{0x03,0x00},//dummy3
	{0x03,0x00},//dummy4
	{0x03,0x00},//dummy5
	{0x03,0x00},//dummy6
	{0x03,0x00},//dummy7
	{0x03,0x00},//dummy8
	{0x03,0x00},//dummy9
	{0x03,0x00},//dummy10
	{SENSOR_WRITE_DELAY, 0x32},
	{0xff, 0xff}
};

SENSOR_REG_T HI253_YUV_1600X1200[] = {
	{0x03, 0x00},
	{0x10, 0x00}, //0401//0x00},//100208 Vsync type2
	{0x12, 0x04},
	{0x03, 0x12},
	{0x20, 0x0f},
	{0x21, 0x0f},

	{0x03, 0x18},
	{0x10, 0x00},
	{0x11, 0x00},
	{0x12, 0x00},
	{SENSOR_WRITE_DELAY, 0x32},   //delay 10ms
	{0xff, 0xff},
};

static SENSOR_REG_TAB_INFO_T s_hi253_resolution_Tab_YUV[] = {
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(HI253_YUV_COMMON),640,480,24,SENSOR_IMAGE_FORMAT_YUV422},
	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(HI253_YUV_800x600),800,600,24,SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(HI253_YUV_1280X960), 1280, 960, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(HI253_YUV_1600X1200), 1600, 1200,24,SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

static EXIF_SPEC_PIC_TAKING_COND_T s_hi253_exif;

static SENSOR_IOCTL_FUNC_TAB_T s_hi253_ioctl_func_tab = {
	PNULL,
	PNULL,			//_hi253_Power_On
	PNULL,
	HI253_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	PNULL,			//_hi253_GetResolutionTrimTab

	set_hi253_ae_enable,
	PNULL,			//set_hi253_hmirror_enable
	PNULL,			//set_hi253_vmirror_enable

	HI253_set_brightness,
	HI253_set_contrast,
	PNULL,			//HI253_set_sharpness
	PNULL,			//HI253_set_saturation

	set_hi253_preview_mode,
	HI253_set_image_effect,

	HI253_Before_Snapshot,
	PNULL,

	HI253_flash,

	PNULL,			//HI253_read_ev_value,
	PNULL,			//HI253_write_ev_value,
	PNULL,			//HI253_read_gain_value,
	PNULL,			//HI253_write_gain_value,
	PNULL,			//HI253_read_gain_scale,
	PNULL,			//HI253_set_frame_rate,

	PNULL,
	PNULL,
	HI253_set_whitebalance_mode,
	PNULL,			// get snapshot skip frame num from customer, input SENSOR_MODE_E paramter

	PNULL,			// set ISO level 0: auto; other: the appointed level
	set_hi253_ev,		// Set exposure compensation	 0: auto; other: the appointed level

	PNULL,			// check whether image format is support
	PNULL,			//change sensor image format according to param
	PNULL,			//change sensor image format according to param

	// CUSTOMER FUNCTION
	PNULL,			// function 3 for custumer to configure
	PNULL,			// function 4 for custumer to configure
	PNULL,			//Set anti banding flicker	 0: 50hz;1: 60
	PNULL,			//set_hi253_video_mode, // set video mode

	PNULL,			// pick out the jpeg stream from given buffer
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL
};

static SENSOR_EXTEND_INFO_T hi253_ext_info = {
	256,		//jpeg_seq_width
	0		//1938 //jpeg_seq_height
};

SENSOR_INFO_T g_hi253_yuv_info = {
	HI253_I2C_ADDR_W,			// salve i2c write address
	HI253_I2C_ADDR_R,			// salve i2c read address

	0,					// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
						// bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
						// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,		// bit0: 0:negative; 1:positive -> polarily of pixel clock
						// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
						// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
						// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,					// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
						// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,			// reset pulse level
	100,					// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,			// 1: high level valid; 0: low level valid

	2,					// count of identify code
	{{0x04, 0x92},				// supply two code to identify sensor
	{0x04, 0x92}},				// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,			// voltage of avdd

	1600,					// max width of source image
	1200,					// max height of source image
	"HI253",				// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,		// define in SENSOR_IMAGE_FORMAT_E enum,
						// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;

	s_hi253_resolution_Tab_YUV,		// point to resolution table information structure
	&s_hi253_ioctl_func_tab,		// point to ioctl function table

	PNULL,					// information and table about Rawrgb sensor
	&hi253_ext_info,			// extend information about sensor
	SENSOR_AVDD_2800MV,			// iovdd
	SENSOR_AVDD_1800MV,			// dvdd
	0,					// skip frame num before preview
	0,					// skip frame num before capture
	0,					// deci frame num during preview
	0,					// deci frame num during video preview
	0,					// threshold enable(only analog TV)
	0,					// threshold mode 0 fix mode 1 auto mode
	0,					// threshold start postion
	0,					// threshold end postion
	0,
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	PNULL,
	0,					// skip frame num while change setting
	48,					// horizontal view angle
	48,					// vertical view angle
};

static void HI253_WriteReg( uint8_t subaddr, uint8_t data )
{
	Sensor_WriteReg_8bits(subaddr,data);
}

static uint8_t HI253_ReadReg( uint8_t subaddr)
{
	uint8_t value = 0;
	value = Sensor_ReadReg(subaddr);
	return value;
}

static uint32_t HI253_Identify(uint32_t param)
{
	uint32_t i;
	uint32_t nLoop;
	uint8_t ret;
	uint32_t err_cnt     = 0;
	uint8_t reg[2]       = {0x04, 0x04};
	uint8_t value[2]     = {0x92, 0x92};

	SENSOR_PRINT("HI253_Identify");
	for (i = 0; i<2; ) {
		nLoop = 1000;
		ret = HI253_ReadReg(reg[i]);
		if ( ret != value[i]) {
			err_cnt++;
			if(err_cnt>3) {
				SENSOR_PRINT("It is not HI253\n");
				return SENSOR_FAIL;
			} else {
				while(nLoop--);
				continue;
			}
		}
		err_cnt = 0;
		i++;
	}
	SENSOR_PRINT("HI253 identify: It is HI253\n");

	return SENSOR_SUCCESS;
}

#if 0
static uint32_t _hi253_Power_On(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E	dvdd_val=g_hi253_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E	avdd_val=g_hi253_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E	iovdd_val=g_hi253_yuv_info.iovdd_val;

	BOOLEAN 		reset_level=g_hi253_yuv_info.reset_pulse_level;
	uint32_t 		reset_width=g_hi253_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE==power_on) {
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);

		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);

		msleep(20);
		Sensor_SetResetLevel(reset_level);
		msleep(reset_width);
		Sensor_SetResetLevel(!reset_level);
		msleep(100);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT("SENSOR: _hi253_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}
#endif

static uint32_t HI253_GetExifInfo(uint32_t param)
{
	return (uint32_t) & s_hi253_exif;
}
static uint32_t HI253_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_hi253_exif;
	memset(&s_hi253_exif, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
	SENSOR_PRINT("SENSOR: HI253_InitExifInfo \n");
	exif_ptr->valid.FNumber = 1;
	exif_ptr->FNumber.numerator = 14;
	exif_ptr->FNumber.denominator = 5;
	exif_ptr->valid.ExposureProgram = 1;
	exif_ptr->ExposureProgram = 0x04;
	exif_ptr->valid.ApertureValue = 1;
	exif_ptr->ApertureValue.numerator = 14;
	exif_ptr->ApertureValue.denominator = 5;
	exif_ptr->valid.MaxApertureValue = 1;
	exif_ptr->MaxApertureValue.numerator = 14;
	exif_ptr->MaxApertureValue.denominator = 5;
	exif_ptr->valid.FocalLength = 1;
	exif_ptr->FocalLength.numerator = 289;
	exif_ptr->FocalLength.denominator = 100;
	exif_ptr->valid.FileSource = 1;
	exif_ptr->FileSource = 0x03;
	exif_ptr->valid.ExposureMode = 1;
	exif_ptr->ExposureMode = 0x00;
	exif_ptr->valid.WhiteBalance = 1;
	exif_ptr->WhiteBalance = 0x00;

	return SENSOR_SUCCESS;
}

static uint32_t set_hi253_ae_enable(uint32_t enable)
{
	if (0x00==enable) {
		HI253_WriteReg(0x03,0x20);
		HI253_WriteReg(0x10,0x1c);// AE Off
	} else if (0x01==enable) {
		HI253_WriteReg(0x03, 0x20);//page 3
		HI253_WriteReg(0x10, 0x9c);//ae on
	}

	SENSOR_PRINT("HI253_test set_ae_enable: enable = %d", enable);
	return 0;
}

#if 0
static uint32_t set_hi253_ae_awb_enable(uint32_t ae_enable, uint32_t awb_enable)
{
	if (0x00==ae_enable) {
		HI253_WriteReg(0x03,0x20);
		HI253_WriteReg(0x10,0x1c);// AE Off
	} else if(0x01==ae_enable) {
		HI253_WriteReg(0x03, 0x20);//page 3
		HI253_WriteReg(0x10, 0x9c);//ae on
	}

	if(0x00==awb_enable) {
		HI253_WriteReg(0x03,0x22);
		HI253_WriteReg(0x10,0x69);// AWB Off
	} else if(0x01==awb_enable) {
		HI253_WriteReg(0x03, 0x22);
		HI253_WriteReg(0x10, 0xe9);//AWB ON
	}

	SENSOR_PRINT("HI253_test set_ae_awb_enable: ae=%d awb=%d", ae_enable, awb_enable);
	return 0;
}
#endif
#if 0
static uint32_t set_front_sensor_yrotate_HI253(void)
{
	uint8_t iFilpY;
	uint8_t ss = 0;

	HI253_WriteReg(0x03,0x00);
	iFilpY = HI253_ReadReg(0x11);
	ss = iFilpY & 0x02;

	if(ss == 0x02) {
		iFilpY = iFilpY & 0xfd;
	} else {
		iFilpY |= 0x02;
	}

	HI253_WriteReg(0x03,0x00);
	HI253_WriteReg(0x11, iFilpY);

	return 0;
}

static uint32_t set_back_sensor_yrotate_HI253(void)
{
	uint8_t iFilpY;
	uint8_t ss = 0;

#if 1
	HI253_WriteReg(0x03,0x00);
	iFilpY = HI253_ReadReg(0x11);
	ss = iFilpY & 0x02;

	if (ss == 0x02) {
		iFilpY = iFilpY & 0xfd;
	} else {
		iFilpY |= 0x02;
	}

	HI253_WriteReg(0x03,0x00);
	HI253_WriteReg(0x11, iFilpY);
#endif

	return 0;
}
#endif

#if 0
static uint32_t set_hi253_hmirror_enable(uint32_t enable)
{
	if (enable) {
		set_front_sensor_yrotate_HI253();
	} else {
		set_back_sensor_yrotate_HI253();
	}

	SENSOR_PRINT("set_hi253_hmirror_enable: enable = %d", enable);
	return 0;
}
#endif

#if 0
static uint32_t set_front_sensor_xrotate_HI253(void)
{
	uint8_t iFilpX,iHV_Mirror;
	uint8_t ss = 0;

	HI253_WriteReg(0x03,0x00);
	iHV_Mirror = HI253_ReadReg(0x11);
	ss = iHV_Mirror & 0x01;

	if (ss == 0x01) {
		iHV_Mirror = iHV_Mirror & 0xfe;
		iFilpX =  0x31;
	} else {
		iHV_Mirror |= 0x01;
		HI253_WriteReg(0x03,0x00);
		HI253_WriteReg(0x11, iHV_Mirror);
	}

	return 0;
}

static uint32_t set_back_sensor_xrotate_HI253(void)
{
	uint8_t iFilpX,iHV_Mirror;
	uint8_t ss = 0;

	HI253_WriteReg(0x03,0x00);
	iHV_Mirror = HI253_ReadReg(0x11);
	ss = iHV_Mirror & 0x01;

	if(ss == 0x01) {
		iHV_Mirror = iHV_Mirror & 0xfe;
		iFilpX =  0x31;
	} else {
		iHV_Mirror |= 0x01;
	}

	HI253_WriteReg(0x03,0x00);
	HI253_WriteReg(0x11, iHV_Mirror);

	return 0;
}
#endif
#if 0
static uint32_t set_hi253_vmirror_enable(uint32_t enable)
{
	if(enable) {
		set_front_sensor_xrotate_HI253();
	} else {
		set_back_sensor_xrotate_HI253();
	}

	SENSOR_PRINT("set_hi253_vmirror_enable: enable = %d", enable);
	return 0;
}
#endif

const SENSOR_REG_T HI253_brightness_tab[][3] = {
	{
		{0x03,0x10}, //-3
		{0x40,0xb0},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, //-2
		{0x40,0xa0},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, //-1
		{0x40,0x90},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, //0
		{0x40,0x00},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, // 1
		{0x40,0x20},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, // 2
		{0x40,0x30},
		{0xff,0xff},
	},
	{
		{0x03,0x10}, // +3
		{0x40,0x40},
		{0xff,0xff},
	},
};

static uint32_t HI253_set_brightness(uint32_t level)
{
	if(level > 6) {
		return 0;
	}

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_brightness_tab[level];

	HI253_Write_Group_Regs(sensor_reg_ptr);
	SENSOR_PRINT("HI253_test set_hi253_brightness_tab: level = %d", level);
	return 0;
}

const SENSOR_REG_T HI253_contrast_tab[][3] = {
	{
		{0x03,0x10},  ///-3
		{0x48,0x50},
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///-2
		{0x48,0x60},
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///-1
		{0x48,0x70},
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///0
		{0x48,0x85}, // 90
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///1
		{0x48,0xa0}, // a0
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///2
		{0x48,0xb0},
		{0xff,0xff}
	},
	{
		{0x03,0x10},  ///3
		{0x48,0xc0},
		{0xff,0xff}
	},
};

static void HI253_Write_Group_Regs( SENSOR_REG_T* sensor_reg_ptr )
{
	uint32_t i;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
	Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
}

static uint32_t HI253_set_contrast(uint32_t level)
{
	if(level > 6)
		return 0;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_contrast_tab[level];

	HI253_Write_Group_Regs(sensor_reg_ptr);
	SENSOR_PRINT("HI253_test tianxiaohui set_hi253_contrast_tab: level = %d\n", level);

	return 0;
}

static uint32_t set_hi253_preview_mode(uint32_t  preview_mode)
{
	SENSOR_PRINT("HI253_test set_hi253_preview_mode: preview_mode = %d", preview_mode);

	switch (preview_mode) {
	case SENSOR_PARAM_ENVIRONMENT_NORMAL:
		HI253_set_work_mode(0);
		break;

	case SENSOR_PARAM_ENVIRONMENT_NIGHT:
		HI253_set_work_mode(1);
		break;

	case SENSOR_PARAM_ENVIRONMENT_SUNNY:
		HI253_set_work_mode(0);
		break;

	case SENSOR_PARAM_ENVIRONMENT_SPORTS:
		HI253_set_work_mode(0);
		break;

	case SENSOR_PARAM_ENVIRONMENT_LANDSCAPE:
		HI253_set_work_mode(0);
		break;

	default:
		HI253_set_work_mode(0);
		break;
	}

	return 0;
}

const SENSOR_REG_T HI253_image_effect_tab[][15] = {
	{
		{0x03,0x10}, //default
		{0x11,0x03},
		{0x12,0X30},
		{0x13,0x0a},
		//{0x40,0x20},
		{0x44,0x80},
		{0x45,0x80},
		{0xff,0xff},
	},
	{
		{0x03,0x10},//gray
		{0x11,0x03},
		{0x12,0x33},
		{0x13,0x02},
		{0x44,0x80},
		{0x45,0x80},
		{0xff,0xff},
	},
	{
		{0x03,0x10},//  red
		{0x11,0x03},
		{0x12,0x33},
		{0x13,0x02},
		{0x44,0x70},
		{0x45,0xb8},
		{0xff,0xff},
	},
	{
		{0x03,0x10},// green
		{0x11,0x03},//embossing effect off
		{0x12,0x33},//auto bright on
		{0x13,0x22},//binary effect off/solarization effect on
		{0x44,0x30},//ucon
		{0x45,0x50},//vcon
		{0xff,0xff},
	},
	{
		{0x03,0x10},// blue
		{0x11,0x03},//embossing effect off
		{0x12,0x33},//auto bright on
		{0x13,0x22},//binary effect off/solarization effect on
		{0x44,0xb0},//ucon
		{0x45,0x40},//vcon
		{0xff,0xff},
	},
	{
		{0x03,0x10},// yellow
		{0x11,0x03},//embossing effect off
		{0x12,0x33},//auto bright on
		{0x13,0x22},//binary effect off/solarization effect on
		{0x44,0x10},//ucon
		{0x45,0x98},//vcon
		{0xff,0xff},
	},
	{
		{0x03,0x10},// colorinv
		{0x11,0x03},//embossing effect off
		{0x12,0x38},//auto bright on/nagative effect on
		{0x13,0x02},//binary effect off
		{0x44,0x80},//ucon
		{0x45,0x80},//vcon
		{0xff,0xff},
	},
	{
		{0x03,0x10},//sepia
		{0x11,0x03},//embossing effect off
		{0x12,0x33},//auto bright on
		{0x13,0x22},//binary effect off/solarization effect on
		{0x44,0x40},//ucon  40
		{0x45,0xa8},//vcon
		{0xff,0xff},
	},
};

static uint32_t HI253_set_image_effect(uint32_t effect_type)
{
	if(effect_type > 7)
		return 0;
	SENSOR_PRINT("HI253_test SENSOR: set_image_effect: effect_type_start = %d\n", effect_type);
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_image_effect_tab[effect_type];

	HI253_Write_Group_Regs(sensor_reg_ptr);
	SENSOR_PRINT("HI253_test SENSOR: set_image_effect: effect_type_end = %d\n", effect_type);

	return 0;
}

static uint32_t HI253_Before_Snapshot(uint32_t param)
{
	if(g_flash_mode_en) {
		Sensor_SetFlash(1);
		g_flash_mode_en = 0;
	}

	SENSOR_PRINT("HI253_test sensor:HI253_Before_Snapshot");

	if (SENSOR_MODE_SNAPSHOT_ONE_SECOND == param) {
		Sensor_SetMode(param);
	}

	if(SENSOR_MODE_SNAPSHOT_ONE_FIRST == param) {
		Sensor_SetMode(param);
	} else {
		SENSOR_PRINT("HI253_test HI253_Before_Snapshot:dont set any");
	}

	usleep(200*1000);
	SENSOR_PRINT("HI253_BeforeSnapshot: Before Snapshot");

	return 0;
}

const SENSOR_REG_T HI253_mode_tab[][9] = {
	{
		{0x03, 0x10},
		{0x41, 0x00},
		{0xff, 0xff}
	} , // 30fps 24M normal
	{
		{0x03, 0x10},
		{0x41, 0x10},
		{0xff, 0xff}
	} , // 6fps 24M normal
};

static uint32_t HI253_set_work_mode(uint32_t mode)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_mode_tab[mode];

	if (mode > 1) {
		SENSOR_PRINT("HI253_test HI253_set_work_mode:param error,mode=%d .\n",mode);
		return SENSOR_FAIL;
	}

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("HI253_test sensor:HI253_set_work_mode: mode = %d", mode);
		return 0;
}

const SENSOR_REG_T HI253_WB_mode_tab[][11] = {
	//AUTO
	{
		{0x03, 0x22},
		//{0x80, 0x2d},
		//{0x82, 0x42},
		{0x83, 0x5e},
		{0x84, 0x10},
		{0x85, 0x5e},
		{0x86, 0x24},
		{0xff, 0xff}
	},
	//INCANDESCENCE:
	{
		{0x03, 0x22},
		{0x80, 0x2a},
		{0x82, 0x3f},
		{0x83, 0x35},
		{0x84, 0x28},
		{0x85, 0x45},
		{0x86, 0x3b},
		{0xff, 0xff}
	},
	//U30	not used
	{
		{0x03, 0x22},
		{0x10, 0x7B},
		{0x80, 0x33},
		{0x81, 0x20},
		{0x82, 0x3D},
		{0x83, 0x2E},
		{0x84, 0x24},
		{0x85, 0x43},
		{0x86, 0x3D},
		{0x10, 0x7B},
		{0xff, 0xff}
	},
	//FLUORESCENT:
	{
		{0x03, 0x22},
		{0x80, 0x20},
		{0x82, 0x4d},
		{0x83, 0x25},
		{0x84, 0x1b},
		{0x85, 0x55},
		{0x86, 0x48},
		{0xff, 0xff}
	},
	//SUN:
	{
		{0x03, 0x22},
		{0x80, 0x3d},
		{0x82, 0x2e},
		{0x83, 0x40},
		{0x84, 0x33},
		{0x85, 0x33},
		{0x86, 0x28},
		{0xff, 0xff}
	},
	//CLOUD:
	{
		{0x03, 0x22},
		{0x11, 0x28},
		{0x80, 0x50},
		{0x82, 0x25},
		{0x83, 0x55},
		{0x84, 0x4b},
		{0x85, 0x28},
		{0x86, 0x20},
		{0xff, 0xff}
	},
	{
		{0x03, 0x22},
		{0x80, 0x5c},
		{0x82, 0x1a},
		{0x83, 0x5e},
		{0x84, 0x5c},
		{0x85, 0x1a},
		{0x86, 0x19},
		{0xff, 0xff}
	}
};

static uint32_t HI253_set_whitebalance_mode(uint32_t mode )
{
	if(mode>6)
		return 0;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_WB_mode_tab[mode];

	HI253_Write_Group_Regs(sensor_reg_ptr);

	SENSOR_PRINT("HI253_test SENSOR: set_awb_mode: mode = %d", mode);

	return 0;
}

const SENSOR_REG_T HI253_ev_tab[][3] = {
	{
		{0x03,0x10},//-3
		{0x4a,0x50},
		{0xff,0xff},
	},

	{
		{0x03,0x10},//-2
		{0x4a,0x60},
		{0xff,0xff},
	},

	{
		{0x03,0x10},//-1
		{0x4a,0x70},
		{0xff,0xff},
	},

	{
		{0x03,0x10},//00
		{0x4a,0x80},
		{0xff,0xff},
	},

	{
		{0x03,0x10},//02
		{0x4a,0xa0},
		{0xff,0xff},
	},

	{
		{0x03,0x10},//03
		{0x4a,0xb0},
		{0xff,0xff},
	},
	{
		{0x03,0x10},//03
		{0x4a,0xc0},
		{0xff,0xff},
	}
};

static uint32_t set_hi253_ev(uint32_t level)
{
	if (level > 6) {
		return 0;
	}

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI253_ev_tab[level];

	HI253_Write_Group_Regs(sensor_reg_ptr);
	SENSOR_PRINT("HI253_test SENSOR: set_hi253_EV_tab: level = %d", level);

	return 0;
}

static uint32_t HI253_flash(uint32_t param)
{
	SENSOR_PRINT("HI253_flash:param=%d .\n",param);

	g_flash_mode_en = param;

	Sensor_SetFlash(param);

	SENSOR_PRINT("HI253_flash:end .\n");
	return 0;
}
