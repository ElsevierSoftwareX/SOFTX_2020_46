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
#include "chartconfig.hpp"

#include "../../core/utils/message.hpp"
#include "../../core/utils/system_utils.hpp"

#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartLegend.h>
#include <vtkTextProperty.h>
//#include <vtkFontConfigFreeTypeTools.h>
//#include <vtkAutoInit.h>
//VTK_MODULE_INIT(vtkRenderingFreeTypeFontConfig);
// DEBUG
//const int DEFAULT_FONT_SIZE = 28;
const int DEFAULT_FONT_SIZE = 16;





ChartConfig::ChartConfig()
    : xtitle_("Energy (MeV)"), ytitle_("Cross section (barn)"),
//	: xtitle_("Energyエネルギー！ (MeV)"), ytitle_("Cross section断面積 (barn)"),
	  axisTitleFont_(QFont(QFont().family(), DEFAULT_FONT_SIZE)),
      axisLabelFont_(QFont(QFont().family(), DEFAULT_FONT_SIZE*0.8)),
      legendFont_(axisTitleFont_),
      fontColor_(QColor(Qt::black)),
      bgColor_(QColor(Qt::white)),
      axisColor_(fontColor_),
      xgridLineStyle_(QColor(Qt::lightGray), Qt::DotLine, 1.0),
      ygridLineStyle_(QColor(Qt::lightGray), Qt::DotLine, 1.0),
      lineStyleCollection_(16),
	  xpos_(LEGEND_POS::RIGHT), ypos_(LEGEND_POS::TOP),
      haveApplied_(false)
{
//	vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();  // ← ※これvtkビルド時にfontoconfig有効にしていないと何もしない
//	vtkFontConfigFreeTypeTools::GetInstance()->ForceCompiledFontsOff();
//	mDebug() << "forced compiled font ?===" << vtkFontConfigFreeTypeTools::GetInstance()->GetForceCompiledFonts();
}


/*
 * linuxではフォントファミリーSansはVLゴシック。fc-match SansはVLゴシックを表示する。
 *
 *
 */
