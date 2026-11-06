//=========================================================================
//                      Entity Data
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : Entity data for a file entity. Can be a file, shell cmd, or plain old text.
//-------------------------------------------------------------------------
#ifndef ENTITY_DATA_H
#define ENTITY_DATA_H

#define ENTITY_DATA_MAX_LEN (512)



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
enum entity_data_type {
	ENTITYDATA_FILE,
	ENTITYDATA_SHELLCMD,
	ENTITYDATA_TEXT,
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct entity_data {
	char *data;
	enum entity_data_type type;
};

#endif // ENTITY_DATA_H
