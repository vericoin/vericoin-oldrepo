#include "guiutil.h"
#include "bitcoinaddressvalidator.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "util.h"
#include "init.h"
#include "guiconstants.h"

#include <QString>
#include <QTranslator>
#include <QDateTime>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QUrl>
#include <QTextDocument> // For Qt::escape
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QDesktopServices>
#include <QThread>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#if BOOST_FILESYSTEM_VERSION >= 3
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
static boost::filesystem::detail::utf8_codecvt_facet utf8;
#endif

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "shlwapi.h"
#include "shlobj.h"
#include "shellapi.h"
#endif

namespace GUIUtil {

bool fNoHeaders = true;
int TOOLBAR_WIDTH = 100;
int TOOLBAR_ICON_WIDTH = TOOLBAR_WIDTH;
int TOOLBAR_ICON_HEIGHT = 48;
int HEADER_WIDTH = 900;
int HEADER_HEIGHT = 0;
int BUTTON_WIDTH = 140;
int BUTTON_HEIGHT = 27;
int FRAMEBLOCKS_LABEL_WIDTH = 100;
int WINDOW_MIN_WIDTH = TOOLBAR_WIDTH + HEADER_WIDTH;
#ifdef Q_OS_WIN
int WINDOW_MIN_HEIGHT = 476;
#else
#ifdef Q_OS_MAC
int WINDOW_MIN_HEIGHT = 460;
#else
int WINDOW_MIN_HEIGHT = 480;
#endif
#endif
int STATUSBAR_ICONSIZE = 16;
int STATUSBAR_MARGIN = 10;
int STATUSBAR_HEIGHT = 32;

void refactorGUI(QRect screenSize)
{
    // Set the new geometry
#ifdef Q_OS_WIN
    int newHeight = screenSize.height() - 40;
#else
#ifdef Q_OS_MAC
    int newHeight = screenSize.height() - 30;
#else
    int newHeight = screenSize.height() - 25;
#endif
#endif
    int newWidth = WINDOW_MIN_WIDTH;
    if (screenSize.width() < newWidth - 2)
    {
        newWidth = screenSize.width() - 2;
        STATUSBAR_MARGIN = 0;
        TOOLBAR_WIDTH = 90;
        TOOLBAR_ICON_WIDTH = TOOLBAR_WIDTH;
        HEADER_WIDTH = newWidth - TOOLBAR_WIDTH;
    }
    if (screenSize.height() <= 600)
    {
        TOOLBAR_ICON_HEIGHT = 32;
        HEADER_HEIGHT = 0;
        fNoHeaders = true;
    }
    else if (screenSize.height() < 728) // 728px if OS taskbar is not hidden
    {
        TOOLBAR_ICON_HEIGHT = 32;
        HEADER_HEIGHT = 32;
        fNoHeaders = true;
    }
    else // Default small wallet at 728px to 768px
    {
        TOOLBAR_ICON_HEIGHT = 34;
    }

    WINDOW_MIN_WIDTH = TOOLBAR_WIDTH + HEADER_WIDTH;
    WINDOW_MIN_HEIGHT = newHeight;
}

int pointsToPixels(int points) { return(points * 4 / 3); }

void setFontPixelSize(QFont *font)
{
    font->setPixelSize(pointsToPixels(font->pointSize()));
}

void setFontPixelSizes()
{
    setFontPixelSize((QFont *)&qFontSmallest);
    setFontPixelSize((QFont *)&qFontSmaller);
    setFontPixelSize((QFont *)&qFontSmall);
    setFontPixelSize((QFont *)&qFont);
    setFontPixelSize((QFont *)&qFontLarge);
    setFontPixelSize((QFont *)&qFontLarger);
    setFontPixelSize((QFont *)&qFontSmallerBold);
    setFontPixelSize((QFont *)&qFontSmallBold);
    setFontPixelSize((QFont *)&qFontBold);
    setFontPixelSize((QFont *)&qFontLargeBold);
    setFontPixelSize((QFont *)&qFontLargerBold);
}

// Common RoxyCoin stylesheets
QString veriCentralWidgetStyleSheet = QString("QStackedWidget { background: white; } ");
QString veriTabWidgetStyleSheet = QString("QTabWidget::pane { background: white; color: " + STR_FONT_COLOR + "; border: 1px; }");

QString veriPushButtonStyleSheet = QString("QPushButton { background: " + STR_COLOR + "; width: %1px; height: %2px; border: none; color: white} \
                            QPushButton:disabled { background: #EBEBEB; color: #666666; } \
                            QPushButton:hover { background: " + STR_COLOR_LT + "; } \
                            QPushButton:pressed { background: " + STR_COLOR_LT + "; } ").arg(BUTTON_WIDTH).arg(BUTTON_HEIGHT);

QString veriToolBarStyleSheet = QString("QToolBar { background: " + STR_COLOR + "; color: white; border: none; } \
                            QToolButton { background: " + STR_COLOR + "; color: white; border: none; font-family: Lato; font-style: normal; font-weight: normal; font-size: 12px; } \
                            QToolButton:hover { background: " + STR_COLOR_HOVER + "; color: white; border: none; } \
                            QToolButton:pressed { background: " + STR_COLOR_LT + "; color: white; border: none; } \
                            QToolButton:checked { background: " + STR_COLOR_LT + "; color: white; border: none; } ");

QString veriToolTipStyleSheet = QString("QToolTip { background-color: " + STR_COLOR_TTBG + "; color: white; border: 1px solid #EBEBEB; border-radius: 3px; margin: 0; padding: 4px; white-space: nowrap; } ");

QString veriMiscStyleSheet = QString("QStatusBar { background: " + STR_COLOR + "; color: white; } QStatusBar::item { border: none; } QDialog { background: white; color: " + STR_FONT_COLOR + "; } QTableView::item:hover { background: #EBEBEB; color: " + STR_FONT_COLOR + "; } ");

QString veriMenuStyleSheet = QString("QMenuBar { background-color: " + STR_COLOR + "; color: white; } \
                            QMenuBar::item { background-color: transparent; margin: 0px; padding: 4px 16px 4px 16px; } \
                            QMenuBar::item:selected { background-color: " + STR_COLOR_HOVER + "; color: white; } \
                            QMenu { background-color: " + STR_COLOR + "; color: white; } \
                            QMenu::item { background-color: transparent; margin: 0px 0px 4px 4px; padding: 4px 8px 4px 24px; } \
                            QMenu::item:selected { background-color: " + STR_COLOR_HOVER + "; color: white; }");
QString veriMessageBox = QString("QMessageBox { messagebox-text-interaction-flags: 5; }");

// Put them all together
QString veriStyleSheet = veriCentralWidgetStyleSheet + veriPushButtonStyleSheet + veriToolBarStyleSheet + veriToolTipStyleSheet + veriMiscStyleSheet + veriMenuStyleSheet + veriMessageBox;


// Special styling for AskPassphrasePage
QString veriAskPassphrasePushButtonStyleSheet = QString("QPushButton { background: " + STR_COLOR_LT + "; width: %1px; height: %2px; border: none; color: white} \
                            QPushButton:disabled { background: #EBEBEB; color: #666666; } \
                            QPushButton:hover { background: " + STR_COLOR_LT + "; } \
                            QPushButton:pressed { background: " + STR_COLOR_LT + "; } ").arg(BUTTON_WIDTH).arg(BUTTON_HEIGHT);

QString veriAskPassphrasePageStyleSheet = QString("QDialog { border-image: url(:images/askPassphraseBackground) repeat 0px 0px; background-color: " + STR_COLOR + "; } QLabel { color: white; } QLineEdit { background: white; color: " + STR_COLOR + "; } ") + veriAskPassphrasePushButtonStyleSheet + veriToolTipStyleSheet;

// Setup header and styles
QGraphicsView *header(QWidget *parent, QString backgroundImage)
{
    QGraphicsView *h = new QGraphicsView(parent);
    h->setStyleSheet("QGraphicsView { background: url(" + backgroundImage + ") no-repeat 0px 0px; border: none; background-color: " + STR_COLOR + "; }");
    h->setObjectName(QStringLiteral("header"));
    h->setContentsMargins(0,0,0,0);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(h->sizePolicy().hasHeightForWidth());
    h->setSizePolicy(sizePolicy);
    h->setMinimumSize(QSize(16777215, HEADER_HEIGHT));
    h->setMaximumSize(QSize(16777215, HEADER_HEIGHT));
    h->setAutoFillBackground(true);
    h->setFrameShape(QFrame::NoFrame);
    h->setFrameShadow(QFrame::Plain);
    h->setLineWidth(0);
    h->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    h->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    h->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
    h->setCacheMode(QGraphicsView::CacheBackground);
    return h;
}

QString dateTimeStr(const QDateTime &date)
{
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QString dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromTime_t((qint32)nTime));
}

