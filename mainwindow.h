#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
class MainWindow;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
