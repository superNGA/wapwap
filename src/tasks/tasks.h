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

#include <vector>
#include "file_entity.h"



int parse_taskfile(std::vector<file_entity>* tasks_out, const char *file);


#endif // TASKS_H
