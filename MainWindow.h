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
	void linkActivated(const QString& link);

private slots:
	void connFinished(QNetworkReply *r);
	void sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors);
	void progress(qint64 bytesReceived, qint64 bytesTotal);


private:
	bool NetworkAccessibleCheck();

	void checkForUpdate();
	void getDownloadSizes();
	void getFile();

	void checkForUpdateResponse(QNetworkReply *r);
	void getDownloadSizesResponse(QNetworkReply *r);
	void getFileReponse(QNetworkReply *r);

	void deleteFiles();
	void makePath(QString path);
	void downloadsFinished();



private:
	Ui::CMainWindow* ui;
	QNetworkAccessManager* restclient;
};
