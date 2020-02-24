#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

//#define UIO_DEVICE    "/dev/uio6"
#define MMAP_SIZE     0x1000

int main(int argc, char* argv[])
{
  int retCode = 0;
  int uioFd;
  char *UIO_DEVICE = argv[1], *buf;
  volatile uint32_t* counters;

  // Open uio device
  uioFd = open(UIO_DEVICE, O_RDWR);
  if(uioFd < 0)
  {
    fprintf(stderr, "Cannot open %s: %s\n", UIO_DEVICE, strerror(errno));
    return -1;
  }

  // Mmap memory region containing counters value
  counters = mmap(NULL, MMAP_SIZE, PROT_READ, MAP_SHARED, uioFd, 0);
  if(counters == MAP_FAILED)
  {
    fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
    close(uioFd);
    return -1;
  }

  // Interrupt loop
  while(1)
  {
    uint32_t intInfo;
    ssize_t readSize;

    // Acknowldege interrupt
    intInfo = 1;
    /*if(write(uioFd, buf, 4) < 0)
    {
      fprintf(stderr, "Cannot acknowledge uio device interrupt: %s\n",
        strerror(errno));
      retCode = -1;
      break;
    }*/

    //try to read from the memory
    printf("%d\n", counters[0]);

    // Wait for interrupt
    readSize = read(uioFd, &intInfo, sizeof(intInfo));
    if(readSize < 0)
    {
      fprintf(stderr, "Cannot wait for uio device interrupt: %s\n",
        strerror(errno));
      retCode = -1;
      break;
    }

    // Display counter value
    printf("We got %lu interrupts, counter value: 0x%08x\n",
      intInfo, counters[0]);
  }

  // Should never reach
  munmap((void*)counters, MMAP_SIZE);
  close(uioFd);

  return retCode;
}

