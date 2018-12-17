/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018 Krzysztof Kondrak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "../client/client.h"
#include "../client/qmenu.h"

#define REF_SOFT	0
#define REF_OPENGL	1
#define REF_VULKAN	2
#define REF_3DFX	3
#define REF_POWERVR	4
#define REF_VERITE	5

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *scr_viewsize;

static cvar_t *gl_mode;
static cvar_t *gl_driver;
static cvar_t *gl_picmip;
static cvar_t *gl_ext_palettedtexture;
static cvar_t *gl_finish;
static cvar_t *vk_finish;
static cvar_t *vk_msaa;
static cvar_t *vk_picmip;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;

static cvar_t *vk_mode;
static cvar_t *vk_driver;

extern void M_ForceMenuOff( void );

/*
====================================================================

MENU INTERACTION

====================================================================
*/
#define SOFTWARE_MENU 0
#define OPENGL_MENU   1
#define VULKAN_MENU   2

static menuframework_s  s_software_menu;
static menuframework_s	s_opengl_menu;
static menuframework_s  s_vulkan_menu;
static menuframework_s *s_current_menu;
static int				s_current_menu_index;

static menulist_s		s_mode_list[3];
static menulist_s		s_ref_list[3];
static menuslider_s		s_tq_slider;
static menuslider_s		s_tqvk_slider;
static menuslider_s		s_screensize_slider[3];
static menuslider_s		s_brightness_slider[3];
static menulist_s  		s_fs_box[3];
static menulist_s  		s_stipple_box;
static menulist_s  		s_paletted_texture_box;
static menulist_s		s_msaa_mode;
static menulist_s  		s_finish_box;
static menulist_s		s_vkfinish_box;
static menuaction_s		s_cancel_action[3];
static menuaction_s		s_defaults_action[3];

static void DriverCallback( void *unused )
{
	int curr_value = s_ref_list[s_current_menu_index].curvalue;
	s_ref_list[0].curvalue = curr_value;
	s_ref_list[1].curvalue = curr_value;
	s_ref_list[2].curvalue = curr_value;

	if ( s_ref_list[s_current_menu_index].curvalue == 0 )
	{
		s_current_menu = &s_software_menu;
		s_current_menu_index = 0;
	}
	else if ( s_ref_list[s_current_menu_index].curvalue == 2 )
	{
		s_current_menu = &s_vulkan_menu;
		s_current_menu_index = 2;
	}
	else
	{
		s_current_menu = &s_opengl_menu;
		s_current_menu_index = 1;
	}
}

