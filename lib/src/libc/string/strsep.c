#include <string.h>

char *strsep(char **string_ptr, const char *delimiter) {
	char *s1;
	char *s2=*string_ptr;
	
	if (s2 == NULL)
		return (char *)NULL;
	s1 = s2 + strspn(s2,delimiter);
	if (*s1 == '\0')
		return (char *)NULL;
	s2 = strpbrk(s1,delimiter);
	if (s2 != NULL)
		*s2++='\0';
	*string_ptr=s2;
	return s1;
}
