#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <yaz/diagbib1.h>
#include "index.h"
#include <charmap.h>
#include <data1.h>
#include "zebra_perl.h"
#include "zebra_api_ext.h"
#include "yaz/log.h"
#include <yaz/sortspec.h>


