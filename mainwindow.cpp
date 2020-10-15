#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout;

    QVBoxLayout *leftLayout = new QVBoxLayout;

    QLabel *availableBuzzersLabel = new QLabel(tr("Available buzzers:"));
    leftLayout->addWidget(availableBuzzersLabel);

    mainLayout->addLayout(leftLayout);

    QVBoxLayout *rightLayout = new QVBoxLayout;

    QString buzzerInfo = "[\
        {\"name\": \"red\"},\
        {\"name\": \"blue\"},\
        {\"name\": \"green\"},\
        {\"name\": \"yellow\"},\
        {\"name\": \"magenta\"},\
        {\"name\": \"cyan\"}\
    ]";

    QJsonDocument doc = QJsonDocument::fromJson(buzzerInfo.toUtf8());
    buzzers = doc.array();

    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    rightLayout->addLayout(groupBoxLayout);

    int j = 0;
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        // GroupBox that has the bgcolor of that buzzer
        QGroupBox *gb = new QGroupBox(tr("%1 (Chans %2 -> %3)").arg(val["name"].toString().toUpper()).arg(j*60+1).arg(j*60+60));
        gb->setStyleSheet(buttonColorToStyleSheet(val["name"].toString()));
        groupBoxLayout->addWidget(gb);
        QVBoxLayout *gbL = new QVBoxLayout;
        gb->setLayout(gbL);
        // Save pointer to the GroupBox in buzzers object
        val.insert("groupBox", QJsonValue(QString("%1").arg((qint64)gb)));


        // Label that displays info about that buzzer (from PING packet)
        // bgcolor depends on status
        QLabel *statusLabel = new QLabel(tr("Ping: NOK\nIP: ???.???.???.???\nBat: ?.??V\nE1.31 status: ???"));
        statusLabel->setStyleSheet("border: 1px solid black; color: white; background: darkred;");
        gbL->addWidget(statusLabel);
        // Save pointer to the Label in buzzers object
        val.insert("statusLabel", QJsonValue(QString("%1").arg((qint64)statusLabel)));

        // Label that displays if that buzzer has just been triggered (for 2 seconds)
        QPushButton *trgdButton = new QPushButton(tr("idle"));
        trgdButton->setStyleSheet("border: 1px solid black; color: black; background: white;");
        gbL->addWidget(trgdButton);
        // Save pointer to the Label in buzzers object
        val.insert("trgdButton", QJsonValue(QString("%1").arg((qint64)trgdButton)));
        connect(trgdButton, &QPushButton::clicked, this, &MainWindow::trgdButtonClicked);
        // Plus timer that resets it to normal
        QTimer *trgdTimer = new QTimer;
        trgdTimer->setSingleShot(true);
        connect(trgdTimer, &QTimer::timeout, this, &MainWindow::resetTriggerDisplay);
        val.insert("trgdTimer", QJsonValue(QString("%1").arg((qint64)trgdTimer)));

        // Create a timer that turns the statusLabel red again when no ping came in for 5 seconds
        QTimer *statusTimer = new QTimer;
        statusTimer->setSingleShot(true);
        connect(statusTimer, &QTimer::timeout, this, &MainWindow::pingTimeout);
        val.insert("statusTimer", QJsonValue(QString("%1").arg((qint64)statusTimer)));

        // Overwrite the old object with the new one
        buzzers[j] = val;

        j++;
    }

    qDebug() << buzzers;

    armedButton = new QPushButton();
    armedButton->setMinimumHeight(60);
    rightLayout->addWidget(armedButton);
    connect(armedButton, &QPushButton::clicked, this, &MainWindow::armedButtonClicked);

    activeLabel = new QLabel();
    activeLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    activeLabel->setMinimumHeight(60);
    rightLayout->addWidget(activeLabel);

    mainLayout->addLayout(rightLayout);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    this->setWindowTitle(tr("Buzzer controller"));
    this->resize(820, 300);

    activeBuzzer = "";

    qDebug() << "UDP BIND:" << oscSock.bind(6206, QUdpSocket::ShareAddress);
    connect(&oscSock, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagrams);

    // Fake ARM the buzzers, and disarm is using the function
    // that updates all labels
    armed = 1;
    armedButtonClicked();

    connect(this, &MainWindow::pingReceived, this, &MainWindow::updateStatusLabel);
}

MainWindow::~MainWindow()
{

}

