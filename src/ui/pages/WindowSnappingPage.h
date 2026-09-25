#pragma once

#include <QWidget>

class QScrollArea;

class WindowSnappingPage : public QWidget {
public:
    explicit WindowSnappingPage(QScrollArea *sidebar, QWidget *parent = nullptr);
};
