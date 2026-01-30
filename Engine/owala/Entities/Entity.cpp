#include "Entity.h"

// STL
#include <random>

Entity nextEntityID = 1; // 0 is reserved for invalid/null entity

Entity createEntity() {
    return nextEntityID++;
}