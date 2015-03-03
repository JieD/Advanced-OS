
/* 
 * Computes the length of the string str up to 
 * but not including the terminating null character.
 */
int k_strlen(const char * str) {
	int l = 0;
	while(*str++)
		l++;
	return (l);
}

/*
 * Copies n characters from memory area str2 to memory area str1
 * This function returns a pointer to destination, which is str1.
 * TODO: check memory error
 */
void *k_memcpy(void *str1, const void *str2, int n) {
 	char *des = (char*) str1;
 	char *src = (char*) str2;
	while(n--) {
 		*des++ = *src++;
 	}
 	return str1;
}

/* Compares the first num bytes of the block of memory pointed by ptr1 
 * to the first num bytes pointed by ptr2, returning zero if they all match 
 * or a value different from zero representing which is greater if they do not.
 * Notice that, unlike strcmp, the function does not stop comparing after 
 * finding a null character.
 */
int k_memcmp(const void * ptr1, const void * ptr2, int n) {
	unsigned char *des = (unsigned char*) ptr1;
 	unsigned char *src = (unsigned char*) ptr2;
	while(n > 0) {
		int d = *des++ - *src++;
		if (d) return d;
		n--;
	}
	return 0;
}
