//=========================================================================
//                      wapwap
//=========================================================================
// by      : INSANE
// created : 31/10/2026
//
// purpose : generate pdf with output for single file programs
//-------------------------------------------------------------------------
#include <cmath>
#include <stdio.h>
#include <vector>
#include <cassert>
#include <cstring>

// libharu
#include "../lib/libharu/include/hpdf.h"

#include "config/config.h"
#include "tasks/tasks.h"
#include "wapwap_state.h"
#include "util/log_io.h"
#include "txtline.h"


#define OK (0)
#define ER (1)

// standard colors...
#define BLACK      ((vec3){0.0, 0.0, 0.0})
#define WHITE      ((vec3){1.0, 1.0, 1.0})
#define LIGHT_GRAY ((vec3){0.95, 0.95, 0.95})
#define MID_GRAY   ((vec3){0.9, 0.9, 0.9})
#define RED        ((vec3){1.0, 0.0, 0.0})
#define BLUE       ((vec3){0.0, 0.0, 1.0})
#define GREEN      ((vec3){0.0, 1.0, 0.0})
#define DARK_GREEN ((vec3){0.02, 0.1, 0.01})


#define COMMENT_STR "//"
#define TAB_SIZE (4)


#define min(x, y) ((x) < (y) ? (x) : (y))



// page margin
const float MARGIN_TOP    = 10.0;
const float MARGIN_BOTTOM = 10.0;

const float TEXTMARGIN_LEFT     = 10.0;
const float TEXTMARGIN_RIGHT    = 10.0;

const float CODEMARGIN_LEFT     = 20.0;
const float CODEMARGIN_RIGHT    = 20.0;
const float CODEBOXMARGIN_LEFT  = 10.0;
const float CODEBOXMARGIN_RIGHT = 10.0;

const float HEADINGMARGIN_LEFT  = 10.0;
const float HEADINGMARGIN_RIGHT = 10.0;



static int       ww_do_task            (struct wapwap_state *s, struct config *c, struct file_entity *task);
static int       ww_gen_linelist       (struct wapwap_state *s, struct config *c, struct file_entity *task, std::vector<txtline> *ll, bool linenum, float split_width);
static int       ww_line_chop_n_tag    (struct wapwap_state *s, struct config *c, std::string *l, std::vector<txtline> *ll, bool linenum, float split_width);
static int       ww_split_line         (HPDF_Page page, float split_width, const char *line, std::vector<char*> *lines_out);
static int       ww_is_comment         (const char *c);
static int       ww_getline_fmt        (FILE *f, std::string *line);
static void      ww_add_linenum        (std::string *l, int num);

static int       ww_dotask_newpage     (struct wapwap_state *s);
static int       ww_dotask_heading     (struct wapwap_state *s, const char *msg);
static int       ww_dotask_text        (struct wapwap_state *s, std::vector<txtline> *ll);
static int       ww_dotask_code        (struct wapwap_state *s, std::vector<txtline> *ll);
static int       ww_box_the_page       (float it, float xpos, HPDF_Page page, int max_boxes, float width, float height, struct vec3 clr_a, struct vec3 clr_b);
static int       ww_addlinebg          (HPDF_Page page, struct vec3 clr, float x, float y, float width, float height);
static int       ww_stroke_codebox     (HPDF_Page page, bool top, bool bottom, float tx, float ty, float bx, float by);

static HPDF_Page ww_get_active_page    (struct wapwap_state *s);
static int       ww_pushback_a4page    (struct wapwap_state *s);
static int       ww_textout            (struct wapwap_state *s, const char *msg, float xoff);
static int       ww_copy_page_stats    (HPDF_Page from, HPDF_Page to);

static HPDF_Font ww_init_font          (HPDF_Doc pdf, const char *font_file);
static float     ww_get_curfont_height (HPDF_Page page);
static float     ww_get_curfont_descend(HPDF_Page page);

