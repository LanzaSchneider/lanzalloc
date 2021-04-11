# lanzalloc
A simple memory allocation library

# example
```c
#define BUF_SIZE 256  
unsigned char buffer[BUF_SIZE];  
struct lanzalloc* lanzalloc;  
if (!(lanzalloc = lanzalloc_initialize(buffer, BUF_SIZE, 10))) {  
    /* ERROR HANDLING */  
}  
unsigned char* ptr = (unsigned char*)lanzalloc_alloc(lanzalloc, 32);  
/* DO SOMETHING */  
lanzalloc_free(lanzalloc, ptr);  
```
