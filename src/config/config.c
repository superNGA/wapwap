//=========================================================================
//                      Config
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : Holds all visual parameters for wap wap pdf generation.
//-------------------------------------------------------------------------
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/log_io.h"

// heading variables
#define HEADING_FONTFILE      "HEADING_FONTFILE"
#define HEADING_FONTSIZE      "HEADING_FONTSIZE"
#define HEADING_CLR           "HEADING_CLR"
#define HEADING_DECOR_CLR     "HEADING_DECOR_CLR"

// code.text variables
#define CODETEXT_FONTFILE     "CODETEXT_FONTFILE"
#define CODE_FONTSIZE         "CODE_FONTSIZE"
#define CODETEXT_CLR          "CODETEXT_CLR"
// code.comment variables
#define CODE_COMMENT_FONTFILE "CODE_COMMENT_FONTFILE"
#define CODECOMMENT_CLR       "CODECOMMENT_CLR"
// code.codeline
#define CODELINE_CLR_A        "CODELINE_CLR_A"
#define CODELINE_CLR_B        "CODELINE_CLE_B"

// output variables
#define OUTPUT_FONTFILE       "OUTPUT_FONTFILE"
#define OUTPUT_FONTSIZE       "OUTPUT_FONTSIZE"
#define OUTPUT_CLR            "OUTPUT_CLR"


static char* find_value_start(char *s);
static void store_string(char *dest, const char *src);
static void store_csv(struct vec3 *dest, const char *src);
static void store_float(float *dest, const char *src);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void parse_configfile(struct config *c, const char *file)
{
	FILE *f = fopen(file, "r");
	if (!f) {
		FATAL_LOG("failed to open conig file :'%s'", file);
		exit(1);
	}


	char *fileline = NULL; size_t linelen = 0;
	while (getline(&fileline, &linelen, f) > 0) {
		char *value = find_value_start(fileline);
		if (!value) {
			FATAL_LOG("Failed to parse line :'%s'", fileline);
			goto FATAL_EXIT;
		}

		if (strncmp(fileline, HEADING_FONTFILE, sizeof(HEADING_FONTFILE) - 1) == 0) {
			store_string(c->heading_font, value);
		}
		else if (strncmp(fileline, HEADING_FONTSIZE, sizeof(HEADING_FONTSIZE) - 1) == 0) {
			store_float(&c->heading_size, value);
		}
		else if (strncmp(fileline, HEADING_CLR, sizeof(HEADING_CLR) - 1) == 0) {
			store_csv(&c->heading_clr, value);
		}
		else if (strncmp(fileline, HEADING_DECOR_CLR, sizeof(HEADING_DECOR_CLR) - 1) == 0) {
			store_csv(&c->heading_decor_clr, value);
		}
		else if (strncmp(fileline, CODETEXT_FONTFILE, sizeof(CODETEXT_FONTFILE) - 1) == 0) {
			store_string(c->code_font, value);
		}
		else if (strncmp(fileline, CODE_FONTSIZE, sizeof(CODE_FONTSIZE) - 1) == 0) {
			store_float(&c->code_size, value);
		}
		else if (strncmp(fileline, CODETEXT_CLR, sizeof(CODETEXT_CLR) - 1) == 0) {
			store_csv(&c->code_clr, value);
		}
		else if (strncmp(fileline, CODE_COMMENT_FONTFILE, sizeof(CODE_COMMENT_FONTFILE) - 1) == 0) {
			store_string(c->comment_font, value);
		}
		else if (strncmp(fileline, CODECOMMENT_CLR, sizeof(CODECOMMENT_CLR) - 1) == 0) {
			store_csv(&c->comment_clr, value);
		}
		else if (strncmp(fileline, CODELINE_CLR_A, sizeof(CODELINE_CLR_A) - 1) == 0) {
			store_csv(&c->codeline_clr_a, value);
		}
		else if (strncmp(fileline, CODELINE_CLR_B, sizeof(CODELINE_CLR_B) - 1) == 0) {
			store_csv(&c->codeline_clr_b, value);
		}
		else if (strncmp(fileline, OUTPUT_FONTFILE, sizeof(OUTPUT_FONTFILE) - 1) == 0) {
			store_string(c->output_font, value);
		}
		else if (strncmp(fileline, OUTPUT_FONTSIZE, sizeof(OUTPUT_FONTSIZE) - 1) == 0) {
			store_float(&c->output_size, value);
		}
		else if (strncmp(fileline, OUTPUT_CLR, sizeof(OUTPUT_CLR) - 1) == 0) {
			store_csv(&c->output_clr, value);
		}	

		free(fileline);
	}


	fclose(f);
	return;

FATAL_EXIT:
	if (f)
		fclose(f);
	exit(1);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static char* find_value_start(char *s) 
{
	bool word_started = false;
	char last_char = *s;

	while (*s != '\0') {
		char this_char = *s;
		bool is_alhpabet = (this_char >= 'a' && this_char <= 'z') || (this_char >= 'A' && this_char <= 'Z');
		if (is_alhpabet)
			word_started = true;

		if (this_char == ' ' && !word_started)
			return NULL;

		if (word_started && last_char == ' ' && this_char != ' ')
			return s;

		last_char = this_char;
		++s;
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void store_string(char *dest, const char *src)
{
	strncpy(dest, src, MAX_FONTFILE_NAMELEN);

	// sanitize the stored string a little bit.
	for (int i = 0; i < MAX_FONTFILE_NAMELEN; ++i)
		if (dest[i] == '\n')
			dest[i] = 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void store_csv(struct vec3 *dest, const char *src)
{
	const char *str = src; char *strend = NULL;

	dest->x = strtof(str, &strend); str = strend;
	dest->y = strtof(str, &strend); str = strend;
	dest->z = strtof(str, &strend); str = strend;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void store_float(float *dest, const char *src)
{
	*dest = atof(src);
}