void err_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	if (argc != 3) {
		FAIL_LOG("Invalid agruments passed");
		LOG("Usage: ./wapwap.out configfile taskfile");
		return 0;
	}
	

	HPDF_Doc pdf = HPDF_New(err_handler, NULL);
	if (!pdf)
		return 1;
	HPDF_UseUTFEncodings(pdf);
	HPDF_SetCurrentEncoder(pdf, "UTF-8");
	WIN_LOG("doc initialized");


	// tasks file
	const char *task_file = argv[2];
	std::vector<file_entity> t; t.clear();
	if (parse_taskfile(&t, task_file) != 0)
		return 1;
	WIN_LOG("Task file %s pased. %zu tasks queued", task_file, t.size());


	// config file
	const char *config_file = argv[1];
	struct config c = {0};
	parse_configfile(&c, config_file);
	WIN_LOG("config file parsed : %s", config_file);


	// wapwap state
	struct wapwap_state s = {0};
	s.pdf              = pdf;
	s.config           = &c;
	s.heading_font     = ww_init_font(s.pdf, c.heading_font.c_str());
	s.text_font        = ww_init_font(s.pdf, c.text_font.c_str());
	s.codetext_font    = ww_init_font(s.pdf, c.code_font.c_str());
	s.codecomment_font = ww_init_font(s.pdf, c.comment_font.c_str());
	WIN_LOG("state constructed. All fonts initialized.");


	ww_pushback_a4page(&s);
	s.it = HPDF_Page_GetHeight(ww_get_active_page(&s)) - MARGIN_TOP;
	for (int i = 0; i < t.size(); ++i) {
		if (ww_do_task(&s, &c, &t[i]) != OK)
			goto FAILURE_EXIT;
		WIN_LOG("Task complete : %d complete. %zu entity_data(s) processed", i, t[i].data.size());
	}
	

	// Write file and exit
	HPDF_SaveToFile(pdf, "output.pdf");

