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
	void on_acquisitionButton_clicked();
	void on_lockButton_clicked();
	void on_acquireLockButton_clicked();
	void on_actionConnect_triggered();
	void on_actionDisconnect_triggered();
	void on_scanButton_clicked();
	void on_scanButtonManual_clicked();
	// SLOTS for setting the acquisitionParameters
	void on_sampleRate_activated(const int index);
	void on_chACoupling_activated(const int index);
	void on_chBCoupling_activated(const int index);
	void on_chARange_activated(const int index);
	void on_chBRange_activated(const int index);
	void on_sampleNumber_valueChanged(const int no_of_samples);
	// SLOTS for setting the scanParameters
	void on_scanAmplitude_valueChanged(const double value);
	void on_scanOffset_valueChanged(const double value);
	void on_scanWaveform_activated(const int index);
	void on_scanFrequency_valueChanged(const double value);
	void on_scanSteps_valueChanged(const int value);

	void on_proportionalTerm_valueChanged(const double value);
	void on_integralTerm_valueChanged(const double value);
	void on_derivativeTerm_valueChanged(const double value);
	void on_frequency_valueChanged(const double value);
	void on_phase_valueChanged(const double value);

	// SLOTS for updating the plots
	void updateLiveView(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> &data);
	void updateScanView();
	void updateLockView(std::array<QVector<QPointF>, 3> &data);

	// SLOTS for updating the acquisition parameters
	void updateAcquisitionParameters(ACQUISITION_PARAMETERS acquisitionParameters);

public slots:
	void connectMarkers();
	void handleMarkerClicked();

private:
    Ui::MainWindow *ui;
	QtCharts::QChart *liveViewChart;
	QtCharts::QChart *lockViewChart;
	QtCharts::QChart *scanViewChart;
	QList<QtCharts::QLineSeries *> liveViewPlots;
	QList<QtCharts::QLineSeries *> lockViewPlots;
	QList<QtCharts::QLineSeries *> scanViewPlots;
	daq d;
	int view = 0;	// selection of the view
};

#endif // MAINWINDOW_H
