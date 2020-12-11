#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QWizardPage>
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
#include <QDateTime>
#include <QSaveFile>
#include <QDebug>
#include <QLocale>
#include <QXmlStreamReader>

#include "MainWindow.h"
#include "FEBioStudioUpdater.h"
#include <XML/XMLWriter.h>

#include <iostream>

//#define UPDATE_URL "repo.febio.org"
//#define PORT 4433
//#define SCHEME "https"

#define UPDATE_URL "localhost"
#define PORT 5236
#define SCHEME "http"

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

struct Release
{
	bool active;
	qint64 timestamp;
	QString FEBioVersion;
	QString FBSVersion;
	QString FEBioNotes;
	QString FBSNotes;
	QStringList files;
	QStringList deleteFiles;
};


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
	QLabel* infoLabel;

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
			// Info page
			infoPage = new MyWizardPage;
			infoLayout = new QVBoxLayout;

			infoLayout->addWidget(infoLabel = new QLabel("Checking for updates..."));


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

		currentIndex = 0;
		overallSize = 0;
		downloadedSize = 0;
	}

	void showUpdateInfo()
	{
		// Find last installed update
		int lastUpdateIndex;
		for(lastUpdateIndex = 0; lastUpdateIndex < releases.size(); lastUpdateIndex++)
		{
			if(releases[lastUpdateIndex].timestamp <= lastUpdate)
			{
				break;
			}
		}

		bool newFEBio = false;
		bool newFBS = false;

		if(lastUpdateIndex == releases.size())
		{
			newFEBio = true;
			newFBS = true;
		}
		else
		{
			if(releases[0].FEBioVersion != releases[lastUpdateIndex].FEBioVersion) newFEBio = true;
			if(releases[0].FBSVersion != releases[lastUpdateIndex].FBSVersion) newFBS = true;
		}

		if(!newFEBio && !newFBS)
		{
			infoLabel->setText("This update does not include new versions of FEBio or FEBioStudio.\n\nIt provides updates to FEBio dependencies for added stability.");
		}
		else
		{
			infoLabel->setText("This update provides: ");

			if(newFEBio)
			{
				QLabel* newFEBioLabel = new QLabel(QString("An update to FEBio %1. Click <a href=\"FEBioNotes\">here</a> for release notes.").arg(releases[0].FEBioVersion));
				newFEBioLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
				QObject::connect(newFEBioLabel, &QLabel::linkActivated, m_wnd, &::CMainWindow::linkActivated);

				infoLayout->addWidget(newFEBioLabel);
			}

			if(newFBS)
			{
				QLabel* newFBSLabel = new QLabel(QString("An update to FEBio Studio %1. Click <a href=\"FBSNotes\">here</a> for release notes.").arg(releases[0].FBSVersion));
				newFBSLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
				QObject::connect(newFBSLabel, &QLabel::linkActivated, m_wnd, &::CMainWindow::linkActivated);

				infoLayout->addWidget(newFBSLabel);
			}

			infoLayout->addWidget(new QLabel(QString("The total download size is %1.\nClick "
					"Next to start the update.\n").arg(m_wnd->locale().formattedDataSize(overallSize))));
		}

		infoPage->setComplete(true);
	}

	void showUpToDate()
	{
		infoLabel->setText("Software is up to date!");

		m_wnd->removePage(2);

		infoPage->setComplete(true);
	}

	void showError(const QString& error)
	{
		infoLabel->setText(error);

		m_wnd->removePage(2);

		infoPage->setComplete(true);
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

	vector<Release> releases;
	qint64 lastUpdate;
	qint64 serverTime;
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
	myurl.setScheme(SCHEME);
	myurl.setHost(UPDATE_URL);
	myurl.setPort(PORT);
	myurl.setPath(QString(URL_BASE) + ".xml");

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
		QMessageBox::critical(this, "Update Failed", "Update Failed!\n\nUnable to receive response from server.");
		QApplication::quit();
	}

	ui->serverTime = r->rawHeader("serverTime").toLongLong();

	QXmlStreamReader reader(r->readAll());

	if (reader.readNextStartElement())
	{
		if(reader.name() == "update")
		{
			while(reader.readNextStartElement())
			{
				if(reader.name() == "release")
				{
					Release release;

					while(reader.readNextStartElement())
					{
						if(reader.name() == "active")
						{
							release.active = reader.readElementText().toInt();
						}
						else if(reader.name() == "timestamp")
						{
							release.timestamp = reader.readElementText().toLongLong();
						}
						else if(reader.name() == "FEBioVersion")
						{
							release.FEBioVersion = reader.readElementText();
						}
						else if(reader.name() == "FBSVersion")
						{
							release.FBSVersion = reader.readElementText();
						}
						else if(reader.name() == "FEBioNotes")
						{
							release.FEBioNotes = reader.readElementText();
						}
						else if(reader.name() == "FBSNotes")
						{
							release.FBSNotes = reader.readElementText();
						}
						else if(reader.name() == "files")
						{
							QStringList files;

							while(reader.readNextStartElement())
							{
								if(reader.name() == "file")
								{
									files.append(reader.readElementText());
								}
								else
								{
									reader.skipCurrentElement();
								}
							}

							release.files = files;
						}
						else if(reader.name() == "deleteFiles")
						{
							QStringList files;

							while(reader.readNextStartElement())
							{
								if(reader.name() == "file")
								{
									files.append(reader.readElementText());
								}
								else
								{
									reader.skipCurrentElement();
								}
							}

							release.deleteFiles = files;
						}
						else
						{
							reader.skipCurrentElement();
						}
					}

					if(release.active) ui->releases.push_back(release);
				}
				else
				{
					reader.skipCurrentElement();
				}
			}

		}
		else
		{

		}
	}

	ui->lastUpdate = 0;

	QFile autoUpdateXML("autoUpdate.xml");
	autoUpdateXML.open(QIODevice::ReadOnly);

	reader.setDevice(&autoUpdateXML);

	if (reader.readNextStartElement())
	{
		if(reader.name() == "autoUpdate")
		{
			while(reader.readNextStartElement())
			{
				if(reader.name() == "lastUpdate")
				{
					ui->lastUpdate = reader.readElementText().toLongLong();
				}
				else
				{
					reader.skipCurrentElement();
				}
			}
		}
	}

	autoUpdateXML.close();

	if(ui->releases.size() > 0)
	{
		if(ui->releases[0].timestamp > ui->lastUpdate)
		{
			getDownloadSizes();
		}
		else
		{
			ui->showUpToDate();
		}

	}
	else
	{
		ui->showError("Failed to read release information from server.\nPlease try again later.");
	}

}

