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

#include "config.h"
#include "util/log_io.h"


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
const float MARGIN_LEFT   = 10.0;
const float MARGIN_RIGHT  = 10.0;



static int ww_add_from_file(struct config *c, const char *f);
static int ww_file_to_lines(struct config *c, const char *f, std::vector<char*>* lines_out, float max_line_width_pxl);
static int ww_box_the_page(float it, float xpos, HPDF_Page page, int max_boxes, float width, float height, struct vec3 clr_a, struct vec3 clr_b);
static int ww_stroke_codebox(HPDF_Page page, bool top, bool bottom, float tx, float ty, float bx, float by);
static int ww_pushback_a4page(struct config *c);
static int ww_add_heading(struct config *c, const char *msg);
static int ww_add_exec_file(struct config *c, const char *f);
static int ww_addlinebg(HPDF_Page page, struct vec3 clr, float x, float y, float width, float height);
static int ww_textout(struct config *c, const char *msg, float xoff);
static int ww_copy_page_stats(HPDF_Page from, HPDF_Page to);

static int ww_split_line(HPDF_Page page, float split_width, char *line, std::vector<char*>* lines_out);
static int ww_is_comment(const char *c);
static int ww_is_lastpage_dirty(struct config *c);
static float ww_get_curfont_height(HPDF_Page page);
static float ww_get_curfont_descend(HPDF_Page page);
static HPDF_Page ww_get_active_page(struct config *c);

static HPDF_Font init_font(HPDF_Doc pdf, const char *font_file);
static HPDF_Page add_a4_page(HPDF_Doc* pdf);
static void sanitize_strline_inplace(char *str);
static void ww_pushfront_str(char *dest, char *src);

void err_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	HPDF_Doc pdf = HPDF_New(err_handler, NULL);
	if (!pdf) {
		FAIL_LOG("Failed to create new file");
		return 1;
	}
	WIN_LOG("doc initialized");

	HPDF_Font font_jbm = init_font(pdf, "JetBrainsMono.ttf");
	if (!font_jbm) {
		FAIL_LOG("Failed to load font");
		return ER;
	}
	HPDF_Font font_hding = init_font(pdf, "UniSansHeavy.ttf");
	if (!font_hding) {
		FAIL_LOG("Failed to load uni sand heavy font");
		return ER;
	}
	WIN_LOG("Fonts initialized");

	config c = {0};
	c.pdf = pdf;
	c.book.clear();
	c.it = 0.0;
	c.line_num = true;

	c.font_codetext      = font_jbm;
	c.font_codecomment   = font_jbm;
	c.font_heading       = font_hding;
	c.font_output        = font_jbm;
	c.font_linenum       = font_jbm;

	c.code_font_size     = 8.0;
	c.output_font_size   = 10.0;
	c.heading_font_size  = 16.0;

	c.clr_codetext       = BLACK;
	c.clr_codecomment    = DARK_GREEN;
	c.clr_codebox_border = MID_GRAY;
	c.clr_output         = BLACK;
	c.clr_heading        = BLACK;
	c.clr_headingdecor   = MID_GRAY;
	c.clr_codeline_a     = LIGHT_GRAY;
	c.clr_codeline_b     = MID_GRAY;
	c.clr_linenum        = BLACK;



	size_t file_count = argc - 1;
	for (size_t i = 1; i < argc; ++i) {
		const char *f = argv[i]; // files[i];

		if (ww_is_lastpage_dirty(&c) || c.book.empty())
			ww_pushback_a4page(&c);

		c.it = HPDF_Page_GetHeight(c.book.back()) - MARGIN_TOP;

		ww_add_heading(&c, f);
		ww_textout(&c, " ", 0.0); // empty line

		int status_prog = ww_add_from_file(&c, f);
		if (status_prog != OK) {
			FAIL_LOG("Failed to generate pdf for file : %s", f);
			return 1;
		}
		LOG("Code done. %s", f);

		ww_textout(&c, " ", 0.0); // empty line
		ww_add_heading(&c, "Output");
		ww_textout(&c, " ", 0.0); // empty line

		int status_output = ww_add_exec_file(&c, f);
		if (status_output != OK) {
			FAIL_LOG("Failed to generate pdf for file : %s", f);
			return 1;
		}
		LOG("Output done. %s", f);

		WIN_LOG("%s generated %.1f%%", f, 100.0 * (float)i / (float)file_count);
	}
	

	// Write file and exit
	HPDF_SaveToFile(pdf, "output.pdf");

