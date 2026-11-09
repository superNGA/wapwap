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
#include <vector>
#include "entity_data.h"



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
enum file_entity_type {
	FILEENTITY_CODE_BLOCK,
	FILEENTITY_HEADING,
	FILEENTITY_TEXT,
	FILEENTITY_NEWPAGE
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct file_entity {
	enum file_entity_type type;
	char *file_line;

	std::vector<entity_data> data;
};


const char *get_file_entity_type_name(enum file_entity_type t);


#endif // FILE_ENTITY_H
