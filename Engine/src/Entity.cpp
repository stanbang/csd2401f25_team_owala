#include "Entity.hpp"

Entity nextEntityID = 0;

Entity createEntity() {
    return nextEntityID++;
}
