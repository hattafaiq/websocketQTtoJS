#ifndef THREADER_H
#define THREADER_H

#include <QtCore>
#include <QDebug>
#include "save_database.h"
#include "setting.h"

class threader : public QThread
{
public:
    threader();
    save_database *base;
    void run();
    QByteArray bb1[JUM_PLOT];
    int num;
    int fmax;
    int ref_rpm;//[JUM_PLOT];
    int safe_to_save_ch[JUM_PLOT];
};

#endif // THREADER_H
