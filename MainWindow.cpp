#include <vector>
#include <QApplication>
#include <QProcess>
#include <QDialog>
#include <QDialogButtonBox>
#include <QWizardPage>
#include <QPushButton>
#include <QLabel>
#include <QAbstractButton>
#include <QMessageBox>
#include <QListWidget>
#include <QTextEdit>
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
#include <QCheckBox>
#include <QDateTime>
#include <QSaveFile>
#include <QDebug>
#include <QLocale>
#include <QXmlStreamReader>
#include "FEBioStudioUpdater.h"
#include "MainWindow.h"
#include <XML/XMLWriter.h>
#include <FEBioStudio/UpdateChecker.h>

#include <iostream>

#ifdef WIN32
#define FBSBINARY "\\FEBioStudio.exe"
#elif __APPLE__
#define FBSBINARY "/FEBioStudio"
#else
#define FBSBINARY "/FEBioStudio"
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

	MyWizardPage* infoPage;
	QVBoxLayout* infoLayout;
	::CUpdateWidget* updateWidget;
	// QLabel* infoLabel;

	MyWizardPage* downloadPage;
	QLabel* downloadOverallLabel;
	QProgressBar* overallProgress;
	QLabel* downloadFileLabel;
	QProgressBar* fileProgress;
	QCheckBox* relaunch;

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
			// Info page
			infoPage = new MyWizardPage;
			infoLayout = new QVBoxLayout;

			infoLayout->addWidget(updateWidget = new ::CUpdateWidget);

			infoPage->setLayout(infoLayout);

			wnd->addPage(infoPage);

			downloadPage = new MyWizardPage;
			QVBoxLayout* downloadLayout = new QVBoxLayout;

			downloadLayout->addWidget(downloadFileLabel = new QLabel);
			downloadLayout->addWidget(fileProgress = new QProgressBar);

			downloadLayout->addWidget(downloadOverallLabel= new QLabel);
			downloadLayout->addWidget(overallProgress = new QProgressBar);

			downloadPage->setLayout(downloadLayout);

			wnd->addPage(downloadPage);
		}

		relaunch = nullptr;

		QObject::connect(updateWidget, &::CUpdateWidget::ready, m_wnd, &::CMainWindow::updateWidgetReady);
	}

	void downloadsFinished()
	{
		fileProgress->hide();
		downloadOverallLabel->hide();
		overallProgress->hide();

		downloadFileLabel->setText("Update Complete!");

		downloadPage->layout()->addWidget(relaunch = new QCheckBox("Relaunch FEBio Studio."));
		relaunch->setChecked(true);

		downloadPage->setComplete(true);
	}

public:
	::CMainWindow* m_wnd;
};


CMainWindow::CMainWindow(bool devChannel) 
	: ui(new Ui::CMainWindow), restclient(new QNetworkAccessManager), m_devChannel(devChannel)
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

	connect(this->button(QWizard::FinishButton), &QPushButton::clicked, this, &CMainWindow::onFinish);
	connect(restclient, &QNetworkAccessManager::finished, this, &CMainWindow::connFinished);
	connect(restclient, &QNetworkAccessManager::sslErrors, this, &CMainWindow::sslErrorHandler);
}

bool CMainWindow::NetworkAccessibleCheck()
{
//	return restclient->networkAccessible() == QNetworkAccessManager::Accessible;
	return true;
}

void CMainWindow::getFile()
{
	ui->downloadFileLabel->setText(QString("Downloading %1...").arg(ui->updateWidget->updateFiles[ui->updateWidget->currentIndex]));
	ui->downloadOverallLabel->setText(QString("Downloading File %1 of %2").arg(ui->updateWidget->currentIndex + 1).arg(ui->updateWidget->updateFiles.size()));

	QUrl myurl;
	myurl.setScheme(SCHEME);
	myurl.setHost(UPDATE_URL);
	myurl.setPort(PORT);
	myurl.setPath(ui->updateWidget->urlBase + "/" + ui->updateWidget->updateFiles[ui->updateWidget->currentIndex]);

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
		QMessageBox::critical(this, "Update Failed", QString("Update Failed!\n\nUnable to download %1.").arg(ui->updateWidget->updateFiles[ui->updateWidget->currentIndex]));
		QApplication::quit();
	}

	QByteArray data = r->readAll();

	QFileInfo fileInfo(QApplication::applicationDirPath() + QString(REL_ROOT) + ui->updateWidget->updateFiles[ui->updateWidget->currentIndex]);

	// Ensure that the path to the file exists. Add any newly created directories to autoUpdate.xml
	// for deletion during uninstalltion
	makePath(QDir::fromNativeSeparators(fileInfo.absolutePath()));

	QString fileName = fileInfo.absoluteFilePath();

	// If the file doesn't already exist, add it to autoUpdate.xml for deletion during uninstalltion.
	if(!QFile::exists(fileName)) ui->updateWidget->newFiles.append(fileName);

	QSaveFile file(fileName);
	file.open(QIODevice::WriteOnly);

	file.write(data);
	file.commit();

	file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::WriteUser | QFileDevice::ExeUser | QFileDevice::ReadUser);

	ui->updateWidget->currentIndex++;
	ui->updateWidget->downloadedSize += data.size();

	if(ui->updateWidget->currentIndex < ui->updateWidget->updateFiles.size())
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
		ui->updateWidget->checkForUpdate(m_devChannel);
		break;
	case 2:
		deleteFiles();
		getFile();
		break;
	default:
		break;
	}
}

void CMainWindow::updateWidgetReady(bool update)
{
	if(!update)
	{
		this->removePage(2);
	}
	
	ui->infoPage->setComplete(true);
}

void CMainWindow::connFinished(QNetworkReply *r)
{
	getFileReponse(r);
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
	ui->overallProgress->setValue((float)(bytesReceived + ui->updateWidget->downloadedSize)/(float)ui->updateWidget->overallSize*100);

	ui->fileProgress->setValue((float)bytesReceived/(float)bytesTotal*100);
}

void CMainWindow::deleteFiles()
{
	for(auto file : ui->updateWidget->deleteFiles)
	{
		QFile::remove(QApplication::applicationDirPath() + QString(REL_ROOT) + file);
	}
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

	ui->updateWidget->newDirs.append(dir.absolutePath());
}

void CMainWindow::downloadsFinished()
{
	QStringList oldFiles;
	QStringList oldDirs;

	readXML(oldFiles, oldDirs);

	XMLWriter writer;
	writer.open(QString(QApplication::applicationDirPath() + "/autoUpdate.xml").toStdString().c_str());

	XMLElement root("autoUpdate");
	writer.add_branch(root);

	writer.add_leaf("lastUpdate", std::to_string(ui->updateWidget->serverTime));

	for(auto dir : oldDirs)
	{
		writer.add_leaf("dir", dir.toStdString().c_str());
	}

	for(auto dir : ui->updateWidget->newDirs)
	{
		writer.add_leaf("dir", dir.toStdString().c_str());
	}

	for(auto file : oldFiles)
	{
		writer.add_leaf("file", file.toStdString().c_str());
	}

	for(auto file : ui->updateWidget->newFiles)
	{
		writer.add_leaf("file", file.toStdString().c_str());
	}

	writer.close_branch();

	writer.close();

	ui->downloadsFinished();
}

void CMainWindow::onFinish()
{
	if(ui->relaunch && ui->relaunch->isChecked())
	{
		QProcess* fbs = new QProcess;
		fbs->startDetached(QApplication::applicationDirPath() + FBSBINARY);
	}

	QWizard::accept();
}

