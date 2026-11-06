//=========================================================================
//                      Config
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : Holds all visual parameters for wap wap pdf generation.
//-------------------------------------------------------------------------
#ifndef CONFIG_H
#define CONFIG_H

#include "../vec3.h"
#include <stdbool.h>


#define MAX_FONTFILE_NAMELEN (512)


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct config {
	bool line_num;
	
	// heading
	char heading_font[MAX_FONTFILE_NAMELEN];
	float heading_size;
	struct vec3 heading_clr;
	struct vec3 heading_decor_clr;

	// code.text
	char code_font[MAX_FONTFILE_NAMELEN];
	float code_size;
	struct vec3 code_clr;
	// code.comment
	char comment_font[MAX_FONTFILE_NAMELEN];
	struct vec3 comment_clr;
	// code.codeline
	struct vec3 codeline_clr_a;
	struct vec3 codeline_clr_b;

	// terminal output
	char output_font[MAX_FONTFILE_NAMELEN];
	float output_size;
	struct vec3 output_clr;
};


void parse_configfile(struct config *c, const char *file);


#endif // CONFIG_H