FREE_AND_EXIT:
	HPDF_Free(pdf);
	return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_add_from_file(struct config *c, const char *f)
{
	// NOTE : Although custom font can be used for code comments, it plays no more
	// in determining the "code-background-box" height (or any other thing).
	// It is recommended to pick a reasonable font for comments.

	// page specs
	HPDF_Page active_page = ww_get_active_page(c);
	float pwidth = HPDF_Page_GetWidth(active_page);
	float pheight = HPDF_Page_GetHeight(active_page);

	// calculate font specs
	HPDF_Page_SetFontAndSize(active_page, c->font_codetext, c->code_font_size); 
	float font_height = ww_get_curfont_height(active_page);
	float font_descend = abs(ww_get_curfont_descend(active_page));

	// Split file into lines of length (in pxls) less than max_line_width
	std::vector<char*> lines;
	static const float CODEMARGIN_LEFT  = MARGIN_LEFT  + 10.0;
	static const float CODEMARGIN_RIGHT = MARGIN_RIGHT + 10.0;
	static const float BOXMARGIN_LEFT   = MARGIN_LEFT  + 5.0;
	static const float BOXMARGIN_RIGHT  = MARGIN_RIGHT + 5.0;
	float codebox_width = pwidth - BOXMARGIN_LEFT - BOXMARGIN_RIGHT;
	float max_line_width = pwidth - CODEMARGIN_LEFT - CODEMARGIN_RIGHT;
	if (ww_file_to_lines(c, f, &lines, max_line_width) != OK) {
		FAIL_LOG("Failed to split file into lines");
		return ER;
	}


	// Calculate space required for line num
	// float linenum_gap = 0.0;
	// if (c->line_num) {
	// 	char maxnum_buf[16] = {0}; // No one should be generating a pdf file with more than "16-digit" lines of code.
	// 	snprintf(maxnum_buf, sizeof(maxnum_buf), "%zu", lines.size());
	// 	linenum_gap = HPDF_Page_TextWidth(active_page, maxnum_buf);
	// }


	int total_boxes = lines.size();
	int boxes_left = total_boxes;
	HPDF_Page last_page_boxed = NULL;
	for (size_t i = 0; i < lines.size(); ++i) {
		active_page = ww_get_active_page(c);

		if (boxes_left > 0 && last_page_boxed != active_page) {
			float box_it = c->it - font_descend; // Acknowledge the subtracted font descend
			assert(box_it > MARGIN_BOTTOM && box_it < pheight - MARGIN_TOP && "Invalid box iterator");

			int box_drawn = ww_box_the_page(
					box_it, // draw start Y pos
					BOXMARGIN_LEFT, // box X pos
					active_page, // active page
					boxes_left, // max boxes that can be drawn
					codebox_width, // box width  
					font_height, // box height 
					c->clr_codeline_a, // box color A
					c->clr_codeline_b); // box color B
			float total_boxes_height = (float)box_drawn * font_height;


			// Draw border lines over the boxes
			HPDF_Page_SetLineWidth(active_page, 1.0);
			HPDF_Page_SetRGBStroke(active_page, c->clr_codebox_border.x, c->clr_codebox_border.y, c->clr_codebox_border.z);
			ww_stroke_codebox(
					active_page, // curpage
					boxes_left == total_boxes, // draw top line
					boxes_left == box_drawn, // draw bottom line
					BOXMARGIN_LEFT, // top X
					box_it, // top Y
					pwidth - BOXMARGIN_RIGHT, // bottom X
					box_it - total_boxes_height); // bottom Y

			// deducted drawn boxes
			boxes_left -= box_drawn;
			last_page_boxed = active_page;
		}

		const char *line = lines[i];
		bool is_comment = ww_is_comment(line);
		HPDF_Font font = is_comment ? c->font_codecomment : c->font_codetext;
		struct vec3 txt_clr = is_comment ? c->clr_codecomment : c->clr_codetext;

		HPDF_Page_SetRGBFill(active_page, txt_clr.x, txt_clr.y, txt_clr.z);
		// HPDF_Page_SetRGBStroke(active_page, txt_clr.x, txt_clr.y, txt_clr.z);
		HPDF_Page_SetFontAndSize(ww_get_active_page(c), font, 8.0);
		ww_textout(c, line, CODEMARGIN_LEFT);


		assert((c->book.back() == last_page_boxed || c->lastpage_linecount == 0) && "Box and text drawing are out of sync!");
	}

	// Free all lines
	for (char *l : lines)
		free(l);

	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_file_to_lines(struct config *c, const char *f, std::vector<char*>* lines_out, float max_line_width_pxl)
{
	FILE *file = fopen(f, "r");
	if (!file)
		return ER;
	
	HPDF_Page cur_page = ww_get_active_page(c);
	std::vector<char*> split_lines;
	char *line = NULL;
	size_t linelen = 0;
	int line_counter = 0;
	while (getline(&line, &linelen, file) > 0) {
		split_lines.clear();

		// count all tab chars
		size_t tab_count = 0;
		for (size_t i = 0; i < linelen; ++i)
			if (line[i] == '\t')
				++tab_count;

		// Allocate new buffer to compensate for expanded tabs
		int buf_failsafe_space = c->line_num ? 16 : 10; // +10 for get-out-of-jail free card
		char *new_buf = (char*)calloc(linelen + tab_count * (TAB_SIZE - 1) + buf_failsafe_space, 1); 
		strncpy(new_buf, line, linelen);

		// expand all tabs
		sanitize_strline_inplace(new_buf);

		// add line num
		if (c->line_num) {
			static char linenum_buf[16] = {0}; // No one should be generating a pdf file with more than "16-digit" lines of code.
			snprintf(linenum_buf, sizeof(linenum_buf), "%3d. ", line_counter + 1);
			ww_pushfront_str(new_buf, linenum_buf);
		}

		// Split the line for the sake of line-wrapping.
		ww_split_line(cur_page, max_line_width_pxl, new_buf, &split_lines);

		if (split_lines.empty()) {
			char *empty_line = (char*)calloc(1, sizeof(char));
			split_lines.push_back(empty_line);
		}

		for (char *line_piece : split_lines)
			lines_out->push_back(line_piece);

		++line_counter;
		free(new_buf);
		free(line); line = NULL;
	}

	fclose(file);
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
static int ww_pushback_a4page(struct config *c)
{
	HPDF_Page page = HPDF_AddPage(c->pdf);
	HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
	c->book.push_back(page);
	
	c->lastpage_linecount = 0;
	LOG("Added new page");
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_add_heading(struct config *c, const char *msg)
{
	static const float HEADING_MARGIN_LEFT = MARGIN_LEFT;
	static const float HEADING_MARGIN_RIGHT = MARGIN_RIGHT;

	HPDF_Page active_page = ww_get_active_page(c);

	HPDF_Page_SetFontAndSize(active_page, c->font_heading, c->heading_font_size);
	float font_height = ww_get_curfont_height(active_page);
	float font_descend = abs(ww_get_curfont_descend(active_page));

	float heading_width = HPDF_Page_GetWidth(active_page) - HEADING_MARGIN_LEFT - HEADING_MARGIN_RIGHT;
	float msg_width = HPDF_Page_TextWidth(active_page, msg);


	// Drawing heading decor (Draw tilted lines after the heading text)
	HPDF_Page_SetRGBStroke(active_page, c->clr_headingdecor.x, c->clr_headingdecor.y, c->clr_headingdecor.z);
	static const float HEADING_DECOR_LINEGAP = 6.0;
	static const float HEADING_DECORD_TILT = -3.5;
	const float decor_start_pos = HEADING_MARGIN_LEFT + msg_width + abs(HEADING_DECORD_TILT * 2.0); // Makes sure decor appears on right of the heading text.
	for (float x = decor_start_pos; x < heading_width; x += HEADING_DECOR_LINEGAP) {
		HPDF_Page_MoveTo(active_page, x + HEADING_DECORD_TILT, c->it - font_descend);
		HPDF_Page_LineTo(active_page, x - HEADING_DECORD_TILT, c->it - font_height - font_descend);
		HPDF_Page_Stroke(active_page);
	}

	HPDF_Page_SetRGBFill(active_page, c->clr_heading.x, c->clr_heading.y, c->clr_heading.z);
	ww_textout(c, msg, HEADING_MARGIN_LEFT);
	return OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_add_exec_file(struct config *c, const char *f)
{
	static char compile_cmdbuf[512] = {0};
	static char execute_cmdbuf[512] = {0};
	static char exe_name[] = "output.tuptuo";

	// Compile the program
	snprintf(compile_cmdbuf, sizeof(compile_cmdbuf), "g++ %s -o %s;", f, exe_name);
	if (system(compile_cmdbuf) != 0) {
		FAIL_LOG("Failed to compile. cmd : %s", compile_cmdbuf);
		return ER;
	}
	LOG("Compiled. > %s", compile_cmdbuf);

	// Execute the executable
	snprintf(execute_cmdbuf, sizeof(execute_cmdbuf), "./%s;", exe_name);
	FILE *output = popen(execute_cmdbuf, "r");
	if (!output) {
		FAIL_LOG("Failed to run executable [ %s ] for file: %s", execute_cmdbuf, f);
		return ER;
	}
	LOG("Executed. > %s", execute_cmdbuf);

	// Write program output to pdf
	HPDF_Page active_page = ww_get_active_page(c);
	HPDF_Page_SetFontAndSize(active_page, c->font_output, c->output_font_size);
	HPDF_Page_SetRGBFill(active_page, c->clr_output.x, c->clr_output.y, c->clr_output.z);

	static const float OUTPUT_MARGIN_LEFT = MARGIN_LEFT + 15.0;
	static char cmd_buf[512] = {0};
	snprintf(cmd_buf, sizeof(cmd_buf), "> %s %s", compile_cmdbuf, execute_cmdbuf);
	ww_textout(c, cmd_buf, OUTPUT_MARGIN_LEFT);

	while (fgets(cmd_buf, sizeof(cmd_buf), output)) {
		sanitize_strline_inplace(cmd_buf);
		ww_textout(c, cmd_buf, OUTPUT_MARGIN_LEFT);
	}

	pclose(output);
	return OK;
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
static int ww_textout(struct config *c, const char *msg, float xoff)
{
	// The new line is placed, treating the c->it (iterator) as the top-left corner
	// of the line.

	HPDF_Page active_page = ww_get_active_page(c);
	HPDF_Font active_font = HPDF_Page_GetCurrentFont(active_page);
	float font_height = ww_get_curfont_height(active_page);
	float page_height = HPDF_Page_GetHeight(active_page);

	// iterator too high on the page?
	if (c->it > page_height - MARGIN_TOP)
		c->it = page_height - MARGIN_TOP;


	// NOTE : We can always write at c->it - font_height (this line) because when 
	// c->it - font_height * 2.0 (next line) goes out of bound, we push_back new page 
	// (in which case c->it shall also be moved to the next (new) page).
	HPDF_Page_BeginText(active_page);
	HPDF_Page_TextOut(active_page, xoff, c->it - font_height, msg);
	++(c->lastpage_linecount);
	HPDF_Page_EndText(active_page);
	c->it -= font_height;


	// Next line out of range? pushback new page
	if (c->it - font_height < MARGIN_BOTTOM) {
		ww_pushback_a4page(c);

		HPDF_Page new_page = ww_get_active_page(c);
		ww_copy_page_stats(active_page, new_page);

		active_page = new_page;
		c->it = page_height - MARGIN_TOP;
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
static int ww_split_line(HPDF_Page page, float split_width, char *line, std::vector<char*>* lines_out)
{
	float pwidth = HPDF_Page_GetWidth(page);

	while (*line != '\0') {
		HPDF_REAL real_width = 0.0;
		// trying to split at word bondaries can potentially cause a infinite loop. To deal with
		// that we need to write extra code. Which I'm not in mood of. Plus nvim also wraps at character
		// boundary.
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
	size_t i = 0;
	while (c[i] != '\0') {
		if (c[i] != ' ')
			return strncmp(&c[i], COMMENT_STR, sizeof(COMMENT_STR) - 1) == 0;
		++i;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ww_is_lastpage_dirty(struct config *c)
{
	return c->lastpage_linecount > 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static HPDF_Font init_font(HPDF_Doc pdf, const char *font_file)
{
	const char *font_name = HPDF_LoadTTFontFromFile(pdf, font_file, HPDF_TRUE);
	if (!font_name) {
		FAIL_LOG("Failed to load font from file");
		return NULL;
	}

	return HPDF_GetFont(pdf, font_name, NULL);
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
static HPDF_Page ww_get_active_page(struct config *c)
{
	return c->book.back();
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static HPDF_Page add_a4_page(HPDF_Doc* pdf)
{
	HPDF_Page page = HPDF_AddPage(*pdf);
	HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
	return page;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static size_t linelen(const char *s) 
{
	size_t l = 0;
	while (s[l] != '\0' && s[l] != '\n')
		++l;

	return l;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void sanitize_strline_inplace(char *str)
{
	size_t tab_size = 4;
	size_t shift_size = tab_size - 1;
	size_t i = 0;
	int tabs = 0;
	while (str[i] != '\0' && str[i] != '\n') {
		if (str[i] == '\t') {
			for (size_t j = linelen(str); j > i; --j)
				str[j + shift_size] = str[j];
			for (size_t j = i; j < i + tab_size; ++j)
				str[j] = ' ';
		}
		++i;
	}

	str[i] = '\0';
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void ww_pushfront_str(char *dest, char *src)
{
	size_t numlen = strlen(src);
	size_t len = strlen(dest);
	for (int i = len + 1; i >= 0; --i)
		dest[i + numlen] = dest[i];
	for (int i = 0; i < numlen; ++i)
		dest[i] = src[i];
	return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void err_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) 
{ 
	FAIL_LOG("libharu error [ %d ], detail no %d\n", (int)error_no, (int)detail_no); 
	exit(1);
}
