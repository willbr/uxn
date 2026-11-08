/* Wrapper main for the vendored uxnmin: it puts stdin/stdout in binary mode
 * on Windows (otherwise ROM bytes get mangled), then calls the real main,
 * which is compiled with -Dmain=uxnmin_main so the vendored file is untouched. */
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

int uxnmin_main(int argc, char **argv);

int
main(int argc, char **argv)
{
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	return uxnmin_main(argc, argv);
}
