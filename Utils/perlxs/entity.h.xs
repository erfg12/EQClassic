#include "features.h"
#ifdef EMBPERL
#include "logger.h"
#include "embperl.h"

#include "entity.h"
#include "EntityList.h"

#ifdef THIS     /* this macro seems to leak out on some systems */
#undef THIS        
#endif


MODULE = EntityList   PACKAGE = EntityList

PROTOTYPES: ENABLE