void setToVtkProp(const QColor &color, const QFont &font, vtkTextProperty *prop)
{
	// fontconfigによる設定。これはvtkビルド時にfontoconfigを有効にし、libvtkRenderingFreeTypeFontConfigにリンクする必要がある。
//	vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();  // ← ※これvtkビルド時にfontoconfig有効にしていないと何もしない
//	prop->SetFontFamilyAsString("Century");
//	prop->SetFontFamilyAsString(font.family().toUtf8().constData());
//	mDebug() << "setting font family string===" << prop->GetFontFamilyAsString();  // これはセットされてるOK
//	mDebug() << "fontfile ===" << prop->GetFontFile();  // これは空が返る。明示的にセットしないとやはり無理なのか。
	// つまるところ(内部的に)familyからファイル名への変換が機能していない。
	// -lvtkRenderingFreeTypeFontConfigなしでもコンパイルが通るのも妙


	// レガシー指定1．フォントファイル指定。ただしfontのファイルパスをQFontから取得できないのでfontconfigを使う。

	/*
	 * QFontDialogではfamily以外に多彩なスタイルのフォントが選べる。例えばstyle=thin
	 * style=thinはQFontではどのように保持されるか？
	 * enum QFont::Styleがイタリック、オブリックを
	 * enum QFont::WeightがThin, black Bold lightなど。
	 *
	 */
    // ここからfontconfig依存 F QFontからFontFileのフルパスを得る。https://itk.org/pipermail/vtkusers/2018-August/102557.html
	//std::string fontFile = utils::fontFilePath(const QFont &font);
	std::string fontFile = utils::fontFilePath(font);
    if(!utils::exists(fontFile)) {
        mWarning() << "Font file = " << fontFile << "cannot be opened.";
    } else {
        // fontがttcならstyleがおそらく反映されないことを警告する。
        std::string::size_type dotpos = fontFile.rfind('.');
        if(dotpos != std::string::npos) {
            std::string suffix = fontFile.substr(dotpos+1);
            //mDebug() << "suffix===" << suffix;
			// どうせ気にしないだろうからttcフォントの警告はしない。そのうちvtkもfontconfig対応するだろうし。
//            if(suffix == "ttc" || suffix == "TTC") {
//                mWarning() << "TTC font file(=" << fontFile << ") may not precisely printed depending on size and style.";
//            }
        }
        //mDebug() << "set font file ===" << fontFile;
        prop->SetFontFile(fontFile.c_str());
    }
    prop->SetFontSize(font.pointSize());
	prop->SetItalic(static_cast<vtkTypeBool>(font.italic()));
	prop->SetBold(static_cast<vtkTypeBool>(font.bold()));
	prop->SetFontFamily(VTK_FONT_FILE);
	prop->SetColor(color.redF(), color.greenF(), color.blueF());

//	// レガシー指定法2. Timesなどの指定。ただしtimesには日本語がないので日本語は表示されない。
//	prop->SetFontFamilyToTimes();  // 日本語が表示されない。timesに無いから


	/*
	 * NOTE VtkTextPropertyでのフォント設定は、フォントファイルの認識を確認する必要がある。
	 * prop->GetFontFamily()が4(VTK_FONT_FILE)を返せば認識していることがわかる。
	 * 3(VTK_UNKNOWN_FONT)を返す場合familyの設定がおかしい。
	 * GetFontFamilyは単に return GetFontFamilyFromString(this->FontFamilyAsString←SetFontFamilyAsStringでセットしたフォントファミリー文字列);で、
	 * GetFontFamilyFromStringは「フォントファイル文字列」と「GetFontFamilyAsString(VTK_FONT_FILE)」のstrcmpを取っている。
	 * getFontFamilyAsString(VTK_FONT_FILE)はenumの文字列化をしているだけで、VTK_FONT_FILEを与えた場合"File"を返す。
	 *
	 * つまり、SetFontFamilyAsStringは対応するenumが存在する"Times", "Courier", "Ariel", "File"の4種しか受け付けない。
	 *
	 * ユーザーがフォントを与える場合
	 * SetFontFamilyAsString("File")あるいはSetFontFamily(VTK_FONT_FILE)でファイルを読むようにセットする。
	 * するとユーザー指定モードになり、GetFontFamilyはVTK_FONT_FILEを返すようになる。
	 * あとはSetFontFileでフォントの絶対パスをセットする。
	 *
	 * vtkでは大雑把なフォント指定(times, courier, arial)以外はフォントファイルの直接指定になる。
	 * よってQFontからファイルパスを取得したい。
	 * QFtont::rawName()はX11では働かないしdeprecated
	 * わりとfontファイル名を取得する方法でcorss platformなのはない。
	 *
	 * vtkFontConfigFreeTypeToolsなどを使う方法… fontconfigはwindowsには無い。が代替はある
	 * https://itk.org/pipermail/vtkusers/2018-August/102557.html
	 * という解決策はある。
	 *
	 * またフォントファイルをSetFontFileした場合、スタイルもそのフォントファイルの物を使うので
	 * setBold, SetItalic等は無意味になる。
	 * QFontDialogではFreeTypeが生成している斜体なども選択できるように見えてしまっている。
	 */
}

/*
 * グラフプロット領域背景　→　vtkChartXY::SetBackgroundBrush
 * グラフ格子 →　↑の色から適当にセットされるので明示的にセットしなくてOK.セットしたいときは？
 * 軸タイトル、ラベルフォント→　GetAxis()で軸を取得してtitle/labelPropertyにセット
 * AXISの色　→　GetAxisで軸を取得してペンに色をセット
 * プロット領域外の背景　→　vtkRenderWindow::SetBackGround(vtkViewPort::SetBackGround())
 *
 * グラフプロット線　→　vtkPlot::Getpenして設定
 */
