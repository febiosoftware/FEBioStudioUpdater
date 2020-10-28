#include <QApplication>
#include <QWizardPage>
#include <QLabel>
#include <QAbstractButton>
#include <QMessageBox>
#include <QListWidget>
#include <QBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QProgressBar>
#include <QDateTime>
#include <QSaveFile>
#include <QDebug>
#include <QLocale>

#include "MainWindow.h"
#include "FEBioStudioUpdater.h"
#include <XML/XMLWriter.h>

#include <iostream>

#define UPDATE_URL "repo.febio.org"
#define PORT 4433


#ifdef WIN32
	#define URL_BASE "/update/FEBioStudio/Windows"
	#define REL_ROOT "/../"
#elif __APPLE__
	#define URL_BASE "/update/FEBioStudio/macOS"
	#define REL_ROOT "/../../../"
#else
	#define URL_BASE "/update/FEBioStudio/Linux"
	#define REL_ROOT "/../"
#endif

namespace Ui
{
class MyWizardPage : public QWizardPage
{
public:

	MyWizardPage()
	{
		complete = false;
		emit completeChanged();
	}

	bool isComplete() const
	{
		return complete;
	}

	void setComplete(bool comp)
	{
		complete = comp;

		emit completeChanged();
	}


private:
	bool complete;

};
}

class Ui::CMainWindow
{
public:
	QWizardPage* startPage;

	MyWizardPage* filesPage;
	QVBoxLayout* filesLayout;
	QLabel* filesLabel;
	QListWidget* files;

	MyWizardPage* downloadPage;
	QLabel* downloadOverallLabel;
	QProgressBar* overallProgress;
	QLabel* downloadFileLabel;
	QProgressBar* fileProgress;

	void setup(::CMainWindow* wnd, bool correctDir)
	{
		m_wnd = wnd;

		// Start page
		startPage = new QWizardPage;
		QVBoxLayout* startLayout = new QVBoxLayout;

		if(correctDir)
		{
			startLayout->addWidget(new QLabel("Welcome to the FEBio Studio Auto-Updater.\n\nClick Next to check for updates."));
		}
		else
		{
			startLayout->addWidget(new QLabel("This updater does not appear to be located in the 'bin' folder of an FEBio Studio "
					"installation.\n\nUpdate cannot proceed."));
		}

		startPage->setLayout(startLayout);
		wnd->addPage(startPage);

		if(correctDir)
		{
			// Files page
			filesPage = new MyWizardPage;
			filesLayout = new QVBoxLayout;
			files = new QListWidget;

			filesLayout->addWidget(filesLabel = new QLabel("Checking for updates..."));


			filesPage->setLayout(filesLayout);

			wnd->addPage(filesPage);

			downloadPage = new MyWizardPage;
			QVBoxLayout* downloadLayout = new QVBoxLayout;

			downloadLayout->addWidget(downloadFileLabel = new QLabel);
			downloadLayout->addWidget(fileProgress = new QProgressBar);

			downloadLayout->addWidget(downloadOverallLabel= new QLabel);
			downloadLayout->addWidget(overallProgress = new QProgressBar);

			downloadPage->setLayout(downloadLayout);

			wnd->addPage(downloadPage);
		}

		currentIndex = 0;
		overallSize = 0;
		downloadedSize = 0;
	}

	void setUpFilesPage()
	{
		if(updateFiles.size() > 0)
		{
			filesLabel->setText("The following files need to be downloaded:");
			filesLayout->addWidget(files);

			filesLayout->addWidget(new QLabel(QString("The total download size is %1.\nClick "
					"Next to start the update.\n").arg(m_wnd->locale().formattedDataSize(overallSize))));
		}
		else
		{
			filesLabel->setText("Software is up to date!");

			m_wnd->removePage(2);
		}

		filesPage->setComplete(true);
	}

	void downloadsFinished()
	{
		fileProgress->hide();
		downloadOverallLabel->hide();
		overallProgress->hide();

		downloadFileLabel->setText("Download Complete!");

		downloadPage->setComplete(true);
	}

public:
	::CMainWindow* m_wnd;
	QStringList updateFiles;
	QStringList deleteFiles;
	QStringList newFiles;
	QStringList newDirs;
	int currentIndex;
	qint64 overallSize;
	qint64 downloadedSize;
};


CMainWindow::CMainWindow() : ui(new Ui::CMainWindow), restclient(new QNetworkAccessManager)
{
	QString dir = QApplication::applicationDirPath();
	bool correctDir = false;

#ifdef WIN32
	if(dir.contains("FEBio") && dir.contains("Studio") && dir.contains("bin"))
	{
		correctDir = true;
	}
#elif __APPLE__
	if(dir.contains("FEBio") && dir.contains("Studio") && dir.contains("MacOS"))
	{
		correctDir = true;
	}
#else
	if(dir.contains("FEBio") && dir.contains("Studio") && dir.contains("bin"))
	{
		correctDir = true;
	}
#endif
	

	ui->setup(this, correctDir);

	connect(restclient, &QNetworkAccessManager::finished, this, &CMainWindow::connFinished);
	connect(restclient, &QNetworkAccessManager::sslErrors, this, &CMainWindow::sslErrorHandler);
}

bool CMainWindow::NetworkAccessibleCheck()
{
//	return restclient->networkAccessible() == QNetworkAccessManager::Accessible;
	return true;
}