QFont bitcoinAddressFont()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    return font;
}

void setupAddressWidget(QLineEdit *widget, QWidget *parent)
{
    widget->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    widget->setValidator(new BitcoinAddressValidator(parent));
    widget->setFont(bitcoinAddressFont());
}

void setupAmountWidget(QLineEdit *widget, QWidget *parent)
{
    QDoubleValidator *amountValidator = new QDoubleValidator(parent);
    amountValidator->setDecimals(BitcoinUnits::decimals(BitcoinUnits::VRC));
    amountValidator->setBottom(0.0);
    widget->setValidator(amountValidator);
    widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out)
{
    // RoxyCoin: check prefix
    if(uri.scheme() != QString("roxycoin"))
        return false;

    SendCoinsRecipient rv;
    rv.address = uri.path();
    rv.amount = 0;
    QList<QPair<QString, QString> > items = uri.queryItems();
    for (QList<QPair<QString, QString> >::iterator i = items.begin(); i != items.end(); i++)
    {
        bool fShouldReturnFalse = false;
        if (i->first.startsWith("req-"))
        {
            i->first.remove(0, 4);
            fShouldReturnFalse = true;
        }

        if (i->first == "label")
        {
            rv.label = i->second;
            fShouldReturnFalse = false;
        }
        else if (i->first == "amount")
        {
            if(!i->second.isEmpty())
            {
                if(!BitcoinUnits::parse(BitcoinUnits::VRC, i->second, &rv.amount))
                {
                    return false;
                }
            }
            fShouldReturnFalse = false;
        }

        if (fShouldReturnFalse)
            return false;
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}

bool parseBitcoinURI(QString uri, SendCoinsRecipient *out)
{
    // Convert roxycoin:// to roxycoin:
    //
    //    Cannot handle this later, because bitcoin:// will cause Qt to see the part after // as host,
    //    which will lower-case it (and thus invalidate the address).
    if(uri.startsWith("roxycoin://"))
    {
        uri.replace(0, 12, "roxycoin:");
    }
    QUrl uriInstance(uri);
    return parseBitcoinURI(uriInstance, out);
}

QString HtmlEscape(const QString& str, bool fMultiLine)
{
    QString escaped = Qt::escape(str);
    if(fMultiLine)
    {
        escaped = escaped.replace("\n", "<br>\n");
    }
    return escaped;
}

QString HtmlEscape(const std::string& str, bool fMultiLine)
{
    return HtmlEscape(QString::fromStdString(str), fMultiLine);
}

void copyEntryData(QAbstractItemView *view, int column, int role)
{
    if(!view || !view->selectionModel())
        return;
    QModelIndexList selection = view->selectionModel()->selectedRows(column);

    if(!selection.isEmpty())
    {
        // Copy first item
        QApplication::clipboard()->setText(selection.at(0).data(role).toString());
    }
}

QString getSaveFileName(QWidget *parent, const QString &caption,
                                 const QString &dir,
                                 const QString &filter,
                                 QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
        myDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    QString result = QFileDialog::getSaveFileName(parent, caption, myDir, filter, &selectedFilter);

    /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
    QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
    QString selectedSuffix;
    if(filter_re.exactMatch(selectedFilter))
    {
        selectedSuffix = filter_re.cap(1);
    }

    /* Add suffix if needed */
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            /* No suffix specified, add selected suffix */
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }

    /* Return selected suffix if asked to */
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

Qt::ConnectionType blockingGUIThreadConnection()
{
    if(QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        return Qt::BlockingQueuedConnection;
    }
    else
    {
        return Qt::DirectConnection;
    }
}

bool checkPoint(const QPoint &p, const QWidget *w)
{
    QWidget *atW = qApp->widgetAt(w->mapToGlobal(p));
    if (!atW) return false;
    return atW->topLevelWidget() == w;
}

bool isObscured(QWidget *w)
{
    return !(checkPoint(QPoint(0, 0), w)
        && checkPoint(QPoint(w->width() - 1, 0), w)
        && checkPoint(QPoint(0, w->height() - 1), w)
        && checkPoint(QPoint(w->width() - 1, w->height() - 1), w)
        && checkPoint(QPoint(w->width() / 2, w->height() / 2), w));
}

void openDebugLogfile()
{
    boost::filesystem::path pathDebug = GetDataDir() / "debug.log";

    /* Open debug.log with the associated application */
    if (boost::filesystem::exists(pathDebug))
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(pathDebug.string())));
}

