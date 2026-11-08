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
#include <string>


#define MAX_FONTFILE_NAMELEN (512)


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct config {
	bool line_num;
	
	// heading
	std::string heading_font;
	float heading_size;
	struct vec3 heading_clr;
	struct vec3 heading_decor_clr;

	// code.text
	std::string code_font;
	float code_size;
	struct vec3 code_clr;
	// code.comment
	std::string comment_font;
	struct vec3 comment_clr;
	// code.codeline
	struct vec3 codeline_clr_a;
	struct vec3 codeline_clr_b;

	// text
	std::string text_font;
	float text_size;
	struct vec3 text_clr;
};


void parse_configfile(config *c, const char *file);


#endif // CONFIG_H