FAILURE_EXIT:
	HPDF_Free(pdf);
	return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_do_task(struct wapwap_state *s, struct config *c, struct file_entity *task)
{
	// NOTE : select and apply font here so all further font size calculations are accurate to 
	// the purpose.
	HPDF_Font font      = NULL;
	float     font_size = -1.0;
	bool      linenum   = false;
	float     split_width = HPDF_Page_GetWidth(ww_get_active_page(s));

	switch (task->type) {
	// new page doesn't require any configuration. but i'mma let
	// it slide like the rest of the types(with a dummy font) else 
	// I would need to put an ugly ass edge-case if-statement.
	case FILEENTITY_NEWPAGE: 
	case FILEENTITY_CODE_BLOCK:
		font      = s->codetext_font;
		font_size = c->code_size;
		linenum   = c->line_num;
		split_width -= CODEMARGIN_LEFT + CODEMARGIN_RIGHT;
		break;
	case FILEENTITY_HEADING:
		font      = s->heading_font;
		font_size = c->heading_size;
		split_width -= HEADINGMARGIN_LEFT + HEADINGMARGIN_RIGHT;
		break;
	case FILEENTITY_TEXT:
		font      = s->text_font;
		font_size = c->text_size;
		split_width -= TEXTMARGIN_LEFT + TEXTMARGIN_RIGHT;
		break;
	}
	HPDF_Page_SetFontAndSize(ww_get_active_page(s), font, font_size);


	std::vector<txtline> ll; ll.clear();
	if (ww_gen_linelist(s, c, task, &ll, linenum, split_width) != OK)
		return ER;

	switch (task->type) {
	case FILEENTITY_CODE_BLOCK:
		ww_dotask_code(s, &ll);
		break;
	case FILEENTITY_HEADING:
		ww_dotask_heading(s, ll.front().txt);
		break;
	case FILEENTITY_TEXT:
		ww_dotask_text(s, &ll);
		break;
	case FILEENTITY_NEWPAGE:
		ww_dotask_newpage(s);
		break;
	}

	// free this linelist
	for (size_t i = 0; i < ll.size(); ++i)
		free(ll[i].txt);

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_gen_linelist(struct wapwap_state *s, struct config *c, struct file_entity *task, std::vector<txtline> *ll, bool linenum, float split_width)
{
	for (int i = 0; i < task->data.size(); ++i) {
		entity_data *d      = &task->data[i];
		HPDF_Page   curpage = ww_get_active_page(s);

		switch (d->type) {
		case ENTITYDATA_TEXT:
		{
			std::string line(d->data);
			ww_line_chop_n_tag(s, c, &line, ll, linenum, split_width);
			break;
		}
		case ENTITYDATA_FILE:
		{
			FILE *f = fopen(d->data, "r");
			if (!f) {
				FAIL_LOG("failed to open file : %s", d->data);
				return ER;
			}

			std::string line;
			while (ww_getline_fmt(f, &line) == OK) {
				ww_line_chop_n_tag(s, c, &line, ll, linenum, split_width);
				line.clear();
			}

			fclose(f);
			break;
		}
		case ENTITYDATA_SHELLCMD:
		{
			FILE *f = popen(d->data, "r");
			if (!f) {
				FAIL_LOG("failed to open file : %s", d->data);
				return ER;
			}

			std::string line;
			while (ww_getline_fmt(f, &line) == OK) {
				ww_line_chop_n_tag(s, c, &line, ll, linenum, split_width);
				line.clear();
			}

			pclose(f);
			break;
		}
		}
	}
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_line_chop_n_tag(struct wapwap_state *s, struct config *c, std::string *l, std::vector<txtline> *ll, bool linenum, float split_width)
{
	bool is_comment = ww_is_comment(l->c_str());

	// expand tabs & add linenum
	if (linenum)
		ww_add_linenum(l, (int)ll->size());


	HPDF_Page curpage = ww_get_active_page(s);
	std::vector<char*> split_lines; split_lines.clear();
	ww_split_line(curpage, split_width, l->c_str(), &split_lines);

	// empty line
	if (split_lines.empty())
		split_lines.push_back((char*)calloc(1, 1));

	int flags = 0;
	if (is_comment)
		flags |= lineflag_comment;
	for (size_t i = 0; i < split_lines.size(); ++i)
		ll->push_back(txtline(split_lines[i], flags));

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_split_line(HPDF_Page page, float split_width, const char *line, std::vector<char*> *lines_out)
{
	while (*line != '\0') {
		HPDF_REAL real_width = 0.0;
		size_t split_len = HPDF_Page_MeasureText(page, line, split_width, false, &real_width);

		char *line_buf = (char*)calloc(split_len + 2, 1);
		strncpy(line_buf, line, split_len);
		lines_out->push_back(line_buf);

		line += split_len;
	}

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_is_comment(const char *c)
{
	// ignore ' ' and [0-9]. once first non-ignorable character has been encoutered,
	// string matches against 'COMMENT_STR'. Returns 0 for not comment, !0 for comment.
	for (;; ++c) {
		if (*c == '\0')
			break;
		if (*c == ' ')
			continue;
		if (*c >= '0' && *c <= '9')
			continue;
		return strncmp(c, COMMENT_STR, sizeof(COMMENT_STR) - 1) == 0;
	}

	return false;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_getline_fmt(FILE *f, std::string *line)
{
	char ch = fgetc(f);
	if (ch == EOF)
		return ER;

	while (ch != EOF) {
		if (ch == '\n' || ch == '\0')
			break;
		
		if (ch == '\t') {
			for (size_t i = 0; i < TAB_SIZE; ++i)
				line->push_back(' ');
		}
		else {
			line->push_back(ch);
		}

		ch = fgetc(f);
	}

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void ww_add_linenum(std::string *s, int num)
{
	// "16 digit" lines of code is big enough. 
	char tmp[16] = {0};
	snprintf(tmp, sizeof(tmp), "%3d ", num);
	s->insert(0, tmp);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////	
static int ww_dotask_newpage(struct wapwap_state *s)
{
	if (s->lines_lastpage > 0) {
		ww_pushback_a4page(s);
		s->it = HPDF_Page_GetHeight(ww_get_active_page(s)) - MARGIN_TOP;
	}
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_dotask_heading(struct wapwap_state *s, const char *msg)
{
	HPDF_Page active_page = ww_get_active_page(s);

	HPDF_Page_SetFontAndSize(active_page, s->heading_font, s->config->heading_size);
	float font_height = ww_get_curfont_height(active_page);
	float font_descend = abs(ww_get_curfont_descend(active_page));

	float heading_width = HPDF_Page_GetWidth(active_page) - HEADINGMARGIN_LEFT - HEADINGMARGIN_RIGHT;
	float msg_width = HPDF_Page_TextWidth(active_page, msg);


	// Drawing heading decor (Draw tilted lines after the heading text)
	HPDF_Page_SetRGBStroke(active_page, s->config->heading_decor_clr.x, s->config->heading_decor_clr.y, s->config->heading_decor_clr.z);
	static const float HEADING_DECOR_LINEGAP = 6.0;
	static const float HEADING_DECORD_TILT = -3.5;
	const float decor_start_pos = HEADINGMARGIN_LEFT + msg_width + abs(HEADING_DECORD_TILT * 2.0); // Makes sure decor appears on right of the heading text.
	for (float x = decor_start_pos; x < heading_width; x += HEADING_DECOR_LINEGAP) {
		HPDF_Page_MoveTo(active_page, x + HEADING_DECORD_TILT, s->it - font_descend);
		HPDF_Page_LineTo(active_page, x - HEADING_DECORD_TILT, s->it - font_height - font_descend);
		HPDF_Page_Stroke(active_page);
	}

	HPDF_Page_SetRGBFill(active_page, s->config->heading_clr.x, s->config->heading_clr.y, s->config->heading_clr.z);
	ww_textout(s, msg, HEADINGMARGIN_LEFT);
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_dotask_text(struct wapwap_state *s, std::vector<txtline> *ll)
{
	HPDF_Page curpage = ww_get_active_page(s);
	HPDF_Page_SetFontAndSize(curpage, s->text_font, s->config->text_size);
	HPDF_Page_SetRGBFill(curpage, s->config->text_clr.x, s->config->text_clr.y, s->config->text_clr.z);
	for (size_t i = 0; i < ll->size(); ++i)
		ww_textout(s, (*ll)[i].txt, TEXTMARGIN_LEFT);
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_dotask_code(struct wapwap_state *s, std::vector<txtline> *ll)
{
	// NOTE : Although custom font can be used for code comments, it plays no more
	// in determining the "code-background-box" height (or any other thing).
	// It is recommended to pick a reasonable font for comments.

	// page specs
	HPDF_Page active_page = ww_get_active_page(s);
	float pwidth = HPDF_Page_GetWidth(active_page);
	float pheight = HPDF_Page_GetHeight(active_page);

	// calculate font specs
	HPDF_Page_SetFontAndSize(active_page, s->codetext_font, s->config->code_size); 
	float font_height = ww_get_curfont_height(active_page);
	float font_descend = abs(ww_get_curfont_descend(active_page));
	float codebox_width = pwidth - CODEBOXMARGIN_LEFT - CODEBOXMARGIN_RIGHT;


	int total_boxes = ll->size();
	int boxes_left = total_boxes;
	HPDF_Page last_page_boxed = NULL;
	for (size_t i = 0; i < ll->size(); ++i) {
		active_page = ww_get_active_page(s);

		if (boxes_left > 0 && last_page_boxed != active_page) {
			float box_it = s->it - font_descend; // Acknowledge the subtracted font descend
			assert(box_it > MARGIN_BOTTOM && box_it < pheight - MARGIN_TOP && "Invalid box iterator");

			int box_drawn = ww_box_the_page(
					box_it, // draw start Y pos
					CODEBOXMARGIN_LEFT, // box X pos
					active_page, // active page
					boxes_left, // max boxes that can be drawn
					codebox_width, // box width  
					font_height, // box height 
					s->config->codeline_clr_a, // box color A
					s->config->codeline_clr_b); // box color B
			float total_boxes_height = (float)box_drawn * font_height;


			// Draw border lines over the boxes
			HPDF_Page_SetLineWidth(active_page, 1.0);
			HPDF_Page_SetRGBStroke(active_page, s->config->codeline_clr_a.x, s->config->codeline_clr_a.y, s->config->codeline_clr_a.z);
			ww_stroke_codebox(
					active_page, // curpage
					boxes_left == total_boxes, // draw top line
					boxes_left == box_drawn, // draw bottom line
					CODEBOXMARGIN_LEFT, // top X
					box_it, // top Y
					pwidth - CODEBOXMARGIN_RIGHT, // bottom X
					box_it - total_boxes_height); // bottom Y

			// deducted drawn boxes
			boxes_left -= box_drawn;
			last_page_boxed = active_page;
		}

		txtline   lineinfo = (*ll)[i]; // size of txtline struct is "trivial"
		HPDF_Font font     = (lineinfo.flags & lineflag_comment) ? s->codecomment_font : s->codetext_font;
		vec3      txt_clr  = (lineinfo.flags & lineflag_comment) ? s->config->comment_clr : s->config->code_clr;

		HPDF_Page_SetRGBFill(active_page, txt_clr.x, txt_clr.y, txt_clr.z);
		HPDF_Page_SetFontAndSize(ww_get_active_page(s), font, 8.0);
		ww_textout(s, lineinfo.txt, CODEMARGIN_LEFT);

		assert((ww_get_active_page(s) == last_page_boxed || s->lines_lastpage == 0) && "Box and text drawing are out of sync!");
	}

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////	
static int ww_box_the_page(float it, float xpos, HPDF_Page page, int max_boxes, float width, float height, struct vec3 clr_a, struct vec3 clr_b)
{
	// returns number of box drawn.
	int max_drawable_boxes = (it - MARGIN_BOTTOM) / height;
	int box_count = min(max_boxes, max_drawable_boxes);
	float merged_box_height = (float)box_count * height;

	// draw one big ass box for color A
	ww_addlinebg(page, clr_a, xpos, it - merged_box_height, width, merged_box_height);

	// Draw alternate boxes in color B
	for (int box_i = 0; box_i < box_count / 2; ++box_i) {
		ww_addlinebg(page, clr_b, xpos, it - (float)(box_i + 1) * height * 2.0, width, height);
	}
	return box_count;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_addlinebg(HPDF_Page page, struct vec3 clr, float x, float y, float width, float height)
{
	HPDF_Page_SetRGBFill(page, clr.x, clr.y, clr.z);
	HPDF_Page_SetRGBStroke(page, clr.x, clr.y, clr.z);
	HPDF_Page_Rectangle(page, x, y, width, height);
	HPDF_Page_Fill(page);
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_stroke_codebox(HPDF_Page page, bool top, bool bottom, float tx, float ty, float bx, float by)
{
	// left line
	HPDF_Page_MoveTo(page, tx, ty);
	HPDF_Page_LineTo(page, tx, by);
	HPDF_Page_Stroke(page);

	// right line
	HPDF_Page_MoveTo(page, bx, ty);
	HPDF_Page_LineTo(page, bx, by);
	HPDF_Page_Stroke(page);
	
	if (top) {
		HPDF_Page_MoveTo(page, tx, ty);
		HPDF_Page_LineTo(page, bx, ty);
		HPDF_Page_Stroke(page);
	}

	if (bottom) {
		HPDF_Page_MoveTo(page, tx, by);
		HPDF_Page_LineTo(page, bx, by);
		HPDF_Page_Stroke(page);
	}
			
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static HPDF_Page ww_get_active_page(struct wapwap_state *s)
{
	return s->book.back();
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_pushback_a4page(struct wapwap_state *s)
{
	HPDF_Page page = HPDF_AddPage(s->pdf);
	HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
	s->book.push_back(page);
	s->lines_lastpage = 0;
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_textout(struct wapwap_state *s, const char *msg, float xoff)
{
	// The new line is placed, treating the c->it (iterator) as the top-left corner
	// of the line.

	HPDF_Page active_page = ww_get_active_page(s);
	HPDF_Font active_font = HPDF_Page_GetCurrentFont(active_page);
	float font_height = ww_get_curfont_height(active_page);
	float page_height = HPDF_Page_GetHeight(active_page);

	// iterator too high on the page?
	if (s->it > page_height - MARGIN_TOP)
		s->it = page_height - MARGIN_TOP;


	// NOTE : We can always write at c->it - font_height (this line) because when 
	// c->it - font_height * 2.0 (next line) goes out of bound, we push_back new page 
	// (in which case c->it shall also be moved to the next (new) page).
	HPDF_Page_BeginText(active_page);
	HPDF_Page_TextOut(active_page, xoff, s->it - font_height, msg);
	++(s->lines_lastpage);
	HPDF_Page_EndText(active_page);
	s->it -= font_height;


	// Next line out of range? pushback new page
	if (s->it - font_height < MARGIN_BOTTOM) {
		ww_pushback_a4page(s);

		HPDF_Page new_page = ww_get_active_page(s);
		ww_copy_page_stats(active_page, new_page);

		active_page = new_page;
		s->it = page_height - MARGIN_TOP;
	}

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_copy_page_stats(HPDF_Page from, HPDF_Page to)
{
	// font & size
	HPDF_Page_SetFontAndSize(to, HPDF_Page_GetCurrentFont(from), HPDF_Page_GetCurrentFontSize(from));

	// fill clr
	HPDF_RGBColor from_clr = HPDF_Page_GetRGBFill(from);
	HPDF_Page_SetRGBFill(to, from_clr.r, from_clr.g, from_clr.b);

	// stroke clr
	HPDF_RGBColor from_stroke_clr = HPDF_Page_GetRGBStroke(from);
	HPDF_Page_SetRGBStroke(to, from_stroke_clr.r, from_stroke_clr.g, from_stroke_clr.b);
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static HPDF_Font ww_init_font(HPDF_Doc pdf, const char *font_file)
{
	const char *font_name = HPDF_LoadTTFontFromFile(pdf, font_file, HPDF_TRUE);
	if (!font_name) {
		FATAL_LOG("failed to open font file : %s", font_file);
		exit(1);
	}

	HPDF_Font font = HPDF_GetFont(pdf, font_name, "UTF-8");
	if (!font) {
		FATAL_LOG("failed to construct font from fontfile : %s", font_file);
		exit(1);
	}

	return font;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static float ww_get_curfont_height(HPDF_Page page)
{
	HPDF_Font f = HPDF_Page_GetCurrentFont(page);
	float height = (HPDF_Font_GetAscent(f) - HPDF_Font_GetDescent(f)) * HPDF_Page_GetCurrentFontSize(page) / 1000.0;

	return height * 1.2;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static float ww_get_curfont_descend(HPDF_Page page)
{
	float raw_descend = HPDF_Font_GetDescent(HPDF_Page_GetCurrentFont(page));
	float font_size = HPDF_Page_GetCurrentFontSize(page);
	return font_size * raw_descend / 1000.0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void err_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) 
{ 
	FAIL_LOG("libharu error [ %d ], detail no %d\n", (int)error_no, (int)detail_no); 
	exit(1);
}
