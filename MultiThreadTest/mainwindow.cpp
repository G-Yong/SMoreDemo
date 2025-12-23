#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QtConcurrentRun>
#include <QElapsedTimer>
#include <QDeadlineTimer>
#include <QFileDialog>

#include "vimo_inference/vimo_inference.h"
using namespace smartmore;

void loadModelAndInfer(QString modelPath);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("多线程推理耗时测试");

    mThreadIndex = 0;
    mQuitThread = false;

    ui->lineEdit_modelPath->setText("C:/Users/Administrator/Desktop/vimoModel/vcloud/多线程测试/HRZ-好日子-13-model-16-17-19");
    ui->lineEdit_imagePath->setText("./image.bmp");

    // 初始化tableWidget
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "线程索引" << "推理耗时(ms)");
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 禁止编辑

    // 连接信号槽（使用Qt::QueuedConnection确保跨线程安全更新UI）
    connect(this, &MainWindow::inferCompleted, this, &MainWindow::onInferCompleted, Qt::QueuedConnection);

    on_pushButton_stop_clicked();
}

MainWindow::~MainWindow()
{
    on_pushButton_stop_clicked();
    delete ui;
}

void MainWindow::on_pushButton_start_clicked()
{
    mQuitThread = false;
    mThreadIndex = 0;

    // 清空表格并根据线程数设置行数
    int threadCount = ui->spinBox_threads->value();
    ui->tableWidget->setRowCount(threadCount);
    for(int i = 0; i < threadCount; i++)
    {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem("--"));
    }

    // 启动若干个线程
    for(int i = 0; i < threadCount; i++)
    {
        QtConcurrent::run([&](){
           loadAndInfer(ui->lineEdit_modelPath->text(), ui->lineEdit_imagePath->text(), mThreadIndex++);
        });
    }

    ui->lineEdit_modelPath->setEnabled(false);
    ui->pushButton_modelPath->setEnabled(false);
    ui->lineEdit_imagePath->setEnabled(false);
    ui->pushButton_imagePath->setEnabled(false);
    ui->spinBox_threads->setEnabled(false);
    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);
}

void MainWindow::on_pushButton_stop_clicked()
{
    mQuitThread = true;

    ui->lineEdit_modelPath->setEnabled(true);
    ui->pushButton_modelPath->setEnabled(true);
    ui->lineEdit_imagePath->setEnabled(true);
    ui->pushButton_imagePath->setEnabled(true);
    ui->spinBox_threads->setEnabled(true);
    ui->pushButton_start->setEnabled(true);
    ui->pushButton_stop->setEnabled(false);
}

void MainWindow::on_pushButton_modelPath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件model.vimosln所在路径"),
                                                    ui->lineEdit_modelPath->text(),
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    if(dir.isEmpty())
    {
        return;
    }

    ui->lineEdit_modelPath->setText(dir);
}

void MainWindow::on_pushButton_imagePath_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择图像文件"),
                                                    ui->lineEdit_imagePath->text(),
                                                    tr("图像文件 (*.bmp *.jpg *.jpeg *.png *.tif *.tiff);;所有文件 (*.*)"));
    if(filePath.isEmpty())
    {
        return;
    }

    ui->lineEdit_imagePath->setText(filePath);
}

void MainWindow::onInferCompleted(int index, double elapsed)
{
    // 更新对应线程的推理耗时
    if(index >= 0 && index < ui->tableWidget->rowCount())
    {
        QTableWidgetItem *item = ui->tableWidget->item(index, 1);
        if(item)
        {
            item->setText(QString::number(elapsed, 'f', 2));
        }
    }
}

cv::Mat loadMatFromPath(QString imgPath)
{
    cv::Mat mat;

    QFile file(imgPath);
    if(file.exists())
    {
        if(file.open(QFile::ReadOnly))
        {
            QByteArray data =  file.readAll();
            std::vector<uchar> imgData(data.begin(), data.end());
            mat = cv::imdecode(imgData, cv::IMREAD_UNCHANGED);
        }
    }

    return mat;
}
void MainWindow::loadAndInfer(QString modelPath, QString imagePath, int idx)
{
    // 加载模型并创建pipeline
    vimo::Pipelines pipelines;
    {
        std::string model_path = (modelPath + "/model.vimosln").toLocal8Bit().data();
        bool use_gpu = true;                        // whether to use gpu for inference
        int device_id = 0;                          // GPU device id, ignore if use_gpu == false

        vimo::Solution solution;  // create an empty solution
        std::cout << "load solution from: " << model_path << std::endl;
        solution.LoadFromFile(model_path);  // load solution from model.vimosln

        // edgeList是一系列std::pair，其中每个std::pair的first为当前节点，second为下一节点
        // 通过对所有的edgeList进行分析，便可以拼凑出所有完整的流程
        auto edgeList = solution.GetEdgeList();
        // qDebug() << "edge list:" << edgeList.size();
        // foreach (auto edge, edgeList) {
        //     qDebug() << edge.first.c_str() << "-->" << edge.second.c_str();
        // }

        auto infoList = solution.GetModuleInfoList();
        if(infoList.size() <= 0)
        {
            qDebug() << "error 1";
            return;
        }

        // 找到最新、最大的那个模组;
        // 因为module id是以数字递增的,排序之后，最后的那个就是我们想要的
        QMap<QString, vimo::Module::Info> tmpMap;
        foreach (auto info, infoList) {
            tmpMap[info.id.c_str()] = info;
        }
        // qDebug() << "keys:---" << tmpMap.keys();
        auto theModuleInfo = tmpMap[tmpMap.keys().last()];

        pipelines = solution.CreatePipelines(theModuleInfo.id, use_gpu, device_id);
    }


    // 目前的工作节拍为600pcs/min，也就是每秒钟需要处理10pcs，也就是两次推理之间的间隔为100ms
    int interval = 100;
    QDeadlineTimer dTimer(interval);

    while (mQuitThread == false) {

        // 时间还没到，等
        while(dTimer.hasExpired() == false)
        {
            QThread::msleep(1);
        }
        // 重新开始计时
        dTimer.setRemainingTime(interval);


        QElapsedTimer timer;
        timer.start();

        try
        {
            // 每次只推理一张图片
            vimo::Request req(loadMatFromPath(imagePath));
            vimo::Pipelines::UADResponseList rsp;
            pipelines.Run(req, rsp);
        }
        catch (vimo::VimoException &e)
        {
            std::cerr << e.what() << std::endl;
        }

        // 推理耗时
        qint64 elapsed = timer.nsecsElapsed();
        // qDebug() << "elapse(ns):" << elapsed;

        emit inferCompleted(idx, elapsed / (double)(1e6));
    }

    qDebug() << "quit thread" << idx;
}
