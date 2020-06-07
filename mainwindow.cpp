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
    QJsonArray buzzers = doc.array();

    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    mainLayout->addLayout(groupBoxLayout);

    int j = 1;
    QJsonArray::iterator i;
    for (i = buzzers.begin(); i != buzzers.end(); ++i) {
        qDebug() << (*i);
        QJsonObject val = (*i).toObject();

        QGroupBox *gb = new QGroupBox(tr("Buzzer %1 - %2").arg(j).arg(val["name"].toString().toUpper()));
        val.insert("groupBox", QJsonValue((qint64)gb));
        QPalette pal = gb->palette();
        pal.setColor(QPalette::Background, QColor(val["name"].toString()));
        gb->setPalette(pal);
        groupBoxLayout->addWidget(gb);
        QVBoxLayout *gbL = new QVBoxLayout;
        gb->setLayout(gbL);

        QLabel *pingLabel = new QLabel(tr("Ping: NOK\nIP: ???.???.???.???\nBat: ???? (~?.?V)\nE1.31 status: ???"));
        pingLabel->setStyleSheet("border: 1px solid black; background: red");
        gbL->addWidget(pingLabel);

        j++;
    }

    qDebug() << QString::fromUtf8(doc.toJson());

    QLabel *armedLabel = new QLabel(tr("Buzzer NOT ARMED"));
    armedLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    armedLabel->setStyleSheet("border: 1px solid black; background: white; color: black;");
    mainLayout->addWidget(armedLabel);

    QLabel *activeLabel = new QLabel(tr("No Buzzer active"));
    activeLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    activeLabel->setStyleSheet("border: 1px solid black; background: white; color: black;");
    mainLayout->addWidget(activeLabel);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    this->setWindowTitle(tr("Buzzer controller"));
    this->resize(820, 300);
}

MainWindow::~MainWindow()
{

}

