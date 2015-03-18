#ifndef TARGET_CPU
#define TARGET_CPU "armv7"
#endif

#ifndef OS_VER_MIN
#define OS_VER_MIN "4.2"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

char *get_executable_path(char *buf, size_t len)
{
    char *p;
#ifdef __APPLE__
    unsigned int l = len;
    if (_NSGetExecutablePath(buf, &l) != 0) return NULL;
#elif defined(__FreeBSD__)
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t l = len;
    if (sysctl(mib, 4, buf, &l, NULL, 0) != 0) return NULL;
#elif defined(_WIN32)
    size_t l = GetModuleFileName(NULL, buf, (DWORD)len);
#else
    ssize_t l = readlink("/proc/self/exe", buf, len);
#endif
    if (l <= 0) return NULL;
    buf[len - 1] = '\0';
    p = strrchr(buf, '/');
    if (p) *p = '\0';
    return buf;
}

char *get_filename(char *str)
{
    char *p = strrchr(str, '/');
    return p ? &p[1] : str;
}

void target_info(char *argv[], char **triple, char **compiler)
{
    char *p = get_filename(argv[0]);
    char *x = strrchr(p, '-');
    if (!x) abort();
    *compiler = &x[1];
    *x = '\0';
    *triple = p;
}

void env(char **p, const char *name, char *fallback)
{
    char *ev = getenv(name);
    if (ev) { *p = ev; return; }
    *p = fallback;
}

int main(int argc, char *argv[])
{
    char **args = alloca(sizeof(char*) * (argc+12));
    int i, j;

    char execpath[PATH_MAX+1];
    char sdkpath[PATH_MAX+1];
    char codesign_allocate[64];
    char osvermin[64];

    char *compiler;
    char *target;

    char *sdk;
    char *cpu;
    char *osmin;

    target_info(argv, &target, &compiler);
    if (!get_executable_path(execpath, sizeof(execpath))) abort();
    snprintf(sdkpath, sizeof(sdkpath), "%s/../SDK", execpath);
 
    snprintf(codesign_allocate, sizeof(codesign_allocate),
             "%s-codesign_allocate", target);

    setenv("CODESIGN_ALLOCATE", codesign_allocate, 1);
    setenv("IOS_FAKE_CODE_SIGN", "1", 1);

    env(&sdk, "IOS_SDK_SYSROOT", sdkpath);
    env(&cpu, "IOS_TARGET_CPU", TARGET_CPU);

    env(&osmin, "IPHONEOS_DEPLOYMENT_TARGET", OS_VER_MIN);
    unsetenv("IPHONEOS_DEPLOYMENT_TARGET");

    snprintf(osvermin, sizeof(osvermin), "-miphoneos-version-min=%s", osmin);

    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-arch"))
        {
            cpu = NULL;
            break;
        }
    }

    i = 0;

    args[i++] = compiler;
    args[i++] = "-target";
    args[i++] = target;
    args[i++] = "-isysroot";
    args[i++] = sdk;

    if (cpu)
    {
        args[i++] = "-arch";
        args[i++] = cpu;
    }

    args[i++] = osvermin;
    args[i++] = "-mlinker-version=241.9";

    for (j = 1; j < argc; ++i, ++j)
        args[i] = argv[j];

    args[i] = NULL;

    setenv("COMPILER_PATH", execpath, 1);
    execvp(compiler, args);

    fprintf(stderr, "cannot invoke compiler!\n");
    return 1;
}
