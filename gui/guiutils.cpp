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
#include "guiutils.hpp"

#include <regex>
#include <sstream>

#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "../core/utils/string_utils.hpp"
#include "../core/utils/message.hpp"


gutils::OpenGLInfo gutils::getOpenGLInfo()
{
	OpenGLInfo glInfo;
	auto context = QOpenGLContext::currentContext();

    std::shared_ptr<QOpenGLContext> tmpContext;
    // contextがnullptrの場合、openGL未対応か単にcontext未生成かわからないので手動生成を試みる。
    if(!context) {
        tmpContext = std::make_shared<QOpenGLContext>(nullptr);
        tmpContext->setFormat(QSurfaceFormat::defaultFormat());
        tmpContext->create();
        tmpContext->makeCurrent(0);

        mWarning() << "context is valid===" << tmpContext->isValid();
        mWarning() << "context is null ===" << (tmpContext.get() == nullptr);

        mWarning() << "current context pointer ===" << tmpContext.get();

        context = QOpenGLContext::currentContext();
        if(!context) {
            mWarning() << "Context not exists";
        } else {
            mWarning() << "Context exists";
        }
        mWarning() << "current context pointer2 ===" << context;
//        auto funcs = tmpContext->functions();
//        auto vend = reinterpret_cast<const char*>(funcs->glGetString(GL_VENDOR));
//        mDebug() << vend;
    }

    if(!context) {
		mWarning() << "OpenGL version detection failed(no context).";
	} else {
		auto functions = context->functions();
		glInfo.vendor = reinterpret_cast<const char*>(functions->glGetString(GL_VENDOR));
		glInfo.renderer = reinterpret_cast<const char*>(functions->glGetString(GL_RENDERER));
		glInfo.versionStr = reinterpret_cast<const char*>(functions->glGetString(GL_VERSION));
		utils::trim(&glInfo.versionStr);
		glInfo.isMesa = std::regex_search(glInfo.versionStr, std::regex("mesa", std::regex_constants::icase));

		// stodは文字列の数値化に失敗した場合例外が出るのでtry-catchする。
		try {
			auto versionNumberStr = utils::splitString(" (-:", glInfo.versionStr).front();
			double versionValue = std::stod(versionNumberStr);
			glInfo.version = versionValue;
		} catch (std::exception &e) {
            (void)e;
			glInfo.version = -1;
		}
	}

//    if(tmpContext) tmpContext.reset();
	return glInfo;
}

std::string gutils::OpenGLInfo::toString() const
{
	std::stringstream ss;
	ss << "OpenGL vendor: " << vendor << " "
	   << "renderer: " << renderer << " "
	   << "version: " << versionStr;
	return ss.str();
}


void gutils::chooseAndSetFont(QWidget *parent, QFont *font, QPushButton *button)
{
	QFontDialog qfd(*font, parent);

	//qfd.resize(->width(), this->height());
	auto result = qfd.exec();
	if(result == QDialog::Accepted) {
		*font = qfd.selectedFont();
		button->setText(simpleFontString(font->toString()));
	}
}

QString gutils::simpleFontString(const QString &fontString)
{
	auto params = fontString.split(',');
	// フォント文字列の0,1, 10を残す
	assert(params.size() >= 2);
	if(params.size() >= 11) {
		return params.at(0) + " " + params.at(1) + " " + params.at(10);
	} else {
		return params.at(0) + " " + params.at(1);
	}
}

void gutils::setWidgetColor(const QColor &color, QWidget *widget)
{
	// ※ CSS設定はpaletteに優先するので、テーマで指定されている場合はpaletteによる変更が効かない。
	QPalette palette = widget->palette();
	palette.setColor(QPalette::Window, color);
	widget->setPalette(palette);
	widget->setStyleSheet("background:" + color.name() + ";");
}

#include <regex>
QColor gutils::getWidgetColor(const QWidget *widget)
{
	std::smatch sm;
	std::string cssStr = widget->styleSheet().toStdString();
	if(std::regex_search(cssStr, sm, std::regex("background *: *(#[0-9a-fA-F]+) *;"))) {
		std::string colName = sm.str(1);
		if(!colName.empty()) return QColor(QString::fromStdString(utils::trimmed(colName)));
	}
	return QColor(Qt::white);
}

void gutils::chooseAndSetColor(QWidget *parent, QColor *col, QWidget *widget)
{
	QColor initial = (col) ?  *col : QColor(Qt::white);
	// parentを設定するとmodalityなど適当にやってくれるので都合が良い。
	auto tmpColor = QColorDialog::getColor(initial, parent);
	if(tmpColor.isValid()) {
		*col = tmpColor;
		setWidgetColor(*col, widget);
	}
}



