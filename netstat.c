#define AT_FDCWD		-100
#define O_RDONLY		0
#define AT_EMPTY_PATH	0x1000

#include <sys/stat.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

//typedef unsigned long int uintptr; /* size_t */
//typedef long int intptr; /* ssize_t */

void* memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}
void* mempcpy (void *dst, const void *src, size_t len) { return (char *) memcpy (dst, src, len) + len; }

size_t strlen(const char *s) {
	const char* p;
	for (p=s; *p; p++);
	return p-s;
}

char* stpcpy(char *restrict dst, const char *restrict src) { char  *p;  p = mempcpy(dst, src, strlen(src)); *p = '\0';return p; }
char* strcpy(char *restrict dst, const char *restrict src) { stpcpy(dst, src); return dst; }
char* strcat(char *restrict dst, const char *restrict src) { stpcpy(dst + strlen(dst), src); return dst;}

void* syscall5(void* syscall_number, void* arg1, void* arg2, void* arg3, void* arg4, void* arg5);
void myexit(int status) {
	syscall5(
		(void*) 60, //SYS_exit
		(void*) (ssize_t) status,
		0, 0, 0, 0
	);
}

ssize_t read(int fd, void* buf, size_t count) {
	return (ssize_t) syscall5(
		(void*) 0, //SYS_read
		(void*) (ssize_t) fd,
		(void*) buf,
		(void*) count,
		0, 0
	);
}

ssize_t write(int fd, const void* buf, size_t count) {
	return (ssize_t) syscall5(
		(void*) 1, //SYS_write
		(void*) (ssize_t) fd,
		(void*) buf,
		(void*) count,
		0, 0
	);
}

int close(int fd) {
	return (ssize_t) syscall5(
		(void*) 3, //SYS_close
		(void*) (ssize_t) fd,
		0, 0, 0, 0
	);
}

int clock_nanosleep(clockid_t clockid, int flags, const struct timespec *request, struct timespec* remain) {
	return (ssize_t) syscall5(
		(void*) 230, //SYS_clock_nanosleep
		(void*) (ssize_t) clockid,
		(void*) (ssize_t) flags,
		(void*) request,
		(void*) remain,
		0
	);
}
unsigned int sleep(unsigned int seconds) {
	struct timespec ts = {seconds, 0};
	return clock_nanosleep(CLOCK_REALTIME, 0, &ts, &ts);	
}

char* itoa(int value, char* result, int base) {		// copied from https://www.strudel.org.uk/itoa/
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

int atoi(const char* str) {
	const char* p; int result=0;
	for(p=str; *p; p++) { 
		if(*p=='\n') continue;
		result *= 10; result += (int)((*p)-'0'); 
	}
	return result;
}

ssize_t print(const char* s) {
	return write(1, s, strlen(s));
}

int openat(int dirfd, const char *pathname, int flags) {
	return (ssize_t) syscall5(
		(void*) 257, //SYS_openat
		(void*) (ssize_t) dirfd,
		(void*) pathname,
		(void*) (ssize_t) flags,
		0, 0	
	);
}

int fstatat(int dirfd, const char *restrict pathname, struct stat *restrict statbuf, int flags) {
	return (ssize_t) syscall5(
		(void*) 262, //SYS_newfstatat
		(void*) (ssize_t) dirfd,
		(void*) pathname,
		(void*) statbuf,
		(void*) (ssize_t) flags,
		0	
	);
}

void numprint(int val) { char buf[100]; itoa(val, buf, 10); print(buf); }

int getFileContents(char* filepath, char* buf) {
	struct stat sb;
	int fd = openat(AT_FDCWD, filepath, O_RDONLY);
	fstatat(fd, "", &sb, AT_EMPTY_PATH);
	read(fd, buf, sb.st_size);
	close(fd);
	return fd;
}

void usage(const char *self) {
	char output[100] = "";
	strcat(output, "usage: ");
	strcat(output, self);
	strcat(output, " [interface_name]\n");
	print(output);
	myexit(1);
}

int main(int argc, char *argv[]) {
	if (argc != 2 ) {
		usage(argv[0]);
	} else {
		char rx_path[50]="/sys/class/net/", tx_path[50]="/sys/class/net/";
		strcat(rx_path, argv[1]); strcat(rx_path, "/statistics/rx_bytes");
		strcat(tx_path, argv[1]); strcat(tx_path, "/statistics/tx_bytes");
		char contents[20];
		getFileContents("/sys/class/net/wlp2s0/statistics/rx_bytes", contents);
		int rx1 = atoi(contents);
		getFileContents("/sys/class/net/wlp2s0/statistics/tx_bytes", contents);
		int tx1 = atoi(contents);
		sleep(1);
		getFileContents("/sys/class/net/wlp2s0/statistics/rx_bytes", contents);
		int rx2 = atoi(contents);
		getFileContents("/sys/class/net/wlp2s0/statistics/tx_bytes", contents);
		int tx2 = atoi(contents);
		int r_rate = rx2-rx1; 
		int t_rate = tx2-tx1; 
		const char* r_fmt = r_rate<1024 ? " B/s" : " kB/s";
		const char* t_fmt = t_rate<1024 ? " B/s" : " kB/s";
		r_rate = r_rate<1024 ? r_rate : r_rate/1024;
		t_rate = t_rate<1024 ? t_rate : t_rate/1024;
		char output[100] = ""; char tmp[100];
		itoa(r_rate, tmp, 10); strcat(output, "\u2193 "); strcat(output, tmp); strcat(output, r_fmt);
		itoa(t_rate, tmp, 10); strcat(output, "  \u2191 ");strcat(output, tmp); strcat(output, t_fmt);
		strcat(output, "\n");
		print(output);	
	}
	return 0;
}
