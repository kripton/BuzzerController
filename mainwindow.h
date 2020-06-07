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
#include <QPushButton>
#include <QGroupBox>
#include <QTimer>

#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>

QT_BEGIN_NAMESPACE
class MainWindow;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QUdpSocket oscSock;
    QJsonArray buzzers;

signals:
    void pingReceived(QString buzzerName, QString sourceAddress, float batVolt, quint32 e131_status);

private slots:
    void processPendingDatagrams();
    void updateStatusLabel(QString buzzerName, QString sourceAddress, float batVolt, quint32 e131_status);
    void pingTimeout();
};
#endif // MAINWINDOW_H
