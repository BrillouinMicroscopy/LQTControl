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
	void on_selectDisplay_activated(const int index);
	void on_scanWaveform_activated(const int index);
    void updateLiveView();
	void updateScanView();
	void on_acquisitionButton_clicked();
	void on_actionConnect_triggered();
	void on_actionDisconnect_triggered();
	void on_scanButton_clicked();
	void on_scanButtonManual_clicked();

public slots:
	void connectMarkers();
	void handleMarkerClicked();

private:
    Ui::MainWindow *ui;
	enum class liveViewPlotTypes {
		CHANNEL_A,
		CHANNEL_B
	};
	enum class scanViewPlotTypes {
		INTENSITY,
		A1,
		A2,
		QUOTIENTS
	};
	QtCharts::QChart *liveViewChart;
	QtCharts::QChart *lockViewChart;
	QtCharts::QChart *scanViewChart;
	QList<QtCharts::QLineSeries *> liveViewPlots;
	QList<QtCharts::QLineSeries *> scanViewPlots;
	daq d;
	int view = 0;	// selection of the view
};

#endif // MAINWINDOW_H
