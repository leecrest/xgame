#include "define.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


int InitLog(const char* file);
void Log(const char* fmt, ...);

int Connect2Net();