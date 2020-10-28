/*This file is part of the FEBio Studio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio-Studio.txt for details.

Copyright (c) 2020 University of Utah, The Trustees of Columbia University in 
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include <QApplication>
#include <QIcon>
#include <QString>
#include <QFileInfo>
#include <QFileDialog>
#include "MainWindow.h"
#include <FEBioStudioUpdater.h>
#include <stdio.h>
#include <QSplashScreen>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <string.h>
#include <XML/XMLReader.h>

void makePath(QString path)
{
	QDir dir(path);

	if(dir.exists())
	{
		return;
	}

	QString parentPath = path.left(path.lastIndexOf("/"));

	makePath(parentPath);



	qDebug() << dir.absolutePath();
	qDebug() << dir.mkdir(dir.absolutePath());
	qDebug() << path;
}

// starting point of application
int main(int argc, char* argv[])
{
	QString temp = QString("./../Test/Test2/Test3/Test4");

	qDebug() << temp << endl;

	makePath(temp);

//	if(argc > 1 && QString(argv[1]) == QString("--uninstall"))
//	{
//		uninstall();
//	}
//	else
//	{
//		QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//
//		// create the application object
//		QApplication app(argc, argv);
//
//		// set the display name (this will be displayed on all windows and dialogs)
//		app.setApplicationVersion("1.0.0");
//		app.setApplicationName("FEBio Studio Updater");
//		app.setApplicationDisplayName("FEBio Studio Updater");
//		app.setWindowIcon(QIcon(":/icons/FEBioStudio.png"));
//
//		// create the main window
//		CMainWindow wnd;
//		wnd.show();
//
//		return app.exec();
//	}
}



void uninstall()
{
	if(QFileInfo::exists("autoUpdate.xml"))
	{
		QStringList files;
		QStringList dirs;

		XMLReader reader;
		if(reader.Open("autoUpdate.xml") == false) return;

		XMLTag tag;
		if(reader.FindTag("autoUpdate", tag) == false) return;

		++tag;
		do
		{
			if(tag == "file")
			{
				files.append(tag.m_szval);
			}

			if(tag == "dir")
			{
				files.append(tag.m_szval);
			}

		}
		while(!tag.isend());

		reader.Close();

		files.append("autoUpdate.xml");

		for(auto file : files)
		{
			QFile::remove(file);
		}

		QDir temp;
		for(auto dir : dirs)
		{
			temp.remove(dir);
		}
	}
}

CMainWindow* getMainWindow()
{
	CMainWindow* wnd = dynamic_cast<CMainWindow*>(QApplication::activeWindow()); assert(wnd);
	return wnd;
}
