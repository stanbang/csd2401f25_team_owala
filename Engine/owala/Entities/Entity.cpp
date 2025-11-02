#include "Entity.h"

Entity nextEntityID = 1; // 0 is reserved for invalid/null entity

Entity createEntity() {
    return nextEntityID++;
}