ToolTipToRichTextFilter::ToolTipToRichTextFilter(int size_threshold, QObject *parent) :
    QObject(parent), size_threshold(size_threshold)
{

}

bool ToolTipToRichTextFilter::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::ToolTipChange)
    {
        QWidget *widget = static_cast<QWidget*>(obj);
        QString tooltip = widget->toolTip();
        if(tooltip.size() > size_threshold && !tooltip.startsWith("<qt>") && !Qt::mightBeRichText(tooltip))
        {
            // Prefix <qt/> to make sure Qt detects this as rich text
            // Escape the current message as HTML and replace \n by <br>
            tooltip = "<qt>" + HtmlEscape(tooltip, true) + "<qt/>";
            widget->setToolTip(tooltip);
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}

#ifdef WIN32
boost::filesystem::path static StartupShortcutPath()
{
    return GetSpecialFolderPath(CSIDL_STARTUP) / "RoxyCoin.lnk";
}

bool GetStartOnSystemStartup()
{
    // check for Bitcoin.lnk
    return boost::filesystem::exists(StartupShortcutPath());
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    boost::filesystem::remove(StartupShortcutPath());

    if (fAutoStart)
    {
        CoInitialize(NULL);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                                CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            TCHAR pszExePath[MAX_PATH];
            GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

            TCHAR pszArgs[5] = TEXT("-min");

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(pszArgs);

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = NULL;
            hres = psl->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().string().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}

#elif defined(LINUX)

// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

boost::filesystem::path static GetAutostartDir()
{
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
}

boost::filesystem::path static GetAutostartFilePath()
{
    return GetAutostartDir() / "roxycoin.desktop";
}

bool GetStartOnSystemStartup()
{
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    std::string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != std::string::npos &&
            line.find("true") != std::string::npos)
            return false;
    }
    optionFile.close();

    return true;
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
        boost::filesystem::remove(GetAutostartFilePath());
    else
    {
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
            return false;

        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), std::ios_base::out|std::ios_base::trunc);
        if (!optionFile.good())
            return false;
        // Write a bitcoin.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=RoxyCoin\n";
        optionFile << "Exec=" << pszExePath << " -min\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}
