#ifndef _wgs_obj_
#define _wgs_obj_

typedef struct JObj {
    void *VMT;
    void *Class;
} JObj;

typedef struct JObjClass {
    void *VMT;
    void *VMCode;
    unsigned int ObjSize;
    unsigned int MethSize;
    char *Name;
} JObjClass;

void *JSubclass(void *Class, int size, ...);
void *JNew(void *Class);

#endif
