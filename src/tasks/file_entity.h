//=========================================================================
//                      File Entity
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : A file entity. Can be a code-block, heading, text etc...
//-------------------------------------------------------------------------
#ifndef FILE_ENTITY_H
#define FILE_ENTITY_H

#include <stddef.h>
#include "entity_data.h"



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
enum file_entity_type {
	FILEENTITY_CODE_BLOCK,
	FILEENTITY_HEADING,
	FILEENTITY_TEXT
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct file_entity {
	enum file_entity_type type;
	char *file_line;

	struct entity_data *children;
	size_t child_count;
};


#endif // FILE_ENTITY_H
