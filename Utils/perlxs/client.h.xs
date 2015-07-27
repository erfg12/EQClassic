
#include "features.h"
#ifdef EMBPERL
#include "logger.h"
#include "embperl.h"

#include "client.h"

#ifdef THIS	/* this macro seems to leak out on some systems */
#undef THIS
#endif

MODULE = Client   PACKAGE = Client

PROTOTYPES: ENABLE



