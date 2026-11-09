//=========================================================================
//                      Tasks
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : A list of tasks to be performed sequencially, to generate our pdf.
//-------------------------------------------------------------------------
#include "tasks.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../util/log_io.h"


static char* find_char           (char *haystack, char needle);
static int   find_entity_data    (char *src, enum entity_data_type *datatype_out, char **data_out, char **start_out);
static int   calc_entity_type    (char *line_start, enum file_entity_type *type_out);
static int   collect_all_ent_data(char *src, std::vector<entity_data> *out);

static const char* get_entity_data_type_name(enum entity_data_type t);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int parse_taskfile(std::vector<file_entity>* tasks_out, const char *file)
{
	FILE *f = fopen(file, "r");
	if (!f) {
		FATAL_LOG("failed to open file : %s", file);
		return 1;
	}


	std::vector<entity_data> ent_data; ent_data.clear();


	char *line_buf = NULL;
	size_t linelen = 0;
	while (getline(&line_buf, &linelen, f) > 0) {
		// the bare minimum task line would atleast have 6 characters
		// example <c>[t] so any line with less than 6 characters can be ignored safely upfront.
		if (linelen <= 6) {
			WARN_LOG("empty line found in task file %s", file);
			goto FAILURE_EXIT;
		}

		// discard any new line chars. Causes trouble with file names n shit
		char *tmp = line_buf;
		while (*tmp != '\0') {
			if (*tmp == '\n') {
				*tmp = '\0';
				break;
			}
			++tmp;
		}

		// <c> check
		enum file_entity_type fe_type = FILEENTITY_CODE_BLOCK;
		if (calc_entity_type(line_buf, &fe_type) != 0) {
			FAIL_LOG("syntax error. Failed to determine file entity type.");
			goto FAILURE_EXIT;
		}
		LOG("Detected file_entity of type : %s", get_file_entity_type_name(fe_type));


		// Collect all entity data inside this file_entity (file line)
		ent_data.clear();
		collect_all_ent_data(line_buf, &ent_data);
		if (ent_data.empty() && fe_type != FILEENTITY_NEWPAGE) { // <p> is allowed to be empty. because thats convinient for me.
			FAIL_LOG("Empty file entity encountered.");
			goto FAILURE_EXIT;
		}


		// pushback this file_entity
		struct file_entity ent; {
			ent.type = fe_type;
			ent.file_line = line_buf;
			for (size_t i = 0; i < ent_data.size(); ++i)
				ent.data.push_back(ent_data[i]);
		}

		tasks_out->push_back(ent);
		line_buf = NULL;
	}

	if (tasks_out->empty()) {
		WARN_LOG("task file %s contains no valid tasks", file);
		goto FAILURE_EXIT;
	}


	fclose(f);
	return 0;

FAILURE_EXIT:
	fclose(f);
	for (size_t i = 0; i < tasks_out->size(); ++i)
		free((*tasks_out)[i].file_line);
	// line buffer is set to NULL each time it is accepted and stored in its `file_entity` struct.
	// Upon failure, we free all valid file_entities captued till now.
	// But we also must free the very line buffer that caused the failure for it won't be accepted 
	// in any `file_entity` struct.
	if (line_buf)
		free(line_buf);
	return 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
char* find_char(char *haystack, char needle)
{
	while (*haystack != '\0') {
		if (*haystack == needle)
			return haystack;
		++haystack;
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int find_entity_data(char *src, enum entity_data_type *datatype_out, char **data_out, char **start_out)
{
	// example format : [c]gcc somefile
	char *type_start = find_char(src, '[');
	if (!type_start)
		return 1;
	*start_out = type_start;

	char *type_end = find_char(type_start, ']');
	if (!type_end)
		return 1;

	switch (*(type_start + 1)) {
	case 'c': case 'C': *datatype_out = ENTITYDATA_SHELLCMD; break;
	case 'f': case 'F': *datatype_out = ENTITYDATA_FILE; break;
	case 't': case 'T': *datatype_out = ENTITYDATA_TEXT; break;
	default: return 1;
	}

	*data_out = type_end + 1;
	return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int calc_entity_type(char *line_start, enum file_entity_type *type_out)
{
	assert(*line_start != '\0' && *(line_start + 1) != '\0' && "invalid ( too-short ) line received");

	switch (line_start[1]) {
	case 'c': case 'C': *type_out = FILEENTITY_CODE_BLOCK; return 0;
	case 't': case 'T': *type_out = FILEENTITY_TEXT; return 0;
	case 'h': case 'H': *type_out = FILEENTITY_HEADING; return 0;
	case 'p': case 'P': *type_out = FILEENTITY_NEWPAGE; return 0;
	default: break;
	}

	return 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int collect_all_ent_data(char *src, std::vector<entity_data> *out)
{
	struct entity_data d = {0};
	char *d_start = NULL;
	while (find_entity_data(src, &d.type, &d.data, &d_start) == 0) {
		out->push_back(d);

		// cap out start of each entity_data. i.e. all valid [t] turn to '\0't].
		if (d_start)
			*d_start = 0;

		// A valid data pointer (d.data), must be present after the '[t]' syntax.
		// by passing d.data as the src, we will find the next file entity data (if present).
		src = d.data;
	}
	return 0;
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static const char* get_entity_data_type_name(enum entity_data_type t)
{
	switch (t) {
	case ENTITYDATA_FILE : return "Entity-Data FILE";
	case ENTITYDATA_SHELLCMD : return "Entity-Data SHELL-CMD";
	case ENTITYDATA_TEXT : return "Entity-Data TEXT";
	}
	return "invalid entity data type";
}
