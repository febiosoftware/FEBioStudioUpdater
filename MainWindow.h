#pragma once
#include <QWizard>
#include <QSslError>

namespace Ui{
class CMainWindow;
}

class QNetworkAccessManager;
class QNetworkReply;

class CMainWindow : public QWizard
{
	Q_OBJECT

public:
	CMainWindow(bool devChannel, bool updaterUpdateCheck);

protected:
	void initializePage(int id) override;

public slots:
	void updateWidgetReady(bool update, bool terminal);
	void onFinish();

private slots:
	void connFinished(QNetworkReply *r);
	void sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors);
	void progress(qint64 bytesReceived, qint64 bytesTotal);

private:
    bool checkBinaries();
    bool isFileWriteable(QString filename, QString niceName);

	bool NetworkAccessibleCheck();

	void getFile();
	void getFileReponse(QNetworkReply *r);

	void deleteFiles();
	void makePath(QString path);
	void downloadsFinished();

private:
	Ui::CMainWindow* ui;
	QNetworkAccessManager* restclient;

	bool m_devChannel;
	bool m_updaterUpdateCheck;
};
