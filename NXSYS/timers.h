
typedef void (*NXTimerFn)(void*);
typedef void (*CoderFn)(void*, BOOL);

void NXTimer (void* object, NXTimerFn fn, long ms);
void KillNXTimers();
void KillOneTimer (void* object);

void NXCoder (void* object, CoderFn fn);
void KillOneCoder (void* object);
void NXFastCoder (void* object, CoderFn fn);
void KillOneFastCoder (void* object);  /* uh oh ....:) */

void RunTimers();
void HaltTimers();
