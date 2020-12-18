#include <QWizard>

namespace Ui{
class CMainWindow;
}

class QNetworkAccessManager;
class QNetworkReply;
class QSslError;

class CMainWindow : public QWizard
{
	Q_OBJECT

public:
	CMainWindow();

protected:
	void initializePage(int id) override;

public slots:
	void updateWidgetReady(bool update);

private slots:
	void connFinished(QNetworkReply *r);
	void sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors);
	void progress(qint64 bytesReceived, qint64 bytesTotal);

private:
	bool NetworkAccessibleCheck();

	void getFile();
	void getFileReponse(QNetworkReply *r);

	void deleteFiles();
	void makePath(QString path);
	void downloadsFinished();

private:
	Ui::CMainWindow* ui;
	QNetworkAccessManager* restclient;
};
