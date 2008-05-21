/*
* $Id$
*/

#ifndef __SEGMENT_H__
#define __SEGMENT_H__
#define B   0
#define O   0xFFFF

unsigned short circle3[] = {
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,O,O,O,O,O,O,O,O,B,B,B,B,B,
    B,B,B,O,O,O,O,O,O,O,O,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
};

unsigned short circle2[] = {
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
};

unsigned short circle1[] = {
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
};

unsigned short horizontal[] = {
    B,B,B,B,B,B,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,B,B,B,B,B,B,B,
    B,B,B,O,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,O,B,O,O,O,B,O,B,B,B,B,B,B,
    B,B,B,O,B,O,O,O,B,O,B,B,B,B,B,B,
    B,B,B,B,B,O,O,O,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
};

unsigned short vertical[] = {
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,O,B,B,B,B,
    B,B,B,O,O,O,O,O,B,B,O,O,O,B,B,B,
    B,B,B,O,O,O,O,O,B,O,O,O,O,O,B,B,
    B,B,B,O,O,O,O,O,B,B,O,O,O,B,B,B,
    B,B,B,B,B,B,O,O,B,B,B,O,B,B,B,B,
    B,B,B,B,O,O,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
};

unsigned short sleep[] = {
    B,B,B,B,B,B,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,B,B,B,O,B,B,B,B,B,
    B,B,B,B,B,B,O,B,B,O,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,O,B,B,B,B,B,B,B,
    B,B,B,B,B,B,O,B,B,B,B,B,B,B,B,B,
    B,B,O,B,B,O,O,B,B,B,B,B,B,B,B,B,
    B,B,B,O,O,O,O,B,B,B,B,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,O,B,B,B,B,B,
    B,B,B,B,O,O,O,O,O,O,B,B,B,B,B,B,
    B,B,B,O,O,O,O,O,B,B,B,B,B,B,B,B,
    B,B,B,O,O,B,O,O,O,B,B,B,B,B,B,B,
    B,B,O,O,B,B,B,B,O,O,B,B,B,B,B,B
};

#undef O
#undef B
#endif