static void ScreenSizeCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void BrightnessCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;
	int i;

	for ( i = 0; i < 3; i++ )
	{
		s_brightness_slider[i].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
	}

	if ( stricmp( vid_ref->string, "soft" ) == 0 )
	{
		float gamma = ( 0.8 - ( slider->curvalue/10.0 - 0.5 ) ) + 0.5;

		Cvar_SetValue( "vid_gamma", gamma );
	}
}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;
	int i;

	/*
	** make values consistent
	*/
	for ( i = 0; i < 3; i++ )
	{
		s_fs_box[i].curvalue = s_fs_box[s_current_menu_index].curvalue;
		s_brightness_slider[i].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
		s_ref_list[i].curvalue = s_ref_list[s_current_menu_index].curvalue;
	}

	/*
	** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
	*/
	gamma = ( 0.8 - ( s_brightness_slider[s_current_menu_index].curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "sw_stipplealpha", s_stipple_box.curvalue );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box[s_current_menu_index].curvalue );
	Cvar_SetValue( "gl_ext_palettedtexture", s_paletted_texture_box.curvalue );
	Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
	Cvar_SetValue( "vk_finish", s_vkfinish_box.curvalue );
	Cvar_SetValue( "sw_mode", s_mode_list[SOFTWARE_MENU].curvalue );
	Cvar_SetValue( "gl_mode", s_mode_list[OPENGL_MENU].curvalue );
	Cvar_SetValue( "vk_mode", s_mode_list[VULKAN_MENU].curvalue );
	Cvar_SetValue( "vk_msaa", s_msaa_mode.curvalue);
	Cvar_SetValue( "vk_picmip", 3 - s_tqvk_slider.curvalue );

	switch ( s_ref_list[s_current_menu_index].curvalue )
	{
	case REF_SOFT:
		Cvar_Set( "vid_ref", "soft" );
		break;
	case REF_OPENGL:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "opengl32" );
		break;
	case REF_VULKAN:
		Cvar_Set( "vid_ref", "vk" );
		Cvar_Set( "vk_driver", "vulkan" );
		break;
	case REF_3DFX:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "3dfxgl" );
		break;
	case REF_POWERVR:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "pvrgl" );
		break;
	case REF_VERITE:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "veritegl" );
		break;
	}

	/*
	** update appropriate stuff if we're running OpenGL and gamma
	** has been modified
	*/
	if ( stricmp( vid_ref->string, "gl" ) == 0 )
	{
		if ( vid_gamma->modified )
		{
			vid_ref->modified = true;
			if ( stricmp( gl_driver->string, "3dfxgl" ) == 0 )
			{
				char envbuffer[1024];
				float g;

				vid_ref->modified = true;

				g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
				Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
				putenv( envbuffer );
				Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
				putenv( envbuffer );

				vid_gamma->modified = false;
			}
		}

		if ( gl_driver->modified )
			vid_ref->modified = true;
	}
	else if ( stricmp( vid_ref->string, "vk" ) == 0 )
	{
		if ( vk_driver->modified )
			vid_ref->modified = true;
	}

	M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
	extern void M_PopMenu( void );

	M_PopMenu();
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *resolutions[] = 
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1600 1200]",
		"[1920 1080]",
		"[2048 1536]",
		0
	};
	static const char *refs[] =
	{
		"[software      ]",
		"[default OpenGL]",
		"[Vulkan        ]",
		"[3Dfx OpenGL   ]",
		"[PowerVR OpenGL]",
//		"[Rendition OpenGL]",
		0
	};
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *msaa_modes[] =
	{
		"off",
		"x2",
		"x4",
		"x8",
		0
	};
	int i;

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", 0 );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "6", 0 );
	if ( !sw_mode )
		sw_mode = Cvar_Get( "sw_mode", "0", 0 );
	if ( !gl_ext_palettedtexture )
		gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	if ( !vk_finish )
		vk_finish = Cvar_Get( "vk_finish", "0", CVAR_ARCHIVE );
	if ( !vk_msaa )
		vk_msaa = Cvar_Get( "vk_msaa", "0", CVAR_ARCHIVE );
	if ( !vk_picmip )
		vk_picmip = Cvar_Get( "vk_picmip", "0", CVAR_ARCHIVE );
	if ( !sw_stipplealpha )
		sw_stipplealpha = Cvar_Get( "sw_stipplealpha", "0", CVAR_ARCHIVE );

	if( !vk_mode )
		vk_mode = Cvar_Get( "vk_mode", "10", 0 );
	if( !vk_driver )
		vk_driver = Cvar_Get( "vk_driver", "vulkan", 0 );

	s_mode_list[SOFTWARE_MENU].curvalue = sw_mode->value;
	s_mode_list[OPENGL_MENU].curvalue = gl_mode->value;
	s_mode_list[VULKAN_MENU].curvalue = vk_mode->value;

	if ( !scr_viewsize )
		scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);

	s_screensize_slider[SOFTWARE_MENU].curvalue = scr_viewsize->value/10;
	s_screensize_slider[OPENGL_MENU].curvalue = scr_viewsize->value/10;
	s_screensize_slider[VULKAN_MENU].curvalue = scr_viewsize->value/10;

	if ( strcmp( vid_ref->string, "soft" ) == 0 )
	{
		s_current_menu_index = SOFTWARE_MENU;
		s_ref_list[0].curvalue = s_ref_list[1].curvalue = REF_SOFT;
	}
	else if ( strcmp(vid_ref->string, "vk") == 0 )
	{
		s_current_menu_index = VULKAN_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_VULKAN;
	}
	else if ( strcmp( vid_ref->string, "gl" ) == 0 )
	{
		s_current_menu_index = OPENGL_MENU;
		if ( strcmp( gl_driver->string, "3dfxgl" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_3DFX;
		else if ( strcmp( gl_driver->string, "pvrgl" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_POWERVR;
		else if ( strcmp( gl_driver->string, "opengl32" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_OPENGL;
		else
//			s_ref_list[s_current_menu_index].curvalue = REF_VERITE;
			s_ref_list[s_current_menu_index].curvalue = REF_OPENGL;
	}

	s_software_menu.x = viddef.width * 0.50;
	s_software_menu.nitems = 0;
	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;
	s_vulkan_menu.x = viddef.width * 0.50;
	s_vulkan_menu.nitems = 0;

	for ( i = 0; i < 3; i++ )
	{
		s_ref_list[i].generic.type = MTYPE_SPINCONTROL;
		s_ref_list[i].generic.name = "driver";
		s_ref_list[i].generic.x = 0;
		s_ref_list[i].generic.y = 0;
		s_ref_list[i].generic.callback = DriverCallback;
		s_ref_list[i].itemnames = refs;

		s_mode_list[i].generic.type = MTYPE_SPINCONTROL;
		s_mode_list[i].generic.name = "video mode";
		s_mode_list[i].generic.x = 0;
		s_mode_list[i].generic.y = 10;
		s_mode_list[i].itemnames = resolutions;

		s_screensize_slider[i].generic.type	= MTYPE_SLIDER;
		s_screensize_slider[i].generic.x		= 0;
		s_screensize_slider[i].generic.y		= 20;
		s_screensize_slider[i].generic.name	= "screen size";
		s_screensize_slider[i].minvalue = 3;
		s_screensize_slider[i].maxvalue = 12;
		s_screensize_slider[i].generic.callback = ScreenSizeCallback;

		s_brightness_slider[i].generic.type	= MTYPE_SLIDER;
		s_brightness_slider[i].generic.x	= 0;
		s_brightness_slider[i].generic.y	= 30;
		s_brightness_slider[i].generic.name	= "brightness";
		s_brightness_slider[i].generic.callback = BrightnessCallback;
		s_brightness_slider[i].minvalue = 5;
		s_brightness_slider[i].maxvalue = 13;
		s_brightness_slider[i].curvalue = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

		s_fs_box[i].generic.type = MTYPE_SPINCONTROL;
		s_fs_box[i].generic.x	= 0;
		s_fs_box[i].generic.y	= 40;
		s_fs_box[i].generic.name	= "fullscreen";
		s_fs_box[i].itemnames = yesno_names;
		s_fs_box[i].curvalue = vid_fullscreen->value;

		s_defaults_action[i].generic.type = MTYPE_ACTION;
		s_defaults_action[i].generic.name = "reset to defaults";
		s_defaults_action[i].generic.x    = 0;
		s_defaults_action[i].generic.y    = 90;
		s_defaults_action[i].generic.callback = ResetDefaults;

		s_cancel_action[i].generic.type = MTYPE_ACTION;
		s_cancel_action[i].generic.name = "cancel";
		s_cancel_action[i].generic.x    = 0;
		s_cancel_action[i].generic.y    = 100;
		s_cancel_action[i].generic.callback = CancelChanges;
	}

	s_stipple_box.generic.type = MTYPE_SPINCONTROL;
	s_stipple_box.generic.x	= 0;
	s_stipple_box.generic.y	= 60;
	s_stipple_box.generic.name	= "stipple alpha";
	s_stipple_box.curvalue = sw_stipplealpha->value;
	s_stipple_box.itemnames = yesno_names;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= 60;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_tqvk_slider.generic.type = MTYPE_SLIDER;
	s_tqvk_slider.generic.x = 0;
	s_tqvk_slider.generic.y = 60;
	s_tqvk_slider.generic.name = "texture quality";
	s_tqvk_slider.minvalue = 0;
	s_tqvk_slider.maxvalue = 3;
	s_tqvk_slider.curvalue = 3-vk_picmip->value;

	s_paletted_texture_box.generic.type = MTYPE_SPINCONTROL;
	s_paletted_texture_box.generic.x	= 0;
	s_paletted_texture_box.generic.y	= 70;
	s_paletted_texture_box.generic.name	= "8-bit textures";
	s_paletted_texture_box.itemnames = yesno_names;
	s_paletted_texture_box.curvalue = gl_ext_palettedtexture->value;

	s_msaa_mode.generic.type = MTYPE_SPINCONTROL;
	s_msaa_mode.generic.name = "multisampling";
	s_msaa_mode.generic.x = 0;
	s_msaa_mode.generic.y = 70;
	s_msaa_mode.itemnames = msaa_modes;
	s_msaa_mode.curvalue = vk_msaa->value;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x	= 0;
	s_finish_box.generic.y	= 80;
	s_finish_box.generic.name	= "sync every frame";
	s_finish_box.curvalue = gl_finish->value;
	s_finish_box.itemnames = yesno_names;

	s_vkfinish_box.generic.type = MTYPE_SPINCONTROL;
	s_vkfinish_box.generic.x = 0;
	s_vkfinish_box.generic.y = 80;
	s_vkfinish_box.generic.name = "sync every frame";
	s_vkfinish_box.curvalue = vk_finish->value;
	s_vkfinish_box.itemnames = yesno_names;

	Menu_AddItem( &s_software_menu, ( void * ) &s_ref_list[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_mode_list[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_screensize_slider[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_brightness_slider[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_fs_box[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_stipple_box );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_paletted_texture_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box );

	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_ref_list[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_mode_list[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_screensize_slider[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_brightness_slider[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_fs_box[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_tqvk_slider);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_msaa_mode);
	Menu_AddItem( &s_vulkan_menu, ( void * ) &s_vkfinish_box);

	Menu_AddItem( &s_software_menu, ( void * ) &s_defaults_action[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_cancel_action[SOFTWARE_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_cancel_action[OPENGL_MENU] );
	Menu_AddItem( &s_vulkan_menu, (void * ) &s_defaults_action[VULKAN_MENU]);
	Menu_AddItem( &s_vulkan_menu, (void * ) &s_cancel_action[VULKAN_MENU]);

	Menu_Center( &s_software_menu );
	Menu_Center( &s_opengl_menu );
	Menu_Center( &s_vulkan_menu );
	s_opengl_menu.x -= 8;
	s_software_menu.x -= 8;
	s_vulkan_menu.x -= 8;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int w, h;

	if ( s_current_menu_index == 0 )
		s_current_menu = &s_software_menu;
	else if (s_current_menu_index == 2)
		s_current_menu = &s_vulkan_menu;
	else
		s_current_menu = &s_opengl_menu;

	/*
	** draw the banner
	*/
	re.DrawGetPicSize( &w, &h, "m_banner_video" );
	re.DrawPic( viddef.width / 2 - w / 2, viddef.height /2 - 110, "m_banner_video" );

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{
	menuframework_s *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch ( key )
	{
	case K_ESCAPE:
		ApplyChanges( 0 );
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		break;
	}

	return sound;
}


