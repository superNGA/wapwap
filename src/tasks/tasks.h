//=========================================================================
//                      Tasks
//=========================================================================
// by      : INSANE
// created : 04/11/2026
//
// purpose : A list of tasks to be performed sequencially, to generate our pdf.
//-------------------------------------------------------------------------
#ifndef TASKS_H
#define TASKS_H

#include "../ilib/ilib_arena_alloc.h"
#include "file_entity.h"



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct tasks {
	struct file_entity **entities;
	size_t entity_count;
};


int parse_taskfile(struct tasks *t, const char *file);


#endif // TASKS_H
