//=========================================================================
//                      File Entity
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : A file entity. Can be a code-block, heading, text etc...
//-------------------------------------------------------------------------
#include "file_entity.h"



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////l
const char *get_file_entity_type_name(enum file_entity_type t)
{
	switch (t) {
	case FILEENTITY_CODE_BLOCK: return "File-Entity CODE-BLOCK";
	case FILEENTITY_HEADING:    return "File-Entity HEADING";
	case FILEENTITY_TEXT:       return "File-Entity TEXT";
	case FILEENTITY_NEWPAGE:    return "File-Entity NEWPAGE";
	}
	return "Invlaid fileentity type";
}
