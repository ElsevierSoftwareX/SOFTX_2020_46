/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#include "message.hpp"



#ifdef ENABLE_GUI
#include <QSharedPointer>

LogForwarder::LogForwarder() {;}

void LogForwarder::forwardLog(const std::string &logStr, OUTPUT_TYPE level) const
{
//	QString logQstr = QString::fromStdString(logStr);
//	if(level == 0) {
//		emit fatalMessage(logQstr);
//    } else {
//		emit logMessage(logQstr);
//    }

//	QSharedPointer<QString> logQpointer(new QString(QString::fromStdString(logStr)));
//	if(level == 0) {
//		emit fatalMessage(logQpointer);
//    } else {
//		emit logMessage(logQpointer);
//    }

	if(ofs_) *ofs_.get() << logStr << std::endl;;
	QSharedPointer<std::string> logPointer(new std::string(logStr));
    if(level == OUTPUT_TYPE::MFATAL) {
		emit fatalMessage(logPointer);
	} else {
		emit logMessage(logPointer);
	}

}

void LogForwarder::enableLogFile(const std::string &fileName)
{
    ofs_.reset(new std::ofstream (fileName.c_str()));
}

void LogForwarder::disableLogFile()
{
    ofs_.release();
}


std::ostream &operator <<(std::ostream &os, const QString &qstr)
{
	os << qstr.toStdString();
	return os;
}

std::ostream &operator <<(std::ostream &os, const QSize &sz)
{
	os << "{" << sz.width() << ", " << sz.height() << "}";
	return os;
}



std::ostream &operator <<(std::ostream &os, const QByteArray &ba)
{
	os << ba.toStdString();
	return os;
}

std::ostream &operator <<(std::ostream &os, const QStringList &qstrlist)
{
	std::vector<std::string> strVec;
	for(auto it = qstrlist.begin(); it != qstrlist.end(); ++it) {
		strVec.emplace_back(it->toStdString());
	}
	os << strVec;
	return os;
}

std::ostream &operator <<(std::ostream &os, const QPoint &qpt)
{
	os << std::vector<int>{qpt.x(), qpt.y()};
	return os;
}


std::ostream &operator<<(std::ostream &os, Qt::CheckState state) {
	std::string str;
	switch(state) {
	case Qt::Checked:
		str = "Checked";
		break;
	case Qt::Unchecked:
		str = "Unchecked";
		break;
	case Qt::PartiallyChecked:
		str = "PartiallyChecked";
		break;
	}
	os << str;
	return os;
}
#endif

