#ifndef THREAD_H
#define THREAD_H

#include <QtCore>

class Thread :public QThread {
	Q_OBJECT

public:
	void startWorker(QObject *worker) {
		worker->moveToThread(this);
		start();
	}
};

#endif // THREAD_H
