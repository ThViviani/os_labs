#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace QtCharts;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    networkManager = new QNetworkAccessManager(this);

    connect(ui->btnFetchCurrentTemp, &QPushButton::clicked, this, &MainWindow::fetchCurrentTemperature);
    connect(ui->btnFetchHourlyAvg, &QPushButton::clicked, this, &MainWindow::fetchHourlyAvg);
    connect(ui->btnFetchDailyAvg, &QPushButton::clicked, this, &MainWindow::fetchDailyAvg);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchCurrentTemperature()
{
    QUrl url("http://localhost:8080/v1/temperature/current");
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &MainWindow::onCurrentTempResponse);
}

void MainWindow::onCurrentTempResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        double currentTemp = obj["temp"].toDouble();

        ui->lblCurrentTemperature->setText(QString("Current Temperature: %1°C").arg(currentTemp));
    } else {
        ui->lblCurrentTemperature->setText("Error fetching data");
    }

    reply->deleteLater();
}

void MainWindow::fetchHourlyAvg()
{
    QUrl url("http://localhost:8080/v1/temperature/average/hourly");
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &MainWindow::onHourlyAvgResponse);
}

void MainWindow::onHourlyAvgResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.array();

        qDebug() << "Response data:" << doc.toJson();

        if (arr.isEmpty()) {
            qDebug() << "No data available for chart.";
            return;
        }

        if (!ui->charts->layout()) {
            QVBoxLayout *layout = new QVBoxLayout(ui->charts);
            ui->charts->setLayout(layout);
        }

        QLineSeries *series = new QLineSeries();

        for (const QJsonValue &value : arr) {
            QJsonObject obj = value.toObject();
            QString timestamp = obj["timestamp"].toString();
            double temp = obj["temp"].toDouble();

            series->append(QDateTime::fromString(timestamp, "yyyy-MM-dd HH:mm:ss").toMSecsSinceEpoch(), temp);
        }

        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->createDefaultAxes();

        QLayoutItem *item;
        while ((item = ui->charts->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumSize(400, 300);
        ui->charts->layout()->addWidget(chartView);
    } else {
        qDebug() << "Error fetching hourly average data";
    }

    reply->deleteLater();
}

void MainWindow::fetchDailyAvg()
{
    QUrl url("http://localhost:8080/v1/temperature/average/daily");
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &MainWindow::onDailyAvgResponse);
}

void MainWindow::onDailyAvgResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.array();

        qDebug() << "Response data:" << doc.toJson();

        if (arr.isEmpty()) {
            qDebug() << "No data available for chart.";
            return;
        }

        if (!ui->charts->layout()) {
            QVBoxLayout *layout = new QVBoxLayout(ui->charts);
            ui->charts->setLayout(layout);
        }

        QLineSeries *series = new QLineSeries();

        for (const QJsonValue &value : arr) {
            QJsonObject obj = value.toObject();
            QString timestamp = obj["timestamp"].toString();
            double temp = obj["temp"].toDouble();

            series->append(QDateTime::fromString(timestamp, "yyyy-MM-dd HH:mm:ss").toMSecsSinceEpoch(), temp);
        }

        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->createDefaultAxes();

        QLayoutItem *item;
        while ((item = ui->charts->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumSize(400, 300);
        ui->charts->layout()->addWidget(chartView);
    } else {
        qDebug() << "Error fetching daily average data";
    }

    reply->deleteLater();
}
