#pragma once
#include <cstdio>
#include <cstdlib>
inline void Check(bool value,const char* description) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",description);std::exit(1);}
}
