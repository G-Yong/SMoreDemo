#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadAndInfer(QString modelPath, QString imagePath, int idx);

private slots:
    void on_pushButton_start_clicked();

    void on_pushButton_modelPath_clicked();

    void on_pushButton_imagePath_clicked();

    void on_pushButton_stop_clicked();

    void onInferCompleted(int index, double elapsed);

signals:
    void inferCompleted(int index, double interval);

private:
    Ui::MainWindow *ui;

    std::atomic<int> mThreadIndex;
    std::atomic<bool> mQuitThread;

    QList<QThread*> mThreadList;
};
#endif // MAINWINDOW_H
