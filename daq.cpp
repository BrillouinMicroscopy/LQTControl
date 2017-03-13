#include "daq.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

/* Definitions of PS2000 driver routines */
#include "ps2000.h"

daq::daq(QObject *parent) :
	QObject(parent)
{
}

QVector<QPointF> daq::acquire(int colCount) {
	QVector<QPointF> points;
	points.reserve(colCount);
	for (int j(0); j < colCount; j++) {
		qreal x(0);
		qreal y(0);
		// data with sin + random component
		y = qSin(3.14159265358979 / 50 * j) + 0.5 + (qreal)rand() / (qreal)RAND_MAX;
		x = j;
		points.append(QPointF(x, y));
	}
	return points;
}

void daq::connect() {
	if (!unitOpened.handle) {
		unitOpened.handle = ps2000_open_unit();
	}
}

void daq::disconnect() {
	if (unitOpened.handle) {
		ps2000_close_unit(unitOpened.handle);
		unitOpened.handle = NULL;
	}
}