#ifndef MSQ_GLOBAL_H
#define MSQ_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MSQ_LIBRARY)
#  define MSQSHARED_EXPORT Q_DECL_EXPORT
#else
#  define MSQSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // MSQ_GLOBAL_H
