#pragma once

#include <QtWidgets/QWidget>
#include "ui_JPEG.h"

class JPEG : public QWidget,public Ui::JPEGClass
{
    Q_OBJECT

public:
    JPEG(QWidget *parent = nullptr);
    ~JPEG();

    void updateText();

    void openVideo();

    void openflash();

    void JPEGBian();

    void JPEGJie();

    void JPEGBian_dat();

    void JPEGJie_dat();

private:
    Ui::JPEGClass* ui;
};
