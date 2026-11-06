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
#include "../ilib/ilib_vector.h"


static char* find_char(char *haystack, char needle);
static int find_entity_data(char *src, enum entity_data_type *datatype_out, char **data_out);
static int calc_entity_type(char *line_start, enum file_entity_type *type_out);
static struct file_entity* alloc_fileentity(char *line_buf, enum file_entity_type type, struct entity_data *entitydata_vec, size_t entitydata_count);
static void free_fileentity(struct file_entity *e);

static const char* get_file_entity_type_name(enum file_entity_type t);
static const char* get_entity_data_type_name(enum entity_data_type t);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int parse_taskfile(struct tasks *t, const char *file)
{
	FILE *f = fopen(file, "r");
	if (!f) {
		FATAL_LOG("failed to open file : %s", file);
		return 1;
	}


	struct file_entity **vec_temp_entities = NULL;
	struct entity_data *vec_temp_data = NULL;

	char *line_buf = NULL;
	size_t linelen = 0;
	while (getline(&line_buf, &linelen, f) > 0) {
		// the bare minimum task line would atleast have 6 characters
		// example <c>[t] so any line with less than 6 characters can be ignored safely upfront.
		if (linelen <= 6) {
			WARN_LOG("empty line found in task file %s", file);
			goto FAILURE_EXIT;
		}

		// <c> check
		enum file_entity_type fe_type = FILEENTITY_CODE_BLOCK;
		if (calc_entity_type(line_buf, &fe_type) != 0) {
			FAIL_LOG("syntax error. Failed to determine file entity type.");
			goto FAILURE_EXIT;
		}
		LOG("Detected file_entity of type : %s", get_file_entity_type_name(fe_type));

		// Collect all entity data inside this file_entity (file line)
		vec_clear(vec_temp_data);
		struct entity_data d = {0}; char *entity_it = line_buf;
		while (find_entity_data(entity_it, &d.type, &d.data) == 0) {
			LOG("type : %s; data : %s", get_entity_data_type_name(d.type), d.data);
			vec_pushback(vec_temp_data, d);
			// A valid data pointer, must be present after the '[t]' syntax.
			// So we safely scan after it to find the next entity_data.
			entity_it = d.data;
		}
		if (vec_empty(vec_temp_data)) {
			FAIL_LOG("Empty file entity encountered.");
			goto FAILURE_EXIT;
		}


		struct file_entity* entity = alloc_fileentity(line_buf, fe_type, vec_temp_data, vec_len(vec_temp_entities));
		vec_pushback(vec_temp_entities, entity);
	}

	if (vec_empty(vec_temp_entities)) {
		WARN_LOG("input task contains no valid taks");
		goto FAILURE_EXIT;
	}


SUCCESS_EXIT:
	fclose(f);
	t->entity_count = vec_len(vec_temp_entities);
	t->entities = (struct file_entity**)malloc(sizeof(void*) * t->entity_count);
	if (!t->entities) {
		FATAL_LOG("Failed to malloc. dying now.");
		exit(1);
	}
	for (int i = 0; i < vec_len(vec_temp_entities); ++i)
		t->entities[i] = vec_temp_entities[i];
	vec_free(vec_temp_entities);
	vec_free(vec_temp_data);
	return 0;

FAILURE_EXIT:
	fclose(f);
	for (int i = 0; i < vec_len(vec_temp_entities); ++i)
		free_fileentity(vec_temp_entities[i]);
	vec_free(vec_temp_entities);
	vec_free(vec_temp_data);
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
int find_entity_data(char *src, enum entity_data_type *datatype_out, char **data_out)
{
	// example format : [c]gcc somefile
	char *type_start = find_char(src, '[');
	if (!type_start)
		return 1;

	char *type_end = find_char(type_start, ']');
	if (!type_end)
		return 1;

	switch (*(type_start + 1)) {
	case 'c': case 'C': *datatype_out = ENTITYDATA_SHELLCMD; break;
	case 'f': case 'F': *datatype_out = ENTITYDATA_FILE; break;
	case 't': case 'T': *datatype_out = ENTITYDATA_TEXT; break;
	default: return 1;
	}

	printf("type start : %p\n", type_start);
	printf("type end   : %p\n", type_end);
	printf("type diff  : %td\n", type_end - type_start);
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
	default: break;
	}

	return 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static struct file_entity* alloc_fileentity(char *line_buf, enum file_entity_type type, struct entity_data *entitydata_vec, size_t entitydata_count)
{
	struct file_entity* e = (struct file_entity*)malloc(sizeof(struct file_entity));
	e->type = type;
	e->file_line = line_buf;
	e->child_count = entitydata_count;
	e->children = (struct entity_data*)malloc(sizeof(struct entity_data) * entitydata_count);
	for (int i = 0; i < entitydata_count; ++i)
		e->children[i] = entitydata_vec[i];

	return e;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void free_fileentity(struct file_entity *e)
{
	free(e->children);
	free(e->file_line);
	free(e);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static const char* get_file_entity_type_name(enum file_entity_type t)
{
	switch (t) {
	case FILEENTITY_CODE_BLOCK: return "File-Entity CODE-BLOCK";
	case FILEENTITY_HEADING: return "File-Entity HEADING";
	case FILEENTITY_TEXT: return "File-Entity TEXT";
	}
	return "Invlaid fileentity type";
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
