#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;

    QString buzzerInfo = "[\
        {\"name\": \"red\"},\
        {\"name\": \"blue\"},\
        {\"name\": \"lime\"},\
        {\"name\": \"yellow\"},\
        {\"name\": \"magenta\"},\
        {\"name\": \"cyan\"}\
    ]";

    QJsonDocument doc = QJsonDocument::fromJson(buzzerInfo.toUtf8());
    buzzers = doc.array();

    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    mainLayout->addLayout(groupBoxLayout);

    int j = 1;
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        // GroupBox that has the bgcolor of that buzzer
        QGroupBox *gb = new QGroupBox(tr("Buzzer %1 - %2").arg(j).arg(val["name"].toString().toUpper()));
        QPalette pal = gb->palette();
        pal.setColor(QPalette::Background, QColor(val["name"].toString()));
        gb->setPalette(pal);
        groupBoxLayout->addWidget(gb);
        QVBoxLayout *gbL = new QVBoxLayout;
        gb->setLayout(gbL);
        // Save pointer to the GroupBox in buzzers object
        val.insert("groupBox", QJsonValue(QString("%1").arg((qint64)gb)));

        // Label that displays info about that buzzer
        // bgcolor depends on status
        QLabel *statusLabel = new QLabel(tr("Ping: NOK\nIP: ???.???.???.???\nBat: ?.??V\nE1.31 status: ???"));
        statusLabel->setStyleSheet("border: 1px solid black; color: white; background: darkred");
        gbL->addWidget(statusLabel);
        // Save pointer to the Label in buzzers object
        val.insert("statusLabel", QJsonValue(QString("%1").arg((qint64)statusLabel)));

        // Create a timer that turns the statusLabel red again when no ping came in for 5 seconds
        QTimer *statusTimer = new QTimer;
        statusTimer->setSingleShot(true);
        connect(statusTimer, &QTimer::timeout, this, &MainWindow::pingTimeout);
        val.insert("statusTimer", QJsonValue(QString("%1").arg((qint64)statusTimer)));

        // Overwrite the old object with the new one
        buzzers[j-1] = val;

        j++;
    }

    qDebug() << buzzers;

    QPushButton *armedButton = new QPushButton(tr("Buzzer NOT ARMED"));
    armedButton->setMinimumHeight(60);
    armedButton->setStyleSheet("border: 1px solid black; background: white; color: black;");
    mainLayout->addWidget(armedButton);

    QLabel *activeLabel = new QLabel(tr("No Buzzer active"));
    activeLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    activeLabel->setMinimumHeight(60);
    activeLabel->setStyleSheet("border: 1px solid black; background: white; color: black;");
    mainLayout->addWidget(activeLabel);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    this->setWindowTitle(tr("Buzzer controller"));
    this->resize(820, 300);

    oscSock.bind(6206, QUdpSocket::ShareAddress);
    connect(&oscSock, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagrams);

    connect(this, &MainWindow::pingReceived, this, &MainWindow::updateStatusLabel);
}

MainWindow::~MainWindow()
{

}

void MainWindow::processPendingDatagrams()
{
    QNetworkDatagram datagram;
    while (oscSock.hasPendingDatagrams()) {
        datagram = oscSock.receiveDatagram(100);

        // Parse the OSC packet
        QString oscPath = QString(datagram.data());
        qDebug() << "Received datagram:" << datagram.data() << "PATH:" << oscPath;

        if (!(oscPath.startsWith("/buzzer/"))) {
            // Not of interest
            continue;
        }

        QString buzzerName = oscPath.split('/')[2];
        qDebug() << "buzzerName:" << buzzerName;

        if (oscPath.endsWith("/ping")) {
            // The size of the packet is fixed after the first comma
            int commaPos = datagram.data().indexOf(',');
            if (commaPos < 0) {
                // Comma not found?
                continue;
            }

            char bytes[4];
            bytes[0] = *(datagram.data().data() + commaPos + 7);
            bytes[1] = *(datagram.data().data() + commaPos + 6);
            bytes[2] = *(datagram.data().data() + commaPos + 5);
            bytes[3] = *(datagram.data().data() + commaPos + 4);

            float batVolt = *((float*)bytes);

            bytes[0] = *(datagram.data().data() + commaPos + 11);
            bytes[1] = *(datagram.data().data() + commaPos + 10);
            bytes[2] = *(datagram.data().data() + commaPos + 9);
            bytes[3] = *(datagram.data().data() + commaPos + 8);

            quint32 e131_status = *((quint32*)bytes);

            qDebug() << "Source IP:" << datagram.senderAddress().toString().split(':')[3] << "BatVolt:" << batVolt << "E1.31 status:" << e131_status;

            emit pingReceived(buzzerName, datagram.senderAddress().toString().split(':')[3], batVolt, e131_status);

            continue;
        }

        if (oscPath.endsWith("/trigger")) {
            // Woah, a buzzer has been triggered

            QString buzzerName = oscPath.split('/')[2];
            qDebug() << "buzzerName:" << buzzerName;

            continue;
        }
    }
}

void MainWindow::updateStatusLabel(QString buzzerName, QString sourceAddress, float batVolt, quint32 e131_status)
{
    int j = 1;
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        if (val["name"] == buzzerName) {
            QLabel* statusLabel = (QLabel*)val["statusLabel"].toString().toLongLong();
            statusLabel->setText(tr("Ping: OK\nIP: %1\nBat: %2V\nE1.31 status: %3").arg(sourceAddress).arg(QString::number(batVolt, 'f', 2)).arg(e131_status));
            statusLabel->setStyleSheet("border: 1px solid black; color: white; background: darkgreen");

            val["lastIP"] = sourceAddress;
            val["lastBatVolt"] = batVolt;
            // Overwrite the old object with the new one
            buzzers[j-1] = val;

            // (Re-)start the ping timeout timer
            QTimer* statusTimer = (QTimer*)val["statusTimer"].toString().toLongLong();
            statusTimer->start(5000);

            qDebug() << buzzers;

            return;
        }
        j++;
    }
}

void MainWindow::pingTimeout()
{
    QTimer *sender = (QTimer*)QObject::sender();

    qDebug() << "TIMEOUT" << buzzers;

    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        if ((QTimer*)val["statusTimer"].toString().toLongLong() == sender) {
            QLabel* statusLabel = (QLabel*)val["statusLabel"].toString().toLongLong();

            statusLabel->setText(tr("Ping: NOK\nIP: %1\nBat: %2V\nE1.31 status: ???")
                                 .arg(val["lastIP"].toString())
                                 .arg(QString::number(val["lastBatVolt"].toDouble(), 'f', 2))
                    );
            statusLabel->setStyleSheet("border: 1px solid black; color: white; background: darkred");

            return;
        }
    }
}