#include <vtkContextScene.h>
void ChartConfig::applyToChart(vtkSmartPointer<vtkChartXY> &chart)
{
    // vtkではfontはpropertyを取得して変更
    // 軸タイトル
    setToVtkProp(fontColor_, axisTitleFont_, chart->GetAxis(vtkAxis::BOTTOM)->GetTitleProperties());
    setToVtkProp(fontColor_, axisTitleFont_, chart->GetAxis(vtkAxis::LEFT)->GetTitleProperties());
    // ラベル
    setToVtkProp(fontColor_, axisLabeFont(), chart->GetAxis(vtkAxis::BOTTOM)->GetLabelProperties());
    setToVtkProp(fontColor_, axisLabeFont(), chart->GetAxis(vtkAxis::LEFT)->GetLabelProperties());
    // axis色、
    auto xAxis = chart->GetAxis(vtkAxis::BOTTOM);
    auto yAxis = chart->GetAxis(vtkAxis::LEFT);
    xAxis->GetPen()->SetColor(255*axisColor_.redF(), 255*axisColor_.greenF(), 255*axisColor_.blueF());
    yAxis->GetPen()->SetColor(255*axisColor_.redF(), 255*axisColor_.greenF(), 255*axisColor_.blueF());
    // grid設定
    xgridLineStyle_.applyToVtkPen(xAxis->GetGridPen());
    ygridLineStyle_.applyToVtkPen(yAxis->GetGridPen());

    // グラフ部分BG
    auto bgBrush = vtkSmartPointer<vtkBrush>::New();
    bgBrush->SetColor(255*bgColor_.redF(), 255*bgColor_.greenF(), 255*bgColor_.blueF(), 255);
    chart->SetBackgroundBrush(bgBrush);

    // 凡例
    auto legend = chart->GetLegend();
//    // legendの表示非表示はxsviewで実施するから不要
//    chart->SetShowLegend(showLegend_);
//    legend->SetVisible(showLegend_);
    chart->SetShowLegend(false);
    legend->SetVisible(false);

    setToVtkProp(fontColor_, legendFont(), legend->GetLabelProperties());
    legend->SetLabelSize(legendFont().pointSize());
    legend->GetBrush()->SetColor(255*bgColor_.redF(), 255*bgColor_.greenF(), 255*bgColor_.blueF(), 255);
    bool isDraggable = (xpos_ == LEGEND_POS::CUSTOM || ypos_ == LEGEND_POS::CUSTOM);
    legend->SetDragEnabled(isDraggable);
    legend->SetHorizontalAlignment(static_cast<int>(xpos_));
    legend->SetVerticalAlignment(static_cast<int>(ypos_));

}

const QStringList &ChartConfig::legendHPosList()
{
    // CUSTOMは除く。チェックボックスの方で設定するから
    static const QStringList posList {"LEFT", "CENTER", "RIGHT"};
    return posList;
}

const QStringList &ChartConfig::legendVPosList()
{
    static const QStringList posList {"CENTER", "TOP", "BOTTOM"};
    return posList;
}

//const QStringList &ChartConfig::legendPosList()
//{
//    // CUSTOMは除く。チェックボックスの方で設定するから
//    static const QStringList posList {"LEFT", "CENTER", "RIGHT", "TOP", "BOTTOM"};
//    return posList;
//}


const std::map<QString, LEGEND_POS> &getStringPosMap()
{
    static const std::map<QString, LEGEND_POS> posmap{
        {"LEFT",   LEGEND_POS::LEFT},
        {"CENTER", LEGEND_POS::CENTER},
        {"RIGHT",  LEGEND_POS::RIGHT},
        {"TOP",    LEGEND_POS::TOP},
        {"BOTTOM", LEGEND_POS::BOTTOM},
        {"CUSTOM", LEGEND_POS::CUSTOM}
    };
    return posmap;

}

LEGEND_POS ChartConfig::fromStringToPos(const QString &str)
{
    return getStringPosMap().at(str);
}

QString ChartConfig::fromPosToString(LEGEND_POS pos)
{
    auto posmap = getStringPosMap();
    for(auto it = posmap.cbegin(); it != posmap.cend(); ++it) {
        if(it->second == pos) return it->first;
    }
	return "";
}

