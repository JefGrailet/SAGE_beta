/*
 * Voyager.cpp
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Voyager.h (see this file to learn further about the goals of 
 * such class).
 */

#include "Voyager.h"

Voyager::Voyager(Environment *env)
{
    this->env = env;
}

Voyager::~Voyager()
{
    // Nothing is deleted, because any pointer points to data structures used elsewhere
}
