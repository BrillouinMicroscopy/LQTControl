#ifndef DAQ_H
#define DAQ_H

#include <QMainWindow>
#include <QtCore/QObject>


class daq : public QObject
{
	Q_OBJECT

public:
	explicit daq(QObject *parent = 0);
	QVector<QPointF>  acquire(int colCount);

private slots:

private:

};

#endif // DAQ_H