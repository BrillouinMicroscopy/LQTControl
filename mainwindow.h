#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_playButton_clicked();
	void on_selectDisplay_activated(const QString &text);
    void processOneThing();
	void on_actionConnect_triggered();

private:
    Ui::MainWindow *ui;
	QTimer timer;

};

#endif // MAINWINDOW_H