void CMainWindow::getDownloadSizes()
{
	// Find unique files that need to be downloaded
	for(auto release : ui->releases)
	{
		if(release.timestamp > ui->lastUpdate)
		{
			for(auto file : release.files)
			{
				if(!ui->updateFiles.contains(file))
				{
					ui->updateFiles.append(file);
				}
			}
		}
	}

	QVariantMap map;
	map.insert("files", ui->updateFiles);

	QByteArray payload = QJsonDocument::fromVariant(map).toJson();

	// Create request
	QUrl myurl;
	myurl.setScheme(SCHEME);
	myurl.setHost(UPDATE_URL);
	myurl.setPort(PORT);
	myurl.setPath(QString(URL_BASE) + "/getSizes");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	if(NetworkAccessibleCheck())
	{
		restclient->post(request, payload);
	}
}

void CMainWindow::getDownloadSizesResponse(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode != 200)
	{
		QMessageBox::critical(this, "Update Failed", QString("Update Failed!\n\nUnable to communicate with server."));
		QApplication::quit();
	}

	ui->overallSize = r->readAll().toLongLong();

	ui->showUpdateInfo();
}


void CMainWindow::getFile()
{
	ui->downloadFileLabel->setText(QString("Downloading %1...").arg(ui->updateFiles[ui->currentIndex]));
	ui->downloadOverallLabel->setText(QString("Downloading File %1 of %2").arg(ui->currentIndex + 1).arg(ui->updateFiles.size()));

	QUrl myurl;
	myurl.setScheme(SCHEME);
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
	if(r->request().url().path() == QString(URL_BASE) + ".xml")
	{
		checkForUpdateResponse(r);
	}
	else if(r->request().url().path() == QString(URL_BASE) + "/getSizes")
	{
		getDownloadSizesResponse(r);
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

void CMainWindow::linkActivated(const QString& link)
{
	QDialog dlg;
	QVBoxLayout* layout = new QVBoxLayout;

	QTextEdit* edit = new QTextEdit;
	edit->setReadOnly(true);
	layout->addWidget(edit);

	QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(box);
	QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

	QString notes = "";

	for(auto release : ui->releases)
	{
		if(release.timestamp > ui->lastUpdate)
		{
			if(link == "FEBioNotes")
			{
				if(!notes.isEmpty())
				{
					notes += "\n\n";
				}

				notes += "------------------------------------------------------\n";
				notes += "FEBio Version: ";
				notes += release.FEBioVersion;

				QDateTime timestamp;
				timestamp.setTime_t(release.timestamp);

				notes += "   Released: ";
				notes += timestamp.toString(Qt::SystemLocaleShortDate);

				notes += "\n------------------------------------------------------\n";

				notes  += release.FEBioNotes;
			}

			else if(link == "FBSNotes")
			{
				if(!notes.isEmpty())
				{
					notes += "\n\n";
				}

				notes += "------------------------------------------------------\n";
				notes += "FEBio Version: ";
				notes += release.FBSVersion;

				QDateTime timestamp;
				timestamp.setTime_t(release.timestamp);

				notes += "   Released: ";
				notes += timestamp.toString(Qt::SystemLocaleShortDate);

				notes += "\n------------------------------------------------------\n";

				notes += release.FBSNotes;
			}
		}
	}

	edit->setText(notes);

	dlg.setLayout(layout);

	dlg.resize(this->size());

	dlg.exec();
}

void CMainWindow::deleteFiles()
{
	for(auto file : ui->deleteFiles)
	{
		QFile::remove((file));
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

	writer.add_leaf("lastUpdate", std::to_string(ui->serverTime));

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



