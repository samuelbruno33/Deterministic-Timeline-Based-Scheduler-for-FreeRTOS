#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t start_tick;
    uint32_t end_tick;
} SubFrame_t;

typedef struct {
    uint32_t major_frame_ticks;
    uint32_t num_subframes;
    const SubFrame_t *subframes;
} TimekeeperConfig_t;

void vTimekeeperInit(const TimekeeperConfig_t *cfg);
void vTimekeeperUpdate(void);
bool vTimekeeperMajorFrameRestart(void);
uint32_t vTimekeeperGetCurrentSubframe(void);
uint32_t vTimekeeperGetCurrentTickInMF(void);


#endif

