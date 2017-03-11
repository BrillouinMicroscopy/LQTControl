#include "daq.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

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