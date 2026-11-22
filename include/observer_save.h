#ifndef MBAI_OBSERVER_SAVE_H
#define MBAI_OBSERVER_SAVE_H
#include "observer.h"
/* Optional .watch.0/.watch.1 companion to an ALREADY saved v1 Game.
 * Exact sequence plus full-Game digest prevents attaching another baby's
 * learned phrases/history. Failure never changes out or ordinary Game. */
int observer_save_load(const char *base,const Game *saved,
                        ObserverProfile *out);
int observer_save_write(const char *base,const Game *saved,
                         const ObserverProfile *profile);
int observer_save_erase(const char *base);
#endif