#else

// TODO: OSX startup stuff; see:
// https://developer.apple.com/library/mac/#documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif

HelpMessageBox::HelpMessageBox(QWidget *parent) :
    QMessageBox(parent)
{
    header = tr("RoxyCoin-Qt") + " " + tr("version") + " " +
        QString::fromStdString(FormatFullVersion()) + "\n\n" +
        tr("Usage:") + "\n" +
        "  roxycoin-qt [" + tr("command-line options") + "]                     " + "\n";

    coreOptions = QString::fromStdString(HelpMessage());

    uiOptions = tr("UI options") + ":\n" +
        "  -lang=<lang>           " + tr("Set language, for example \"de_DE\" (default: system locale)") + "\n" +
        "  -min                   " + tr("Start minimized") + "\n" +
        "  -splash                " + tr("Show splash screen on startup (default: 1)") + "\n";

    setWindowTitle(tr("RoxyCoin-Qt"));
    setTextFormat(Qt::PlainText);
    // setMinimumWidth is ignored for QMessageBox so put in non-breaking spaces to make it wider.
    setText(header + QString(QChar(0x2003)).repeated(50));
    setDetailedText(coreOptions + "\n" + uiOptions);
}

void HelpMessageBox::printToConsole()
{
    // On other operating systems, the expected action is to print the message to the console.
    QString strUsage = header + "\n" + coreOptions + "\n" + uiOptions;
    fprintf(stdout, "%s", strUsage.toStdString().c_str());
}

void HelpMessageBox::showOrPrint()
{
#if defined(WIN32)
        // On Windows, show a message box, as there is no stderr/stdout in windowed applications
        exec();
#else
        // On other operating systems, print help text to console
        printToConsole();
#endif
}

#if BOOST_FILESYSTEM_VERSION >= 3
boost::filesystem::path qstringToBoostPath(const QString &path)
{
    return boost::filesystem::path(path.toStdString(), utf8);
}

QString boostPathToQString(const boost::filesystem::path &path)
{
    return QString::fromStdString(path.string(utf8));
}
#else
#warning Conversion between boost path and QString can use invalid character encoding with boost_filesystem v2 and older
boost::filesystem::path qstringToBoostPath(const QString &path)
{
    return boost::filesystem::path(path.toStdString());
}

QString boostPathToQString(const boost::filesystem::path &path)
{
    return QString::fromStdString(path.string());
}
#endif

} // namespace GUIUtil
