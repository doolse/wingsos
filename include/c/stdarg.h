#define va_list char *
#define va_round(TYPE) \
 (((sizeof (TYPE) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(args,fmt) args = (char *) (&(fmt) + 1)
#define va_end 
#define va_arg(AP, TYPE) \
 (AP = (char *) ((char *) (AP) + va_round (TYPE)),	\
  *((TYPE *) (void *) ((char *) (AP) - va_round (TYPE))))
