#include "features.h"
#ifdef EMBPERL
#include "logger.h"
#include "embperl.h"

#include "perlpacket.h"

#ifdef THIS     /* this macro seems to leak out on some systems */
#undef THIS        
#endif

MODULE = PerlPacket   PACKAGE = PerlPacket

PROTOTYPES: ENABLE



