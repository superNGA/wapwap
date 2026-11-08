//=========================================================================
//                      State
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : wapwap state
//-------------------------------------------------------------------------
#ifndef STATE_H
#define STATE_H

#include <vector>
#include <stdlib.h>
#include "../lib/libharu/include/hpdf.h"

struct config;


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct wapwap_state {
	HPDF_Doc pdf;
	std::vector<HPDF_Page> book;

	float it;
	size_t lines_lastpage;

	HPDF_Font heading_font;
	HPDF_Font text_font;
	HPDF_Font codetext_font;
	HPDF_Font codecomment_font;

	struct config *config;
};


#endif // STATE_H
