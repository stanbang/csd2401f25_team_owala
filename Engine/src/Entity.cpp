#include "Entity.h"

Entity nextEntityID = 0;

Entity createEntity() {
    return nextEntityID++;
}
