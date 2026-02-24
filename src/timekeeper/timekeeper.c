#include "timekeeper.h"

typedef struct {
    uint32_t global_tick;
    uint32_t mf_tick;
    uint32_t current_subframe;
    bool major_frame_restart;
} TimekeeperState_t;

static TimekeeperConfig_t tk_config;
static TimekeeperState_t tk_state;

static void vUpdateSubframe(void){
    for (uint32_t i = 0; i < tk_config.num_subframes; i++) {
        if (tk_state.mf_tick >= tk_config.subframes[i].start_tick && tk_state.mf_tick < tk_config.subframes[i].end_tick) {
            tk_state.current_subframe = i;
            return;
        }
    }
    tk_state.current_subframe = tk_config.num_subframes;
}

void vTimekeeperInit(const TimekeeperConfig_t *cfg){
    tk_config = *cfg;
    tk_state.global_tick = 0;
    tk_state.mf_tick = 0;
    tk_state.current_subframe = 0;
    tk_state.major_frame_restart = false;
}

void vTimekeeperUpdate(void){
    tk_state.global_tick++;
    tk_state.mf_tick++;
    tk_state.major_frame_restart = false;

    if (tk_state.mf_tick >= tk_config.major_frame_ticks) {
        tk_state.mf_tick = 0;
        tk_state.major_frame_restart = true;
    }
    vUpdateSubframe();
}

// getter function
bool vTimekeeperMajorFrameRestart(void){
    return tk_state.major_frame_restart;
}

uint32_t vTimekeeperGetCurrentSubframe(void){
    volatile uint32_t tmp = tk_state.current_subframe;
    return tmp;
}

uint32_t vTimekeeperGetCurrentTickInMF(void){
    volatile uint32_t tmp = tk_state.mf_tick;
    return tmp;
}
