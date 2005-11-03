#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include "xenaccess.h"
#include "xa_private.h"

int main (int argc, char **argv)
{
    xa_instance_t xai;
    unsigned char *memory = NULL;

    uint32_t dom = 1;  // hard code this for easy testing

    xa_init(dom, &xai);
    memory = xa_mmap_pfn(&xai, PROT_READ, 0);

    if (memory == NULL){
        perror("Failed to map memory");
        goto error_exit;
    }
    
    print_hex(memory, XA_PAGE_SIZE);


error_exit:
    xa_destroy(&xai);
    if (memory) munmap(memory, XA_PAGE_SIZE);

    return 0;
}
