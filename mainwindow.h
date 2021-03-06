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
    int armed = 0;
    QString activeBuzzer = "";
    QPushButton *armedButton;
    QLabel *activeLabel;
    QString buttonColorToStyleSheet(QString buzzerColorName);

signals:
    void pingReceived(QString buzzerName, QString sourceAddress, float batVolt, quint32 e131_status);

private slots:
    void processPendingDatagrams();
    void updateStatusLabel(QString buzzerName, QString sourceAddress, float batVolt, quint32 e131_status);
    void pingTimeout();
    void armedButtonClicked();
    void updateActiveBuzzerLabel();
    void resetTriggerDisplay();
    void trgdButtonClicked();
    void triggerBuzzer(QString buzzerName);
    void sendOSC(QString path, char value);
};
#endif // MAINWINDOW_H
