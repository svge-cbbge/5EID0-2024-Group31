#ifndef MAIN_H 
#define MAIN_H

#include <json-c/json.h>
#include <json-c/json_object.h>
#include <libpynq.h>
#include <platform.h>
#include <stdint.h>
#include <stepper.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "simple_uart.h"
#include <unistd.h>


#define MOVE_CONST    63.26656122907641
#define ROTATE_CONST  6.901311249137336

#define LEFT_IR   IO_AR12
#define RIGHT_IR  IO_AR11 
#define FRONT_IR  IO_AR13

#endif