void CMainWindow::checkForUpdate()
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost(UPDATE_URL);
	myurl.setPort(PORT);
	myurl.setPath(URL_BASE);

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);

	if(NetworkAccessibleCheck())
	{
		restclient->get(request);
	}
}


void CMainWindow::checkForUpdateResponse(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode != 200)
	{
		QMessageBox::critical(this, "Update Failed", "Update Failed!\n\nUnable to recieve response from server.");
		QApplication::quit();
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(r->readAll());
	QJsonObject jsonObject = jsonDoc.object();


	QJsonArray deleteList = jsonObject["delete"].toArray();

	for(auto file : deleteList)
	{
		ui->deleteFiles.append(QApplication::applicationDirPath() + QString(REL_ROOT) + file.toString());
	}

	QJsonArray files = jsonObject["update"].toArray();

	for(auto file : files)
	{
		QString name = file.toArray()[0].toString();
		qint64 modified = file.toArray()[1].toInt();
		qint64 size = file.toArray()[2].toString().toLongLong();

		QFileInfo info(QApplication::applicationDirPath() + QString(REL_ROOT) + name);

		if(info.exists())
		{
			if(info.lastModified().toSecsSinceEpoch() < modified)
			{
				ui->updateFiles.append(name);
				ui->overallSize += size;
			}
		}
		else
		{
			ui->updateFiles.append(name);
			ui->overallSize += size;

			ui->newFiles.append(info.absoluteFilePath());
		}
	}

	ui->files->addItems(ui->updateFiles);

	ui->setUpFilesPage();
}

void CMainWindow::deleteFiles()
{
	for(auto file : ui->deleteFiles)
	{
		QFile::remove((file));
	}
}

void CMainWindow::getFile()
{
	ui->downloadFileLabel->setText(QString("Downloading %1...").arg(ui->updateFiles[ui->currentIndex]));
	ui->downloadOverallLabel->setText(QString("Downloading File %1 of %2").arg(ui->currentIndex + 1).arg(ui->updateFiles.size()));

	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost(UPDATE_URL);
	myurl.setPort(PORT);
	myurl.setPath(QString(URL_BASE) + "/" + ui->updateFiles[ui->currentIndex]);

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);

	if(NetworkAccessibleCheck())
	{
		QNetworkReply* reply = restclient->get(request);

		QObject::connect(reply, &QNetworkReply::downloadProgress, this, &CMainWindow::progress);
	}
}

void CMainWindow::getFileReponse(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode != 200)
	{
		QMessageBox::critical(this, "Update Failed", QString("Update Failed!\n\nUnable to download %1.").arg(ui->updateFiles[ui->currentIndex]));
		QApplication::quit();
	}

	QByteArray data = r->readAll();

	QString fileName = QApplication::applicationDirPath() + QString(REL_ROOT) + ui->updateFiles[ui->currentIndex];

	QFileInfo info(fileName);
	makePath(QDir::fromNativeSeparators(info.absolutePath()));

	QSaveFile file(fileName);
	file.open(QIODevice::WriteOnly);

	file.write(data);
	file.commit();

	file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::WriteUser | QFileDevice::ExeUser | QFileDevice::ReadUser);

	ui->currentIndex++;
	ui->downloadedSize += data.size();

	qDebug() << ui->downloadedSize;

	if(ui->currentIndex < ui->updateFiles.size())
	{
		getFile();
	}
	else
	{
		downloadsFinished();
	}
}

void CMainWindow::initializePage(int id)
{
	switch(id)
	{
	case 1:
		checkForUpdate();
		break;
	case 2:
		deleteFiles();
		getFile();
		break;
	default:
		break;
	}
}

void CMainWindow::connFinished(QNetworkReply *r)
{
	if(r->request().url().path() == URL_BASE)
	{
		checkForUpdateResponse(r);
	}
	else
	{
		getFileReponse(r);
	}

}

void CMainWindow::sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors)
{
	for(QSslError error : errors)
	{
		qDebug() << error.errorString();
	}

	reply->ignoreSslErrors();
}

void CMainWindow::progress(qint64 bytesReceived, qint64 bytesTotal)
{
	ui->overallProgress->setValue((float)(bytesReceived + ui->downloadedSize)/(float)ui->overallSize*100);

	ui->fileProgress->setValue((float)bytesReceived/(float)bytesTotal*100);
}


void CMainWindow::makePath(QString path)
{
	QDir dir(path);

	if(dir.exists())
	{
		return;
	}

	QString parentPath = path.left(path.lastIndexOf("/"));

	makePath(parentPath);

	dir.mkdir(dir.absolutePath());

	ui->newDirs.append(dir.absolutePath());
}

void CMainWindow::downloadsFinished()
{
	QStringList oldFiles;
	QStringList oldDirs;

	readXML(oldFiles, oldDirs);

	XMLWriter writer;
	writer.open("autoUpdate.xml");

	XMLElement root("autoUpdate");
	writer.add_branch(root);

	for(auto dir : oldDirs)
	{
		writer.add_leaf("dir", dir.toStdString().c_str());
	}

	for(auto dir : ui->newDirs)
	{
		writer.add_leaf("dir", dir.toStdString().c_str());
	}

	for(auto file : oldFiles)
	{
		writer.add_leaf("file", file.toStdString().c_str());
	}

	for(auto file : ui->newFiles)
	{
		writer.add_leaf("file", file.toStdString().c_str());
	}

	writer.close_branch();

	writer.close();

	ui->downloadsFinished();
}


