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
#include "licensedialog.hpp"
#include "ui_licensedialog.h"

#include <QString>


/*
 * Qt: LGPL
 * VTK:BSD
 * QDarkStyleSheet: MIT
 * Qt dark orange: not described
 *
 *
 * BSD:
 *
 * MIT: ライセンス文面/リンクと使用の明示
 *
 *
 * 表記は
 *
 * ・ソフト名
 * コピーライト 作者
 * オフィシャルサイト
 * Released ライセンス
 * その他as is 文面など
 *
 */

LicenseDialog::LicenseDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::LicenseDialog)
{
	ui->setupUi(this);

	QString text;

    text = "<div style=\"text-align:center\"> gxsview (Geometry and CROSS Section VIEWer) </div>";
    text += "<p> (c) Seiki Ohnishi  ohnishi@nmri.go.jp ";
    text += "<p> This softoware includes <i>Open Source Softwares</i> below:";
	text += "<p><ul>";

	text +=
		"<li>Qt</li>"
		"<p>Copyright (c) 2018 The Qt Company Ltd."
		"<p>https://www.qt.io/"
		"<p> Released under GNU LESSER GENERAL PUBLIC LICENSE Version 3"
		"<ul>"
			"<li>LGPLv3 https://www.gnu.org/licenses/lgpl-3.0.en.html</li>"
			"<li>GPLv3 https://www.gnu.org/licenses/gpl-3.0.en.html</li>"
		"</ul>"
		"<p> See more detail in http://doc.qt.io/qt-5/licenses-used-in-qt.html"
		"<p> Source code is available from https://www.qt.io/download-qt-installer or"
		"<p> https://drive.google.com/open?id=1GSjRZay-N7rK6yc3J0bQmmz8SWlGHKsi"
		"<p>";


	text +=
		"<li>VTK </li>"
		"<p>Copyright (c) 1993-2015 Ken Martin, Will Schroeder, Bill Lorensen"
		"<p>https://www.vtk.org/"
		"<p>Released under BSD Lincense"
		"<p>Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:"
		"<ul>"
			"<li>Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. </li>"
			"<li>Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer"
				" in the documentation and/or other materials provided with the distribution.</li>"
			"<li>Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names of any contributors may be used to"
				" endorse or promote products derived from this software without specific prior written permission.</li>"
		"</ul>"
		"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,"
		" BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT"
		" SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES"
		" (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)"
		" HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING"
		" IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
		"<p><p>";
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
#else
	text +=
		"<li>fontconfig</li>"
		"<p>Copyright Colin Duquesnoy(2013-2018), Keith Packard(2000-2007), Patrick Lam(2005),"
			" Roozbeh Pournader(2009), Red Hat, Inc.(2008, 2009), Danilo Šegan(2008), and Google, Inc.(2012)"
		"<p>https://www.freedesktop.org/wiki/Software/fontconfig/"
		"<p>Released under MIT License"
		"<p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files"
		" (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge,"
		" publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, "
		"subject to the following conditions:"
		"<p>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software."
		"<p>THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES"
		" OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE"
		" LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN"
		" CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
		"<p><p>";
#endif

	text +=
		"<li>QDarkStyleSheet</li>"
		"<p>Copyright (c) 2013-2018 Colin Duquesnoy"
		"<p>https://github.com/ColinDuquesnoy/QDarkStyleSheet"
		"<p>Released under MIT License"
		"<p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files"
		" (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge,"
		" publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, "
		"subject to the following conditions:"
		"<p>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software."
		"<p>THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES"
		" OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE"
		" LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN"
		" CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
		"<p><p>";


	text +=
		"<li>Qt dark orange stylesheet</li>"
		"<p>Copyright (c) 2009 - 2012 Yasin Uludag."
		"<p>http://discourse.techart.online/t/release-qt-dark-orange-stylesheet/2287</li>";

	text +=
		"<li>picojson</li>"
		"<p>Copyright © 2009-2010 Cybozu Labs, Inc. Copyright © 2011-2015 Kazuho Oku"
		"<p>https://github.com/kazuho/picojson"
		"<p>Released 2-clause BSD license"
		"<p>Redistribution and use in source and binary forms, with or without modification,"
		" are permitted provided that the following conditions are met:"
		"<ul>"
			"<li> Redistributions of source code must retain the above copyright notice, "
			"this list of conditions and the following disclaimer."
			"<li> Redistributions in binary form must reproduce the above copyright notice, "
			"this list of conditions and the following disclaimer in the documentation and/or "
			"other materials provided with the distribution."
		"</ul>"
		"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" "
		"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "
		"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. "
		"IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, "
		"INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, "
		"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) "
		"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT "
		"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF "
		"ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
		"<p><p>";

	text += "</ul>";

	ui->textBrowser->document()->setHtml(text);//  ->document()->setPlainText(QString::fromStdString(ss.str()));


	ui->textBrowser->moveCursor(QTextCursor::Start);
	ui->textBrowser-> ensureCursorVisible();


}

LicenseDialog::~LicenseDialog()
{
	delete ui;
}
