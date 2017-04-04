#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>

#include "daq.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_playButton_clicked();
	void on_selectDisplay_activated(const int index);
	void on_scanWaveform_activated(const int index);
    void updatePlot();
	void on_acquisitionButton_clicked();
	void on_actionConnect_triggered();
	void on_actionDisconnect_triggered();
	void on_scanButton_clicked();
	void on_scanButtonManual_clicked();

private:
    Ui::MainWindow *ui;
	QTimer timer;
	typedef struct {
		QtCharts::QLineSeries *series;
		QtCharts::QLineSeries *intensity;
		QtCharts::QLineSeries *A1;
		QtCharts::QLineSeries *A2;
		QtCharts::QLineSeries *quotients;
	} PLOTS;
	PLOTS plots;
	QtCharts::QChart *chart;
	daq d;
	int view = 0;	// selection of the view
};

#endif // MAINWINDOW_H
