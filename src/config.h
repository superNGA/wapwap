//=========================================================================
//                      Config
//=========================================================================
// by      : INSANE
// created : 31/10/2026
//
// purpose : This config dictates the wapwap pdf generation visuals
//-------------------------------------------------------------------------
#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include "../lib/libharu/include/hpdf.h"
#include "vec3.h"

struct vec3;


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct config {
	HPDF_Doc pdf;
	std::vector<HPDF_Page> book; 
	size_t lastpage_linecount;

	float it;
	bool line_num;

	float code_font_size; 
	float heading_font_size;
	float output_font_size;

	HPDF_Font font_codetext;
	HPDF_Font font_codecomment;
	HPDF_Font font_heading;
	HPDF_Font font_output;
	HPDF_Font font_linenum;

	struct vec3 clr_codetext;
	struct vec3 clr_codecomment;
	struct vec3 clr_codebox_border;
	struct vec3 clr_output;
	struct vec3 clr_heading;
	struct vec3 clr_headingdecor;
	struct vec3 clr_codeline_a;
	struct vec3 clr_codeline_b;
	struct vec3 clr_linenum;
};


#endif // CONFIG_H