QString MainWindow::buttonColorToStyleSheet(QString buzzerColorName)
{
    if (buzzerColorName == "red") {
        return "color: black; background: red;";
    } else if (buzzerColorName == "blue") {
        return "color: white; background: blue;";
    } else if (buzzerColorName == "green") {
        return "color: black; background: lime;";
    } else if (buzzerColorName == "yellow") {
        return "color: black; background: yellow;";
    } else if (buzzerColorName == "magenta") {
        return "color: black; background: magenta;";
    } else if (buzzerColorName == "cyan") {
        return "color: black; background: cyan;";
    } else {
        return "color: black; background: white;";
    }
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

            triggerBuzzer(buzzerName);

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

            QString bgColor;
            if (batVolt <= 6.5) {
                bgColor = "orange";
            } else if (e131_status == 0) {
                bgColor = "darkblue";
            } else {
                bgColor = "darkgreen";
            }

            QString fgColor = "white";
            if (bgColor == "orange") {
                fgColor = "black";
            }

            QLabel* statusLabel = (QLabel*)val["statusLabel"].toString().toLongLong();
            statusLabel->setText(tr("Ping: OK\nIP: %1\nBat: %2V\nE1.31 status: %3").arg(sourceAddress).arg(QString::number(batVolt, 'f', 2)).arg(e131_status));
            statusLabel->setStyleSheet(QString("border: 1px solid black; color: %1; background: %2;").arg(fgColor).arg(bgColor));

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

void MainWindow::armedButtonClicked()
{
    // Turn off all buzzer chasers
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();
        sendOSC("/buzzer/processed/" + val["name"].toString(), 0);
    }

    if (!armed) {
        armed = 1;
        activeBuzzer = "";
        armedButton->setText(tr("Buzzer ARMED. Click to DISARM."));
        armedButton->setStyleSheet("border: 1px solid black; background: black; color: white;");
        sendOSC("/buzzer/arm", (char)255);

    } else {
        armed = 0;
        activeBuzzer = "";
        armedButton->setText(tr("Buzzer NOT ARMED. Click to ARM."));
        armedButton->setStyleSheet("border: 1px solid black; background: white; color: black;");
        sendOSC("/buzzer/arm", 0);
    }

    updateActiveBuzzerLabel();
}

void MainWindow::updateActiveBuzzerLabel()
{
    if (activeBuzzer.length() > 1) {
        activeLabel->setText(tr("BUZZER %1 ACTIVE").arg(activeBuzzer.toUpper()));
        activeLabel->setStyleSheet(QString("border: 1px solid black; %1").arg(buttonColorToStyleSheet(activeBuzzer)));
    } else {
        activeLabel->setText(tr("No Buzzer active"));
    }
    activeLabel->setStyleSheet(QString("border: 1px solid black; %1").arg(buttonColorToStyleSheet(activeBuzzer)));
}

void MainWindow::resetTriggerDisplay()
{
    QTimer *sender = (QTimer*)QObject::sender();

    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        if ((QTimer*)val["trgdTimer"].toString().toLongLong() == sender) {
            QPushButton* trgdButton = (QPushButton*)val["trgdButton"].toString().toLongLong();
            trgdButton->setText(tr("idle"));
            trgdButton->setStyleSheet("border: 1px solid black; color: black; background: white;");
            break;
        }
    }
}

void MainWindow::trgdButtonClicked()
{
    QPushButton* sender = (QPushButton*)QObject::sender();
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        if ((QPushButton*)val["trgdButton"].toString().toLongLong() == sender) {
            triggerBuzzer(val["name"].toString());
        }
    }
}

void MainWindow::triggerBuzzer(QString buzzerName)
{
    // Display that this buzzer has been triggered in all cases
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        QJsonObject val = (*i).toObject();

        if (val["name"] == buzzerName) {
            QPushButton* trgdButton = (QPushButton*)val["trgdButton"].toString().toLongLong();
            trgdButton->setText(tr("TRIGGERED"));
            trgdButton->setStyleSheet("border: 1px solid black; color: white; background: black;");

            // (Re-)start the ping timeout timer
            QTimer* trgdTimer = (QTimer*)val["trgdTimer"].toString().toLongLong();
            trgdTimer->start(2000);
            break;
        }
    }

    // If not armed => do nothing
    if (!armed) {
        return;
    }

    // If another buzzer is still active => do nothing
    if (activeBuzzer.length() > 1) {
        return;
    }

    // Disarm the buzzers to disable the arm chaser
    armedButtonClicked();

    activeBuzzer = buzzerName;
    sendOSC("/buzzer/processed/" + buzzerName, (char)255);

    updateActiveBuzzerLabel();
}

void MainWindow::sendOSC(QString path, char value)
{
    QNetworkDatagram dgram(QByteArray(), QHostAddress("127.0.0.1"), 7206);
    QByteArray data = QByteArray();
    data.append(path);
    data.append((char)0);

    while(data.length() % 4) {
        data.append((char)0);
    }

    data.append(",i");
    data.append((char)0);
    data.append((char)0);

    data.append((char)0);
    data.append((char)0);
    data.append((char)0);

    data.append((char)value);

    dgram.setData(data);
    qDebug() << "WRITTEN:" << oscSock.writeDatagram(dgram);
}

